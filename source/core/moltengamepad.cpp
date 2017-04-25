#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>
#include <glob.h>
#include "devices/generic/generic.h"
#include "parser.h"
#include "protocols/ostream_protocol.h"
#include "protocols/socket/socket_in.h"

//FUTURE WORK: Make it easier to specify additional virtpad styles.

const virtpad_settings default_padstyle = {
  {"Virtual Gamepad (MoltenGamepad)", "", 1, 1, 1}, //u_ids
  false, //dpad_as_hat
  true, //analog_triggers
  false, //rumble
};

const virtpad_settings xpad_padstyle = {
  {"Microsoft X-Box 360 pad", "", 0x045e, 0x028e, 0x110}, //u_ids
  true, //dpad_as_hat
  true, //analog_triggers
  false, //rumble
};


volatile bool fifo_looping;

std::vector<std::string> find_xdg_config_dirs(std::string commandline_override) {
  std::vector<std::string> dirs;
  //First check for XDG_CONFIG_HOME or use the override instead.
  //The override is a bit redundant, but backwards compatibility is good for now.
  if (!commandline_override.empty()) {
    dirs.push_back(commandline_override);
  } else {
    const char* config_home = getenv("XDG_CONFIG_HOME");
    std::string confdir;
    if (config_home && config_home[0] != '\0') {
      confdir = std::string(config_home);
    } else {
      //It was unset, so try its specified default
      if (getenv("HOME")) {
        confdir = std::string(getenv("HOME")) + "/.config/";
      }
    }
    if (!confdir.empty()) {
      mkdir(confdir.c_str(), 0755);
      dirs.push_back(confdir + "/moltengamepad/");
      mkdir(dirs.front().c_str(), 0755);
    }
  }
  
  //Now check XDG_CONFIG_DIRS
  const char* config_dirs = getenv("XDG_CONFIG_DIRS");
  if (config_dirs && config_dirs[0] != '\0') {
    std::string confdirs = std::string(config_dirs);
    //have to split on colons
    std::stringstream stream(confdirs);
    std::string dir;
    while (std::getline(stream, dir, ':')) {
        dirs.push_back(dir + "/moltengamepad/");
    }
  } else {
    //It was unset, so try its specified default
    dirs.push_back("/etc/xdg/moltengamepad/");
  }
  
  return dirs;
}

std::string moltengamepad::locate(file_category cat, std::string path) {
  std::string commandline_override = "";
  std::string category_prefix = "";
  if (!path.empty() && path.front() == '/')
    return path; //this is an absolute path.

  switch (cat) {
    case FILE_CONFIG:
      break; //the override handled elsewhere to affect all categories
    case FILE_PROFILE:
      commandline_override = opts->get<std::string>("profile_dir");
      category_prefix = "/profiles/";
      break;
    case FILE_GENDEV:
      commandline_override = opts->get<std::string>("gendev_dir");
      category_prefix = "/gendevices/";
      break;
    case FILE_OPTIONS:
      category_prefix = "/options/";
      break;
    case FILE_PLUGIN:
      category_prefix = "/plugins/";
      break;
  }

  std::vector<std::string> dirs = xdg_config_dirs;
  if (!commandline_override.empty())
    dirs.insert(dirs.begin(),commandline_override);

  for (auto dir : dirs) {
    std::string fullpath = dir + "/" + category_prefix + path;
    if (access((fullpath).c_str(), R_OK) != -1) {
      char* resolved = realpath(fullpath.c_str(), nullptr);
      if (resolved) {
        fullpath = std::string(resolved);
        free(resolved);
      }
      debug_print(DEBUG_VERBOSE, 2, "locate: able to read ", fullpath.c_str());
      return fullpath;
    } else {
      debug_print(DEBUG_VERBOSE, 2, "locate: unable to read ", fullpath.c_str());
    }
  }

  return "";
}

std::vector<std::string> moltengamepad::locate_glob(file_category cat, std::string pathglob) {
  std::string commandline_override = "";
  //if this is a relative path, we need to do extra logic:
  // -use appropriate prefix to match the category
  // -cascade the file glob according to XDG directories.
  //if this is an absolute path, we don't need to do anything other than the glob.
  std::string category_prefix = "";
  std::vector<std::string> files;
  std::vector<std::string> dirs;
  if (pathglob.empty() || pathglob[0] != '/') {
    //relative path
    switch (cat) {
      case FILE_CONFIG:
        break; //the override handled elsewhere to affect all categories
      case FILE_PROFILE:
        commandline_override = opts->get<std::string>("profile_dir");
        category_prefix = "/profiles/";
        break;
      case FILE_GENDEV:
        commandline_override = opts->get<std::string>("gendev_dir");
        category_prefix = "/gendevices/";
        break;
      case FILE_OPTIONS:
        category_prefix = "/options/";
        break;
      case FILE_PLUGIN:
        category_prefix = "/plugins/";
        break;
    }
    dirs.insert(dirs.begin(),xdg_config_dirs.begin(),xdg_config_dirs.end());
    if (!commandline_override.empty())
      dirs.insert(dirs.begin(),commandline_override);
  } else {
    //absolute path
    dirs.push_back("");
    //the prefix is still empty,
    //so we end up globbing "//" + absolute file glob
  }

  //we want higher precedence files to "shadow" those with lesser precedence.
  //This is achieved by checking to see if we have seen this filename before.
  std::unordered_set<std::string> unique_finds;
  for (auto dir : dirs) {
    std::string fullpath = dir + "/" + category_prefix + "/";
    glob_t globbuffer;
    glob((fullpath+pathglob).c_str(), 0, nullptr, &globbuffer);
    debug_print(DEBUG_VERBOSE, 2, "glob: ", (fullpath+pathglob).c_str());
    for (int i = 0; i < globbuffer.gl_pathc; i++) {
      
      char* resolved = realpath(globbuffer.gl_pathv[i], nullptr);
      if (resolved) {
        //try to strip off our directory path, to see the part matched by the glob.
        if (strstr(globbuffer.gl_pathv[i], fullpath.c_str()) == globbuffer.gl_pathv[i]) {
          //then we have detected our directory prefix successfully.
          const char* extracted = globbuffer.gl_pathv[i] + fullpath.size();
          auto check = unique_finds.insert(std::string(extracted));
          if (check.second == false) {
            //this is a duplicate!
            debug_print(DEBUG_INFO, 2, "\tglob file shadowed by higher precedence file: ", resolved);
            free(resolved);
            continue;
          }
        }
        files.push_back(std::string(resolved));
        debug_print(DEBUG_VERBOSE, 2, "\tglob file: ", resolved);
        free(resolved);
      }
    }

    globfree(&globbuffer);
  }

  return files;
}



int fifo_loop(moltengamepad* mg) {
  while (fifo_looping) {
    std::ifstream file;
    std::string path;
    mg->opts->get<std::string>("fifo_path",path);
    file.open(path, std::istream::in);
    if (file.fail()) break;
    shell_loop(mg, file);
    file.close();
  }
  return 0;
}

int clear_fifo(const char* fifo_path, bool force_remote_exit) {
  int fifo = open(fifo_path, O_WRONLY | O_NONBLOCK); //need O_NONBLOCK or we hang if there was no listener!
  debug_print(DEBUG_INFO,2,"Destroying FIFO at ", fifo_path);
  unlink(fifo_path);
  if (fifo >= 0) {
    if (force_remote_exit) {
      //tell the listener to stop everything
      write(fifo,"\nexit_process_leave_fifo\n",25);
    } else {
      //tell the listener to refresh their fifo session and see that it is gone.
      //(keeps process alive, but stops FIFO thread.)
      //This is needed when we call clear_fifo() while already trying to stop everything!
      write(fifo,"\nquit\n",6);
    }
    close(fifo);
  }
}

const option_decl general_options[] = {

  {"num_gamepads", "Number of virtual gamepads to create", "4", MG_INT},
  {"dpad_as_hat", "Use a hat to represent the dpad, instead of 4 separate buttons", "false", MG_BOOL},
  {"mimic_xpad", "Set virtual devices to match a wired Xbox 360 controller", "false", MG_BOOL},
  {"make_keyboard", "Make a virtual keyboard/mouse device", "true", MG_BOOL},
  {"config_dir", "A directory to use instead of $XDG_CONFIG_HOME/moltengamepad", "", MG_STRING},
  {"profile_dir", "A directory to check for profiles before the config directories", "", MG_STRING},
  {"gendev_dir", "A directory to check for generic driver descriptions before the config directories", "", MG_STRING},
  {"make_fifo", "Make a FIFO that processes any commands written to it", "false", MG_BOOL},
  {"replace_fifo", "Before making the FIFO, tell any existing listeners to exit", "false", MG_BOOL},
  {"fifo_path", "Location to create the FIFO", "", MG_STRING},
  {"uinput_path", "Location of the uinput node", "", MG_STRING},
  {"daemon", "Run in daemon mode", "false", MG_BOOL},
  {"pidfile", "Location to write the PID when in daemon mode", "", MG_STRING},
  {"enumerate", "Check for already connected devices", "true", MG_BOOL},
  {"monitor", "Listen for device connections/disconnections", "true", MG_BOOL},
  {"rumble", "Process controller rumble effects", "false", MG_BOOL},
  {"stay_alive", "Keep process running even after standard input is closed", "false", MG_BOOL},
  {"make_socket", "Make a socket to communicate with clients", "false", MG_BOOL},
  {"socket_path", "Location to create the socket", "", MG_STRING},
  {"auto_profile_subdir", "automatically load profiles in this subdir. of the profile dir. on start up", "auto/", MG_STRING},
  {"", "", ""},
};

const char* keywords[] = {
  "",
  "load",
  "save",
  "print",
  "move",
  "clear",
  "alter",
  "moltengamepad",
  "mg",
  "to",
  "from",
  "nothing",
  "none",
  "general",
  "default",
  "slot",
  "slots",
  "profile",
  "profiles",
  "device",
  "devices",
  "set",
  "setting",
  nullptr,
};

int config_parse_line(moltengamepad* mg, std::vector<token>& line, context context, options& opt, config_extras* extra);

int moltengamepad::init() {
  //This whole function is pretty bad in handling the config directories not being present.
  //But at least we aren't just spilling into the user's top level home directory.
  stdout = new ostream_protocol(std::cout, std::cerr);
  if (geteuid() == 0) {
    std::cout << "warning: Running MoltenGamepad as root is not recommended. Instead, please run as a non-superuser that has been granted the appropriate permissions." << std::endl;
  }
  //load config dirs from environment variables
  xdg_config_dirs = find_xdg_config_dirs(opts->get<std::string>("config_dir"));
  
  //Initialize static parser variables
  MGparser::load_translators(this);
  for (int i = 0; keywords[i]; i++)
    forbidden_ids.insert(std::string(keywords[i]));
  
  //Load moltengamepad.cfg if it exists
  config_extras cfg;
  //First, lock some options that can't be changed at this point.
  opts->lock("daemon",true);
  opts->lock("pidfile",true);
  opts->lock("config_dir",true);
  opts->lock("stay_alive",true);
  std::string cfgfile = locate(FILE_CONFIG,"moltengamepad.cfg");
  if (!cfgfile.empty()) {
    std::cout << "loading " << cfgfile << std::endl;
    loop_file(cfgfile, [this, &cfg] (std::vector<token>& tokens, context ctx) {
      config_parse_line(this, tokens, ctx, *(this->opts), &cfg);
      return 0;
    });
  } else {
    std::cout << "No moltengamepad.cfg found." << std::endl;
  }
  //Now that we have all config parsed,
  //check for FIFO to quit early if needed.
  if (opts->get<bool>("make_fifo") || opts->get<bool>("replace_fifo")) {
    //unlock option so we can set/clear it if needed.
    opts->lock("fifo_path",false);
    opts->lock("make_fifo",false);
    //set make_fifo so that we don't need to check replace_fifo again later.
    //(--replace-fifo implies --make-fifo)
    opts->set("make_fifo","true");
    //try to use $XDG_RUNTIME_DIR, only if set.
    const char* run_dir = getenv("XDG_RUNTIME_DIR");
    std::string fifo_path;
    opts->get<std::string>("fifo_path",fifo_path);
    if (fifo_path.empty() && run_dir) {
      fifo_path = std::string(run_dir) + "/mg.fifo";
    }
    if (fifo_path.empty()) {
      stdout->err(0,"Could not locate fifo path. Use the --fifo-path command line argument.");
      throw std::runtime_error("FIFO failed."); //Abort so we don't accidentally run without a means of control.
    }
    if (opts->get<bool>("replace_fifo")) {
      debug_print(DEBUG_NONE, 2, "Replacing FIFO at ", fifo_path.c_str());
      clear_fifo(fifo_path.c_str(), true);
    } else {
      debug_print(DEBUG_NONE,2,"Making FIFO at ", fifo_path.c_str());
    }
    int ret = mkfifo(fifo_path.c_str(), 0660);
    if (ret < 0)  {
      perror("making fifo:");
      //clear them!
      opts->set("make_fifo","false");
      opts->set("fifo_path","");
      //lock them! No further changes.
      opts->lock("make_fifo",true);
      opts->lock("fifo_path",true);
      throw std::runtime_error("FIFO failed.");
    } else {
      opts->set("fifo_path",fifo_path);
      opts->lock("fifo_path",true);
      opts->lock("make_fifo",true);
    }
  }

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  if (opts->get<bool>("make_socket")) {
    std::string socket_path;
    opts->get<std::string>("socket_path",socket_path);
    this->sock = make_socket(socket_path, address);
    if (sock < 0) {
      debug_print(DEBUG_NONE,1,"Making socket failed. Perhaps --socket-path is needed.");
      opts->lock("socket_path",false);
      opts->set("socket_path","");
      opts->lock("socket_path",true);
      opts->lock("make_socket",false);
      opts->set("make_socket","false");
      opts->lock("make_socket",true);
      throw std::runtime_error("Socket failed.");
    } else {
      opts->lock("socket_path",false);
      opts->set("socket_path",socket_path);
      opts->lock("socket_path",true);
    }
  }

  //build the gamepad profile
  gamepad->gamepad_defaults();
  gamepad->name = "gamepad";
  gamepad->set_group_alias("left_stick","left_x left_y");
  gamepad->set_group_alias("right_stick","right_x right_y");
  add_profile(gamepad.get());
  ids_in_use.insert("gamepad");

  //set up our padstyles and our slot manager
  virtpad_settings padstyle = default_padstyle;
  opts->get<bool>("dpad_as_hat",padstyle.dpad_as_hat);
  if (opts->get<bool>("mimic_xpad")) padstyle = xpad_padstyle;
  opts->get<bool>("rumble",padstyle.rumble);
  slots = new slot_manager(opts->get<int>("num_gamepads"), opts->get<bool>("make_keyboard"), padstyle);

  //add standard streams
  drivers.add_listener(stdout);
  plugs.add_listener(stdout);
  slots->log.add_listener(stdout);

  //add built in drivers
  //They have priority over both plugin drivers and gendev drivers.
  init_plugin_api();
  init_generic_callbacks();
  //even with no built ins, this does some needed initialization...
  load_builtins(this);


  std::string confdir = locate(FILE_CONFIG,"");
  if (!confdir.empty()) {
    if (opts->get<std::string>("profile_dir").empty()) mkdir((confdir + "/profiles/").c_str(), 0755);
    if (opts->get<std::string>("gendev_dir").empty()) mkdir((confdir + "/gendevices/").c_str(), 0755);
    mkdir((confdir + "/options/").c_str(), 0755);
    mkdir((confdir + "/plugins/").c_str(), 0755);
  }

  std::string slotcfg = locate(FILE_OPTIONS, "slots.cfg");
  if (!slotcfg.empty()) {
    slots->log.take_message(0,"reading slots options from " + slotcfg);
    loop_file(slotcfg, [this] (std::vector<token>& tokens, context ctx) {
      config_parse_line(this, tokens, ctx, this->slots->opts, nullptr);
      return 0;
    });
  }

  //load our dynamic plugins.
  //They have priority over gendev drivers.
  if (LOAD_PLUGINS) {
    auto plugin_files = locate_glob(FILE_PLUGIN,"*.so");
    if (!plugin_files.empty()) {
      std::cout << "plugin: Loading plugins." << std::endl;
    } else {
      std::cout << "plugin: no plugins found." << std::endl;
    }
    for (auto path : plugin_files) {
      load_plugin(path);
    }
  }

  //file glob the gendev .cfg files to add more drivers
  auto gendev_files = locate_glob(FILE_GENDEV,"*.cfg");

  for (auto path : gendev_files) {
    std::ifstream file;
    file.open(path, std::istream::in);
    //If we can read it, feed it to our gendev parser
    if (!file.fail()) {
      int ret = generic_config_loop(this, file, path);
    }
  }

  //Run any startup profiles before beginning the udev searches

  //First, check the auto profile subdir.
  //Insert them to the front of startup_profiles.
  //The ones read from moltengamepad.cfg were specified explicitly,
  //so they should take precedence (and be read later).
  std::string auto_prof_dir = opts->get<std::string>("auto_profile_subdir");
  if (!auto_prof_dir.empty()) {
    std::vector<std::string> auto_profiles = locate_glob(FILE_PROFILE,auto_prof_dir+"/*");
    cfg.startup_profiles.insert(cfg.startup_profiles.begin(), auto_profiles.begin(), auto_profiles.end());
  }

  for (auto profile_path : cfg.startup_profiles) {
    std::string fullpath = locate(FILE_PROFILE,profile_path);
    if (fullpath.empty() || profile_path.empty()) {
      std::cout << "Could not locate profile file \"" << profile_path << "\"" << std::endl;
      continue;
    }
    std::ifstream file;
    if (!fullpath.empty()) {
      file.open(fullpath, std::ifstream::in);
      if (!file.fail()) {
        std::cout << "Loading profiles from " << fullpath << std::endl;
        shell_loop(this, file);
      } else {
        std::cout << "Failed to open profile file " << fullpath << std::endl;
      }
       file.close();
    }
  }

  //start the udev thread
  udev.set_managers(&managers);
  udev.set_uinput(slots->get_uinput());
  if (opts->get<bool>("monitor")) udev.start_monitor();
  if (opts->get<bool>("enumerate"))   udev.enumerate();

  //Finally start reading FIFO now that everything is up and running.
  if (opts->get<bool>("make_fifo")) {
    fifo_looping = true;
    remote_handler = new std::thread(fifo_loop, this);
    remote_handler->detach();
    delete remote_handler;
    remote_handler = nullptr;
  }

  if (opts->get<bool>("make_socket")) {
    start_socket_listener(this, address);
  }



}

moltengamepad::~moltengamepad() {

  //drop a shared ptr, let it clean up
  gamepad = nullptr;

  udev.set_managers(nullptr);


  std::cout << "Shutting down." << std::endl;

  //Clean up our fifo + send a message to ensure the thread clears out.
  if (opts->get<bool>("make_fifo")) {
    std::string path;
    fifo_looping = false;
    opts->get("fifo_path",path);
    clear_fifo(path.c_str(), false); //do not force exit, as we are already exiting!
  }

  //clean up socket
  if (opts->get<bool>("make_socket")) {
    debug_print(DEBUG_INFO,3,"cleaning up socket at \"",opts->get<std::string>("socket_path").c_str(),"\"");
    close(sock);
    unlink(opts->get<std::string>("socket_path").c_str());
  }

  //remove devices
  //done first to protect from devices assuming their manager exists.
  devices.clear();

  //delete managers
  for (auto it = managers.begin(); it != managers.end(); ++it) {
    delete(*it);
  }

  //delete output slots
  if (slots)
    delete slots;

  if (stdout)
    delete stdout;
}


device_manager* moltengamepad::add_manager(manager_plugin manager, void* manager_plug_data) {
  std::lock_guard<std::mutex> guard(id_list_lock);
  std::string manager_name(manager.name);
  bool destroyed = false;
  if (forbidden_ids.find(manager_name) != forbidden_ids.end()) {
    drivers.err(0,"manager name " + manager_name + " is invalid.");
    destroyed = true;
  }
  if (ids_in_use.find(manager_name) != ids_in_use.end()) {
    drivers.err(0,"redundant manager " + manager_name + " ignored.");
    destroyed = true;
  }
  if (destroyed) {
    if (manager.destroy)
      manager.destroy(manager_plug_data);
    return nullptr;
  }

  auto man = new device_manager(this, manager, manager_plug_data);
  managers.push_back(man);
  add_profile(man->mapprofile.get());
  if (manager.subscribe_to_gamepad_profile)
    gamepad->copy_into(man->mapprofile, true, false);

  if (man->has_options) {
    auto filepath = locate(FILE_OPTIONS, manager_name + ".cfg");
    if (!filepath.empty()) {
      drivers.take_message(0,"reading driver options from " + filepath);
      loop_file(filepath, [this, man] (std::vector<token>& tokens, context ctx) {
        config_parse_line(this, tokens, ctx, man->opts, nullptr);
        return 0;
      });
    }
  }
  if (manager.start)
    manager.start(man->plug_data);
  drivers.take_message(0,man->name + " driver initialized.");
  ids_in_use.insert(man->name);
  return man;
};

device_manager* moltengamepad::find_manager(const char* name) const {
  for (auto it = managers.begin(); it != managers.end(); it++) {
    if (!strcmp((*it)->name.c_str(), name)) return (*it);
  }
  return nullptr;
}

std::shared_ptr<input_source> moltengamepad::find_device(const char* name) const {
  std::lock_guard<std::mutex> guard(device_list_lock);
  for (auto it = devices.begin(); it != devices.end(); it++) {

    if (!strcmp((*it)->get_name().c_str(), name)) return (*it);
  }
  return nullptr;
}
std::shared_ptr<input_source> moltengamepad::add_device(input_source* source, device_manager* manager, std::string name_stem) {

  std::shared_ptr<input_source> ptr(source);

  device_list_lock.lock();
  std::lock_guard<std::mutex> guard(id_list_lock);
  if (forbidden_ids.find(name_stem) != forbidden_ids.end())
    return nullptr;

  
  //try to find an unused name. It is okay if this is slow.
  std::string proposal;
  bool available = false;
  for (int i = 1; i < 64; i++) {
    proposal = name_stem + std::to_string(i);
    if (ids_in_use.find(proposal) == ids_in_use.end()) {
      available = true;
      break; //This proposal is available!
    }
  }
  
  if (!available) {
    plugs.err(0,"could not find available name for " + name_stem);
    return nullptr;
  }
  //Set the device and profile name, send a message, link the profile, and finally start the device thread.
  ptr->set_name(proposal);
  ids_in_use.insert(proposal);
  devices.push_back(ptr);
  auto devprof = source->get_profile();
  devprof->name = source->get_name();
  add_profile(devprof.get());
  devprof->add_device(ptr);
  manager->mapprofile->copy_into(devprof, true, true);

  if (ptr->plugin.init)
    ptr->plugin.init(ptr->plug_data, ptr.get());

  plugs.device_plug(0, source, "add");
  device_list_lock.unlock();
  ptr->start_thread();
  if (slots->opts.get<bool>("auto_assign"))
    slots->request_slot(ptr.get());
  return ptr;
}

std::shared_ptr<input_source> moltengamepad::add_device(device_manager* manager, device_plugin dev, void* dev_plug_data) {
  input_source* ptr = new input_source(manager, dev, dev_plug_data);
  return add_device(ptr, manager, std::string(dev.name_stem));
}

int moltengamepad::remove_device(input_source* source) {
  device_list_lock.lock();
  std::lock_guard<std::mutex> guard(id_list_lock);
  for (int i = 0; i < devices.size(); i++) {
    if (source == devices[i].get()) {
      plugs.device_plug(0, source, "remove");
      remove_profile(devices[i]->get_profile().get());
      ids_in_use.erase(source->get_name());
      devices.erase(devices.begin() + i);
      i--;
    }
  }
  device_list_lock.unlock();
  return 0;
}

void moltengamepad::for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func) {
  device_list_lock.lock();
  for (auto dev : devices)
    func(dev);
  device_list_lock.unlock();
}

std::shared_ptr<profile> moltengamepad::find_profile(const std::string& name) {
  std::lock_guard<std::mutex> guard(profile_list_lock);
  for (auto it = profiles.begin(); it != profiles.end(); it++) {
    auto ptr = it->lock();
    if (name == ptr->name) return ptr;
  }
  return nullptr;
}

void moltengamepad::add_profile(profile* profile) {
  profile_list_lock.lock();
  profiles.push_back(profile->get_shared_ptr());
  ids_in_use.insert(profile->name);
  profile_list_lock.unlock();
}

void moltengamepad::remove_profile(profile* profile) {
  profile_list_lock.lock();
  for (int i = 0; i < profiles.size(); i++) {
    if (profile == profiles[i].lock().get()) {
      profiles.erase(profiles.begin() + i);
      i--;
    }
  }
  profile_list_lock.unlock();
}

void moltengamepad::for_all_profiles(std::function<void (std::shared_ptr<profile>&)> func) {
  profile_list_lock.lock();
  for (auto prof : profiles) {
    auto ptr = prof.lock();
    if (ptr) func(ptr);
  }
  profile_list_lock.unlock();
}


int loop_file(const std::string path, std::function<int (std::vector<token>&, context)> func) {
  if (path.empty()) return -1;
  char* buff = new char [1024];
  context context = {1, path};
  std::ifstream file;
  file.open(path, std::istream::in);
  if (file.fail()) return -5;
  while (!QUIT_APPLICATION) {
    file.getline(buff, 1024);
    auto tokens = tokenize(std::string(buff));
    int ret = func(tokens, context);
    if (ret) break;
    if (file.eof()) break;
    context.line_number++;

  }

  delete[] buff;
  return 0;
}

int moltengamepad::set_option(std::string& category, std::string& name, std::string& value) {
  //need to properly ensure the lifetime of referenced objects...
  //For the slot_manager however, the lifetime is the same as this entire process.
  int ret = FAILURE;
  run_on_options(category, [&ret, &name, &value] (options* opts) { ret = opts->set(name,value); });

  return ret;
}

void moltengamepad::list_options(std::string& category, std::vector<option_info>& list) const {
  run_on_options(category, [&list] (options* opts) { opts->list_options(list); });
}

void moltengamepad::run_on_options(std::string& category, std::function<void (options*)> func) const {
  //need to properly ensure the lifetime of referenced objects...
  //For the slot_manager however, the lifetime is the same as this entire process.
  if (category == "slot" || category == "slots") {
    func(&slots->opts);
    return;
  }
  //For device managers, the lifetime is currently also tied to the entire process
  device_manager* man = find_manager(category.c_str());
  if (man) {
    func(&man->opts);
    return;
  }
};

void escape_string(std::string& str) {
  auto it = str.begin();
  while (it != str.end()) {
    if (*it == '\\') {
      str.insert(it,'\\');
      it++;
    }
    it++;
  }
  it = str.begin();
  while (it != str.end()) {
    if (*it == '"') {
      str.insert(it,'\\');
      it++;
    }
    it++;
  }
}

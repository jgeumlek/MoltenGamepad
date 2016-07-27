#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>
#include <glob.h>
#include "devices/wiimote/wiimote.h"
#include "devices/generic/generic.h"

#ifdef BUILD_STEAM_CONTROLLER_DRIVER
#include "devices/steamcontroller/steam_controller.h"
#endif

//FUTURE WORK: Make it easier to specify additional virtpad styles.

const virtpad_settings default_padstyle = {
  {"Virtual Gamepad (MoltenGamepad)", 1, 1, 1}, //u_ids
  false, //dpad_as_hat
  true, //analog_triggers
  "SEWN", //facemap_1234
};

const virtpad_settings xpad_padstyle = {
  {"Microsoft X-Box 360 pad", 0x045e, 0x028e, 0x110}, //u_ids
  true, //dpad_as_hat
  true, //analog_triggers
  "SEWN", //facemap_1234
};



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
      confdir = std::string(confdir);
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
    while (getline(stream, dir, ':')) {
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
      commandline_override = options.profile_dir;
      category_prefix = "/profiles/";
      break;
    case FILE_GENDEV:
      commandline_override = options.gendev_dir;
      category_prefix = "/gendevices/";
      break;
  }

  std::vector<std::string> dirs = xdg_config_dirs;
  if (!commandline_override.empty())
    dirs.insert(dirs.begin(),commandline_override);

  for (auto dir : dirs) {
    std::string fullpath = dir + category_prefix + path;
    if (access((fullpath).c_str(), R_OK) != -1) {
      return fullpath;
    }
  }

  return "";
}

std::vector<std::string> moltengamepad::locate_glob(file_category cat, std::string pathglob) {
  std::string commandline_override = "";
  std::string category_prefix = "";
  
  switch (cat) {
    case FILE_CONFIG:
      break; //the override handled elsewhere to affect all categories
    case FILE_PROFILE:
      commandline_override = options.profile_dir;
      category_prefix = "/profiles/";
      break;
    case FILE_GENDEV:
      commandline_override = options.gendev_dir;
      category_prefix = "/gendevices/";
      break;
  }
  
  std::vector<std::string> files;
  std::vector<std::string> dirs;
  dirs.insert(dirs.begin(),xdg_config_dirs.begin(),xdg_config_dirs.end());
  if (!commandline_override.empty())
    dirs.insert(dirs.begin(),commandline_override);

  for (auto dir : dirs) {
    std::string fullpath = dir + category_prefix + pathglob;
    glob_t globbuffer;
    glob(fullpath.c_str(), 0, nullptr, &globbuffer);

    for (int i = 0; i < globbuffer.gl_pathc; i++) {
      files.push_back(std::string(globbuffer.gl_pathv[i]));
    }

    globfree(&globbuffer);
  }

  return files;
}



int fifo_loop(moltengamepad* mg) {
  bool keep_looping = true;
  while (keep_looping) {
    std::ifstream file;
    file.open(mg->options.fifo_path, std::istream::in);
    if (file.fail()) break;
    shell_loop(mg, file);
    file.close();
  }
}

int moltengamepad::init() {
  //This whole function is pretty bad in handling the config directories not being present.
  //But at least we aren't just spilling into the user's top level home directory.
  
  //load config dirs from environment variables
  xdg_config_dirs = find_xdg_config_dirs(options.config_dir);
  //build the gamepad profile
  gamepad->gamepad_defaults();
  gamepad->name = "gamepad";
  add_profile(gamepad.get());
  //set up our padstyles and our slot manager
  virtpad_settings padstyle = default_padstyle;
  padstyle.dpad_as_hat = options.dpad_as_hat;
  if (options.mimic_xpad) padstyle = xpad_padstyle;
  slots = new slot_manager(options.num_gamepads, options.make_keyboard, padstyle);
  //add standard streams
  drivers.add_listener(1);
  plugs.add_listener(1);
  errors.add_listener(2);

  //add built in drivers
  managers.push_back(new wiimote_manager(this));
  drivers.take_message("wiimote driver initialized.");
#ifdef BUILD_STEAM_CONTROLLER_DRIVER
  managers.push_back(new steam_controller_manager(this));
  drivers.take_message("steamcontroller driver initialized.");
#endif

  

  std::string confdir = locate(FILE_CONFIG,"");
  if (!confdir.empty()) {
    if (options.profile_dir.empty()) mkdir((confdir + "/profiles/").c_str(), 0755);
    if (options.gendev_dir.empty()) mkdir((confdir + "/gendevices/").c_str(), 0755);
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


  //add driver profiles
  for (auto man : managers)
    add_profile(man->mapprofile.get());

  //start the udev thread
  udev.set_managers(&managers);
  udev.set_uinput(slots->get_uinput());
  if (options.listen_for_devices) udev.start_monitor();
  if (options.look_for_devices)   udev.enumerate();

  //start listening on FIFO if needed.
  if (options.make_fifo) {
    //try to use $XDG_RUNTIME_DIR, only if set.
    const char* run_dir = getenv("XDG_RUNTIME_DIR");
    if (options.fifo_path.empty() && run_dir) {
      options.fifo_path = std::string(run_dir) + "/moltengamepad";
    }
    if (options.fifo_path.empty()) {
      errors.take_message("Could not locate fifo path. Use the --fifo-path command line argument.");
      throw - 1; //Abort so we don't accidentally run without a means of control.
    }
    int ret = mkfifo(options.fifo_path.c_str(), 0660);
    if (ret < 0)  {
      perror("making fifo:");
      options.fifo_path = "";
      throw -1;
    } else {
      remote_handler = new std::thread(fifo_loop, this);
    }
  }

}

moltengamepad::~moltengamepad() {

  //drop a shared ptr, let it clean up
  gamepad = nullptr;

  udev.set_managers(nullptr);

  //No need to print into the void...
  if(!options.daemon)
    std::cout << "Shutting down." << std::endl;

  //Clean up our fifo + send a message to ensure the thread clears out.
  if (options.make_fifo) {
    std::ofstream fifo;
    fifo.open(options.fifo_path, std::ostream::out);
    unlink(options.fifo_path.c_str());
    fifo << "quit" << std::endl;
    if (remote_handler) {
      remote_handler->join();
      delete remote_handler;
    }
  }

  //remove devices
  //done first to protect from devices assuming their manager exists.
  devices.clear();

  //delete managers
  for (auto it = managers.begin(); it != managers.end(); ++it) {
    delete(*it);
  }

  //delete output slots
  delete slots;
}



device_manager* moltengamepad::find_manager(const char* name) {
  for (auto it = managers.begin(); it != managers.end(); it++) {
    if (!strcmp((*it)->name.c_str(), name)) return (*it);
  }
  return nullptr;
}

std::shared_ptr<input_source> moltengamepad::find_device(const char* name) {
  std::lock_guard<std::mutex> guard(device_list_lock);
  for (auto it = devices.begin(); it != devices.end(); it++) {

    if (!strcmp((*it)->get_name().c_str(), name)) return (*it);
  }
  return nullptr;
}
std::shared_ptr<input_source> moltengamepad::add_device(input_source* source, device_manager* manager, std::string name_stem) {
  std::shared_ptr<input_source> ptr(source);
  device_list_lock.lock();
  
  //try to find an unused name. It is okay if this is slow.
  std::string proposal;
  bool available;
  for (int i = 1; i < 64; i++) {
    proposal = name_stem + std::to_string(i);
    available = true;
    for (auto dev : devices) {
      if (dev->get_name() == proposal) {
        available = false;
	break;
      }
    }
    if (available) break;
  }
  
  if (!available) {
    errors.take_message("could not find available name for " + name_stem);
    return ptr;
  }
  //Set the device and profile name, send a message, link the profile, and finally start the device thread.
  ptr->set_name(proposal);
  devices.push_back(ptr);
  plugs.take_message("device " + source->get_name() + " added.");
  auto devprof = source->get_profile();
  devprof->name = source->get_name();
  add_profile(devprof.get());
  devprof->add_device(ptr);
  device_list_lock.unlock();
  ptr->start_thread();
  return ptr;
}

void moltengamepad::remove_device(input_source* source) {
  device_list_lock.lock();
  
  for (int i = 0; i < devices.size(); i++) {
    if (source == devices[i].get()) {
      plugs.take_message("device " + source->get_name() + " removed.");
      remove_profile(devices[i]->get_profile().get());
      devices.erase(devices.begin() + i);
      i--;
    }
  }
  device_list_lock.unlock();
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

int read_bool(const std::string value, std::function<void (bool)> process) {
  if (value == "true") {
    process(true);
    return 0;
  }
  if (value == "false") {
    process(false);
    return 0;
  }
  return -1;
}

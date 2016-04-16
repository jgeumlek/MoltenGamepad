#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>
#include <glob.h>
#include <devices/wiimote/wiimote.h>
#include <devices/generic/generic.h>


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

moltengamepad::~moltengamepad() {

  gamepad = nullptr;

  udev.set_managers(nullptr);
  if (udev.monitor_thread) {
    udev.stop_thread = true;
    int signal = 0;
    write(udev.pipe_fd, &signal, sizeof(signal));
    udev.monitor_thread->join();
    delete udev.monitor_thread;
    udev.monitor_thread = nullptr;
  }
  if(!options.daemon)
    std::cout << "Shutting down." << std::endl;

  std::ofstream fifo;
  fifo.open(options.fifo_path, std::ostream::out);
  unlink(options.fifo_path.c_str());
  fifo << "quit" << std::endl;
  if (remote_handler) {
    remote_handler->join();
    delete remote_handler;
  }

  for (auto it = managers.begin(); it != managers.end(); ++it) {
    delete(*it);
  }


  devices.clear();

  delete slots;
}

std::string find_config_folder() {
  const char* config_home = getenv("XDG_CONFIG_HOME");
  if (!config_home || config_home[0] == '\0') {
    return (std::string(getenv("HOME")) + std::string("/.config/moltengamepad"));
  }
  return std::string(config_home) + "/moltengamepad";
};



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

  //figure out config folders
  if (options.config_dir.empty()) options.config_dir = find_config_folder();

  mkdir(options.config_dir.c_str(), 0770);

  if (options.profile_dir.empty()) options.profile_dir = options.config_dir + "/profiles/";
  mkdir((options.profile_dir).c_str(), 0770);

  if (options.gendev_dir.empty()) options.gendev_dir = options.config_dir + "/gendevices/";
  mkdir((options.gendev_dir).c_str(), 0770);

  //file glob the gendev .cfg files to add more drivers
  glob_t globbuffer;
  std::string fileglob = options.gendev_dir + "/*.cfg";
  glob(fileglob.c_str(), 0, nullptr, &globbuffer);

  for (int i = 0; i < globbuffer.gl_pathc; i++) {
    std::ifstream file;
    file.open(globbuffer.gl_pathv[i], std::istream::in);

    if (!file.fail()) {
      int ret = generic_config_loop(this, file);
      if (ret) errors.take_message("generic device config " + std::string(globbuffer.gl_pathv[i]) + " failed.");
    }
  }

  globfree(&globbuffer);

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
    const char* run_dir = getenv("XDG_RUNTIME_DIR");
    if (options.fifo_path.empty() && run_dir) {
      options.fifo_path = std::string(run_dir) + "/moltengamepad";
    }
    if (options.fifo_path.empty()) {
      errors.take_message("Could not locate fifo path. Use the --fifo-path command line argument.");
      throw - 1;
    }
    int ret = mkfifo(options.fifo_path.c_str(), 0666);
    if (ret < 0)  {
      perror("making fifo:");
      options.fifo_path = "";
      throw - 1;
    } else {
      remote_handler = new std::thread(fifo_loop, this);
    }
  }

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

    if (!strcmp((*it)->name.c_str(), name)) return (*it);
  }
  return nullptr;
}
std::shared_ptr<input_source> moltengamepad::add_device(input_source* source) {
  std::shared_ptr<input_source> ptr(source);
  device_list_lock.lock();
  devices.push_back(ptr);
  plugs.take_message("device " + source->name + " added.");
  auto devprof = source->get_profile();
  devprof->name = source->name;
  add_profile(devprof.get());
  device_list_lock.unlock();
  return ptr;
}

void moltengamepad::remove_device(input_source* source) {
  device_list_lock.lock();
  plugs.take_message("device " + source->name + " removed.");
  for (int i = 0; i < devices.size(); i++) {
    if (source == devices[i].get()) {
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

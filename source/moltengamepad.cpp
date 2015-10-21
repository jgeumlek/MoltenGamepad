#include "moltengamepad.h"
#include <iostream>
#include <sys/stat.h>

moltengamepad::moltengamepad() {
  slots = new slot_manager();
}

moltengamepad::~moltengamepad() {
  std::cout<< "Shutting down."<< std::endl;
  delete slots;
  udev.set_managers(nullptr);
  if (udev.monitor_thread) {
    udev.stop_thread = true;
    int signal = 0;
    write(udev.pipe_fd, &signal, sizeof(signal));
    udev.monitor_thread->join();
    delete udev.monitor_thread;
    udev.monitor_thread = nullptr;
  }
  for (auto it = devs.begin(); it != devs.end(); ++it) {
    delete (*it);
  }
}

std::string find_config_folder() {
  const char *config_home = getenv("XDG_CONFIG_HOME");
  if (!config_home || config_home[0] == '\0') {
    return (std::string(getenv("HOME")) + std::string("/.config/"));
  }
};

std::string find_profile_folder() {
  return "";
};

int moltengamepad::init() {
  
  devs.push_back( new wiimotes(slots));

  udev.set_managers(&devs);
  udev.start_monitor();
  udev.enumerate();
  if (options.config_dir.empty()) options.config_dir = find_config_folder();
  
  std::cout<< options.config_dir+"/moltengamepad"<< std::endl;
  mkdir((options.config_dir + "/moltengamepad").c_str(),0770);
  
  options.config_dir = options.config_dir + "/moltengamepad/";
  options.profile_dir = options.config_dir + "/profiles/";
  mkdir((options.profile_dir).c_str(),0770);
}



device_manager* moltengamepad::find_manager(const char* name) {
  for (auto it = devs.begin(); it != devs.end(); it++) {
    if (!strcmp((*it)->name,name)) return (*it);
  }
  return nullptr;
}

input_source* moltengamepad::find_device(const char* name) {
  for (auto it = devs.begin(); it != devs.end(); it++) {
    input_source* dev = (*it)->find_device(name);
    if (dev) return dev;
  }
  return nullptr;
}

#include "moltengamepad.h"
#include <iostream>

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

int moltengamepad::init() {
  
  devs.push_back( new wiimotes(slots));

  udev.set_managers(&devs);
  udev.start_monitor();
  udev.enumerate();

  

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

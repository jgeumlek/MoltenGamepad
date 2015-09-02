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

  int what;
  std::cin >> what;



}

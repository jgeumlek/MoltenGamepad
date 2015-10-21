#ifndef MOLTENGAMEPAD_H
#define MOLTENGAMEPAD_H

#include <vector>
#include <thread>
#include <string>
#include <devices/device.h>
#include <devices/wiimote.h>
#include "slot_manager.h"
#include "uinput.h"
#include "udev.h"

class moltengamepad {
public:
  struct mg_options {
    bool look_for_devices = true;
    bool listen_for_devices = true;
    bool make_keyboard = false;
    bool make_mouse = false;
    bool make_pointer = false;
    int  num_gamepads = 4;
    std::string config_dir;
    std::string profile_dir;
    
  } options;

  std::vector<device_manager*> devs;
  slot_manager* slots;
  udev_handler udev;

  moltengamepad();
  ~moltengamepad();
  int init();
  int stop();
  
  device_manager* find_manager(const char* name);
  input_source* find_device(const char* name);

private:
  bool udev_loop = true;
  void udev_run();
  std::thread* udev_thread();


};


#endif

#ifndef MOLTENGAMEPAD_H
#define MOLTENGAMEPAD_H

#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include "devices/device.h"
#include "slot_manager.h"
#include "uinput.h"
#include "udev.h"
#include "signalflags.h"
#include "messages.h"

#define VERSION_STRING "alpha"

class slot_manager;
class device_manager;
class udev_handler;
class input_source;


class moltengamepad {
public:
  struct mg_options {
    bool look_for_devices = true;
    bool listen_for_devices = true;
    bool make_fifo = false;
    bool make_keyboard = true;
    bool make_mouse = false;
    bool make_pointer = false;
    bool dpad_as_hat = false;
    bool mimic_xpad = false;
    int  num_gamepads = 4;
    std::string config_dir;
    std::string profile_dir;
    std::string gendev_dir;
    std::string fifo_path;
    std::string uinput_path;

  } options;

  std::vector<device_manager*> managers;
  std::vector<std::shared_ptr<input_source>> devices;
  slot_manager* slots;
  udev_handler udev;
  simple_messenger drivers;
  simple_messenger plugs;
  simple_messenger errors;

  moltengamepad() : drivers("driver"), plugs("hotplug"), errors("error") {};
  moltengamepad(moltengamepad::mg_options options) : drivers("driver"), plugs("hotplug"), errors("error"), options(options) {};
  ~moltengamepad();
  int init();
  int stop();

  device_manager* find_manager(const char* name);
  std::shared_ptr<input_source> find_device(const char* name);
  void add_device(input_source* source);
  void remove_device(input_source* source);
  void for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func);

private:
  bool udev_loop = true;

  void udev_run();
  std::thread* udev_thread;
  std::thread* remote_handler = nullptr;
  std::mutex  device_list_lock;


};



int shell_loop(moltengamepad* mg, std::istream& in);


#endif

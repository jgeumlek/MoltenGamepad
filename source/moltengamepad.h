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
#include "profile.h"

#define VERSION_STRING "alpha"

class slot_manager;
class device_manager;
class udev_handler;
class input_source;
class profile;

//We have a few different file types, each in their own dirs.
//need to handle handles:
//  -following XDG spec, including systemwide fallback folders (/etc/xdg/moltengamepad)
//  -overrides given via commandline
enum file_category {
  FILE_CONFIG,       //The root config folder
  FILE_PROFILE,      //Profiles, aka mappings.
  FILE_GENDEV,       //Generic driver descriptors
  FILE_DRIVER_SET,   //Driver-specific settings (FUTURE)
  FILE_DEVICE_SET,   //Device-specific settings (FUTURE)
};


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
    bool daemon = false;
    int  num_gamepads = 4;
    std::string config_dir;
    std::string profile_dir;
    std::string gendev_dir;
    std::string fifo_path;
    std::string uinput_path;
    std::string pidfile;

  } options;

  std::vector<device_manager*> managers;
  std::vector<std::shared_ptr<input_source>> devices;
  std::vector<std::weak_ptr<profile>> profiles;
  slot_manager* slots;
  udev_handler udev;
  simple_messenger drivers;
  simple_messenger plugs;
  simple_messenger errors;
  std::shared_ptr<profile> gamepad = std::make_shared<profile>();

  moltengamepad() : drivers("driver"), plugs("hotplug"), errors("error") {};
  moltengamepad(moltengamepad::mg_options options) : drivers("driver"), plugs("hotplug"), errors("error"), options(options) {};
  ~moltengamepad();
  int init();
  int stop();

  //simply report the location of a file to be read
  //Use an empty path to get the appropriate directory to create a file in.
  std::string locate(file_category cat, std::string path);
  std::vector<std::string> locate_glob(file_category cat, std::string pathglob);

  device_manager* find_manager(const char* name);
  std::shared_ptr<input_source> find_device(const char* name);
  std::shared_ptr<input_source> add_device(input_source* source, device_manager* manager, std::string name_stem);
  void remove_device(input_source* source);
  void for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func);

  std::shared_ptr<profile> find_profile(const std::string& name);
  void add_profile(profile* profile);
  void remove_profile(profile* source);
  void for_all_profiles(std::function<void (std::shared_ptr<profile>&)> func);

private:
  
  std::thread* remote_handler = nullptr;
  std::mutex  device_list_lock;
  std::mutex  profile_list_lock;
  std::vector<std::string> xdg_config_dirs;


};



int shell_loop(moltengamepad* mg, std::istream& in);


#endif

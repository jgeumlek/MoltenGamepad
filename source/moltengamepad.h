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
class options;

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



extern const option_info general_options[];

class moltengamepad {
public:
  
  std::vector<device_manager*> managers;
  std::vector<std::shared_ptr<input_source>> devices;
  std::vector<std::weak_ptr<profile>> profiles;
  slot_manager* slots;
  udev_handler udev;
  simple_messenger drivers;
  simple_messenger plugs;
  simple_messenger errors;
  options* opts;
  std::shared_ptr<profile> gamepad = std::make_shared<profile>();

  moltengamepad(options* opts) : drivers("driver"), plugs("hotplug"), errors("error"), opts(opts) {};
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

//non-options that are also read from the config file
struct config_extras {
  std::vector<std::string> startup_profiles; //profiles to load at start up
};

struct context {
  int line_number;
  std::string path;
};


int shell_loop(moltengamepad* mg, std::istream& in);



struct token;

int loop_file(const std::string path, std::function<int (std::vector<token>&, context)>);

#endif

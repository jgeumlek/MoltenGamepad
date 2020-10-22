#ifndef MOLTENGAMEPAD_H
#define MOLTENGAMEPAD_H

#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <unordered_set>
#include "devices/device.h"
#include "slot_manager.h"
#include "uinput.h"
#include "udev.h"
#include "signalflags.h"
#include "protocols/message_stream.h"
#include "profile.h"
#include "plugin_loader.h"
#include "protocols/protocols.h"

#define VERSION_STRING "1.2.1"


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
  FILE_OPTIONS,      //Global options, each file is a category.
  FILE_DEVICE_SET,   //Device-specific settings (FUTURE)
  FILE_PLUGIN,       //Plugin .so files, such as drivers.
};



extern const option_decl general_options[];

class moltengamepad {
public:
  
  std::vector<device_manager*> managers;
  std::vector<std::shared_ptr<input_source>> devices;
  std::vector<std::weak_ptr<profile>> profiles;
  std::unordered_set<std::string> forbidden_ids; //keywords can't be ids (avoid ambiguity)
  std::unordered_set<std::string> ids_in_use; //prevent duplicate names to avoid ambiguity
  slot_manager* slots = nullptr;
  udev_handler udev;
  message_protocol* stdout = nullptr;
  message_stream drivers;
  message_stream plugs;
  options* opts;
  int sock = -1; //socket fd
  std::shared_ptr<profile> gamepad = std::make_shared<profile>();

  moltengamepad(options* opts) : drivers("driver"), plugs("hotplug"), opts(opts) {};
  ~moltengamepad();
  int init();
  int stop();

  //simply report the location of a file to be read
  //Use an empty path to get the appropriate directory to create a file in.
  std::string locate(file_category cat, std::string path);
  std::vector<std::string> locate_glob(file_category cat, std::string pathglob);


  device_manager* add_manager(manager_plugin manager, void* manager_plug_data);
  device_manager* find_manager(const char* name) const;
  std::shared_ptr<input_source> find_device(const char* name) const;
  std::shared_ptr<input_source> add_device(input_source* source, device_manager* manager, std::string name_stem);
  std::shared_ptr<input_source> add_device(device_manager* manager, device_plugin dev, void* dev_plug_data);
  int remove_device(input_source* source);
  void for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func);

  std::shared_ptr<profile> find_profile(const std::string& name);
  void add_profile(profile* profile);
  void remove_profile(profile* source);
  void for_all_profiles(std::function<void (std::shared_ptr<profile>&)> func);
  int set_option(std::string& category, std::string& name, std::string& value);
  void list_options(std::string& category, std::vector<option_info>& list) const;

private:
  
  std::thread* remote_handler = nullptr;
  mutable std::mutex  device_list_lock;
  mutable std::mutex  profile_list_lock;
  mutable std::mutex  id_list_lock;
  std::vector<std::string> xdg_config_dirs;
  void run_on_options(std::string& category, std::function<void (options*)> func) const;


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

void escape_string(std::string& string);


struct token;

int loop_file(const std::string path, std::function<int (std::vector<token>&, context)>);

#endif

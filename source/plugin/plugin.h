#pragma once
#include "../core/mg_types.h"
#include <libudev.h>


class input_source;
class device_manager;

struct device_methods {
  void* (*plug_data) (const input_source* dev);
  int (*watch_file) (input_source* dev, int fd, void* tag);
  int (*toggle_event) (input_source* dev, int id, event_state state);
  int (*send_value) (input_source* dev, int id, int64_t value);
  int (*send_syn_report) (input_source* dev);
  int (*remove_option) (input_source* dev, const char* opname);
};

struct device_plugin {
  const char* name_stem;
  const char* uniq;
  const char* phys;
  int (*init) (input_source* dev);
  int (*destroy) (void* plug_data);
  const char* (*get_description) (const input_source* dev);
  const char* (*get_type) (const input_source* dev);
  int (*process_event) (input_source* dev, void* tag);
  int (*process_option) (input_source* dev, const char* opname, MGField opvalue);
};

struct manager_methods {
  void* (*plug_data) (const device_manager*);
  int (*register_event) (device_manager*, event_decl ev);
  int (*register_dev_option) (device_manager*, option_decl opt);
  int (*register_alias) (device_manager*, const char* external, const char* local);
  int (*register_manager_option) (device_manager*, option_decl opt);
  input_source* (*add_device) (device_manager*, device_plugin, void* dev_plug_data);
  int (*remove_device) (device_manager*, input_source*);
  int (*print) (device_manager*, const char* message);
};

struct manager_plugin {
  const char* name;
  bool subscribe_to_gamepad_profile;
  int (*init) (device_manager*);
  int (*destroy) (void* plug_data);
  int (*start) (device_manager*);
  int (*process_udev_event) (device_manager*, struct udev* udev, struct udev_device* dev);
  int (*process_manager_option) (device_manager*, const char* opname, MGField opvalue);
};

class moltengamepad;

struct moltengamepad_methods {
  device_manager* (*add_manager) (moltengamepad*, manager_plugin, void* manager_plug_data);
  int (*request_slot) (moltengamepad*, input_source*);
};

struct plugin_api {
  struct moltengamepad_methods mg;
  struct manager_methods manager;
  struct device_methods device;
};

int register_plugin( int (*init) (moltengamepad*, plugin_api));

extern int (*plugin_init) (moltengamepad*, plugin_api);
  
  
  

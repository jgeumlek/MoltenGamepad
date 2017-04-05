#include "example_plugin.h"
#include <cstring>

//This file provides the entry point that
//fills out the needed function pointers.

//A plugin should change the manager .name
//and device .name_stem fields, as well
//as the device get_description().

//Not many other changes should be needed here, other
//than changing variable names as needed.

device_plugin example_dev;

PLUGIN_INIT(example)(plugin_api api) {
  if (!API_EXACT(api))
    return -1;
  //set static vars
  example_manager::methods = *(api.head.manager);
  example_device::methods = *(api.head.device);
  example_manager* manager = new example_manager();

  //set manager call backs
  manager_plugin example_man;
  memset(&example_man, 0, sizeof(example_man));
  example_man.size = sizeof(example_man);
  example_man.name = example_manager::name;
  example_man.subscribe_to_gamepad_profile = true;
  example_man.init = [] (void* plug_data, device_manager* ref) -> int {
    return ((example_manager*)plug_data)->init(ref);
  };
  example_man.destroy = [] (void* data) -> int {
    delete (example_manager*) data;
    return 0;
  };
  example_man.start = [] (void* data) { 
    return ((example_manager*)data)->start();
  };
  example_man.process_manager_option = [] (void* ref, const char* opname, MGField opvalue) {
    return ((example_manager*)ref)->process_option(opname, opvalue);
  };
  example_man.process_udev_event = [] (void* data, struct udev* udev, struct udev_device* dev) {
    return ((example_manager*)data)->accept_device(udev, dev);
  };

  //set device call backs
  memset(&example_dev, 0, sizeof(example_dev));
  example_dev.size = sizeof(example_dev);
  example_dev.name_stem = example_device::name_stem;
  example_dev.uniq = "";
  example_dev.phys = "";
  example_dev.init = [] (void* data, input_source* ref) -> int {
    return ((example_device*)data)->init(ref);
  };
  example_dev.destroy = [] (void* data) -> int {
    delete (example_device*) data;
    return 0;
  };
  example_dev.get_description = [] (const void* data) {
    return "Example Plugin Device";
  };
  example_dev.get_type = [] (const void* data) {
    return "gamepad";
  };
  example_dev.process_event = [] (void* data, void* tag) -> int {
    ((example_device*)data)->process(tag);
    return 0;
  };
  example_dev.process_option = [] (void* data, const char* opname, MGField opvalue) {
    return ((example_device*)data)->process_option(opname, opvalue);
  };
  example_dev.upload_ff = nullptr;
  example_dev.erase_ff = nullptr;
  example_dev.play_ff = nullptr;

  api.mg.add_manager(example_man,  manager);
  return 0;
}

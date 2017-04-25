#include "joycon.h"

//This file provides the entry point that
//fills out the needed function pointers.

//A plugin should change the manager .name
//and device .name_stem fields, as well
//as the device get_description().

//Not many other changes should be needed here, other
//than changing variable names as needed.

device_plugin joycon_dev;

//sending this request results in getting the JoyCon state back for processing.
const uint8_t request_state[] = {1,0};


PLUGIN_INIT(joycon)(plugin_api api) {
  if (!API_EXACT(api))
    return -1;
  //set static vars
  joycon_manager::methods = *(api.head.manager);
  joycon::methods = *(api.head.device);
  joycon_manager::grab_permissions = api.head.mg->grab_permissions;
  joycon_manager* manager = new joycon_manager();

  //set manager call backs
  manager_plugin joycon_man;
  memset(&joycon_man, 0, sizeof(joycon_man));
  joycon_man.size = sizeof(joycon_man);
  joycon_man.name = joycon_manager::name;
  joycon_man.subscribe_to_gamepad_profile = true;
  joycon_man.init = [] (void* plug_data, device_manager* ref) -> int {
    return ((joycon_manager*)plug_data)->init(ref);
  };
  joycon_man.destroy = [] (void* data) -> int {
    delete (joycon_manager*) data;
    return 0;
  };
  joycon_man.start = [] (void* data) { 
    return ((joycon_manager*)data)->start();
  };
  joycon_man.process_manager_option = [] (void* ref, const char* opname, MGField opvalue) {
    return ((joycon_manager*)ref)->process_option(opname, opvalue);
  };
  joycon_man.process_udev_event = [] (void* data, struct udev* udev, struct udev_device* dev) {
    return ((joycon_manager*)data)->accept_device(udev, dev);
  };

  //set device call backs
  memset(&joycon_dev, 0, sizeof(joycon_dev));
  joycon_dev.size = sizeof(joycon_dev);
  joycon_dev.name_stem = joycon::name_stem;
  joycon_dev.uniq = "";
  joycon_dev.phys = "";
  joycon_dev.init = [] (void* data, input_source* ref) -> int {
    return ((joycon*)data)->init(ref);
  };
  joycon_dev.destroy = [] (void* data) -> int {
    delete (joycon*) data;
    return 0;
  };
  joycon_dev.get_description = [] (const void* data) {
    return ((joycon*)data)->get_description();
  };
  joycon_dev.get_type = [] (const void* data) {
    return "gamepad";
  };
  joycon_dev.process_event = [] (void* data, void* tag) -> int {
    ((joycon*)data)->process(tag);
    return 0;
  };
  joycon_dev.process_option = [] (void* data, const char* opname, MGField opvalue) {
    return ((joycon*)data)->process_option(opname, opvalue);
  };
  joycon_dev.upload_ff = nullptr;
  joycon_dev.erase_ff = nullptr;
  joycon_dev.play_ff = nullptr;

  joycon_dev.process_recurring_event = [] (void* data) -> int {
    ((joycon*)data)->process_recurring_event();
    return 0;
  };

  api.mg.add_manager(joycon_man,  manager);
  return 0;
}

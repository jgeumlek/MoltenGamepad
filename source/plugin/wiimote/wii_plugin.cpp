#include "wiimote.h"

device_plugin wiidev;

int wiimote_plugin_init(plugin_api api) {
  //set static vars
  wiimote_manager::methods = api.manager;
  wiimote::methods = api.device;
  wiimote_manager::request_slot = api.mg.request_slot;
  wiimote_manager::grab_permissions = api.mg.grab_permissions;
  wiimote_manager* manager = new wiimote_manager();

  //set manager call backs
  manager_plugin wiiman;
  wiiman.name = "wiimote";
  wiiman.subscribe_to_gamepad_profile = true;
  wiiman.init = [] (void* wm, device_manager* ref) -> int {
    return ((wiimote_manager*)wm)->init(ref);
  };
  wiiman.destroy = [] (void* data) -> int {
    delete (wiimote_manager*) data;
    return 0;
  };
  wiiman.start = [] (void*) { return 0;};
  wiiman.process_manager_option = [] (void* ref, const char* opname, MGField opvalue) {
    return ((wiimote_manager*)ref)->process_manager_option(opname, opvalue);
  };
  wiiman.process_udev_event = [] (void* ref, struct udev* udev, struct udev_device* dev) {
    return ((wiimote_manager*)ref)->accept_device(udev, dev);
  };

  //set device callbacks
  wiidev.name_stem = "wm";
  wiidev.uniq = "";
  wiidev.phys = "";
  wiidev.init = [] (void* wm, input_source* ref) -> int {
    ((wiimote*)wm)->ref = ref;
    return 0;
  };
  wiidev.destroy = [] (void* data) -> int {
    delete (wiimote*) data;
    return 0;
  };
  wiidev.get_description = [] (const void* ref) {
    return ((const wiimote*)ref)->get_description();
  };
  wiidev.get_type = [] (const void* ref) {
    return ((const wiimote*)ref)->get_type();
  };
  wiidev.process_event = [] (void* ref, void* tag) -> int {
    ((wiimote*)ref)->process(tag);
    return 0;
  };
  wiidev.process_option = [] (void* ref, const char* opname, MGField opvalue) {
    return ((wiimote*)ref)->process_option(opname, opvalue);
  };

  api.mg.add_manager(wiiman, manager);
  return 0;
}



int wiimote_loaded = register_plugin(&wiimote_plugin_init);

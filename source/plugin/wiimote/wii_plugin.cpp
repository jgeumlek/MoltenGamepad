#include "wiimote.h"

moltengamepad_methods mg_methods;
moltengamepad* mg;
device_plugin wiidev;

wiimote_manager* get_man(device_manager* ref) {
  return (wiimote_manager*) wiimote_manager::methods.plug_data(ref);
}

wiimote* get_dev(input_source* ref) {
  return (wiimote*) wiimote::methods.plug_data(ref);
}

const wiimote* get_const_dev(const input_source* ref) {
  return (const wiimote*) wiimote::methods.plug_data(ref);
}
  

int wiimote_plugin_init(moltengamepad* mg_ref, plugin_api api) {
  //set static vars
  mg = mg_ref;
  mg_methods = api.mg;
  wiimote_manager::methods = api.manager;
  wiimote::methods = api.device;
  wiimote_manager* manager = new wiimote_manager();

  //set manager call backs
  manager_plugin wiiman;
  wiiman.name = "wiimote";
  wiiman.subscribe_to_gamepad_profile = true;
  wiiman.init = [] (device_manager* ref) -> int {
    wiimote_manager* wm = get_man(ref);
    return wm->init(ref);
  };
  wiiman.destroy = [] (void* data) -> int {
    delete (wiimote_manager*) data;
    return 0;
  };
  wiiman.start = [] (device_manager*) { return 0;};
  wiiman.process_manager_option = nullptr;
  wiiman.process_udev_event = [] (device_manager* ref, struct udev* udev, struct udev_device* dev) {
    return get_man(ref)->accept_device(udev, dev);
  };

  wiidev.name_stem = "wm";
  wiidev.uniq = "";
  wiidev.phys = "";
  wiidev.init = [] (input_source* ref) -> int {
    get_dev(ref)->ref = ref;
    return 0;
  };
  wiidev.destroy = [] (void* data) -> int {
    delete (wiimote*) data;
    return 0;
  };
  wiidev.get_description = [] (const input_source* ref) {
    return get_const_dev(ref)->get_description();
  };
  wiidev.get_type = [] (const input_source* ref) {
    return get_const_dev(ref)->get_type();
  };
  wiidev.process_event = [] (input_source* ref, void* tag) -> int {
    get_dev(ref)->process(tag);
    return 0;
  };
  wiidev.process_option = [] (input_source* ref, const char* opname, MGField opvalue) {
    return get_dev(ref)->process_option(opname, opvalue);
  };

  mg_methods.add_manager(mg_ref, wiiman,  manager);
  return 0;
}


int wiimote_loaded = register_plugin(&wiimote_plugin_init);

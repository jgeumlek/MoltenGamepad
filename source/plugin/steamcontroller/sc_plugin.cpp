#include "steam_controller.h"
#include <cstring>

device_plugin scdev;

PLUGIN_INIT(steamcontroller)(plugin_api api) {
  if (!API_EXACT(api))
    return -1;
  //set static vars
  steam_controller_manager::methods = *(api.head.manager);
  steam_controller::methods = *(api.head.device);
  steam_controller_manager* manager = new steam_controller_manager();

  //set manager call backs
  manager_plugin scman;
  memset(&scman, 0, sizeof(scman));
  scman.size = sizeof(scman);
  scman.name = "steamcontroller";
  scman.subscribe_to_gamepad_profile = true;
  scman.init = [] (void* plug_data, device_manager* ref) -> int {
    return ((steam_controller_manager*)plug_data)->init(ref);
  };
  scman.destroy = [] (void* data) -> int {
    delete (steam_controller_manager*) data;
    return 0;
  };
  scman.start = [] (void* data) { 
    return ((steam_controller_manager*)data)->start();
  };
  scman.process_manager_option = nullptr;
  scman.process_udev_event = [] (void* data, struct udev* udev, struct udev_device* dev) {
    return -1; //This driver doesn't use udev events!
  };

  //set device call backs
  memset(&scdev, 0, sizeof(scdev));
  scdev.size = sizeof(scdev);
  scdev.name_stem = "sc";
  scdev.uniq = "";
  scdev.phys = "";
  scdev.init = [] (void* data, input_source* ref) -> int {
    steam_controller* sc = ((steam_controller*)data);
    sc->ref = ref;
    steam_controller::methods.watch_file(ref, sc->statepipe[0], sc->statepipe);
    return 0;
  };
  scdev.destroy = [] (void* data) -> int {
    delete (steam_controller*) data;
    return 0;
  };
  scdev.get_description = [] (const void* data) {
    return "Steam Controller";
  };
  scdev.get_type = [] (const void* data) {
    return "gamepad";
  };
  scdev.process_event = [] (void* data, void* tag) -> int {
    ((steam_controller*)data)->process(tag);
    return 0;
  };
  scdev.process_option = [] (void* data, const char* opname, MGField opvalue) {
    return ((steam_controller*)data)->process_option(opname, opvalue);
  };
  scdev.upload_ff = nullptr;
  scdev.erase_ff = nullptr;
  scdev.play_ff = nullptr;

  api.mg.add_manager(scman,  manager);
  return 0;
}

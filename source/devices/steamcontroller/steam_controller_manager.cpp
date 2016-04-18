#ifdef BUILD_STEAM_CONTROLLER_DRIVER
#include "steam_controller.h"

steam_controller_manager::steam_controller_manager(moltengamepad* mg) : device_manager(mg,"steamcontroller"), sc_context(std::bind(&steam_controller_manager::on_controller_gained, this, std::placeholders::_1), std::bind(&steam_controller_manager::on_controller_lost, this, std::placeholders::_1)) {

  init_profile();
  //need to do a thread for hot plugs, since these don't go through udev.
  keep_scanning = true;

  sc_context_thread = new std::thread([this] () {
    while (keep_scanning)
    sc_context.handle_events(1000);
  });

}

input_source* steam_controller_manager::find_device(const char* name) {
  for (auto dev : sc_devs) {
    if (!strcmp(dev.second->name.c_str(), name)) return dev.second;
  }
  return nullptr;
}



enum entry_type steam_controller_manager::entry_type(const char* name) {
  auto alias = mapprofile->get_alias(std::string(name));
  if (!alias.empty())
    name = alias.c_str();
  int ret = lookup_steamcont_event(name);
  if (ret != -1) {
    return steamcont_events[ret].type;
  }

  return NO_ENTRY;
}

void steam_controller_manager::on_controller_gained(scraw::controller sc) {
  scraw::controller* ref = new scraw::controller(sc);
  steam_controller* steamcont = new steam_controller(ref, mg->slots, this);
  sc_devs[sc] = steamcont;
  mg->add_device(steamcont, this, "sc");
  std::shared_ptr<profile> devprofile = steamcont->get_profile();
  mapprofile->copy_into(devprofile,true);
}
void steam_controller_manager::on_controller_lost(scraw::controller sc) {
  auto dev = sc_devs.find(sc);
  if (dev != sc_devs.end()) {
    mg->remove_device(dev->second);
    sc_devs.erase(dev);
  }
}


void steam_controller_manager::init_profile() {
  //Init some event translators
  auto map = &mapprofile->mapping;
  (*map)["a"] =    {new btn2btn(BTN_SOUTH), DEV_KEY};
  (*map)["b"] =    {new btn2btn(BTN_EAST), DEV_KEY};
  (*map)["x"] =    {new btn2btn(BTN_WEST), DEV_KEY};
  (*map)["y"] =    {new btn2btn(BTN_NORTH), DEV_KEY};
  (*map)["down"] = {new btn2btn(BTN_DPAD_DOWN), DEV_KEY};
  (*map)["up"] = {new btn2btn(BTN_DPAD_UP), DEV_KEY};
  (*map)["left"] =   {new btn2btn(BTN_DPAD_LEFT), DEV_KEY};
  (*map)["right"] = {new btn2btn(BTN_DPAD_RIGHT), DEV_KEY};
  (*map)["mode"] = {new btn2btn(BTN_MODE), DEV_KEY};
  (*map)["forward"] = {new btn2btn(BTN_START), DEV_KEY};
  (*map)["back"] = {new btn2btn(BTN_SELECT), DEV_KEY};

  (*map)["stick_click"] =    {new btn2btn(BTN_THUMBL), DEV_KEY};
  (*map)["left_pad_click"] =    {new btn2btn(BTN_THUMBL), DEV_KEY};
  (*map)["right_pad_click"] =    {new btn2btn(BTN_THUMBR), DEV_KEY};
  (*map)["tl"] =    {new btn2btn(BTN_TL), DEV_KEY};
  (*map)["tr"] =    {new btn2btn(BTN_TR), DEV_KEY};
  (*map)["tl2"] =   {new btn2btn(BTN_TL2), DEV_KEY};
  (*map)["tr2"] =   {new btn2btn(BTN_TR2), DEV_KEY};

  (*map)["lgrip"] = {new event_translator, DEV_KEY};
  (*map)["rgrip"] = {new event_translator, DEV_KEY};
  (*map)["left_pad_touch"] = {new event_translator, DEV_KEY};
  (*map)["right_pad_touch"] = {new event_translator, DEV_KEY};


  (*map)["stick_x"] = {new axis2axis(ABS_X, 1), DEV_AXIS};
  (*map)["stick_y"] = {new axis2axis(ABS_Y, 1), DEV_AXIS};

  (*map)["left_pad_x"] = {new axis2btns(BTN_DPAD_LEFT, BTN_DPAD_RIGHT), DEV_AXIS};
  (*map)["left_pad_y"] = {new axis2btns(BTN_DPAD_UP, BTN_DPAD_DOWN), DEV_AXIS};
  (*map)["right_pad_x"] = {new axis2axis(ABS_RX, 1), DEV_AXIS};
  (*map)["right_pad_y"] = {new axis2axis(ABS_RY, 1), DEV_AXIS};

  (*map)["tr2_axis"] = {new axis2axis(ABS_RZ, 1), DEV_AXIS};
  (*map)["tl2_axis"] = {new axis2axis(ABS_Z, 1), DEV_AXIS};
 
  //Init some aliases to act like a standardized game pad
  mapprofile->set_alias("primary","a");
  mapprofile->set_alias("secondary","b");
  mapprofile->set_alias("third","x");
  mapprofile->set_alias("fourth","y");
  mapprofile->set_alias("leftright","left_pad_x");
  mapprofile->set_alias("updown","left_pad_y");
  mapprofile->set_alias("start","forward");
  mapprofile->set_alias("select","back");
  mapprofile->set_alias("thumbl","stick_click");
  mapprofile->set_alias("thumbr","right_pad_click");
  mapprofile->set_alias("left_x","stick_x");
  mapprofile->set_alias("left_y","stick_y");
  mapprofile->set_alias("right_x","right_pad_x");
  mapprofile->set_alias("right_y","right_pad_y");
  
  mg->gamepad->copy_into(mapprofile, true);

};
#endif

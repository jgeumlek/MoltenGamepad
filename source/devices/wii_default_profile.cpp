#include "wiimote.h"
#include "../profile.h"

void wiimotes::init_profile() {
  auto map= &mapprofile.mapping;
  (*map)["wm_a"] =    new btn2btn(BTN_WEST);
  (*map)["wm_b"] =    new btn2btn(BTN_NORTH);
  (*map)["wm_left"] = new btn2btn(BTN_DPAD_DOWN);
  (*map)["wm_right"]= new btn2btn(BTN_DPAD_UP);
  (*map)["wm_up"] =   new btn2btn(BTN_DPAD_LEFT);
  (*map)["wm_down"] = new btn2btn(BTN_DPAD_RIGHT);
  (*map)["wm_1"] =    new btn2btn(BTN_EAST);
  (*map)["wm_2"] =    new btn2btn(BTN_SOUTH);
  (*map)["wm_home"] = new btn2btn(BTN_MODE);
  (*map)["wm_plus"] = new btn2btn(BTN_START);
  (*map)["wm_minus"]= new btn2btn(BTN_SELECT);
  
  (*map)["nk_a"] =    new btn2btn(BTN_SOUTH);
  (*map)["nk_b"] =    new btn2btn(BTN_EAST);
  (*map)["nk_left"] = new btn2btn(BTN_DPAD_LEFT);
  (*map)["nk_right"]= new btn2btn(BTN_DPAD_RIGHT);
  (*map)["nk_up"] =   new btn2btn(BTN_DPAD_UP);
  (*map)["nk_down"] = new btn2btn(BTN_DPAD_DOWN);
  (*map)["nk_1"] =    new btn2btn(BTN_TL);
  (*map)["nk_2"] =    new btn2btn(BTN_TR);
  (*map)["nk_home"] = new btn2btn(BTN_MODE);
  (*map)["nk_plus"] = new btn2btn(BTN_START);
  (*map)["nk_minus"]= new btn2btn(BTN_SELECT);
  (*map)["nk_c"] =    new btn2btn(BTN_NORTH);
  (*map)["nk_z"] =    new btn2btn(BTN_WEST);
  
  (*map)["cc_a"] =    new btn2btn(BTN_SOUTH);
  (*map)["cc_b"] =    new btn2btn(BTN_EAST);
  (*map)["cc_x"] =    new btn2btn(BTN_WEST);
  (*map)["cc_y"] =    new btn2btn(BTN_NORTH);
  (*map)["cc_left"] = new btn2btn(BTN_DPAD_LEFT);
  (*map)["cc_right"]= new btn2btn(BTN_DPAD_RIGHT);
  (*map)["cc_up"] =   new btn2btn(BTN_DPAD_UP);
  (*map)["cc_down"] = new btn2btn(BTN_DPAD_DOWN);
  (*map)["cc_home"] = new btn2btn(BTN_MODE);
  (*map)["cc_plus"] = new btn2btn(BTN_START);
  (*map)["cc_minus"]= new btn2btn(BTN_SELECT);
  (*map)["cc_l"] =    new btn2btn(BTN_TL);
  (*map)["cc_r"] =    new btn2btn(BTN_TR);
  (*map)["cc_zl"] =   new btn2btn(BTN_TL2);
  (*map)["cc_zr"] =   new btn2btn(BTN_TR2);
  
  
  (*map)["nk_stick_x"] = new axis2axis(ABS_X,1);
  (*map)["nk_stick_y"] = new axis2axis(ABS_Y,1);
 
  (*map)["cc_left_x"] = new axis2axis(ABS_X,1);
  (*map)["cc_left_y"] = new axis2axis(ABS_Y,1);
  (*map)["cc_right_x"] = new axis2axis(ABS_RX,1);
  (*map)["cc_right_y"] = new axis2axis(ABS_RY,1);
  
}

void wiimote::load_profile(profile* map) {
  memset(key_trans, 0, sizeof(key_trans));
  
  for (int i = 0; wiimote_events_keys[i].name != nullptr; i++) {
    key_trans[wiimote_events_keys[i].id] = map->copy_mapping(wiimote_events_keys[i].name);
  }
  
  for (int i = 0; wiimote_events_axes[i].name != nullptr; i++) {
    abs_trans[wiimote_events_axes[i].id] = map->copy_mapping(wiimote_events_axes[i].name);
  }
  
}
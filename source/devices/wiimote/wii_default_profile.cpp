#include "wiimote.h"
#include "../../profile.h"

void wiimotes::init_profile() {
  //Init some event translators
  auto map = &mapprofile.mapping;
  (*map)["wm_a"] =    {new btn2btn(BTN_WEST), DEV_KEY};
  (*map)["wm_b"] =    {new btn2btn(BTN_NORTH), DEV_KEY};
  (*map)["wm_left"] = {new btn2btn(BTN_DPAD_DOWN), DEV_KEY};
  (*map)["wm_right"] = {new btn2btn(BTN_DPAD_UP), DEV_KEY};
  (*map)["wm_up"] =   {new btn2btn(BTN_DPAD_LEFT), DEV_KEY};
  (*map)["wm_down"] = {new btn2btn(BTN_DPAD_RIGHT), DEV_KEY};
  (*map)["wm_1"] =    {new btn2btn(BTN_EAST), DEV_KEY};
  (*map)["wm_2"] =    {new btn2btn(BTN_SOUTH), DEV_KEY};
  (*map)["wm_home"] = {new btn2btn(BTN_MODE), DEV_KEY};
  (*map)["wm_plus"] = {new btn2btn(BTN_START), DEV_KEY};
  (*map)["wm_minus"] = {new btn2btn(BTN_SELECT), DEV_KEY};

  (*map)["nk_a"] =    {new btn2btn(BTN_SOUTH), DEV_KEY};
  (*map)["nk_b"] =    {new btn2btn(BTN_EAST), DEV_KEY};
  (*map)["nk_left"] = {new btn2btn(BTN_DPAD_LEFT), DEV_KEY};
  (*map)["nk_right"] = {new btn2btn(BTN_DPAD_RIGHT), DEV_KEY};
  (*map)["nk_up"] =   {new btn2btn(BTN_DPAD_UP), DEV_KEY};
  (*map)["nk_down"] = {new btn2btn(BTN_DPAD_DOWN), DEV_KEY};
  (*map)["nk_1"] =    {new btn2btn(BTN_TL), DEV_KEY};
  (*map)["nk_2"] =    {new btn2btn(BTN_TR), DEV_KEY};
  (*map)["nk_home"] = {new btn2btn(BTN_MODE), DEV_KEY};
  (*map)["nk_plus"] = {new btn2btn(BTN_START), DEV_KEY};
  (*map)["nk_minus"] = {new btn2btn(BTN_SELECT), DEV_KEY};
  (*map)["nk_c"] =    {new btn2btn(BTN_NORTH), DEV_KEY};
  (*map)["nk_z"] =    {new btn2btn(BTN_WEST), DEV_KEY};

  (*map)["cc_a"] =    {new btn2btn(BTN_SOUTH), DEV_KEY};
  (*map)["cc_b"] =    {new btn2btn(BTN_EAST), DEV_KEY};
  (*map)["cc_x"] =    {new btn2btn(BTN_WEST), DEV_KEY};
  (*map)["cc_y"] =    {new btn2btn(BTN_NORTH), DEV_KEY};
  (*map)["cc_left"] = {new btn2btn(BTN_DPAD_LEFT), DEV_KEY};
  (*map)["cc_right"] = {new btn2btn(BTN_DPAD_RIGHT), DEV_KEY};
  (*map)["cc_up"] =   {new btn2btn(BTN_DPAD_UP), DEV_KEY};
  (*map)["cc_down"] = {new btn2btn(BTN_DPAD_DOWN), DEV_KEY};
  (*map)["cc_home"] = {new btn2btn(BTN_MODE), DEV_KEY};
  (*map)["cc_plus"] = {new btn2btn(BTN_START), DEV_KEY};
  (*map)["cc_minus"] = {new btn2btn(BTN_SELECT), DEV_KEY};
  (*map)["cc_l"] =    {new btn2btn(BTN_TL), DEV_KEY};
  (*map)["cc_r"] =    {new btn2btn(BTN_TR), DEV_KEY};
  (*map)["cc_zl"] =   {new btn2btn(BTN_TL2), DEV_KEY};
  (*map)["cc_zr"] =   {new btn2btn(BTN_TR2), DEV_KEY};
  (*map)["cc_thumbl"] =   {new btn2btn(BTN_THUMBL), DEV_KEY};
  (*map)["cc_thumbr"] =   {new btn2btn(BTN_THUMBR), DEV_KEY};


  (*map)["nk_stick_x"] = {new axis2axis(ABS_X, 1), DEV_AXIS};
  (*map)["nk_stick_y"] = {new axis2axis(ABS_Y, 1), DEV_AXIS};

  (*map)["cc_left_x"] = {new axis2axis(ABS_X, 1), DEV_AXIS};
  (*map)["cc_left_y"] = {new axis2axis(ABS_Y, 1), DEV_AXIS};
  (*map)["cc_right_x"] = {new axis2axis(ABS_RX, 1), DEV_AXIS};
  (*map)["cc_right_y"] = {new axis2axis(ABS_RY, 1), DEV_AXIS};

  //Init some aliases to act like a standardized game pad
  mapprofile.set_alias("primary","cc_a");
  mapprofile.set_alias("secondary","cc_b");
  mapprofile.set_alias("third","cc_x");
  mapprofile.set_alias("fourth","cc_y");
  mapprofile.set_alias("left","cc_left");
  mapprofile.set_alias("right","cc_right");
  mapprofile.set_alias("up","cc_up");
  mapprofile.set_alias("down","cc_down");
  mapprofile.set_alias("mode","cc_home");
  mapprofile.set_alias("start","cc_plus");
  mapprofile.set_alias("select","cc_minus");
  mapprofile.set_alias("tl","cc_l");
  mapprofile.set_alias("tr","cc_r");
  mapprofile.set_alias("tl2","cc_zl");
  mapprofile.set_alias("tr2","cc_zr");
  mapprofile.set_alias("thumbl","cc_thumbl");
  mapprofile.set_alias("thumbr","cc_thumbr");
  mapprofile.set_alias("left_x","cc_left_x");
  mapprofile.set_alias("left_y","cc_left_y");
  mapprofile.set_alias("right_x","cc_right_x");
  mapprofile.set_alias("right_y","cc_right_y");

};


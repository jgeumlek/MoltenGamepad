#include "wiimote.h"
#include "../../profile.h"

void wiimotes::init_profile() {
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

};


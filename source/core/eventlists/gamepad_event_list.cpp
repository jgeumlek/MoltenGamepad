#include "eventlist.h"


const event_listing gamepad_btn_list[] = {
  {BTN_SOUTH, "first", "Primary face button (Confirm)"},
  {BTN_EAST, "second", "Second face button (Go Back)"},
  {BTN_WEST, "third", "Third face button"},
  {BTN_NORTH, "fourth", "Fourth face button"},
  {BTN_START, "start", "Start button"},
  {BTN_SELECT, "select", "Select button"},
  {BTN_MODE, "mode", "Special button, often with a logo"},
  {BTN_TL, "tl", "Upper left trigger"},
  {BTN_TL2, "tl2", "Lower left trigger"},
  {BTN_TR, "tr", "Upper right trigger"},
  {BTN_TR2, "tr2", "Lower left trigger"},
  {BTN_THUMBL, "thumbl", "Left thumb stick click"},
  {BTN_THUMBR, "thumbr", "Right thumb sitck click"},
#ifndef DPAD_AS_HAT
  {BTN_DPAD_UP, "up", "Up on the dpad"},
  {BTN_DPAD_DOWN, "down", "Down on the dpad"},
  {BTN_DPAD_LEFT, "left", "Left on the dpad"},
  {BTN_DPAD_RIGHT, "right", "Right on the dpad"},
#endif
  {BTN_SOUTH, "primary", "Primary face button (Confirm)"},
  {BTN_EAST, "secondary", "Second face button (Go Back)"},
  { -1, nullptr, nullptr},
};

const event_listing gamepad_alias[] = {
  { -2, "south", "primary"},
  { -2, "east", "secondary"},
  { -2, "west", "third"},
  { -2, "north", "fourth"},
};

const event_listing gamepad_axis_list[] = {
  {ABS_X, "left_x", "Left stick X-axis"},
  {ABS_Y, "left_y", "Left stick Y-axis"},
  {ABS_RX, "right_x", "Right stick X-axis"},
  {ABS_RY, "right_y", "Right stick Y-axis"},
  {ABS_Z, "tl2_axis", "Analog lower left trigger"},
  {ABS_RZ, "tr2_axis", "Analog lower right trigger"},
#ifndef DPAD_AS_HAT
  {ABS_HAT0X, "leftright", "left/right on the dpad"},
  {ABS_HAT0Y, "updown", "up/own on the dpad"},
#endif
  { -1, nullptr, nullptr},
};

#pragma once
#include "virtual_device.h"

struct virtpad_settings {
  uinput_ids u_ids;
  bool dpad_as_hat;
  bool analog_triggers;
  bool rumble;
};

class virtual_gamepad : public virtual_device {
public:
  bool dpad_as_hat = false;
  bool analog_triggers = false;
  virtual_gamepad(std::string name, std::string descr, virtpad_settings settings, slot_manager* slot_man, uinput* ui);
  virtual ~virtual_gamepad();
  virtual void take_event(struct input_event in);
  virtual void clear_outputs();
  virtual void init();
protected:

  virtpad_settings padstyle;

  int face_1234[4] = {BTN_SOUTH, BTN_EAST, BTN_WEST, BTN_NORTH};
  virtual void destroy_uinput_devs();
  int uinput_fd;

};

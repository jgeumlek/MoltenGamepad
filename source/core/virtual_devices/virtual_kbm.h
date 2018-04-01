#pragma once
#include "virtual_device.h"


class virtual_keyboard : public virtual_device {
public:
  virtual_keyboard(std::string name, std::string descr, uinput_ids keyboard_ids, uinput_ids mouse_ids, slot_manager* slot_man, uinput* ui);
  virtual void take_event(struct input_event in);
  virtual ~virtual_keyboard();

protected:

  uinput_ids kb_ids;
  uinput_ids rel_mouse_ids;
  int kb_fd = -1;
  int rel_mouse_fd = -1;
  virtual void destroy_uinput_devs();
};


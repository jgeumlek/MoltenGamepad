#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include "uinput.h"
#include "eventlists/eventlist.h"
#include <iostream>
#include <string>
#include <map>

#define OPTION_ACCEPTED 0


class virtual_device {
public:
  std::string name;
  std::string descr;
  virtual_device(std::string name) : name(name) {};
  virtual_device(std::string name, std::string descr) : name(name), descr(descr) {};
  virtual ~virtual_device();
  virtual void take_event(struct input_event in) {
  }

  void update_option(std::string option, std::string value) {
    if (options.find(option) == options.end()) return;
    if (process_option(option, value) == OPTION_ACCEPTED)
      options[option] = value;
  }

  int pad_count = 0;
  std::map<std::string, std::string> options;
protected:
  int uinput_fd = -1;

  virtual int process_option(std::string name, std::string value) {
    return -1;
  };
};



struct virtpad_settings {
  uinput_ids u_ids;
  bool dpad_as_hat;
  bool analog_triggers;
  std::string facemap_1234;
};

class virtual_gamepad : public virtual_device {
public:
  bool dpad_as_hat = false;
  bool analog_triggers = false;
  virtual_gamepad(std::string name, std::string descr, virtpad_settings settings, uinput* ui);
  virtual void take_event(struct input_event in);
protected:
  virtual int process_option(std::string name, std::string value);

  virtpad_settings padstyle;

  int face_1234[4] = {BTN_SOUTH, BTN_EAST, BTN_WEST, BTN_NORTH};
  void set_face_map(std::string map);
  std::string get_face_map();
};

class virtual_keyboard : public virtual_device {
public:
  virtual_keyboard(std::string name, std::string descr, uinput_ids u_ids, uinput* ui);
  virtual void take_event(struct input_event in) {
    write(uinput_fd, &in, sizeof(in));
  };
protected:

  uinput_ids u_ids;
  virtual int process_option(std::string name, std::string value);
};

class debug_device : public virtual_device {
public:
  debug_device(std::string name, std::string descr) : virtual_device(name, descr) {};
  virtual void take_event(struct input_event in) {
    if (in.type == EV_KEY) std::cout << name << ": " << in.code << " " << in.value << "(" << get_key_name(in.code) << ")" << std::endl;
    if (in.type == EV_ABS) std::cout << name << ": " << in.code << " " << in.value << "(" << get_axis_name(in.code) << ")" << std::endl;
    if (in.type == EV_REL) std::cout << name << ": " << in.code << " " << in.value << "(" << get_rel_name(in.code) << ")" << std::endl;
  };
};

#endif

#ifndef OUTPUT_SLOT_H
#define OUTPUT_SLOT_H

#include "uinput.h"
#include "eventlists/eventlist.h"
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <functional>

class input_source;

enum slot_state { SLOT_ACTIVE, SLOT_INACTIVE, SLOT_CLOSED, SLOT_DISABLED};

class output_slot {
public:
  std::string name;
  std::string descr;
  output_slot(std::string name) : name(name) { effects[0].id = -1;};
  output_slot(std::string name, std::string descr) : name(name), descr(descr) {effects[0].id = -1;};
  virtual ~output_slot();
  virtual void take_event(struct input_event in) {
  }

  virtual bool accept_device(std::shared_ptr<input_source> dev);
  virtual bool add_device(std::shared_ptr<input_source> dev);
  virtual bool remove_device(input_source* dev);

  int upload_ff(const ff_effect& effect);
  int erase_ff(int id);
  int play_ff(int id, int reptitions);

  virtual void clear_outputs();
  virtual void close_virt_device();
  void for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func);

  int pad_count = 0;
  std::map<std::string, std::string> options;
  slot_state state = SLOT_INACTIVE;
  ff_effect effects[1];
protected:
  int uinput_fd = -1;
  std::vector<std::weak_ptr<input_source>> devices;
  std::mutex lock;
  bool device_opened = true;
  uinput* ui = nullptr;
};



struct virtpad_settings {
  uinput_ids u_ids;
  bool dpad_as_hat;
  bool analog_triggers;
  bool rumble;
};

class virtual_gamepad : public output_slot {
public:
  bool dpad_as_hat = false;
  bool analog_triggers = false;
  virtual_gamepad(std::string name, std::string descr, virtpad_settings settings, uinput* ui);
  virtual void take_event(struct input_event in);
  virtual bool accept_device(std::shared_ptr<input_source> dev);
protected:

  virtpad_settings padstyle;

  int face_1234[4] = {BTN_SOUTH, BTN_EAST, BTN_WEST, BTN_NORTH};

};

class virtual_keyboard : public output_slot {
public:
  virtual_keyboard(std::string name, std::string descr, uinput_ids keyboard_ids, uinput_ids mouse_ids, uinput* ui);
  virtual void take_event(struct input_event in);

protected:

  uinput_ids u_ids;
  int mouse_fd = -1;
};

class debug_device : public output_slot {
public:
  debug_device(std::string name, std::string descr) : output_slot(name, descr) {};
  virtual void take_event(struct input_event in) {
    const char* event_name = nullptr;
    if (in.type == EV_KEY) event_name = get_key_name(in.code);
    if (in.type == EV_ABS) event_name = get_axis_name(in.code);
    if (in.type == EV_REL) event_name = get_rel_name(in.code);
    if (in.type == EV_SYN) return; //Don't print anything, just a SYN_REPORT.
    if (!event_name) event_name = "???";
    std::cout << name << ": " << in.code << " " << in.value << "(" << event_name << ")" << std::endl;
  };
};

#endif

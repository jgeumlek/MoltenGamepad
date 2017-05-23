#ifndef virtual_device_H
#define virtual_device_H

#include "../uinput.h"
#include "../eventlists/eventlist.h"
#include <iostream>
#include <string>
#include <map>
#include <memory>

class input_source;

enum slot_state { SLOT_ACTIVE, SLOT_INACTIVE, SLOT_CLOSED, SLOT_DISABLED};

class virtual_device {
public:
  std::string name;
  std::string descr;
  virtual_device(std::string name) : name(name) { effects[0].id = -1;};
  virtual_device(std::string name, std::string descr) : name(name), descr(descr) {effects[0].id = -1;};
  virtual ~virtual_device();
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
  std::vector<std::weak_ptr<input_source>> devices;
  std::mutex lock;
  bool device_opened = true;
  uinput* ui = nullptr;
  virtual void destroy_uinput_devs() {};
};






class debug_device : public virtual_device {
public:
  debug_device(std::string name, std::string descr) : virtual_device(name, descr) {};
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

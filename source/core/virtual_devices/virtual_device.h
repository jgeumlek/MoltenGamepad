#ifndef virtual_device_H
#define virtual_device_H

#include "../uinput.h"
#include "../eventlists/eventlist.h"
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <functional>

class input_source;
class slot_manager;


class virtual_device : public std::enable_shared_from_this<virtual_device> {
public:
  std::string name;
  std::string descr;
  virtual_device(std::string name, slot_manager* slot_man) : name(name), slot_man(slot_man) { effects[0].id = -1;};
  virtual_device(std::string name, std::string descr, slot_manager* slot_man) : name(name), descr(descr), slot_man(slot_man) {effects[0].id = -1;};
  virtual ~virtual_device();
  virtual void take_event(struct input_event in) {
  }

  virtual void init() {
  }

  bool accept_device(std::shared_ptr<input_source> dev);
  bool add_device(std::shared_ptr<input_source> dev);
  bool remove_device(input_source* dev);

  int upload_ff(const ff_effect& effect);
  int erase_ff(int id);
  int play_ff(int id, int reptitions);

  virtual void clear_outputs();
  bool close_virt_device();
  void for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func);
  uint input_source_count();
  void send_start_press(int milliseconds);

  void take_delayed_event(struct input_event in, int milliseconds);
  void check_delayed_events();

  void ref();
  void unref();

  ff_effect effects[1];
protected:
  std::vector<std::weak_ptr<input_source>> devices;
  std::vector<input_event> delayed_events;
  std::vector<std::shared_ptr<virtual_device>> extra_refs;
  //Bad stuff happens on some kernels if a uinput device is destroyed
  //while it still has uploaded FF effects.
  std::shared_ptr<virtual_device> self_ref_to_prevent_deletion_with_ff;
  std::mutex lock;
  std::mutex delayed_events_lock;
  bool device_opened = true;
  uinput* ui = nullptr;
  slot_manager* slot_man;
  virtual void destroy_uinput_devs() {};
  void process_becoming_empty();
};






class debug_device : public virtual_device {
public:
  debug_device(std::string name, std::string descr, slot_manager* slot_man) : virtual_device(name, descr, slot_man) {};
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

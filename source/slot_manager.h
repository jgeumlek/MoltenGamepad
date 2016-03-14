#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>
#include <vector>

#include "uinput.h"
#include "virtual_device.h"
#include "devices/device.h"

class input_source;

enum virtpad_type { LINUX_PAD, DPAD_AS_HAT_PAD };

class slot_manager {
public:


  slot_manager(int num_pads, bool keys, const virtpad_settings& padstyle);

  ~slot_manager();

  void request_slot(input_source* dev);
  void move_to_slot(input_source* dev, virtual_device* target);
  void remove(input_source* dev);


  virtual_device* find_slot(std::string name);
  virtual_device* keyboard = nullptr;
  virtual_device* dummyslot = nullptr;
  virtual_device* debugslot = nullptr;
  std::vector<virtual_device*> slots;
private:
  virtpad_type padtype;
  void remove_from(virtual_device* slot);

  bool slots_on_demand = false;

  uinput* ui;
  std::mutex lock;
  int num_slots = 2;
};

#endif

#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>
#include <vector>

#include "uinput.h"
#include "output_slot.h"
#include "devices/device.h"
#include "messages.h"

class input_source;

enum virtpad_type { LINUX_PAD, DPAD_AS_HAT_PAD };

class slot_manager {
public:


  slot_manager(int num_pads, bool keys, const virtpad_settings& padstyle);

  ~slot_manager();

  int request_slot(input_source* dev);
  void move_to_slot(input_source* dev, output_slot* target);
  void remove(input_source* dev);

  const uinput* get_uinput() { return ui; };

  output_slot* find_slot(std::string name);
  output_slot* keyboard = nullptr;
  output_slot* dummyslot = nullptr;
  output_slot* debugslot = nullptr;
  std::vector<output_slot*> slots;
private:
  virtpad_type padtype;
  void remove_from(output_slot* slot);
  void move_device(input_source* dev, output_slot* target);

  bool slots_on_demand = false;

  uinput* ui;
  std::mutex lock;
  int num_slots = 2;
  simple_messenger log;
};

#endif

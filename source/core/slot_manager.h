#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>
#include <vector>

#include "uinput.h"
#include "output_slot.h"
#include "devices/device.h"
#include "messages.h"
#include "options.h"

class input_source;

class slot_manager {
public:


  slot_manager(int max_pads, bool keys, const virtpad_settings& padstyle);

  ~slot_manager();

  int request_slot(input_source* dev);
  void move_to_slot(input_source* dev, output_slot* target);

  const uinput* get_uinput() { return ui; };

  output_slot* find_slot(std::string name);
  output_slot* keyboard = nullptr;
  output_slot* dummyslot = nullptr;
  output_slot* debugslot = nullptr;
  options opts;
  std::vector<output_slot*> slots;
private:
  void remove_from(output_slot* slot);
  void move_device(input_source* dev, output_slot* target);
  int process_option(std::string& name, MGField value);

  bool slots_on_demand = false;

  uinput* ui;
  std::mutex lock;
  int min_pads = 1;
  int max_pads = 4;
  int active_pads = 4;
  bool persistent_slots = true;
  simple_messenger log;
};

#endif

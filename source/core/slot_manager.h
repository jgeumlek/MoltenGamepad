#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>
#include <vector>

#include "uinput.h"
#include "output_slot.h"
#include "devices/device.h"
#include "protocols/message_stream.h"
#include "options.h"

class input_source;

class slot_manager {
public:
  enum id_type {NAME_ID, UNIQ_ID, PHYS_ID};

  slot_manager(int max_pads, bool keys, const virtpad_settings& padstyle);

  ~slot_manager();

  int request_slot(input_source* dev);
  void move_to_slot(input_source* dev, output_slot* target);
  void id_based_assign(slot_manager::id_type, std::string id, output_slot* slot); //tie an id to a specific slot for autoassignment
  void for_all_assignments(std::function<void (slot_manager::id_type, std::string, output_slot*)> func);

  const uinput* get_uinput() { return ui; };

  output_slot* find_slot(std::string name);
  output_slot* keyboard = nullptr;
  output_slot* dummyslot = nullptr;
  output_slot* debugslot = nullptr;
  options opts;
  std::vector<output_slot*> slots;
  message_stream log;
private:
  void remove_from(output_slot* slot);
  void move_device(input_source* dev, output_slot* target);
  int process_option(std::string& name, MGField value);
  output_slot* find_id_based_assignment(input_source* dev);

  bool slots_on_demand = false;

  uinput* ui;
  std::mutex lock;
  int min_pads = 1;
  int max_pads = 4;
  int active_pads = 4;
  bool persistent_slots = true;
  std::map<std::pair<id_type,std::string>,output_slot*> id_slot_assignments;
};

#endif

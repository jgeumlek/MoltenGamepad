#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>
#include <vector>

#include "uinput.h"
#include "virtual_devices/virtual_device.h"
#include "virtual_devices/virtual_gamepad.h"
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
  void move_to_slot(input_source* dev, virtual_device* target);
  void id_based_assign(slot_manager::id_type, std::string id, virtual_device* slot); //tie an id to a specific slot for autoassignment
  void for_all_assignments(std::function<void (slot_manager::id_type, std::string, virtual_device*)> func);

  const uinput* get_uinput() { return ui; };

  virtual_device* find_slot(std::string name);
  virtual_device* keyboard = nullptr;
  virtual_device* dummyslot = nullptr;
  virtual_device* debugslot = nullptr;

  std::vector<virtual_device*> slots;
  message_stream log;
  options opts;
private:
  void remove_from(virtual_device* slot);
  void move_device(input_source* dev, virtual_device* target);
  int process_option(std::string& name, MGField value);
  virtual_device* find_id_based_assignment(input_source* dev);

  bool slots_on_demand = false;

  uinput* ui;
  std::mutex lock;
  int min_pads = 1;
  int max_pads;
  int active_pads = 4;
  bool persistent_slots = true;
  std::map<std::pair<id_type,std::string>,virtual_device*> id_slot_assignments;
};

#endif

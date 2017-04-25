#include "slot_manager.h"

slot_manager::slot_manager(int max_pads, bool keys, const virtpad_settings& padstyle) : log("slot"), max_pads(max_pads), opts([&] (std::string& name, MGField value) { return process_option(name, value); })
{
  ui = new uinput();
  dummyslot = new output_slot("blank", "Dummy slot (ignores all events)");
  debugslot = new debug_device("debugslot", "Prints out all received events");
  debugslot->state = SLOT_ACTIVE;

  for (int i = 0; i < max_pads; i++) {
    slots.push_back(new virtual_gamepad("virtpad" + std::to_string(i + 1), "A virtual gamepad", padstyle, ui));
    slots[i]->state = SLOT_ACTIVE;
  }
  opts.register_option({"active_pads","Number of virtpad slots currently active for assignment.", std::to_string(max_pads).c_str(), MG_INT});
  opts.register_option({"auto_assign","Assign devices to an output slot upon connection.", "false", MG_BOOL});

  if (keys) {
    keyboard = new virtual_keyboard("keyboard", "A virtual keyboard", {"Virtual Keyboard (MoltenGamepad)", "moltengamepad/keyboard", 1, 1, 1}, {"Virtual Mouse (MoltenGamepad)", "moltengamepad/keyboard", 1, 1, 1}, ui);
    keyboard->state = SLOT_ACTIVE;
  } else {
    keyboard = new output_slot("keyboard", "Disabled virtual keyboard slot");
    keyboard->state = SLOT_DISABLED;
  }

  if (padstyle.rumble)
    ui->start_ff_thread();
}

slot_manager::~slot_manager() {
  for (auto slot : slots)
    slot->close_virt_device();

  for (auto slot : slots)
    delete slot;
  delete ui;
  delete dummyslot;
  delete keyboard;
  delete debugslot;
}

int slot_manager::request_slot(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
  if (dev->get_slot())
    return 0;
  output_slot* assigned = find_id_based_assignment(dev);
  if (assigned) {
    move_device(dev,assigned);
    return 0;
  }
  if (dev->get_type() == "keyboard" || active_pads == 0) {
    move_device(dev,keyboard);
    return 0;
  }
  if (active_pads == 1) {
    move_device(dev, slots[0]);
    return 0;
  }
  for (int i = 0; i < active_pads; i++) {
    if (slots[i]->accept_device(dev->shared_from_this())) {
      move_device(dev, slots[i]);
      return 0;
    }
  }
  move_device(dev,dummyslot);

  return 0;
}

void slot_manager::move_to_slot(input_source* dev, output_slot* target) {
  lock.lock();
  move_device(dev,target);
  lock.unlock();
}

void slot_manager::move_device(input_source* dev, output_slot* target) {
  //private, should only be called with lock acquired
  if (!dev) return;
  dev->set_slot(target);
  log.device_slot(0, dev, target);
}

void slot_manager::id_based_assign(slot_manager::id_type type, std::string id, output_slot* slot) {
  std::lock_guard<std::mutex> guard(lock);
  std::pair<slot_manager::id_type, std::string> key = std::make_pair(type,id);
  if (!slot) {
    id_slot_assignments.erase(key);
    return;
  }
  id_slot_assignments[key] = slot;
  return;
}

output_slot* slot_manager::find_id_based_assignment(input_source* dev) {
  //private func, called while lock is held.
  //Currently hardcoded priority:
  // uniq assignment -> name assignment -> phys assignment.
  if (id_slot_assignments.empty())
    return nullptr;
  std::pair<slot_manager::id_type, std::string> key = std::make_pair(UNIQ_ID,dev->get_uniq());
  auto slot = id_slot_assignments.find(key);
  if (slot != id_slot_assignments.end())
    return slot->second;
  key = std::make_pair(NAME_ID,dev->get_name());
  slot = id_slot_assignments.find(key);
  if (slot != id_slot_assignments.end())
    return slot->second;
  key = std::make_pair(PHYS_ID,dev->get_phys());
  slot = id_slot_assignments.find(key);
  if (slot != id_slot_assignments.end())
    return slot->second;
  return nullptr;
}

void slot_manager::for_all_assignments(std::function<void (slot_manager::id_type, std::string, output_slot*)> func) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto entry : id_slot_assignments)
    func(entry.first.first, entry.first.second, entry.second);
}

output_slot* slot_manager::find_slot(std::string slotname) {
  for (auto it = slots.begin(); it != slots.end(); it++) {
    if ((*it)->name == slotname) return *it;
  }
  if (slotname == "keyboard" || slotname == keyboard->name) {
    return keyboard;
  }
  if (slotname == dummyslot->name) return dummyslot;
  if (slotname == debugslot->name) return debugslot;
  return nullptr;
}

int slot_manager::process_option(std::string& name, MGField value) {
  std::lock_guard<std::mutex> guard(lock);
  if (name == "active_pads" && value.integer >= 0 && value.integer <= max_pads) {
    active_pads = value.integer;
    for (int i = 0; i < max_pads; i++) {
      slots[i]->state = (i < active_pads) ? SLOT_ACTIVE : SLOT_INACTIVE;
    }
    return 0;
  }

  return -1;
}

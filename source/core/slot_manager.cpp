#include "slot_manager.h"
#include "virtual_devices/virtual_kbm.h"

slot_manager::slot_manager(int max_pads, bool keys, const virtpad_settings& padstyle) : log("slot"),  opts([&] (std::string& name, MGField value){ return process_option(name, value); }), max_pads(max_pads)
{
  ui = new uinput();
  dummyslot.virt_dev = new virtual_device("blank", "Dummy slot (ignores all events)", this);
  debugslot.virt_dev = new debug_device("debugslot", "Prints out all received events", this);
  debugslot.state = SLOT_ACTIVE;
  dummyslot.state = SLOT_ACTIVE;

  for (int i = 0; i < max_pads; i++) {
    output_slot new_slot;

    new_slot.virt_dev = new virtual_gamepad("virtpad" + std::to_string(i + 1), "A virtual gamepad", padstyle, this, ui);
    new_slot.state = SLOT_ACTIVE;
    new_slot.has_devices = false;
    slots.push_back(new_slot);
  }
  opts.register_option({"active_pads","Number of virtpad slots currently active for assignment.", std::to_string(max_pads).c_str(), MG_INT});
  opts.register_option({"auto_assign","Assign devices to an output slot upon connection.", "false", MG_BOOL});
  opts.register_option({"press_start_on_disconnect","Try to pause by pressing start when a slot is empty. Can be \"any\", \"last\", or \"none\".", "none", MG_STRING});
  opts.register_option({"press_start_ms","Duration in milliseconds of the simulated start press", "20", MG_INT});

  if (keys) {
    uinput_ids kb_ids = {"Virtual Keyboard (MoltenGamepad)", "moltengamepad/keyboard", 1, 1, 1};
    uinput_ids rel_mouse_ids = {"Virtual Mouse (MoltenGamepad)", "moltengamepad/relmouse", 1, 1, 1};

    keyboard.virt_dev = new virtual_keyboard("keyboard", "A virtual keyboard", kb_ids, rel_mouse_ids, this, ui);
    keyboard.state = SLOT_ACTIVE;
  } else {
    keyboard.virt_dev = new virtual_device("keyboard", "Disabled virtual keyboard slot", this);
    keyboard.state = SLOT_DISABLED;
  }

  if (padstyle.rumble)
    ui->start_ff_thread();
}

slot_manager::~slot_manager() {
  for (auto slot : slots)
    slot.virt_dev->close_virt_device();

  for (auto slot : slots)
    delete slot.virt_dev;
  delete ui;
  delete dummyslot.virt_dev;
  delete keyboard.virt_dev;
  delete debugslot.virt_dev;
}

int slot_manager::request_slot(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
  if (dev->get_slot())
    return 0;
  virtual_device* assigned = find_id_based_assignment(dev);
  if (assigned) {
    move_device(dev,assigned);
    return 0;
  }
  if (dev->get_type() == "keyboard" || active_pads == 0) {
    move_device(dev,keyboard.virt_dev);
    return 0;
  }
  if (active_pads == 1) {
    move_device(dev, slots[0].virt_dev);
    return 0;
  }
  for (int i = 0; i < active_pads; i++) {
    if (slots[i].virt_dev->accept_device(dev->shared_from_this())) {
      move_device(dev, slots[i].virt_dev);
      return 0;
    }
  }
  move_device(dev,dummyslot.virt_dev);

  return 0;
}

void slot_manager::move_to_slot(input_source* dev, virtual_device* target) {
  //don't detect emptiness if we are just moving the last input source from one slot to another.
  if (!target)
    update_slot_emptiness();
  lock.lock();
  move_device(dev,target);
  lock.unlock();
  if (!target)
    process_slot_emptiness();
}

void slot_manager::move_device(input_source* dev, virtual_device* target) {
  //private, should only be called with lock acquired
  if (!dev) return;
  dev->set_slot(target);
  log.device_slot(0, dev, target);
}

void slot_manager::id_based_assign(slot_manager::id_type type, std::string id, virtual_device* slot) {
  std::lock_guard<std::mutex> guard(lock);
  std::pair<slot_manager::id_type, std::string> key = std::make_pair(type,id);
  if (!slot) {
    id_slot_assignments.erase(key);
    return;
  }
  id_slot_assignments[key] = slot;
  return;
}

virtual_device* slot_manager::find_id_based_assignment(input_source* dev) {
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

void slot_manager::for_all_assignments(std::function<void (slot_manager::id_type, std::string, virtual_device*)> func) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto entry : id_slot_assignments)
    func(entry.first.first, entry.first.second, entry.second);
}

virtual_device* slot_manager::find_slot(std::string slotname) {
  for (auto it = slots.begin(); it != slots.end(); it++) {
    if (it->virt_dev->name == slotname) return it->virt_dev;
  }
  if (slotname == "keyboard" || slotname == keyboard.virt_dev->name) {
    return keyboard.virt_dev;
  }
  if (slotname == dummyslot.virt_dev->name) return dummyslot.virt_dev;
  if (slotname == debugslot.virt_dev->name) return debugslot.virt_dev;
  return nullptr;
}

int slot_manager::process_option(std::string& name, MGField value) {
  std::lock_guard<std::mutex> guard(lock);
  if (name == "active_pads" && value.integer >= 0 && value.integer <= max_pads) {
    active_pads = value.integer;
    for (int i = 0; i < max_pads; i++) {
      slots[i].state = (i < active_pads) ? SLOT_ACTIVE : SLOT_INACTIVE;
    }
    return 0;
  }
  if (name == "press_start_on_disconnect" && value.string) {
    std::string text(value.string);
    if (text == "any") {
      press_start_on_last_disconnect = false;
      press_start_on_any_disconnect = true;
      return 0;
    }
    if (text == "last") {
      press_start_on_any_disconnect = false;
      press_start_on_last_disconnect = true;
      return 0;
    }
    if (text == "none") {
      press_start_on_last_disconnect = false;
      press_start_on_any_disconnect = false;
      return 0;
    }
    return -1;
  }
  if (name == "press_start_ms") {
      start_press_milliseconds = value.integer;
      return 0;
  }


  return -1;
}

//just caches the boolean values on if devices are empty.
void slot_manager::update_slot_emptiness() {
  std::lock_guard<std::mutex> guard(lock);
  for (output_slot& slot : slots)
    slot.has_devices = slot.virt_dev->input_source_count() > 0;
}
//checks against cached info to see if our last slot just became empty.
void slot_manager::process_slot_emptiness() {
  //if we aren't faking a start press on the last connected slot,
  //then we don't need to do this check at all...
  if (!press_start_on_last_disconnect)
    return;
  std::lock_guard<std::mutex> guard(lock);
  //check if all are empty... nothing to do.
  bool already_all_empty = true;
  for (output_slot& slot : slots) {
    if (slot.has_devices)
      already_all_empty = false;
  }

  //if not empty in our cached values, lets update them.
  //We need to detect the case where a single slot went
  //nonempty to empty and this caused all slots to be empty.
  if (!already_all_empty) {
    bool now_all_empty = true;
    output_slot last_slot;
    last_slot.virt_dev = nullptr;
    for (output_slot& slot : slots) {
      //check if this is a slot that is newly empty
      bool had_devices = slot.has_devices;
      slot.has_devices = slot.virt_dev->input_source_count() > 0;
      if (had_devices && !slot.has_devices)
        last_slot = slot;
      //check to see if all updated slots are now empty
      if (slot.has_devices)
        now_all_empty = false;
    }
    //we weren't all empty in the cache, but now we are...
    if (now_all_empty && last_slot.virt_dev) {
      last_slot.virt_dev->send_start_press(start_press_milliseconds);
    }
  }
}


void slot_manager::tick_all_slots() {
  std::lock_guard<std::mutex> guard(lock);
  dummyslot.virt_dev->check_delayed_events();
  debugslot.virt_dev->check_delayed_events();
  keyboard.virt_dev->check_delayed_events();
  for (output_slot& slot : slots) {
    slot.virt_dev->check_delayed_events();
  }
}

#include "slot_manager.h"

slot_manager::slot_manager(int max_pads, bool keys, const virtpad_settings& padstyle) : log("slot"), max_pads(max_pads), opts([&] (std::string& name, MGField value) { return process_option(name, value); })
{
  ui = new uinput();
  dummyslot = new output_slot("blank", "Dummy slot (ignores all events)");
  debugslot = new debug_device("debugslot", "Prints out all received events");
  debugslot->state = SLOT_ACTIVE;
  if (keys) {
    keyboard = new virtual_keyboard("keyboard", "A virtual keyboard", {"Virtual Keyboard (MoltenGamepad)", 1, 1, 1}, {"Virtual Mouse (MoltenGamepad)", 1, 1, 1}, ui);
    keyboard->state = SLOT_ACTIVE;
  } else {
    keyboard = new output_slot("keyboard", "Disabled virtual keyboard slot");
    keyboard->state = SLOT_DISABLED;
  }

  for (int i = 0; i < max_pads; i++) {
    slots.push_back(new virtual_gamepad("virtpad" + std::to_string(i + 1), "A virtual gamepad", padstyle, ui));
    slots[i]->state = SLOT_ACTIVE;
  }
  log.add_listener(1);
  opts.register_option({"active_pads","Number of virtpad slots currently active for assignment.", std::to_string(max_pads).c_str(), MG_INT});
  opts.register_option({"auto_assign","Assign devices to an output slot upon connection.", "false", MG_BOOL});
}

slot_manager::~slot_manager() {
  for (auto slot : slots)
    delete slot;
  delete ui;
  delete dummyslot;
  delete keyboard;
  delete debugslot;
}

int slot_manager::request_slot(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
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
  if (dev->out_dev == target) return;
  if (dev->out_dev) {
    dev->out_dev->remove_device(dev);
  }
  if (target) {
    target->add_device(dev->shared_from_this());
  }
  dev->set_slot(target);
  if (target) {
    log.take_message(dev->get_name() + " assigned to slot " + target->name);
  } else {
    log.take_message(dev->get_name() + " not assigned to any slot");
  }
}


void slot_manager::remove(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
  if (!dev || !dev->out_dev) return;
  dev->out_dev->remove_device(dev);
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

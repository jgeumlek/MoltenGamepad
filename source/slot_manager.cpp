#include "slot_manager.h"

slot_manager::slot_manager(int num_pads, bool keys, const virtpad_settings& padstyle) : log("slot") {
  ui = new uinput();
  dummyslot = new output_slot("blank", "Dummy slot (ignores all events)");
  debugslot = new debug_device("debugslot", "Prints out all received events");
  if (keys) {
    keyboard = new virtual_keyboard("keyboard", "A virtual keyboard", {"Virtual Keyboard (MoltenGamepad)", 1, 1, 1}, ui);
  } else {
    keyboard = new output_slot("keyboard(disabled)", "Disabled virtual keyboard slot");
  }

  for (int i = 0; i < num_pads; i++) {
    slots.push_back(new virtual_gamepad("virtpad" + std::to_string(i + 1), "A virtual gamepad", padstyle, ui));
  }
  log.add_listener(1);
}

slot_manager::~slot_manager() {
  for (auto slot : slots)
    delete slot;
  delete ui;
  delete dummyslot;
  delete keyboard;
  delete debugslot;
}

void slot_manager::request_slot(input_source* dev) {
  if (dev->getType() == input_source::KEYBOARD) {
    lock.lock();
    move_device(dev,keyboard);
    lock.unlock();
    return;
  }
  lock.lock();
  for (int i = 0; i < slots.size(); i++) {
    if (slots[i]->accepting()) {
      move_device(dev,slots[i]);
      lock.unlock();
      return;
    }
  }
  move_device(dev,dummyslot);

  lock.unlock();
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
    remove_from(dev->out_dev);
  }
  if (target) {
    target->pad_count += 1;
  }
  dev->set_slot(target);
  if (target) {
    log.text(dev->name + " assigned to slot " + target->name);
  } else {
    log.text(dev->name + " not assigned to any slot");
  }
}

void slot_manager::remove_from(output_slot* slot) {
  //private, should only be called with lock acquired
  slot->pad_count -= 1;

}

void slot_manager::remove(input_source* dev) {
  if (!dev || !dev->out_dev) return;
  lock.lock();
  remove_from(dev->out_dev);
  lock.unlock();
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

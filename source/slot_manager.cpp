#include "slot_manager.h"

slot_manager::slot_manager(int num_pads, bool keys, bool dpad_as_hat) {
     ui = new uinput();
     dummyslot = new virtual_device("blank");
     dummyslot->descr = "Dummy slot (ignores all events)";
     if (keys) {
       keyboard = new virtual_keyboard("keyboard",ui);
     } else {
       keyboard = new virtual_device("keyboard(disabled)");
       keyboard->descr = "Disabled Keyboard slot";
     }
     
     for (int i = 0; i < num_pads; i++) {
      slots.push_back(new virtual_gamepad("virtpad" + std::to_string(i+1),dpad_as_hat,true,ui));
     }
}

slot_manager::~slot_manager() {
      for (auto slot : slots)
        delete slot;
      delete ui;
      delete dummyslot;
      delete keyboard;
}

void slot_manager::request_slot(input_source* dev) {
  lock.lock();
  for (int i = 0; i < slots.size(); i++) {
    if (slots[i]->pad_count == 0) {
      dev->set_slot(slots[i]);
      slots[i]->pad_count += 1;
      lock.unlock();
      return;
    }
  }
  dev->set_slot(dummyslot);
  
  lock.unlock();
}

void slot_manager::move_to_slot(input_source* dev, virtual_device* target) {
  if (!dev) return;
  if (dev->out_dev == target) return;
  lock.lock();
  if (dev->out_dev) {
    remove_from(dev->out_dev);
  }
  if (target) {
    target->pad_count += 1;
  }
  dev->set_slot(target);
  lock.unlock();
}

void slot_manager::remove_from(virtual_device* slot) {
  //private, should only be called with lock acquired
  slot->pad_count -= 1;
  
}

void slot_manager::remove(input_source* dev) {
  if (!dev || !dev->out_dev) return;
  lock.lock();
  remove_from(dev->out_dev);
  lock.unlock();
}
  

virtual_device* slot_manager::find_slot(std::string slotname) {
  for (auto it = slots.begin(); it != slots.end(); it++) {
    if ((*it)->name == slotname) return *it;
  }
  if (slotname == "keyboard" || slotname == keyboard->name) {
    return keyboard;
  }
  if (slotname == dummyslot->name) return dummyslot;
  return nullptr;
}
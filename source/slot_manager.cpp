#include "slot_manager.h"

slot_manager::slot_manager() {
     ui = new uinput();
     slots.push_back(new virtual_gamepad(ui));
     slots.push_back(new virtual_gamepad(ui));
}

slot_manager::~slot_manager() {
      for (auto slot : slots)
        delete slot;
      delete ui;
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
  dev->set_slot(&dummyslot);
  
  lock.unlock();
}

void slot_manager::remove_from(virtual_device* slot) {
  lock.lock();
  slot->pad_count -= 1;
  
  lock.unlock();
}


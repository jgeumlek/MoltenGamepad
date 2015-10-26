#include "slot_manager.h"

slot_manager::slot_manager() {
     ui = new uinput();
     devs[0] = new virtual_gamepad(ui);
     devs[1] = new virtual_gamepad(ui);
}

slot_manager::~slot_manager() {
      delete devs[1];
      delete devs[0];
      delete ui;
}

void slot_manager::request_slot(input_source* dev) {
  lock.lock();
  for (int i = 0; i < num_slots; i++) {
    if (devs[i]->pad_count == 0) {
      dev->set_slot(devs[i]);
      devs[i]->pad_count += 1;
      lock.unlock();
      return;
    }
  }
  dev->set_slot(devs[0]);
  devs[0]->pad_count += 1;
  
  lock.unlock();
}

void slot_manager::remove_from(virtual_device* slot) {
  lock.lock();
  slot->pad_count -= 1;
  
  lock.unlock();
}


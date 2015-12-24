#include "slot_manager.h"

slot_manager::slot_manager(int num_pads, bool keys, bool dpad_as_hat) {
     ui = new uinput();
     dummyslot = new virtual_device("blank");
     if (keys) {
       keyboard = new virtual_keyboard("keyboard",ui);
     } else {
       keyboard = new virtual_device("keyboard(disabled)");
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

void slot_manager::remove_from(virtual_device* slot) {
  lock.lock();
  slot->pad_count -= 1;
  
  lock.unlock();
}


#include "slot_manager.h"

slot_manager::slot_manager() {
     ui = new uinput();
     devs[1] = new virtual_gamepad(ui);
     devs[0] = new virtual_keyboard(ui);
}

slot_manager::~slot_manager() {
      delete devs[1];
      delete devs[0];
      delete ui;
}
   

#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include "uinput.h"
#include "virtual_device.h"

class slot_manager {
public:
   virtual_device* devs[3];

   slot_manager();

   ~slot_manager();

private:
   uinput* ui;
};

#endif

#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>

#include "uinput.h"
#include "virtual_device.h"
#include "devices/device.h"

class input_source;

class slot_manager {
public:
   

   slot_manager();

   ~slot_manager();
   
   void request_slot(input_source* dev);

private:
   uinput* ui;
   virtual_device* devs[2];
   std::mutex lock;
};

#endif

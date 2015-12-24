#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include <mutex>
#include <vector>

#include "uinput.h"
#include "virtual_device.h"
#include "devices/device.h"

class input_source;

enum virtpad_type { LINUX_PAD, DPAD_AS_HAT_PAD };

class slot_manager {
public:
   

   slot_manager(int num_pads, bool keys, bool dpad_as_hat);

   ~slot_manager();
   
   void request_slot(input_source* dev);
   
   void remove_from(virtual_device* slot);
   virtual_device* keyboard;
private:
   virtpad_type padtype;
   virtual_device* dummyslot;
   
   bool slots_on_demand = false;
   
   uinput* ui;
   std::vector<virtual_device*> slots;
   std::mutex lock;
   int num_slots = 2;
};

#endif

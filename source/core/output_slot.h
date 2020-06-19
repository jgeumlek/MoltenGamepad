#pragma once
#include "virtual_devices/virtual_device.h"
#include <string>


enum slot_state { SLOT_ENABLED, SLOT_DISABLED};

struct output_slot {
  std::shared_ptr<virtual_device> virt_dev;
  std::string name;
  slot_state state;
  bool has_devices;
};

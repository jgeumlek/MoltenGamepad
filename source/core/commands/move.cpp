#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define MOVE_USAGE "USAGE:\n\tmove <device> to <slot>\n\t\"all\" can be used to refer to all devices\n\t\"nothing\" can be used as a slot name to remove from all slots\n\t\"auto\" can be used as a slot name to let the slot manager decide"
int do_move(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 4) {
    out->print(MOVE_USAGE);
    return -1;
  }
  if (command.at(2).value != "to") {
    out->print(MOVE_USAGE);
    return -1;
  }
  std::string devname = command.at(1).value;
  std::string slotname = command.at(3).value;
  std::shared_ptr<input_source> dev = mg->find_device(devname.c_str());
  std::shared_ptr<virtual_device> slot = mg->slots->find_slot(slotname);
  mg->slots->update_slot_emptiness();
  if (!dev.get() && devname != "all") {
    out->err("device " + devname + " not found.");
    return -1;
  };
  if (!slot && slotname == "auto") {
    if (devname != "all") {
      mg->slots->request_slot(dev.get());
    } else {
      mg->for_all_devices( [mg] (auto dev) {
        mg->slots->request_slot(dev.get());
      });
    }
    return 0;
  }
  if (!slot && slotname != "nothing") {
    out->err("slot " + slotname + " not available.");
    return -1;
  };
  if (devname != "all") {
    mg->slots->move_to_slot(dev.get(), slot);
  } else {
    mg->for_all_devices( [&slot,mg] (auto dev) {
      mg->slots->move_to_slot(dev.get(), slot);
    });
  }
  mg->slots->update_slot_emptiness();
  return 0;
}


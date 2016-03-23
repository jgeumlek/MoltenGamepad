#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define MOVE_USAGE "USAGE:\n\tmove <device> to <slot>\n\t\"all\" can be used to refer to all devices\n\t\"nothing\" can be used as a slot name to remove from all slots"
int MGparser::do_move(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 4) {
    out.take_message(MOVE_USAGE);
    return -1;
  }
  if (command.at(2).value != "to") {
    out.take_message(MOVE_USAGE);
    return -1;
  }
  std::string devname = command.at(1).value;
  std::string slotname = command.at(3).value;
  std::shared_ptr<input_source> dev = mg->find_device(devname.c_str());
  output_slot* slot = mg->slots->find_slot(slotname);
  if (!dev.get() && devname != "all") {
    out.take_message("device " + devname + " not found.\n" + MOVE_USAGE);
    return -1;
  };
  if (!slot && slotname != "nothing") {
    out.take_message("slot " + devname + " not found.\n" + MOVE_USAGE);
    return -1;
  };
  if (devname != "all") {
    mg->slots->move_to_slot(dev.get(), slot);
  } else {
    mg->for_all_devices( [slot,mg] (auto dev) {
      mg->slots->move_to_slot(dev.get(), slot);
    });
  }
  return 0;
}


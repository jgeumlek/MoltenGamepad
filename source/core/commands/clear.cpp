#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define CLEAR_USAGE "USAGE:\n\tclear <slot> \n\t\"allpads\" may be used as a slot name to refer to all gamepad slots"
int do_clear(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 2) {
    out->print(CLEAR_USAGE);
    return -1;
  }
  std::string slotname = command.at(1).value;

  if (slotname == "allpads") {
    for (auto slot : mg->slots->slots) {
      slot->clear_outputs();
    }
    return 0;
  }
  output_slot* slot = mg->slots->find_slot(slotname);
  if (!slot) {
    out->err("slot " + slotname + " not found.");
    return -1;
  }
  slot->clear_outputs();
  out->take_message("cleared output events on slot.");
  return 0;

}


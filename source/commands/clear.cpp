#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define CLEAR_USAGE "USAGE:\n\tclear <slot> \n\t\"allpads\" may be used as a slot name to refer to all gamepad slots"
int do_clear(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 2) {
    std::cout << CLEAR_USAGE << std::endl;
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
    std::cout << "slot " << slotname << " not found.\n" << CLEAR_USAGE << std::endl;
    return -1;
  }
  slot->clear_outputs();
  return 0;

}


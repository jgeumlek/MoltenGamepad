#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define ALTERSLOT_USAGE "USAGE:\n\talterslot <slot> <setting> <value>\n\t\"allpads\" may be used as a slot name to refer to all gamepad slots"
int do_alterslot(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  out->print("The alterslot command is deprecated and will be removed in a future version.");
  if (command.size() < 4) {
    out->print(ALTERSLOT_USAGE);
    return -1;
  }
  std::string slotname = command.at(1).value;
  std::string setting = command.at(2).value;
  std::string value = command.at(3).value;

  if (slotname == "allpads") {
    for (auto slot : mg->slots->slots) {
      slot->update_option(setting, value);
    }
    return 0;
  }
  output_slot* slot = mg->slots->find_slot(slotname);
  if (!slot) return -1;
  slot->update_option(setting, value);
  return 0;

}

#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

#define ASSIGN_USAGE "USAGE:\n\tassign slot <slot> to <type> <id>\n\ttype can be \"name\", \"phys\", or \"uniq\".\n\tid should be the identifier string of the relevant type to be assigned.\n\tSet slot to \"nothing\" to clear this assignment."
int do_assign(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 6) {
    out->print(ASSIGN_USAGE);
    return -1;
  }
  std::string type = command.at(4).value;
  std::string id = command.at(5).value;
  std::string slotname = command.at(2).value;

  output_slot* slot = mg->slots->find_slot(slotname);
  if (!slot && slotname != "nothing") {
    out->err("slot " + slotname + " not found.");
    return -1;
  };
  if (type != "name" && type != "phys" && type != "uniq") {
    out->err("type " + type + " was not one of the valid types: name, phys, uniq.");
    return -1;
  };
  slot_manager::id_type typeval = slot_manager::NAME_ID;
  if (type == "phys")
    typeval = slot_manager::PHYS_ID;
  if (type == "uniq")
    typeval = slot_manager::UNIQ_ID;
  
  mg->slots->id_based_assign(typeval, id, slot);
  return 0;

}

#include "../moltengamepad.h"
#include "../parser.h"


#define SET_USAGE "USAGE:\n\tset <category> <option name> = <value>\n\tuse \"print options\" to see available categories.\n"\
"\tuse \"print options <category>\" to see available options in a category."
int do_set(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 5) {
    out->print(SET_USAGE);
    return -1;
  }
  if (command.at(3).type != TK_EQUAL) {
    out->print(SET_USAGE);
    return -1;
  }
  std::string category = command.at(1).value;
  std::string opname = command.at(2).value;
  std::string opvalue = command.at(4).value;
  int ret = mg->set_option(category, opname, opvalue);
  if (ret) {
    out->err("error setting option " + opname + " = " + opvalue);
  } else {
    out->take_message("option " + opname + " was set to " + opvalue);
  }
  return ret;
}


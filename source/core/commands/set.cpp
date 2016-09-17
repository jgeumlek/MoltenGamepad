#include "../moltengamepad.h"
#include "../parser.h"


#define SET_USAGE "USAGE:\n\tset <category> <option name> = <value>\n\tuse \"print options\" to see available categories."
int do_set(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 5) {
    std::cout << SET_USAGE << std::endl;
    return -1;
  }
  if (command.at(3).type != TK_EQUAL) {
    std::cout << SET_USAGE << std::endl;
    return -1;
  }
  std::string category = command.at(1).value;
  std::string opname = command.at(2).value;
  std::string opvalue = command.at(4).value;
  int ret = mg->set_option(category, opname, opvalue);
  if (ret)
    std::cout << "error setting option " + opname + " = " + opvalue << std::endl;
  return ret;
}


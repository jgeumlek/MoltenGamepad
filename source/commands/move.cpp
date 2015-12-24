#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define MOVE_USAGE "USAGE:\n\tmove <device> to <slot>"
int do_move(moltengamepad* mg, std::vector<token> &command) {
  if (command.size() < 4) {std::cout<<MOVE_USAGE<<std::endl;return -1;}
  if (command.at(2).value != "to") {std::cout<<MOVE_USAGE<<std::endl;return -1;}
  std::string devname = command.at(1).value;
  std::string slotname = command.at(3).value;
  input_source* dev = mg->find_device(devname.c_str());
  virtual_device* slot = mg->slots->find_slot(slotname);
  if (!dev) {std::cout << "device " << devname << "not found.\n"<<MOVE_USAGE << std::endl; return -1;};
  if (!slot) {std::cout << "slot " << slotname << "not found.\n"<<MOVE_USAGE << std::endl; return -1;};
  mg->slots->move_to_slot(dev,slot);
  return 0;
}
  

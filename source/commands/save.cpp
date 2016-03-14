#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

int do_print_profile(moltengamepad* mg, std::string name, std::ostream& out);

#define SAVE_USAGE "USAGE:\n\tsave profiles to <filename>\n\tFile will be placed in the profile directory"
int do_save(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 4) {
    std::cout << SAVE_USAGE << std::endl;
    return -1;
  };
  if (command.at(1).value != "profiles" || command.at(2).value != "to") {
    std::cout << SAVE_USAGE << std::endl;
    return -1;
  };
  std::string filename = mg->options.profile_dir + "/" + command.at(3).value;
  for (int i = 4; i < command.size(); i++) {
    if (command.at(i).type != TK_ENDL) filename += command.at(i).value;
  }
  std::cout << "attempting to save to " << filename << std::endl;
  std::ofstream file;
  file.open(filename, std::ofstream::out);
  if (file.fail()) {
    std::cout << "could not open file" << std::endl;
    return -2;
  }
  for (auto it = mg->devs.begin(); it != mg->devs.end(); it++) {
    file << "[" << (*it)->name << "]" << std::endl;
    do_print_profile(mg, (*it)->name, file);
  }
  file.close();
  return 0;
}

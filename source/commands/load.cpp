#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define LOAD_USAGE "USAGE:\n\tload profiles from <filename>\n\tFile will be searched for in the profile directory"
int do_load(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 4) {
    std::cout << LOAD_USAGE << std::endl;
    return -1;
  }
  if (command.at(1).value != "profiles" || command.at(2).value != "from") {
    std::cout << LOAD_USAGE << std::endl;
    return -1;
  }
  std::string filename = command.at(3).value;
  for (int i = 4; i < command.size(); i++) {
    if (command.at(i).type != TK_ENDL) filename += command.at(i).value;
  }
  filename = mg->locate(FILE_PROFILE,filename);
  std::cout << "attempting to load from " << filename << std::endl;
  std::ifstream file;
  file.open(filename, std::ifstream::in);
  if (file.fail()) {
    std::cout << "could not open file" << std::endl;
    return -2;
  }
  shell_loop(mg, file);
  file.close();
  return 0;
}


#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define LOAD_USAGE "USAGE:\n\tload profiles from <filename>\n\tFile will be searched for in the profile directory"
int do_load(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 4) {
    out->print(LOAD_USAGE);
    return -1;
  }
  if (command.at(1).value != "profiles" || command.at(2).value != "from") {
    out->print(LOAD_USAGE);
    return -1;
  }
  std::string filename = command.at(3).value;
  for (int i = 4; i < command.size(); i++) {
    if (command.at(i).type != TK_ENDL) filename += command.at(i).value;
  }
  std::string foundname = mg->locate(FILE_PROFILE,filename);
  if (foundname.empty()) {
    out->err("could not locate file " + filename);
    return -2;
  }
  out->print("attempting to load from " + foundname);
  std::ifstream file;
  file.open(foundname, std::ifstream::in);
  if (file.fail()) {
    out->err("could not open file " + foundname);
    return -2;
  }
  shell_loop(mg, file);
  file.close();
  return 0;
}


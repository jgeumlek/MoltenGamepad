#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define LOAD_USAGE "USAGE:\n\tload profiles from <filename>\n\tFile will be searched for in the profile directory\n"
int MGparser::do_load(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 4) {
    out.print(LOAD_USAGE);
    return -1;
  }
  if (command.at(1).value != "profiles" || command.at(2).value != "from") {
    out.print(LOAD_USAGE);
    return -1;
  }
  std::string filename = mg->options.profile_dir + "/" + command.at(3).value;
  for (int i = 4; i < command.size(); i++) {
    if (command.at(i).type != TK_ENDL) filename += command.at(i).value;
  }
  out.text("attempting to load from " + filename);
  std::ifstream file;
  file.open(filename, std::ifstream::in);
  if (file.fail()) {
    out.text("could not open file");
    return -2;
  }
  shell_loop(mg, file, 1, message_stream::PLAINTEXT);
  file.close();
  return 0;
}


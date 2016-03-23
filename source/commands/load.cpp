#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"


#define LOAD_USAGE "USAGE:\n\tload profiles from <filename>\n\tFile will be searched for in the profile directory"
int MGparser::do_load(moltengamepad* mg, std::vector<token>& command) {
  if (command.size() < 4) {
    out.take_message(LOAD_USAGE);
    return -1;
  }
  if (command.at(1).value != "profiles" || command.at(2).value != "from") {
    out.take_message(LOAD_USAGE);
    return -1;
  }
  std::string filename = mg->options.profile_dir + "/" + command.at(3).value;
  for (int i = 4; i < command.size(); i++) {
    if (command.at(i).type != TK_ENDL) filename += command.at(i).value;
  }
  out.take_message("attempting to load from " + filename);
  std::ifstream file;
  file.open(filename, std::ifstream::in);
  if (file.fail()) {
    out.take_message("could not open file");
    return -2;
  }
  shell_loop(mg, file, 1, message_stream::PLAINTEXT);
  file.close();
  return 0;
}


#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"



int do_load(moltengamepad* mg, std::vector<token> &command) {
  if (command.size() < 4) return -1;
  if (command.at(1).value != "profiles" || command.at(2).value != "from") return -1;
  std::string filename = mg->options.profile_dir + "/" +command.at(3).value;
  std::cout << "attempting to load from " << filename << std::endl;
  std::ifstream file;
  file.open(filename, std::ifstream::in);
  if (file.fail()) return -2;
  shell_loop(mg,file);
  file.close();
  return 0;
}
  

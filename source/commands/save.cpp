#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

int do_print_profile(moltengamepad* mg, std::string name, std::ostream &out);

int do_save(moltengamepad* mg, std::vector<token> &command) {
  if (command.size() < 4) return -1;
  if (command.at(1).value != "profiles" || command.at(2).value != "to") return -1;
  std::string filename = mg->options.profile_dir + "/" +command.at(3).value;
  std::cout << "attempting to save to " << filename << std::endl;
  std::ofstream file;
  file.open(filename, std::ofstream::out);
  if (file.fail()) return -2;
  for (auto it = mg->devs.begin(); it != mg->devs.end(); it++) {
    file << "[" << (*it)->name << "]" << std::endl;
    do_print_profile(mg,(*it)->name,file);
  }
  file.close();
  return 0;
}

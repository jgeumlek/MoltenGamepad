#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

int do_print_profile(moltengamepad* mg, std::string name, std::ostream& out);

#define SAVE_USAGE "USAGE:\n\tsave profiles [profile name, ...] to <filename>\n\tFile will be placed in the profile directory"
int do_save(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 4) {
    out->print(SAVE_USAGE);
    return -1;
  };
  if (command.at(1).value != "profiles" && command.at(1).value != "profile") {
    out->print(SAVE_USAGE);
    return -1;
  };

  std::vector<std::string> profiles_to_save;
  int i = 2;
  for (i = 2; i < command.size() && command.at(i).value != "to"; i++) {
    if (command.at(i).type == TK_IDENT)
      profiles_to_save.push_back(command.at(i).value);
  }

  if (i >= command.size() - 1) {
    out->print(SAVE_USAGE);
    return -1; //Ran out of tokens before getting to the filename
  }
  i++;
  
  std::string filename = command.at(i).value;
  i++;
  for ( ; i < command.size(); i++) {
    if (command.at(i).type != TK_ENDL) filename += command.at(i).value;
  }
  //If it is not an absolute path, place it relative the profile directory
  if (!filename.empty() && filename.front() != '/')
    filename = mg->locate(FILE_PROFILE,"") + "/" + filename;

  char* resolved = realpath(filename.c_str(), nullptr);
  if (resolved) {
    filename = std::string(resolved);
    free(resolved);
  }
  out->print("attempting to save to " + filename);
  std::ofstream file;
  file.open(filename, std::ofstream::out);
  if (file.fail()) {
    out->err("could not open file " + filename);
    return -2;
  }

  if (profiles_to_save.empty()) {
    //Just save the various manager profiles, ignore the transient device profiles.
    for (auto it = mg->managers.begin(); it != mg->managers.end(); it++)
      profiles_to_save.push_back((*it)->name);
  } 

  for (auto prof : profiles_to_save) {
    //TODO: Refactor so we can detect a profile doesn't exist earlier.
    out->print("saving profile ");
    file << "[" << prof << "]" << std::endl;
    do_print_profile(mg, prof, file);
  }

  file.close();
  return 0;
}

#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "parser.h"


int do_save(moltengamepad* mg, std::vector<token> &command);
int do_print(moltengamepad* mg, std::vector<token> &command);
int do_load(moltengamepad* mg, std::vector<token> &command);
  

int do_command(moltengamepad* mg, std::vector<token> &command) {
  if (command.empty()) return -1;
  if (command.front().value == "print") {
    device_delete_lock.lock();
    do_print(mg,command);
    device_delete_lock.unlock();
  }
    
  if (command.front().value == "save") return do_save(mg,command);
  if (command.front().value == "load") return do_load(mg,command);
  return 0;
}



int shell_loop(moltengamepad* mg, std::istream &in) {
  bool keep_looping = true;
  std::string header = "";
  char* buff = new char [1024];
  while(keep_looping) {
    in.getline(buff,1024);
    
    auto tokens = tokenize(std::string(buff));
    
    if (!tokens.empty() && tokens.front().value == "quit") {
      keep_looping = false;
    }
    
    parse_line(tokens,header,mg);
    
    
    if (in.eof()) break;
    

  }
  
    delete[] buff;
}


#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "parser.h"

int do_print_profile(moltengamepad* mg, std::string name, std::ostream &out) {
  device_manager* man = mg->find_manager(name.c_str());
  if (man) {
    profile* profile = &(man->mapprofile);
    for (auto it = profile->mapping.begin(); it != profile->mapping.end(); it++) {
      out << man->name << "." << it->first << " = " << it->second->to_string() << std::endl;
    }
    
    return 0;
  }
  return -1;
    
}
int do_print_drivers(moltengamepad* mg, std::string name, std::ostream &out) {
  if (name.empty()) {
    for (auto drv : mg->devs) {
      out << drv->name <<  std::endl;
    }
    return 0;
  }
  device_manager* man = mg->find_manager(name.c_str());
  if (man) {
    name_list list;
    man->list_devs(list);
    for (auto e : list) {
        out << e.name << ":\t" << e.descr << std::endl;
    }
    
    
  }
  input_source* dev =mg->find_device(name.c_str());
  if (dev) {
    cat_list cats;
    dev->list_events(cats);
    for (auto v : cats) {
      out << "[" << v.name << "]" << std::endl;
      for (auto e : v.entries) {
        out << e.name << ":\t" << e.descr << std::endl;
      }
    }
    
  }
  return 0;
  
}

int do_print(moltengamepad* mg, std::vector<token> &command) {
  if (command.size() < 2) return -1;
  std::string arg = (command.size() >= 3) ? command.at(2).value : "";
  if (command.at(1).value == "drivers") return do_print_drivers(mg,arg,std::cout);
  
  if (command.at(1).value == "profile") return do_print_profile(mg,arg,std::cout);
  
  
  return 0;
}

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
  

int do_command(moltengamepad* mg, std::vector<token> &command) {
  if (command.empty()) return -1;
  if (command.front().value == "print") return do_print(mg,command);
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


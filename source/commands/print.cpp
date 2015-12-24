#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

void print_profile(profile &profile, std::ostream &out) {
  for (auto it = profile.mapping.begin(); it != profile.mapping.end(); it++) {
    out << profile.name << "." << it->first << " = " << it->second->to_string() << std::endl;
  }
  for (auto chord : profile.chords) {
    out << profile.name << ".(" << chord.first.first << ","<< chord.first.second <<") = " << chord.second->to_string() << std::endl;
  }
}

int do_print_profile(moltengamepad* mg, std::string name, std::ostream &out) {
  device_manager* man = mg->find_manager(name.c_str());
  if (man) {
    profile* profile = &(man->mapprofile);
    print_profile(*profile,out);
    
    return 0;
  } else {
    input_source* dev =mg->find_device(name.c_str());
    if (dev) {
      profile profile;
      dev->export_profile(&profile);
      profile.name = dev->name;
      print_profile(profile,out);
    }
  }
  return -1;
    
}

void print_devs(device_manager* man, std::ostream &out) {
  name_list list;
  man->list_devs(list);
  out << "(" << man->name << ")\n";
  if (list.empty()) { out << "  no devices" << std::endl; }
  for (auto e : list) {
      out << e.name << ":\t" << e.descr << std::endl;
  }
}

int do_print_devs(moltengamepad* mg, std::string driver, std::ostream &out) {
  if (!driver.empty()) {
    device_manager* man = mg->find_manager(driver.c_str());
    if (man) {
      print_devs(man,out);
      return 0;
    }
    return -1;
  }
  
  for (auto driver : mg->devs) {
    print_devs(driver,out);
  }
  
  return 0;
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
  std::string arg = (command.size() >= 3 && command.at(2).type == TK_IDENT) ? command.at(2).value : "";
  if (command.at(1).value == "drivers") return do_print_drivers(mg,arg,std::cout);
  if (command.at(1).value == "devices") return do_print_devs(mg,arg,std::cout);
  
  if (command.at(1).value == "profile") return do_print_profile(mg,arg,std::cout);
  
  
  return 0;
}



#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

void print_profile(profile &profile, std::ostream &out) {
  for (auto it = profile.mapping.begin(); it != profile.mapping.end(); it++) {
    out << profile.name << "." << it->first << " = ";
    MGTransDef def;
    it->second.trans->fill_def(def);
    MGparser::print_def(it->second.type,def,out);
    out << std::endl;
  }
  for (auto entry : profile.adv_trans) {
    out << profile.name << ".(" << entry.first << ") = ";
    MGTransDef def;
    entry.second.trans->fill_def(def);
    MGparser::print_def(NO_ENTRY,def,out);
    out << std::endl;
  }
  for (auto it = profile.options.begin(); it != profile.options.end(); it++) {
    out << profile.name << ".?" << it->first << " = " << it->second << std::endl;
  }
  if (profile.mapping.empty() && profile.adv_trans.empty() && profile.options.empty())
    out << "#(empty profile)"<<std::endl;
}

int do_print_profile(moltengamepad* mg, std::string name, std::ostream &out) {
  if (name.empty()) {
    for (auto man : mg->devs) {
      out << man->name << std::endl;
      name_list names;
      man->list_devs(names);
      for (auto e : names)
        out << e.name << std::endl;
    }
  }
  
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

void print_driver_dev_list(device_manager* man, std::ostream &out) {
  name_list list;
  man->list_devs(list);
  out << "(" << man->name << ")\n";
  if (list.empty()) { out << "  no devices" << std::endl; }
  for (auto e : list) {
      out << e.name << ":\t" << e.descr << std::endl;
  }
}

int do_print_devs(moltengamepad* mg, std::string name, std::ostream &out) {
  if (!name.empty()) {
    input_source* dev =mg->find_device(name.c_str());
    if (dev) {
      out << dev->name << " events:"<<std::endl;
      cat_list cats;
      dev->list_events(cats);
      for (auto v : cats) {
        out << "[" << v.name << "]" << std::endl;
        for (auto e : v.entries) {
          out << e.name << ":\t" << e.descr << std::endl;
        }
      }
      
      std::vector<source_option> list;
      dev->list_options(list);
      out << "\n"<<dev->name << " options:"<<std::endl;
      for (auto v: list) {
        out << "?" << v.name <<" = " << v.value <<"\n\t" << v.descr << std::endl;
      }
      if (list.empty()) { out <<"(no options)" << std::endl;}
      
      return 0;
    }
    
    return -1;
  }
  
  for (auto driver : mg->devs) {
    print_driver_dev_list(driver,out);
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
    print_driver_dev_list(man,out);
  }
  
  return 0;
  
}

int do_print_slots(moltengamepad* mg, std::string name, std::ostream &out) {
  if (name.empty()) {
    for (auto slot : mg->slots->slots) {
      out << slot->name << ":\t" << slot->descr <<  std::endl;
    }
    out << mg->slots->keyboard->name << ":\t" << mg->slots->keyboard->descr <<std::endl;
    out << mg->slots->dummyslot->name<< ":\t" << mg->slots->dummyslot->descr <<std::endl;
    if (mg->slots->debugslot) out << mg->slots->debugslot->name<< ":\t" << mg->slots->debugslot->descr <<std::endl;
    return 0;
  }
  virtual_device* slot = mg->slots->find_slot(name);
  if (slot) {
    out << slot->name << ":\t" << slot->descr << std::endl;
    for (auto e : slot->options) {
      out << "\t"<< e.first << " = " << e.second << std::endl;
    }
  }
  
  return 0;
  
}

#define PRINT_USAGE ""\
"USAGE:\n\tprint <type> [element]\n"\
"\ttypes recognized: drivers, devices, profiles, slots\n"\
"\tprint <type> will list all elements of that type\n"\
"\tprint <type> [element] will show detailed info on that element\n"
int do_print(moltengamepad* mg, std::vector<token> &command) {
  if (command.size() < 2) {std::cout<<PRINT_USAGE<<std::endl; return -1;}
  std::string arg = (command.size() >= 3 && command.at(2).type == TK_IDENT) ? command.at(2).value : "";
  if (command.at(1).value == "drivers") return do_print_drivers(mg,arg,std::cout);
  if (command.at(1).value == "devices") return do_print_devs(mg,arg,std::cout);
  
  if (command.at(1).value == "profiles") return do_print_profile(mg,arg,std::cout);
  if (command.at(1).value == "slots") return do_print_slots(mg,arg,std::cout);
  
  std::cout<<PRINT_USAGE<<std::endl;
  return 0;
}



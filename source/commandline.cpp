#include "moltengamepad.h"
#include <iostream>
#include <string>
#include <cctype>

char* remove_space(char* string) {
  while (*string && isspace(*string)) string++;
  if (!*string) return string;
  char* end = string;
  while (*end) *end++;
  end--;
  
  while (isspace(*end) && end != string) end--;
  *(end+1) = '\0';
  return string;
}

event_translator* make_trans(enum entry_type intype, char* outname);

int do_command(moltengamepad* mg, char* leftside, char* rightside) {
  
  if (!strncmp(leftside,"print",5)) {
    char* arg1 = leftside+5;
    while(*arg1 && isspace(*arg1)) arg1++;
    arg1 = remove_space(rightside);
    device_manager* man = mg->find_manager(arg1);
    //std::cout <<"comm:" << leftside << " arg:" << arg1 << std::endl;
    if (man) {
      profile* profile = &(man->mapprofile);
      for (auto it = profile->mapping.begin(); it != profile->mapping.end(); it++) {
        std::cout << man->name << "." << it->first << " = " << it->second->to_string() << std::endl;
      }
    }
  }
}

int do_update(moltengamepad* mg, const char* header, char* leftside, char* rightside) {
  char* dot = strchr(leftside,'.');
  //Allow prefixing a different header via a .;
  if (dot != nullptr) {
    *dot = '\0';
    header = remove_space(leftside);
    leftside = remove_space(dot+1);
  }
  //std::cout << "H:" << header << " L:" << leftside << " R:" << rightside << std::endl;
  
  
  if (*header == '\0' || !strcmp(header,"moltengamepad")) {
    do_command(mg, leftside, rightside);
    return 0;
  }
  
  device_manager* man = mg->find_manager(header);
  if (man != nullptr) {
    enum entry_type entry_type = man->entry_type(leftside);
    //OPTION, KEY, AXIS, NOT_FOUND
    std::cout << header << "->" << leftside << " is " << entry_type << std::endl;
    if (entry_type == DEV_KEY || entry_type == DEV_AXIS) {
      event_translator* trans = make_trans(entry_type, rightside);
      if (trans) man->update_maps(leftside,trans);
      if (trans) std::cout << trans->to_string() << std::endl;
      if (trans) delete trans;
    }
    
    return 0;
  }
  
  input_source* dev = mg->find_device(header);
  if (dev != nullptr) {
    enum entry_type entry_type = dev->entry_type(leftside);
    //OPTION, KEY, AXIS, NOT_FOUND
    dev->update_map(leftside,nullptr);
    return 0;
  }
    
}

int shell_loop(moltengamepad* mg, std::istream &in) {
  bool keep_looping = true;
  std::string header = "";
  char* buff = new char [1024];
  while(keep_looping) {
    in.getline(buff,1024);
    if (in.eof()) break;
    
    
    
    char* comm = buff;
    
    while (*comm && isspace(*comm)) comm++;
    if (!*comm || *comm == '#' || *comm == '\n') continue;
    
    if (*comm == '[') {
      comm++;
      while (*comm && isspace(*comm)) comm++;
      if (!*comm) continue;
      
      if (*comm == ']') {
        header = "\0";
        continue;
      }
      
      char* end = comm;
      while (*end && *end != ']') *end++;
      while (*end != ']' && end != comm) end--;
      
      if (end == comm) continue;
      end--;
      while (isspace(*end)) end--;
      *(end+1) = '\0';
      
      header = comm;
      continue;
    }
      
    
    char* arg = comm;
    while (*arg && isalnum(*arg) || *arg == '.' || *arg == '_') *arg++;
    
    if (!*arg) {
      if (!strcmp(comm,"quit")) {
        keep_looping = false;
        continue;
      }
    }
    
    char* rightside = arg;
    
    while (*rightside && *rightside != '=') rightside++;
    if (!*rightside) rightside--;
    rightside++;
    
    rightside = remove_space(rightside);
    
    if (*arg) {
      *arg = '\0';
      arg++;
    }
    if (!*rightside) rightside = arg; 
    
    
    do_update(mg, header.c_str(), comm, rightside);
      
    
  }
  
    delete[] buff;
}

event_translator* make_trans(enum entry_type intype, char* outname) {
  int direction = 1;
  if (outname[0] == '-')  {
    direction = -1;
    outname++;
  }
  struct event_info out_info = lookup_event(outname);
  
  enum out_type outtype = out_info.type;
  
  //std::cout << outname << " is a " << out_info.type << std::endl;
  
  if (intype == DEV_KEY) {
    if (outtype == OUT_KEY)
      return new btn2btn(out_info.value);
    if (outtype == OUT_ABS)
      return new btn2axis(out_info.value, direction);
  }
  
  if (intype == DEV_AXIS) {
    if (outtype == OUT_ABS)
      return new axis2axis(out_info.value,direction);
    
    //Try to split the name into two buttons.
    char* split = strchr(outname,',');
    if (split) {
      *split = '\0';
      char* neg = remove_space(outname);
      char* pos = remove_space(split+1);
      int negbtn = get_key_id(neg);
      int posbtn = get_key_id(pos);
      if (negbtn == -1 || posbtn == -1)
        return nullptr;
      
      return new axis2btns(negbtn,posbtn);
    }
  }
  
  return nullptr;
}

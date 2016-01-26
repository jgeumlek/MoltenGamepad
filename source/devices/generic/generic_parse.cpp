#include <iostream>
#include "generic.h"
#include "../../parser.h"
#include "../../moltengamepad.h"
#include "../../eventlists/eventlist.h"





void generic_assignment_line(std::vector<token> &line, generic_driver_info* &info, moltengamepad* mg) {
  
  auto it = line.begin();
  if (it == line.end()) return;
  
  std::string field = line.front().value;
  std::string prefix = "";
  
  it++;
  
  if (it == line.end()) return;
  
  if ((*it).type == TK_DOT) {
    it++;
    if (it == line.end()) return;
    prefix = field;
    field = (*it).value;
    it++;
    
  }
    
  
  if ((*it).type != TK_EQUAL) return;
  
  it++; //Skip past the "="
  
  if (it == line.end()) return;
  
  std::string value = (*it).value;
  std::string descr = "(generic)";
  
  it++;
  
  if (it != line.end() && (*it).type == TK_COMMA)  {
    it++;
    if (it != line.end() && (*it).type == TK_IDENT)  {
      descr = (*it).value;
    }
  }

  if (field == "name") {
    info->name = value;
    return;
  }

  if (field == "devname") {
    info->devname = value;
    return;
  }

  if (field == "exclusive" && value == "true") {
    info->grab_ioctl = true;
    return;
  }

  if (field == "change_permissions" && value == "true") {
    info->grab_chmod = true;
    return;
  }

  if (field == "flatten" && value == "true") {
    info->flatten = true;
    return;
  }
  
  if (field == "split") {
    try {
      int split_count  = std::stoi(value);
      info->split = split_count;
      
    } catch (...) {
      
    }
    return;
  }
  
  int split_id = 1;
  if (!prefix.empty()) {
    try {
      split_id  = std::stoi(prefix);
      
    } catch (...) {
      
    }
  }
  
  int id = get_key_id(field.c_str());
  if (id != -1) {
    gen_source_event ev;
    ev.name = value;
    ev.id = id;
    ev.descr = descr;
    ev.type = DEV_KEY;
    ev.split_id = split_id;
    info->events.push_back(ev);
    return;
  }
  
  id = get_axis_id(field.c_str());
  if (id != -1) {
    gen_source_event ev;
    ev.name = value;
    ev.id = id;
    ev.descr = descr;
    ev.type = DEV_AXIS;
    ev.split_id = split_id;
    info->events.push_back(ev);
    return;
  }
  
  
  
}

void generic_parse_line(std::vector<token> &line, generic_driver_info* &info, moltengamepad* mg) {
 
  
  
  if (find_token_type(TK_HEADER_OPEN,line)) {
    std::string newhead;
    do_header_line(line, newhead);
    if (!newhead.empty()) {
      
      
      if (info->events.size() > 0 && !info->name.empty() && !info->devname.empty()) {
        mg->devs.push_back(new generic_manager(mg,*info));
        info = new generic_driver_info;
      }
      info->matches.push_back({0,0,std::string(newhead)});
    }
    return;
  }
  
  if (find_token_type(TK_EQUAL, line)) {
    generic_assignment_line(line, info, mg);
    return;
  }
  
}

int generic_config_loop(moltengamepad* mg, std::istream &in) {
  bool keep_looping = true;
  std::string header = "";
  char* buff = new char [1024];
  bool failed = false;
  struct generic_driver_info* info = new generic_driver_info;
  while(keep_looping) {
    in.getline(buff,1024);
    
    auto tokens = tokenize(std::string(buff));
    
    if (!tokens.empty() && tokens.front().value == "quit") {
      keep_looping = false;
    }
    
    generic_parse_line(tokens,info,mg);
    
    
    if (in.eof()) break;
    

  }
  
  if (info->events.size() > 0 && !info->name.empty() && !info->devname.empty()) {
        mg->devs.push_back(new generic_manager(mg,*info));
  } else {
    delete info;
    failed = true;
  }
  
    delete[] buff;
    return -failed;
}


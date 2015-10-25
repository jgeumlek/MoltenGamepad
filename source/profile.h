#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include "event_change.h"

class profile {
public:
  const char* name;
  std::unordered_map<std::string, event_translator*> mapping;
  
  event_translator* get_mapping(std::string in_event_name) {
    auto it = mapping.find(in_event_name);
    if (it == mapping.end()) return nullptr;
    return (it->second);
  }
  
  event_translator* copy_mapping(std::string in_event_name) {
    auto it = mapping.find(in_event_name);
    if (it == mapping.end()) return new event_translator();
    return (it->second->clone());
  }
  
  void set_mapping(std::string in_event_name, event_translator* mapper) {
    event_translator* oldmap = get_mapping(in_event_name);
    if (oldmap) delete oldmap;
    mapping.erase(in_event_name);
    mapping[in_event_name] = mapper;
  }
  
  ~profile() {
    for (auto it = mapping.begin(); it != mapping.end(); it++) {
      if (it->second) delete it->second;
    }
    
    mapping.clear();
  }
    
};

#endif
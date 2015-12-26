#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include <map>
#include "event_change.h"

typedef std::pair<std::string,std::string> str_pair;

class profile {
public:
  const char* name;
  std::unordered_map<std::string, event_translator*> mapping;
  std::unordered_map<std::string, std::string> options;
  std::map<str_pair, event_translator*> chords;
  
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
  
  void set_chord(std::string ev1, std::string ev2, event_translator* mapper) {
    str_pair key(ev1,ev2);
    auto it = chords.find(key);
    if (it != chords.end()) {
      delete it->second;
      chords.erase(it);
    }
    chords[key] = mapper;
  }
  
  void set_option(std::string opname, std::string value) {
    options.erase(opname);
    options[opname] = value;
  }
  
  std::string get_option(std::string opname) {
    auto it = options.find(opname);
    if (it == options.end()) return "";
    return it->second;
  }
    
  
  ~profile() {
    for (auto it = mapping.begin(); it != mapping.end(); it++) {
      if (it->second) delete it->second;
    }
    
    mapping.clear();
   }
    
};

#endif
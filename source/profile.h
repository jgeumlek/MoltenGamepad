#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include "event_change.h"

class profile {
public:
  const char* name;
  unordered_map<std::string, event_translator*> mapping;
  event_translator* get_mapping(std::string in_event_name);
  void set_mapping(std::string in_event_name, event_translator* mapper)
};

#endif
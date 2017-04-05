#pragma once
#include <string>

#include "../plugin/plugin.h"
//Anything needed by plugins goes in the above header.
//The following is used in a few places and should be relocated

class moltengamepad;

struct option_info {
  std::string name;
  std::string descr;
  MGField value;
  bool locked = false;
  std::string stringval; //Just because allocation is simpler outside the MGField union.
};

struct source_event {
  int id;
  const char* name;
  const char* descr;
  enum entry_type type;
  int64_t value;
  event_state state;
};

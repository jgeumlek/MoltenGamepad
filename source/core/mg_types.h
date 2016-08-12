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



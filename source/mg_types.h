#pragma once


enum MGType {
  MG_KEY_TRANS,
  MG_REL_TRANS,
  MG_AXIS_TRANS,
  MG_TRANS,
  MG_ADVANCED_TRANS,
  MG_KEY,
  MG_AXIS,
  MG_REL,
  MG_STRING,
  MG_INT,
  MG_FLOAT, //Not implemented...
  MG_BOOL,
  MG_SLOT,
  MG_KEYBOARD_SLOT,
  MG_NULL,
};


struct option_info {
  std::string name;
  std::string descr;
  std::string value;
  MGType type;
  bool locked = false;
}; 

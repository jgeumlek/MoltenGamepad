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



enum entry_type {DEV_OPTION, DEV_KEY, DEV_AXIS, DEV_REL, NO_ENTRY} ;

class event_translator;
class advanced_event_translator;
class output_slot;

struct MGField {
  MGType type;
  union {
    event_translator* trans;
    advanced_event_translator* adv_trans;
    int key;
    int axis;
    int rel;
    char* string;
    int integer;
    bool boolean;
    float real;
    output_slot* slot;
  };
};

struct event_decl {
  const char* name;
  const char* descr;
  enum entry_type type;
  const char* default_mapping = "";
};

struct option_decl {
  const char* name;
  const char* descr;
  const char* value;
  MGType type;
};

struct option_info {
  std::string name;
  std::string descr;
  MGField value;
  bool locked = false;
  std::string stringval; //Just because allocation is simpler outside the union.
};



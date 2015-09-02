#ifndef EVENT_CHANGE_H
#define EVENT_CHANGE_H
#include "virtual_device.h"
#include <linux/input.h>


#define EVENT_KEY 0
#define EVENT_AXIS 1

struct mg_ev {
  int type;
  long long value;
};

class event_translator {
public:
  virtual void process(struct mg_ev, virtual_device* out) {
  }
  
  void write_out(struct input_event ev, virtual_device* out);
};

class btn2btn : event_translator {
public:
  int out_button;
};

class btn2axis : event_translator {
public:
  int out_axis;
  int direction;
};

class axis2axis : event_translator {
public:
  int out_axis;
  int direction;
};

class axis2btns : event_translator {
public:
  int neg_btn;
  int pos_btn;
  
};

#endif
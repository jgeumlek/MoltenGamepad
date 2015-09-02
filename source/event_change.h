#ifndef EVENT_CHANGE_H
#define EVENT_CHANGE_H
#include "virtual_device.h"
#include <linux/input.h>


#define EVENT_KEY 0
#define EVENT_AXIS 1

#define RANGE 32768

struct mg_ev {
  int type;
  long long value;
};

class event_translator {
public:
  virtual void process(struct mg_ev ev, virtual_device* out) {
  }
  
  void write_out(struct input_event ev, virtual_device* out) {
    out->take_event(ev);
  }
};

class btn2btn : public event_translator {
public:
  int out_button;
  btn2btn(int out) : out_button(out) {
  }
  
  virtual void process(struct mg_ev ev, virtual_device* out) {
    struct input_event out_ev;
    out_ev.type = EV_KEY;
    out_ev.code = out_button;
    out_ev.value = ev.value;
    write_out(out_ev,out);
  }
};

class btn2axis : public event_translator {
public:
  int out_axis;
  int direction;
};

class axis2axis : public event_translator {
public:
  int out_axis;
  int direction;
  axis2axis(int axis, int dir) : out_axis(axis), direction(dir) {
  }
  virtual void process(struct mg_ev ev, virtual_device* out) {
    int value = ev.value*direction;
    if (value < -RANGE) value = -RANGE;
    if (value > RANGE) value = RANGE;
    struct input_event out_ev;
    out_ev.type = EV_ABS;
    out_ev.code = out_axis;
    out_ev.value = value;
    write_out(out_ev,out);
  }
};

class axis2btns : public event_translator {
public:
  int neg_btn;
  int pos_btn;
  
};

#endif
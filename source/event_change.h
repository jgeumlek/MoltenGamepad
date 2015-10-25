#ifndef EVENT_CHANGE_H
#define EVENT_CHANGE_H
#include "virtual_device.h"
#include <linux/input.h>
#include <string>
#include "eventlists/eventlist.h"


#define EVENT_KEY 0
#define EVENT_AXIS 1

#define RANGE 32768

struct mg_ev {
  long long value;
};

class event_translator {
public:
  virtual void process(struct mg_ev ev, virtual_device* out) {
  }
  
  void write_out(struct input_event ev, virtual_device* out) {
    out->take_event(ev);
  }
  
  virtual std::string to_string() {
    return "nothing";
  }
  
  virtual event_translator* clone() {return new event_translator(*this);}
  
};

class btn2btn : public event_translator {
public:
  int out_button;
  btn2btn(int out) : out_button(out) {
  }
  
  virtual void process(struct mg_ev ev, virtual_device* out) {
    struct input_event out_ev;
    memset(&out_ev,0,sizeof(out_ev));
    out_ev.type = EV_KEY;
    out_ev.code = out_button;
    out_ev.value = ev.value;
    write_out(out_ev,out);
  }
  virtual std::string to_string() {
    std::string text = "btn2btn(";
    text += std::string(get_key_name(out_button));
    text += ")";
    return text;
  }
  
  virtual btn2btn* clone() {return new btn2btn(*this);}
};

class btn2axis : public event_translator {
public:
  int out_axis;
  int direction;
  
  btn2axis(int out_axis, int direction) : out_axis(out_axis), direction(direction) {
  }
  
  virtual void process(struct mg_ev ev, virtual_device* out) {
    struct input_event out_ev;
    memset(&out_ev,0,sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = out_axis;
    out_ev.value = ev.value*direction*RANGE;
    write_out(out_ev,out);
  }
  
  virtual std::string to_string() {
    std::string text = "btn2axis(";
    text += std::string(get_axis_name(out_axis));
    text += ",";
    text += std::to_string(direction);
    text += ")";
    return text;
  }
  
  virtual btn2axis* clone() {return new btn2axis(*this);}
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
    memset(&out_ev,0,sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = out_axis;
    out_ev.value = value;
    write_out(out_ev,out);
  }
  virtual std::string to_string() {
    std::string text = "axis2axis(";
    text += std::string(get_axis_name(out_axis));
    text += ",";
    text += std::to_string(direction);
    text += ")";
    return text;
  }
  virtual axis2axis* clone() {return new axis2axis(*this);}
};

class axis2btns : public event_translator {
public:
  int neg_btn;
  int pos_btn;
  axis2btns(int neg_btn, int pos_btn) : neg_btn(neg_btn), pos_btn(pos_btn) {
  }
  
  virtual void process(struct mg_ev ev, virtual_device* out) {
    struct input_event out_ev;
    memset(&out_ev,0,sizeof(out_ev));
    out_ev.type = EV_KEY;
    out_ev.code = neg_btn;
    out_ev.value = ev.value < -.8*RANGE;
    write_out(out_ev,out);
    
    out_ev.type = EV_KEY;
    out_ev.code = pos_btn;
    out_ev.value = ev.value > .8*RANGE;
    write_out(out_ev,out);
    
  }
  
  virtual std::string to_string() {
    std::string text = "axis2btns(";
    text += std::string(get_key_name(neg_btn));
    text += ",";
    text += std::string(get_key_name(pos_btn));
    text += ")";
    return text;
  }
  
  virtual axis2btns* clone() {return new axis2btns(*this);}

};



#endif
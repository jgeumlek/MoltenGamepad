#ifndef EVENT_CHANGE_H
#define EVENT_CHANGE_H
#include "virtual_device.h"
#include "devices/device.h"
#include <linux/input.h>
#include <string>
#include "eventlists/eventlist.h"


#define EVENT_KEY 0
#define EVENT_AXIS 1

#define RANGE 32768

struct mg_ev {
  long long value;
};

class input_source;

//A simple event translator. Takes one input event, and translates it. Essentially just a "pipe".
class event_translator {
public:
  virtual void process(struct mg_ev ev, virtual_device* out) {
  }
  
  void write_out(struct input_event ev, virtual_device* out) {
    out->take_event(ev);
  }

  //Should return the syntax used to make it.
  //TODO: just go ahead and cache the string used to make it!
  virtual std::string to_string() {
    return "nothing";
  }
  
  //event translators are passed around via cloning.
  //This isn't just for memory management, but also
  //lets the profile store a translator as a "prototype"
  virtual event_translator* clone() {return new event_translator(*this);}
  
  virtual ~event_translator() {};
  
};

//A more complicated event translator. It can request to listen to multiple events.
class advanced_event_translator {
public:
  //Attach to an input source. Acts as an initializer.
  virtual void attach(input_source* source) {};
  //Return true to block the input source's native handling of this event.
  virtual bool claim_event(int id, mg_ev event) { return false; };
  //Similar to the above, acts as a prototype method.
  virtual advanced_event_translator* clone() {return new advanced_event_translator(*this);}
  virtual ~advanced_event_translator() {};
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
  virtual std::string to_string_long() {
    std::string text = "btn2btn(";
    text += std::string(get_key_name(out_button));
    text += ")";
    return text;
  }
  virtual std::string to_string() {
    const char* btn_name = get_key_name(out_button);
    if (!btn_name) return to_string_long();
    if (btn_name[0] == 'k') return to_string_long();
    return std::string(btn_name);
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
  
  virtual std::string to_string_long() {
    std::string text = "btn2axis(";
    text += std::string(get_axis_name(out_axis));
    text += ",";
    text += std::to_string(direction);
    text += ")";
    return text;
  }
  
  virtual std::string to_string() {
    if (direction != -1 && direction != +1) return to_string_long();
    std::string prefix = direction > 0 ? "+" : "-";
    return prefix + std::string(get_axis_name(out_axis));
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
  virtual std::string to_string_long() {
    std::string text = "axis2axis(";
    text += std::string(get_axis_name(out_axis));
    text += ",";
    text += std::to_string(direction);
    text += ")";
    return text;
  }
  virtual std::string to_string() {
    if (direction != -1 && direction != +1) return to_string_long();
    std::string prefix = direction > 0 ? "+" : "-";
    return prefix + std::string(get_axis_name(out_axis));
  }
  virtual axis2axis* clone() {return new axis2axis(*this);}
};

class axis2btns : public event_translator {
public:
  int neg_btn;
  int pos_btn;
  
  int neg_cache = 0;
  int pos_cache = 0;
  axis2btns(int neg_btn, int pos_btn) : neg_btn(neg_btn), pos_btn(pos_btn) {
  }

  virtual void process(struct mg_ev ev, virtual_device* out) {
    struct input_event out_ev;
    memset(&out_ev,0,sizeof(out_ev));
    out_ev.type = EV_KEY;
    out_ev.code = neg_btn;
    out_ev.value = ev.value < -.8*RANGE;
    if (out_ev.value != neg_cache) {
      write_out(out_ev,out);
      neg_cache = out_ev.value;
    }
    
    out_ev.type = EV_KEY;
    out_ev.code = pos_btn;
    out_ev.value = ev.value > .8*RANGE;
    if (out_ev.value != pos_cache) {
      write_out(out_ev,out);
      pos_cache = out_ev.value;
    }
    
  }
  
  virtual std::string to_string_long() {
    std::string text = "axis2btns(";
    text += std::string(get_key_name(neg_btn));
    text += ",";
    text += std::string(get_key_name(pos_btn));
    text += ")";
    return text;
  }
  virtual std::string to_string() {
    return std::string(get_key_name(neg_btn)) + "," + std::string(get_key_name(pos_btn));
  }
  
  virtual axis2btns* clone() {return new axis2btns(*this);}

};

class redirect_trans : public event_translator {
public:
  event_translator* trans = nullptr;
  virtual_device* redirected;
  
  redirect_trans(event_translator* trans, virtual_device* redirected) :  redirected(redirected) {
    this->trans = trans;
  }
  
  ~redirect_trans() {
    if (trans) delete trans;
  }
  
  virtual void process(struct mg_ev ev, virtual_device* out) {
    trans->process(ev,redirected);
    struct input_event out_ev;
    memset(&out_ev,0,sizeof(out_ev));
    out_ev.type = EV_SYN;
    out_ev.code = SYN_REPORT;
    out_ev.value = 0;
    write_out(out_ev,redirected);
    
  }
  virtual std::string to_string() {
    return "redirect(" + trans->to_string() + "," + redirected->name + ")";
  }
  
  virtual redirect_trans* clone() {return new redirect_trans(trans->clone(),redirected);}
  
};

class keyboard_redirect : public redirect_trans {
public:
  int key_code;
  keyboard_redirect(int key_code, event_translator* trans, virtual_device* redirected ) : redirect_trans(trans,redirected) {
    this->key_code = key_code;
  }
  
  virtual std::string to_string() {
    return "key(" + std::string(get_key_name(key_code))+")";
  }
  
  virtual keyboard_redirect* clone() {return new keyboard_redirect(key_code,this->trans->clone(),redirected);}
};
  
  



#endif
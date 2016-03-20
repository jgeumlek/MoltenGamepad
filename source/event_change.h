#ifndef EVENT_CHANGE_H
#define EVENT_CHANGE_H
#include "output_slot.h"
#include <linux/input.h>
#include <string>
#include "eventlists/eventlist.h"
#include "devices/device.h"


#define EVENT_KEY 0
#define EVENT_AXIS 1

#define RANGE 32768

struct mg_ev {
  long long value;
};

class input_source;
class event_translator;
class advanced_event_translator;

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
  MG_SLOT,
  MG_KEYBOARD_SLOT,
  MG_NULL,
};
//Maybe add a varargs or tuple type?

struct MGField {
  MGType type;
  union {
    event_translator* trans;
    advanced_event_translator* adv_trans;
    int key;
    int axis;
    int rel;
    std::string* string;
    int integer;
    float real;
    output_slot* slot;
  };
};

struct MGTransDef {
  std::string identifier;
  std::vector<MGField> fields;
};

//A simple event translator. Takes one input event, and translates it. Essentially just a "pipe".
class event_translator {
public:
  virtual void process(struct mg_ev ev, output_slot* out) {
  }

  void write_out(struct input_event ev, output_slot* out) {
    out->take_event(ev);
  }

  //event translators are passed around via cloning.
  //This isn't just for memory management, but also
  //lets the profile store a translator as a "prototype"
  virtual event_translator* clone() {
    return new event_translator(*this);
  }

  virtual ~event_translator() {};

  event_translator(std::vector<MGField>& fields) {};
  virtual void fill_def(MGTransDef& def) {
    def.identifier = "nothing";
  }
  event_translator() {};
};

//A more complicated event translator. It can request to listen to multiple events.
class advanced_event_translator {
public:
  //Initialize any values needed with this input source
  virtual void init(input_source* source) {};
  //Called when the device's thread is ready for attaching.
  virtual void attach(input_source* source) {};
  //Return true to block the input source's native handling of this event.
  virtual bool claim_event(int id, mg_ev event) {
    return false;
  };
  //Similar to the above, acts as a prototype method.
  virtual advanced_event_translator* clone() {
    return new advanced_event_translator(*this);
  }
  virtual ~advanced_event_translator() {};

  advanced_event_translator(std::vector<MGField>& fields) {};
  advanced_event_translator() {};
  virtual void fill_def(MGTransDef& def) {
    def.identifier = "nothing";
  }
};


class btn2btn : public event_translator {
public:
  int out_button;
  btn2btn(int out) : out_button(out) {
  }

  virtual void process(struct mg_ev ev, output_slot* out) {
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_KEY;
    out_ev.code = out_button;
    out_ev.value = ev.value;
    write_out(out_ev, out);
  }

  virtual btn2btn* clone() {
    return new btn2btn(*this);
  }

  static const MGType fields[];
  btn2btn(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

class btn2axis : public event_translator {
public:
  int out_axis;
  int direction;

  btn2axis(int out_axis, int direction) : out_axis(out_axis), direction(direction) {
  }

  virtual void process(struct mg_ev ev, output_slot* out) {
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = out_axis;
    out_ev.value = ev.value * direction * RANGE;
    write_out(out_ev, out);
  }

  virtual btn2axis* clone() {
    return new btn2axis(*this);
  }

  static const MGType fields[];
  btn2axis(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

class axis2axis : public event_translator {
public:
  int out_axis;
  int direction;
  axis2axis(int axis, int dir) : out_axis(axis), direction(dir) {
  }
  virtual void process(struct mg_ev ev, output_slot* out) {
    int value = ev.value * direction;
    if (value < -RANGE) value = -RANGE;
    if (value > RANGE) value = RANGE;
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = out_axis;
    out_ev.value = value;
    write_out(out_ev, out);
  }

  virtual axis2axis* clone() {
    return new axis2axis(*this);
  }

  static const MGType fields[];
  axis2axis(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

class axis2btns : public event_translator {
public:
  int neg_btn;
  int pos_btn;

  int neg_cache = 0;
  int pos_cache = 0;
  axis2btns(int neg_btn, int pos_btn) : neg_btn(neg_btn), pos_btn(pos_btn) {
  }

  virtual void process(struct mg_ev ev, output_slot* out) {
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_KEY;
    out_ev.code = neg_btn;
    out_ev.value = ev.value < -.8 * RANGE;
    if (out_ev.value != neg_cache) {
      write_out(out_ev, out);
      neg_cache = out_ev.value;
    }

    out_ev.type = EV_KEY;
    out_ev.code = pos_btn;
    out_ev.value = ev.value > .8 * RANGE;
    if (out_ev.value != pos_cache) {
      write_out(out_ev, out);
      pos_cache = out_ev.value;
    }

  }

  virtual axis2btns* clone() {
    return new axis2btns(*this);
  }

  static const MGType fields[];
  axis2btns(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);

};

class redirect_trans : public event_translator {
public:
  event_translator* trans = nullptr;
  output_slot* redirected;

  redirect_trans(event_translator* trans, output_slot* redirected) :  redirected(redirected) {
    this->trans = trans->clone();
  }

  ~redirect_trans() {
    if (trans) delete trans;
  }

  virtual void process(struct mg_ev ev, output_slot* out) {
    trans->process(ev, redirected);
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_SYN;
    out_ev.code = SYN_REPORT;
    out_ev.value = 0;
    write_out(out_ev, redirected);

  }

  virtual redirect_trans* clone() {
    return new redirect_trans(trans->clone(), redirected);
  }

  static const MGType fields[];
  redirect_trans(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);

protected:
  redirect_trans() {};

};

class keyboard_redirect : public redirect_trans {
public:
  int key_code;
  keyboard_redirect(int key_code, event_translator* trans, output_slot* redirected) : redirect_trans(trans, redirected) {
    this->key_code = key_code;
  }

  virtual keyboard_redirect* clone() {
    return new keyboard_redirect(key_code, this->trans->clone(), redirected);
  }

  static const MGType fields[];
  keyboard_redirect(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

class multitrans : public event_translator {
public:
  std::vector<event_translator*> translist;
  multitrans(std::vector<event_translator*>& translist) {
    for (auto trans : translist)
      this->translist.push_back(trans->clone());
  }
  ~multitrans() {
    for (auto trans : translist)
      delete trans;
  }
  virtual void process(struct mg_ev ev, output_slot* out) {
    for (auto trans : translist)
      trans->process(ev, out);
  }

  virtual multitrans* clone() {
    return new multitrans(translist);
  }

  static const MGType fields[];
  multitrans(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};




class simple_chord : public advanced_event_translator {
public:
  std::vector<std::string> event_names;
  std::vector<int> event_ids;
  std::vector<int> event_vals;
  int output_cache = 0;
  input_source* source = nullptr;
  event_translator* out_trans = nullptr;
  output_slot** out_dev_ptr;

  simple_chord(std::vector<std::string> event_names, event_translator* trans) : event_names(event_names), out_trans(trans) {};

  virtual ~simple_chord();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);

  virtual bool claim_event(int id, mg_ev event);
  virtual advanced_event_translator* clone() {
    return new simple_chord(event_names, out_trans->clone());
  }

  static const MGType fields[];
  simple_chord(std::vector<std::string> event_names, std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
protected:
  simple_chord() {};
};

class exclusive_chord : public simple_chord {
public:

  exclusive_chord(std::vector<std::string> event_names, event_translator* trans) : simple_chord(event_names, trans) {};

  std::thread* thread = nullptr;
  std::vector<int> chord_hits;



  virtual void init(input_source* source);
  virtual bool claim_event(int id, mg_ev event);
  virtual advanced_event_translator* clone() {
    return new exclusive_chord(event_names, out_trans->clone());
  }

  void thread_func();
  volatile bool thread_active;

  static const MGType fields[];
  exclusive_chord(std::vector<std::string> event_names, std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

//TODO: Do an advanced_event_translator for taking circular x/y axes and making them square.




#endif

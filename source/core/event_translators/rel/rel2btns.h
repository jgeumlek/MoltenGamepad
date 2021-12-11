#pragma once
#include "../event_change.h"

class rel2btns : public event_translator {
public:
  int neg_btn; // key code for button mapped to negative input
  int pos_btn; // key code for button mapped to positive input
  float btn_hold_time; // how long to hold down button for before releasing (seconds)
  mutable struct timeval neg_btn_pressed = {0,0}; // when was negative button last pressed (or 0 if not being pressed)
  mutable struct timeval pos_btn_pressed = {0,0}; // when was positive button last pressed (or 0 if not being pressed)

  rel2btns(int neg_btn, int pos_btn) : neg_btn(neg_btn), pos_btn(pos_btn), btn_hold_time(.05) {
  }

  virtual void process(struct mg_ev ev, virtual_device* out);
  virtual void process_recurring(virtual_device* out) const;
  virtual bool wants_recurring_events();

  virtual rel2btns* clone() {
    return new rel2btns(*this);
  }

  static const char* decl;
  rel2btns(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

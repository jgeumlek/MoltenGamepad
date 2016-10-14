#pragma once
#include "../event_change.h"

class axis2rel : public event_translator {
public:
  int out_rel;
  int speed = 1;
  volatile int value = 0;
  axis2rel(int out, int speed) : out_rel(out), speed(speed) {
  }


  virtual void process(struct mg_ev ev, output_slot* out);
  virtual void process_recurring(output_slot* out) const;
  virtual bool wants_recurring_events();

  virtual axis2rel* clone() {
    return new axis2rel(*this);
  }

  static const char* decl;
  axis2rel(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

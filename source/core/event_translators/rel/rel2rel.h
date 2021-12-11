#pragma once
#include "../event_change.h"

class rel2rel : public event_translator {
public:
  int out_rel; // event code for output
  float speed = 1; // speed multiplier

  rel2rel(int out, float speed) : out_rel(out), speed(speed) {
  }

  virtual void process(struct mg_ev ev, virtual_device* out);
  virtual void process_recurring(virtual_device* out) const;
  virtual bool wants_recurring_events();

  virtual rel2rel* clone() {
    return new rel2rel(*this);
  }

  static const char* decl;
  rel2rel(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

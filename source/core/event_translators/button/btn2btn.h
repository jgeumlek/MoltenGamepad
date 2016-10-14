#pragma once
#include "../event_change.h"

class btn2btn : public event_translator {
public:
  int out_button;
  btn2btn(int out) : out_button(out) {
  }

  virtual void process(struct mg_ev ev, output_slot* out);

  virtual btn2btn* clone() {
    return new btn2btn(*this);
  }

  static const char* decl;
  btn2btn(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

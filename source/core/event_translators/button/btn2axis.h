#pragma once
#include "../event_change.h"

class btn2axis : public event_translator {
public:
  int out_axis;
  int direction;

  btn2axis(int out_axis, int direction) : out_axis(out_axis), direction(direction) {
  }

  virtual void process(struct mg_ev ev, output_slot* out);

  virtual btn2axis* clone() {
    return new btn2axis(*this);
  }

  static const char* decl;
  btn2axis(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

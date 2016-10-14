#pragma once
#include "../event_change.h"

class axis2btns : public event_translator {
public:
  int neg_btn;
  int pos_btn;

  int neg_cache = 0;
  int pos_cache = 0;
  axis2btns(int neg_btn, int pos_btn) : neg_btn(neg_btn), pos_btn(pos_btn) {
  }

  virtual void process(struct mg_ev ev, output_slot* out);

  virtual axis2btns* clone() {
    return new axis2btns(*this);
  }

  static const char* decl;
  axis2btns(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);

};

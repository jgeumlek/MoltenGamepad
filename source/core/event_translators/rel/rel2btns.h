#pragma once
#include "../event_change.h"

class rel2btns : public event_translator {
public:
  int neg_btn;
  int pos_btn;
  rel2btns(int neg_btn, int pos_btn) : neg_btn(neg_btn), pos_btn(pos_btn) {
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

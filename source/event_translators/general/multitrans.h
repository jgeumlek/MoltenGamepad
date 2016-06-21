#pragma once
#include "../event_change.h"

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
  virtual void process(struct mg_ev ev, output_slot* out);

  virtual multitrans* clone() {
    return new multitrans(translist);
  }

  static const MGType fields[];
  multitrans(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

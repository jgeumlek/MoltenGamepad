#include "multitrans.h"
#include "../event_translator_macros.h"

void multitrans::process(struct mg_ev ev, output_slot* out) {
  for (auto trans : translist)
    trans->process(ev, out);
}

const MGType multitrans::fields[] = { MG_TRANS, MG_TRANS, MG_NULL };
multitrans::multitrans(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  event_translator* next;
  READ_TRANS(next,MG_TRANS);
  translist.push_back(next);
  READ_TRANS(next,MG_TRANS);
  translist.push_back(next);
}
void multitrans::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("multi");
  for (auto trans : translist) {
    FILL_DEF_TRANS(trans,MG_TRANS);
  }
}

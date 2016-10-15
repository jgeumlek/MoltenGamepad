#include "multitrans.h"
#include "../event_translator_macros.h"

void multitrans::process(struct mg_ev ev, output_slot* out) {
  for (auto trans : translist)
    trans->process(ev, out);
}

void multitrans::process_recurring(output_slot* out) const {
  for (auto trans : translist)
    trans->process_recurring(out);
}

void multitrans::attach(input_source* source) {
  for (auto trans : translist)
    trans->attach(source);
}

bool multitrans::wants_recurring_events() {
  for (auto trans : translist)
    if (trans->wants_recurring_events())
      return true;

  return false;
}

const char* multitrans::decl = "event = multi(trans [])";
multitrans::multitrans(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  event_translator* next;
  while(HAS_NEXT) {
    READ_TRANS(next,MG_TRANS);
    translist.push_back(next);
  }
}
void multitrans::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("multi");
  for (auto trans : translist) {
    FILL_DEF_TRANS(trans,MG_TRANS);
  }
}

#include "redirect_trans.h"
#include "../event_translator_macros.h"

void redirect_trans::process(struct mg_ev ev, output_slot* out) {
  trans->process(ev, redirected);
  //Since we are redirecting, we can't guarantee when else a SYN_REPORT
  //might be sent. We send it here ourselves.
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_SYN;
  out_ev.code = SYN_REPORT;
  out_ev.value = 0;
  write_out(out_ev, redirected);

}

void redirect_trans::process_recurring(output_slot* out) const {
  trans->process_recurring(redirected);
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_SYN;
  out_ev.code = SYN_REPORT;
  out_ev.value = 0;
  write_out(out_ev, redirected);
}

void redirect_trans::attach(input_source* source) {
  trans->attach(source);
}

bool redirect_trans::wants_recurring_events() {
  return trans->wants_recurring_events();
}

const char* redirect_trans::decl = "event = redirect(trans, slot)";
redirect_trans::redirect_trans(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_TRANS(trans,MG_TRANS);
  READ_SLOT(redirected);
}
void redirect_trans::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("redirect");
  FILL_DEF_TRANS(trans,MG_TRANS);
  FILL_DEF_SLOT(redirected);
}

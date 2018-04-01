#include "redirect_trans.h"
#include "../event_translator_macros.h"

void redirect_trans::process(struct mg_ev ev, virtual_device* out) {
  auto redirected_strong = redirected.lock();
  if (!redirected_strong)
    return;
  trans->process(ev, redirected_strong.get());
  //Since we are redirecting, we can't guarantee when else a SYN_REPORT
  //might be sent. We send it here ourselves.
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_SYN;
  out_ev.code = SYN_REPORT;
  out_ev.value = 0;
  write_out(out_ev, redirected_strong.get());

}

void redirect_trans::process_recurring(virtual_device* out) const {
  auto redirected_strong = redirected.lock();
  if (!redirected_strong)
    return;
  trans->process_recurring(redirected_strong.get());
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_SYN;
  out_ev.code = SYN_REPORT;
  out_ev.value = 0;
  write_out(out_ev, redirected_strong.get());
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
  READ_SLOT_REF(redirected);
}
void redirect_trans::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("redirect");
  FILL_DEF_TRANS(trans,MG_TRANS);
  auto redirected_strong = redirected.lock();
  FILL_DEF_SLOT_FROM_REF(redirected_strong);
}

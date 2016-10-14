#include "btn2btn.h"
#include "../event_translator_macros.h"

void btn2btn::process(struct mg_ev ev, output_slot* out) {
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.code = out_button;
  out_ev.value = ev.value;
  write_out(out_ev, out);
}
const char* btn2btn::decl = "key = btn2btn(key_code)";
btn2btn::btn2btn(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_KEY(out_button);
}
void btn2btn::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("btn2btn");
  FILL_DEF_KEY(out_button);
}

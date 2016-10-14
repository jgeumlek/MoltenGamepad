#include "axis2btns.h"
#include "../event_translator_macros.h"

void axis2btns::process(struct mg_ev ev, output_slot* out) {
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.code = neg_btn;
  out_ev.value = ev.value < -.5 * ABS_RANGE;
  if (out_ev.value != neg_cache) {
    write_out(out_ev, out);
    neg_cache = out_ev.value;
  }

  out_ev.type = EV_KEY;
  out_ev.code = pos_btn;
  out_ev.value = ev.value > .5 * ABS_RANGE;
  if (out_ev.value != pos_cache) {
    write_out(out_ev, out);
    pos_cache = out_ev.value;
  }

}

const char* axis2btns::decl = "axis = axis2btns(key_code, key_code)";
axis2btns::axis2btns(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_KEY(neg_btn);
  READ_KEY(pos_btn);
}
void axis2btns::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("axis2btns");
  FILL_DEF_KEY(neg_btn);
  FILL_DEF_KEY(pos_btn);
}

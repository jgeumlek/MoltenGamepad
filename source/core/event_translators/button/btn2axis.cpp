#include "btn2axis.h"
#include "../event_translator_macros.h"

void btn2axis::process(struct mg_ev ev, output_slot* out) {
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_ABS;
  out_ev.code = out_axis;
  out_ev.value = ev.value * direction * ABS_RANGE;
  write_out(out_ev, out);
}

const char* btn2axis::decl = "key = btn2axis(axis_code, int direction=1)";
btn2axis::btn2axis(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_AXIS(out_axis);
  READ_INT(direction);
}
void btn2axis::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("btn2axis");
  FILL_DEF_AXIS(out_axis);
  FILL_DEF_INT(direction);
}

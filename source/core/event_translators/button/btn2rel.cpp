#include "btn2rel.h"
#include "../event_translator_macros.h"

bool btn2rel::wants_recurring_events() {
  return true;
}



void btn2rel::process(struct mg_ev ev, output_slot* out) {
  value = ev.value ? speed : 0;
}

void btn2rel::process_recurring(output_slot* out) const {
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_REL;
  out_ev.code = out_rel;
  out_ev.value = value;
  if (value) write_out(out_ev, out);
}

const char* btn2rel::decl = "key = btn2rel(rel_code, int speed=3)";
btn2rel::btn2rel(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_REL(out_rel);
  READ_INT(speed);
}
void btn2rel::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("btn2rel");
  FILL_DEF_REL(out_rel);
  FILL_DEF_INT(speed);
}

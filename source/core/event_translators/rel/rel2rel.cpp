#include "rel2rel.h"
#include "../event_translator_macros.h"

bool rel2rel::wants_recurring_events() {
  return false;
}

// create output rel events for input events
void rel2rel::process(struct mg_ev ev, virtual_device* out) {
  // initialize event
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_REL;
  out_ev.code = out_rel;
  out_ev.value = ev.value * speed;

  // send event
  write_out(out_ev, out);
}

void rel2rel::process_recurring(virtual_device* out) const {
}

const char* rel2rel::decl = "rel = rel2rel(rel_code, float speed=1)";
rel2rel::rel2rel(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_REL(out_rel);
  READ_FLOAT(speed);
}
void rel2rel::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("rel2rel");
  FILL_DEF_REL(out_rel);
  FILL_DEF_FLOAT(speed);
}

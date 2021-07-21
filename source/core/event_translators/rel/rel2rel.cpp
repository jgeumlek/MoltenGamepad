#include "rel2rel.h"
#include "../event_translator_macros.h"

bool rel2rel::wants_recurring_events() {
  return false;
}



void rel2rel::process(struct mg_ev ev, virtual_device* out) {
  // value = ev.value * speed;
  // std::cout << "process " << value << std::endl;
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_REL;
  out_ev.code = out_rel;
  out_ev.value = ev.value * speed;
  write_out(out_ev, out);

  // struct input_event out_ev2;
  // memset(&out_ev2, 0, sizeof(out_ev2));
  // out_ev2.type = EV_REL;
  // out_ev2.code = out_rel;
  // out_ev2.value = ev.value * speed;
  // write_out(out_ev2, out);
  // std::cout << "process " << ev.value * speed << std::endl;
  // std::cout << "process " << out_rel << " " << ev.value * speed << std::endl;
}

void rel2rel::process_recurring(virtual_device* out) const {
  // struct input_event out_ev;
  // memset(&out_ev, 0, sizeof(out_ev));
  // out_ev.type = EV_REL;
  // out_ev.code = out_rel;
  // out_ev.value = value;
  // if (value) {
  //   write_out(out_ev, out);
  //   std::cout << "process_recurring " << value << std::endl;
  // }
}

const char* rel2rel::decl = "rel = rel2rel(rel_code, int speed=1)";
rel2rel::rel2rel(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_REL(out_rel);
  READ_INT(speed);
}
void rel2rel::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("rel2rel");
  FILL_DEF_REL(out_rel);
  FILL_DEF_INT(speed);
}

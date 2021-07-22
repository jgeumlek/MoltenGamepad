#include "rel2btns.h"
#include "../event_translator_macros.h"

bool rel2btns::wants_recurring_events() {
  return false;
}

void rel2btns::process(struct mg_ev ev, virtual_device* out) {
  int val = ev.value;
  int btn_code;
  if (val == 0) {
    return;
  } else if (val > 0) {
    btn_code = pos_btn;
  } else {
    btn_code = neg_btn;
  }

  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.code = btn_code;
  out_ev.value = 1;
  write_out(out_ev, out);

  out_ev.type = EV_KEY;
  out_ev.code = btn_code;
  out_ev.value = 0;
  write_out(out_ev, out);
}

void rel2btns::process_recurring(virtual_device* out) const {
}

const char* rel2btns::decl = "rel = rel2btns(key_code, key_code)";
rel2btns::rel2btns(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_KEY(neg_btn);
  READ_KEY(pos_btn);
  std::cout << "rel2btns init vec " << neg_btn << " " << pos_btn << std::endl;
}
void rel2btns::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("rel2btns");
  FILL_DEF_KEY(neg_btn);
  FILL_DEF_KEY(pos_btn);
}

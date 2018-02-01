#include "simple_chord.h"
#include "../event_translator_macros.h"

const char* simple_chord::decl = "key [] = chord(key_trans)";
simple_chord::simple_chord(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_TRANS(out_trans,MG_KEY_TRANS);
}

bool simple_chord::set_mapped_events(const std::vector<source_event>& listened) {
  num_events = listened.size();
  for (auto ev : listened) {
    if ((ev.type != DEV_KEY) && (ev.type != DEV_AXIS))
      return false;
    event_vals.push_back(ev.value);
    event_thres.push_back(ev.type == DEV_KEY?0:ABS_RANGE/2);
  }
  return true;
}
  
void simple_chord::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("chord");
  FILL_DEF_TRANS(out_trans,MG_KEY_TRANS);
}

void simple_chord::init(input_source* source) {

};

void simple_chord::attach(input_source* source) {

  this->owner = source;
};

simple_chord::~simple_chord() {
  if (out_trans) delete out_trans;
}

bool simple_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  event_vals[id] = event.value > event_thres[id];
  for (int i = 0; i < event_vals.size(); i++) {
    output = output && (event_vals[i]);
  }
  if (output != output_cache) {
    output_slot* out_dev = owner->get_slot();
    if (out_dev) out_trans->process({output}, out_dev);
    output_cache = output;
  }

  return false;
};

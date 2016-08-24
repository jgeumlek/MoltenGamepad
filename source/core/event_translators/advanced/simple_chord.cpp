#include "simple_chord.h"
#include "../event_translator_macros.h"

const MGType simple_chord::fields[] = { MG_KEY_TRANS, MG_NULL };
simple_chord::simple_chord(std::vector<std::string> event_names, std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_TRANS(out_trans,MG_KEY_TRANS);
  this->event_names = event_names;
}
void simple_chord::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("simple");
  FILL_DEF_TRANS(out_trans,MG_KEY_TRANS);
}

void simple_chord::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();

  for (auto name : event_names) {
    event_ids.push_back(-1);
    event_vals.push_back(0);
  }
  for (int i = 0; i < event_names.size(); i++) {
    std::string looking_for = event_names[i];
    std::string alias = source->get_alias(looking_for);
    if (!alias.empty()) looking_for = alias;
    for (auto event : events) {
      if (!strcmp(event.name, looking_for.c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
        break;
      }
    }
  }

};

void simple_chord::attach(input_source* source) {
  for (int id : event_ids)
    source->add_listener(id, this);

  this->owner = source;
};

simple_chord::~simple_chord() {
  if (owner) {
    for (int id : event_ids) {
      owner->remove_listener(id, this);
    }
  }
  if (out_trans) delete out_trans;
}

bool simple_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  for (int i = 0; i < event_ids.size(); i++) {
    if (id == event_ids[i]) {
      event_vals[i] = event.value;
    }
    output = output && (event_vals[i]);
  }
  if (output != output_cache) {
    output_slot* out_dev = owner->get_slot();
    if (out_dev) out_trans->process({output}, out_dev);
    output_cache = output;
  }

  return false;
};

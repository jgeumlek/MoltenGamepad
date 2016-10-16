#include "exclusive_chord.h"
#include "../event_translator_macros.h"

const char* exclusive_chord::decl = "key [] = exclusive(key_trans)";
exclusive_chord::exclusive_chord(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_TRANS(out_trans,MG_KEY_TRANS);
}
void exclusive_chord::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("exclusive");
  FILL_DEF_TRANS(out_trans,MG_KEY_TRANS);
}

bool exclusive_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  int index;
  int old_val;

  for (int i = 0; i < event_ids.size(); i++) {
    if (id == event_ids[i]) {
      index = i;
      old_val = event_vals[i];
      event_vals[i] = event.value;
      if (event.value != old_val) chord_hits[i] = event.value;
      
    }
    output = output && (chord_hits[i]);
  }

  //if not thread, start thread.
  if (!output && !chord_active && event.value && !old_val) {

    chord_active = true;
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    chord_hits[index] = event.value;
    tick_count = 2;
  }

  if (output && output != output_cache) {
    //chord succeeded. Send event only if thread hasn't timed out.

    output_slot* out_dev = owner->get_slot();
    if (out_dev && chord_active) out_trans->process({output}, out_dev);

    output_cache = output;
    chord_active = false;
  }
  if (!output && output != output_cache) {

    //chord released. clear out everything.
    output_slot* out_dev = owner->get_slot();
    if (out_dev) out_trans->process({output}, out_dev);
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    output_cache = output;
  }
  //If key up, let it pass.
  //If we have no pending chord and our output says we failed, let it pass.

  if (!event.value || (!output_cache && !chord_active)) return false; //Pass along key up events.

  //We have a thread still going, or we hit a chord claiming this event.

  return true;
};

void exclusive_chord::process_recurring(output_slot* out) const {
  if (chord_active && tick_count > 0) {
    tick_count--;
  }
  if (tick_count > 0)
    return;
  if (chord_active && tick_count == 0) {
    //send out events
    for (int i = 0; i < event_ids.size(); i++) {
      if (chord_hits[i]) owner->inject_event(event_ids[i], event_vals[i], true);
      chord_hits[i] = 0;
    }
    chord_active = false;
  }
    
}

void exclusive_chord::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();
  owner = source;
  for (auto name : event_names) {
    event_ids.push_back(-1);
    event_vals.push_back(0);
    chord_hits.push_back(0);
  }

  for (int i = 0; i < event_names.size(); i++) {
    std::string looking_for = event_names[i];
    std::string alias = source->get_alias(looking_for);
    if (!alias.empty()) looking_for = alias;

    for (auto event : events) {
      if (!strcmp(event.name, looking_for.c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
      }
    }
  }

  tick_count = 2;
  chord_active = false;

};




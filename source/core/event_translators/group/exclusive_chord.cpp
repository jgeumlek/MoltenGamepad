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

bool exclusive_chord::set_mapped_events(const std::vector<source_event>& listened) {
  for (auto ev : listened) {
    event_vals.push_back(ev.value);
    event_ids.push_back(ev.id);
    chord_hits.push_back(0);
  }
}

bool exclusive_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  int index;
  int old_val = event_vals[id];
  event_vals[id] = event.value;
  if (event.value != old_val)
    chord_hits[id] = event.value;

  for (int i = 0; i < chord_hits.size(); i++) {
    output = output && (chord_hits[i]);
  }

  //if not thread, start thread.
  if (!output && !chord_active && event.value && !old_val) {

    chord_active = true;
    //clear all hits, we need all to be hit in the timespan.
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    //of course, do still set the one that just happened.
    chord_hits[id] = event.value;
    //the time span is two ticks, or about 20ms
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

  tick_count = 2;
  chord_active = false;

};




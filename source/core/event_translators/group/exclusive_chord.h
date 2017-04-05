#pragma once
#include "../event_change.h"
#include "simple_chord.h"

class exclusive_chord : public simple_chord {
public:

  exclusive_chord(event_translator* trans) : simple_chord(trans) {};
  mutable std::vector<int> chord_hits;
  std::vector<int> event_ids; //this translator needs to store source ids to re-inject events.

  virtual void init(input_source* source);
  virtual bool claim_event(int id, mg_ev event);
  virtual group_translator* clone() {
    return new exclusive_chord(out_trans->clone());
  }

  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  mutable bool chord_active;
  mutable int tick_count;

  virtual bool wants_recurring_events() { return true; };
  virtual void process_recurring(output_slot* out) const;

  static const char* decl;
  exclusive_chord(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

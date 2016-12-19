#pragma once
#include "../event_change.h"

class simple_chord : public advanced_event_translator {
public:
  std::vector<int> event_vals;
  int output_cache = 0;
  input_source* owner = nullptr;
  event_translator* out_trans = nullptr;

  simple_chord(event_translator* trans) : out_trans(trans) {};

  virtual ~simple_chord();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);
  virtual advanced_event_translator* clone() {
    return new simple_chord(out_trans->clone());
  }

  //allow chords to be set without clearing other translations.
  //index is relative to the list of mapped events.
  virtual bool wants_exclusive(int index) { return false; };

  static const char* decl;
  simple_chord(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
protected:
  simple_chord() {};
  int num_events;
};

#pragma once
#include "../event_change.h"

class simple_chord : public advanced_event_translator {
public:
  std::vector<std::string> event_names;
  std::vector<int> event_ids;
  std::vector<int> event_vals;
  int output_cache = 0;
  input_source* source = nullptr;
  event_translator* out_trans = nullptr;
  output_slot** out_dev_ptr;

  simple_chord(std::vector<std::string> event_names, event_translator* trans) : event_names(event_names), out_trans(trans) {};

  virtual ~simple_chord();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);

  virtual bool claim_event(int id, mg_ev event);
  virtual advanced_event_translator* clone() {
    return new simple_chord(event_names, out_trans->clone());
  }

  static const MGType fields[];
  simple_chord(std::vector<std::string> event_names, std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
protected:
  simple_chord() {};
};

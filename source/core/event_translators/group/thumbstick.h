#pragma once
#include "../event_change.h"

class thumb_stick : public group_translator {
public:
  int event_ids[2];
  int64_t event_vals[2];
  int64_t cached_output[2];
  int x_axis;
  int y_axis;
  input_source* owner = nullptr;

  //setting field_flags is optional, but this surpresses printing out those default values later.
  //note that this constructor is only called in hard-coded situations, not during parsing.
  thumb_stick(int x_axis, int y_axis) : x_axis(x_axis), y_axis(y_axis), deadzone(.15), outzone(.1) {
    field_flags.push_back(0); //x_axis
    field_flags.push_back(0); //y_axis
    field_flags.push_back(FLAG_DEFAULT); //deadzone
    field_flags.push_back(FLAG_DEFAULT); //outzone
  };
  virtual ~thumb_stick();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);
  virtual void process_syn_report(output_slot* out);

  virtual group_translator* clone() {
    return new thumb_stick(*this);
  }

  static const char* decl;
  thumb_stick(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
  float deadzone,outzone;
};

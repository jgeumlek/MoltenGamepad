#pragma once
#include "../event_change.h"

class stick_dpad : public group_translator {
public:
  int event_ids[2];
  int64_t event_vals[2];
  int64_t cached_output[2];
  int fourway = false;
  float dir_degrees;
  input_source* owner = nullptr;

  //setting field_flags is optional, but this surpresses printing out those default values later.
  //note that this constructor is only called in hard-coded situations, not during parsing.
  stick_dpad() : dir_degrees(135), deadzone(.6) {
    field_flags.push_back(FLAG_DEFAULT); //dir_degrees
    field_flags.push_back(FLAG_DEFAULT); //deadzone
  };
  virtual ~stick_dpad();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);
  virtual void process_syn_report(output_slot* out);

  virtual group_translator* clone() {
    return new stick_dpad(*this);
  }

  static const char* decl;
  stick_dpad(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
  float deadzone;
};

#pragma once
#include "../event_change.h"

class thumb_stick : public advanced_event_translator {
public:
  std::vector<std::string> event_names;
  int event_ids[2];
  int event_vals[2];
  int outputs[2];
  input_source* owner = nullptr;

  virtual ~thumb_stick();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<std::string>& event_names);

  virtual bool claim_event(int id, mg_ev event);
  virtual advanced_event_translator* clone() {
    return new thumb_stick(*this);
  }

  static const char* decl;
  thumb_stick(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
  float deadzone,outzone,angle_snap;
};

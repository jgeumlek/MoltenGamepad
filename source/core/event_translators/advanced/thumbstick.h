#pragma once
#include "../event_change.h"

class thumb_stick : public advanced_event_translator {
public:
  int event_ids[2];
  int64_t event_vals[2];
  int64_t cached_output[2];
  int x_axis;
  int y_axis;
  input_source* owner = nullptr;

  thumb_stick(int x_axis, int y_axis) : x_axis(x_axis), y_axis(y_axis), deadzone(.1), outzone(.01), angle_snap(0) {};
  virtual ~thumb_stick();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);
  virtual void process_syn_report(output_slot* out);

  virtual advanced_event_translator* clone() {
    return new thumb_stick(*this);
  }

  static const char* decl;
  thumb_stick(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
  float deadzone,outzone,angle_snap;
};

#pragma once
#include "../event_change.h"

class wiigyromouse : public group_translator {
public:
  int64_t accels[3] = {0, 0, 0};
  mutable float (*gyros)[3] = nullptr;
  mutable int gyro_buffer_index = 0;
  mutable float remainders[2] = {0 , 0};
  int64_t current_gyros[3] = {0,0,0};
  float smooth_factor;
  int buffer_size;
  float xscale, yscale;
  float min_offset, max_offset;
  float deadzone;
  float scaled_sq_deadzone;
  int use_accels;
  float dampen_factor;
  int dampen_period;
  mutable int dampening_ticks = 0;

  bool ratchet_active = false;
  std::vector<bool> dampening_buttons;
  input_source* owner = nullptr;

  virtual ~wiigyromouse();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);
  virtual void process_syn_report(output_slot* out);

  virtual bool clear_other_translations() { return false; };

  virtual group_translator* clone() {
    return new wiigyromouse(*this);
  }

  virtual bool wants_recurring_events() { return true; };
  virtual void process_recurring(output_slot* out) const;

  static const char* decl;
  wiigyromouse(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

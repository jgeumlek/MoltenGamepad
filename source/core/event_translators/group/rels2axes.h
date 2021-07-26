#pragma once
#include "../event_change.h"

class rels2axes : public group_translator {
public:
  int movement_goal[2] = {0, 0}; // A "movement goal" for the joystick to catch up to. Inputs will be added to the goal, and outputs will be subtracted. Since the joystick can't go past its maximum value, a large input movement may take a bit of time for the output to catch up. 
  int64_t cached_output[2] = {0, 0}; // last joystick output so we don't send the same output twice
  int x_axis; // event code for joystick x-axis
  int y_axis; // event code for joystick y-axis
  input_source* owner = nullptr; // input source
  struct timeval last_time = {0, 0}; // time stamp for last time output was calculated (for delta-time in calculation)
  float scale; // scale from input to output (higher value = mouse "moves" more = joystick is moved more/longer for the same mouse movement)
  float antideadzone; // use to adjust for a joystick deadzone: won't output inside this deadzone (deadzone is fraction out of 1)
  float curve_adjust; // use to adjust for a joystick curve response: smaller values (<1) will counteract a joystick that has increased sensitivity for smaller values

  //setting field_flags is optional, but this surpresses printing out those default values later.
  //note that this constructor is only called in hard-coded situations, not during parsing.
  rels2axes(int x_axis, int y_axis) : x_axis(x_axis), y_axis(y_axis), scale(.5), antideadzone(.1), curve_adjust(.5) {
    field_flags.push_back(0); //x_axis
    field_flags.push_back(0); //y_axis
    field_flags.push_back(FLAG_DEFAULT); //scale
    field_flags.push_back(FLAG_DEFAULT); //antideadzone
    field_flags.push_back(FLAG_DEFAULT); //curve_adjust
  };
  virtual ~rels2axes();

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);
  virtual void process_syn_report(virtual_device* out);

  virtual group_translator* clone() {
    return new rels2axes(*this);
  }

  virtual bool wants_recurring_events() { return true; };

  static const char* decl;
  rels2axes(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

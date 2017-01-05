#include "thumbstick.h"
#include "../event_translator_macros.h"
#include <cmath>

const char* thumb_stick::decl = "axis, axis = stick(axis_code x, axis_code y, float deadzone=.15, float outzone=.1)";
thumb_stick::thumb_stick(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_AXIS(x_axis);
  READ_AXIS(y_axis);

  READ_FLOAT(deadzone);
  READ_FLOAT(outzone);
}

bool thumb_stick::set_mapped_events(const std::vector<source_event>& listened) {
  if (listened.size() >= 2) {
    event_vals[0] = listened[0].value;
    event_vals[1] = listened[1].value;
  }
}
  
void thumb_stick::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("stick");
  FILL_DEF_AXIS(x_axis);
  FILL_DEF_AXIS(y_axis);
  FILL_DEF_FLOAT(deadzone);
  FILL_DEF_FLOAT(outzone);
}

void thumb_stick::init(input_source* source) {
  this->owner = source;
  

};

void thumb_stick::attach(input_source* source) {
  this->owner = source;
};

thumb_stick::~thumb_stick() {
}

bool thumb_stick::claim_event(int id, mg_ev event) {
  event_vals[id] = event.value;
  return false;
};

void thumb_stick::process_syn_report(output_slot* out) {
  int64_t output[2];
  float x = ((float) event_vals[0])/ABS_RANGE;
  float y = ((float) event_vals[1])/ABS_RANGE;
  float radius =sqrt(x*x + y*y);
  if (radius > 1-outzone) radius = 1-outzone;
  float angle = atan2(y,x);
  if (radius <= deadzone) {
    output[0] = 0;
    output[1] = 0;
  } else {
    float scaled_radius = (radius - deadzone)/(1-outzone - deadzone);
    //todo: angle snap
    output[0] = ABS_RANGE*scaled_radius*cos(angle);
    output[1] = ABS_RANGE*scaled_radius*sin(angle);
  }
  if (output[0] != cached_output[0] || output[1] != cached_output[1]) {
    cached_output[0] = output[0];
    cached_output[1] = output[1];
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = x_axis;
    out_ev.value = output[0];
    out->take_event(out_ev);
    out_ev.code = y_axis;
    out_ev.value = output[1];
    out->take_event(out_ev);
  }
};
  

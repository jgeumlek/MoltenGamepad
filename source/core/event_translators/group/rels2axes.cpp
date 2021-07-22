#include "rels2axes.h"
#include "../event_translator_macros.h"
#include <cmath>

const char* rels2axes::decl = "rel, rel = rels2axes(axis_code x, axis_code y, float scale=.5, float antideadzone=.1, float curve_adjust=.5)";
rels2axes::rels2axes(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_AXIS(x_axis);
  READ_AXIS(y_axis);

  READ_FLOAT(scale);
  READ_FLOAT(antideadzone);
  READ_FLOAT(curve_adjust);
}

bool rels2axes::set_mapped_events(const std::vector<source_event>& listened) {
  if (listened.size() >= 2) {
    movement_goal[0] += listened[0].value;
    movement_goal[1] += listened[1].value;
    // std::cout << "rels2axes::set_mapped_events" << std::endl;
  }
  return true;
}
  
void rels2axes::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("rels2axes");
  FILL_DEF_AXIS(x_axis);
  FILL_DEF_AXIS(y_axis);
  FILL_DEF_FLOAT(scale);
  FILL_DEF_FLOAT(antideadzone);
  FILL_DEF_FLOAT(curve_adjust);
}

void rels2axes::init(input_source* source) {
  this->owner = source;
  

};

void rels2axes::attach(input_source* source) {
  this->owner = source;
};

rels2axes::~rels2axes() {
}

bool rels2axes::claim_event(int id, mg_ev event) {
  movement_goal[id] += event.value;
  return false;
};

void rels2axes::process_syn_report(virtual_device* out) {
  // get time delta
  struct timeval new_time;
  gettimeofday(&new_time, NULL);
  int dt_microseconds = (new_time.tv_sec - last_time.tv_sec) * 1000000 + (new_time.tv_usec - last_time.tv_usec);
  if (last_time.tv_sec == 0) {
    last_time = new_time;
    return;
  }
  last_time = new_time;

  // get angle and length to goal
  float angle_radians = std::atan2(movement_goal[1], movement_goal[0]);
  float d_length = std::sqrt((float) (std::pow(movement_goal[0], 2) + std::pow(movement_goal[1], 2)));

  // get length of joystick from center (out of 1)
  d_length = d_length * scale * 1000 / dt_microseconds;
  if (d_length > 1) {d_length = 1;}
  if (d_length < std::pow(antideadzone, 1/curve_adjust)) {d_length = 0;}

  // get joystick x/y (out of 1)
  float dx = d_length * std::cos(angle_radians);;
  float dy = d_length * std::sin(angle_radians);

  // update goal
  movement_goal[0] -= dx * dt_microseconds / scale / 1000;
  movement_goal[1] -= dy * dt_microseconds / scale / 1000;

  // adjust for joystick curve
  float jx = std::pow(std::abs(dx), curve_adjust) * ((dx > 0) ? 1 : -1);
  float jy = std::pow(std::abs(dy), curve_adjust) * ((dy > 0) ? 1 : -1);

  // out of 1 to out of ABS_RANGE
  int jx_val = jx * ABS_RANGE;
  int jy_val = jy * ABS_RANGE;

  // send to joystick output
  if (jx_val != cached_output[0] || jy_val != cached_output[1]) {
    cached_output[0] = jx_val;
    cached_output[1] = jy_val;
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = x_axis;
    out_ev.value = jx_val;
    out->take_event(out_ev);
    out_ev.code = y_axis;
    out_ev.value = jy_val;
    out->take_event(out_ev);
  }
};
  

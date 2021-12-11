#pragma once
#include "../event_change.h"

class axis2axis : public event_translator {
public:
  int out_axis;
  int direction;
  // following 3 parameters are used for calibrating uncalibrated joysticks: ranges [min, middle] and [middle, max] map to [-ABS_RANGE, 0] and [0, ABS_RANGE] respectively
  int min;
  int middle;
  int max;
  axis2axis(int axis, int dir, int calibrate_min, int calibrate_middle, int calibrate_max) : out_axis(axis), direction(dir), min(calibrate_min), middle(calibrate_middle), max(calibrate_max) {
  }
  virtual void process(struct mg_ev ev, virtual_device* out) {
    int value = ev.value;
    // apply joystick calibration
    if (value < middle) {
      value = ((value - middle) * ABS_RANGE) / (middle - min);
    } else {
      value = ((value - middle) * ABS_RANGE) / (max - middle);
    }
    // apply direction
    value *= direction;
    // max joystick value is ABS_RANGE
    if (value < -ABS_RANGE) value = -ABS_RANGE;
    if (value > ABS_RANGE) value = ABS_RANGE;
    // output event
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = out_axis;
    out_ev.value = value;
    write_out(out_ev, out);
  }

  virtual axis2axis* clone() {
    return new axis2axis(*this);
  }

  static const char* decl;
  axis2axis(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

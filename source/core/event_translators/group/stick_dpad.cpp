#include "stick_dpad.h"
#include "../event_translator_macros.h"
#include <cmath>

const char* stick_dpad::decl = "axis, axis = dpad(float dir_width=135,float deadzone=.6)";

//dir_width is a measure in degrees of how wide each activation area should be.
//e.g. dir_width = 90 has the four directions cover the entire circumference without overlap.
//     135 degrees makes 8 equal regions for the four directions and the four diagonals.
//     <90 degrees makes regions where no direction is activated.


stick_dpad::stick_dpad(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_FLOAT(dir_degrees);
  READ_FLOAT(deadzone);
  if (dir_degrees > 180) //code does not handle >180 correctly.
    dir_degrees = 180;
  if (dir_degrees < 0) //negative (and zero) values aren't useful...
    dir_degrees = 0;
}

bool stick_dpad::set_mapped_events(const std::vector<source_event>& listened) {
  if (listened.size() >= 2) {
    event_vals[0] = listened[0].value;
    event_vals[1] = listened[1].value;
  }
}
  
void stick_dpad::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("dpad");
  FILL_DEF_FLOAT(dir_degrees);
  FILL_DEF_FLOAT(deadzone);
}

void stick_dpad::init(input_source* source) {
  this->owner = source;
};

void stick_dpad::attach(input_source* source) {
  this->owner = source;
};

stick_dpad::~stick_dpad() {
}

bool stick_dpad::claim_event(int id, mg_ev event) {
  event_vals[id] = event.value;
  return false;
};



void stick_dpad::process_syn_report(output_slot* out) {
  int8_t output[2];
  float x = ((float) event_vals[0])/ABS_RANGE;
  float y = ((float) event_vals[1])/ABS_RANGE;
  float radius =sqrt(x*x + y*y);
  output[0] = 0;
  output[1] = 0;
  if (radius > deadzone) {
    float angle = atan2(y,x)*180/M_PI;
    float half_width = dir_degrees/2;
    if (angle > -half_width && angle < half_width)
      output[0] = 1; //right
    else if (angle < -180 + half_width || angle > 180 - half_width) //boundary! range is -180 to 180...
      output[0] = -1; //left
    if (angle > 90-half_width && angle < 90 + half_width)
      output[1] = 1; //down
    else if (angle > -90-half_width && angle < -90+half_width)
      output[1] = -1; //up
    //You might thing down and up are reversed here, and well, they are.
    //The trig functions y-axis is flipped relative to how we read controller events.
    //We should be calling atan2(-y,x), but it is easier to just flip these range checks.
  }
  //clear the wrong side first just to avoid breaking things that don't expect to ever
  //see two opposite directions at the same time.
  if (output[0] != cached_output[0]) {
    cached_output[0] = output[0];
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_KEY;
    //clear the "wrong direction", if it exists.
    out_ev.code = output[0] < 0 ? BTN_DPAD_RIGHT : BTN_DPAD_LEFT;
    out_ev.value = 0;
    out->take_event(out_ev);
    //set the correct direction, or clear it also if the output is 0.
    out_ev.code = output[0] < 0 ? BTN_DPAD_LEFT : BTN_DPAD_RIGHT;
    out_ev.value = output[0] == 0 ? 0 : 1;
    out->take_event(out_ev);
  }
  if (output[1] != cached_output[1]) {
    cached_output[1] = output[1];
    struct input_event out_ev;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_KEY;
    //clear the "wrong direction", if it exists.
    out_ev.code = output[1] < 0 ? BTN_DPAD_DOWN : BTN_DPAD_UP;
    out_ev.value = 0;
    out->take_event(out_ev);
    //set the correct direction, or clear it also if the output is 0.
    out_ev.code = output[1] < 0 ? BTN_DPAD_UP : BTN_DPAD_DOWN;
    out_ev.value = output[1] == 0 ? 0 : 1;
    out->take_event(out_ev);
  }
};
  

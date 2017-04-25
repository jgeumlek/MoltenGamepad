#include "wiigyromouse.h"
#include "../event_translator_macros.h"
#include <cmath>

//wiimote.(wm_accels,wm_gyros,wm_down, ratchet_btn, dampen_btns) = wiigyromouse(...)
const char* wiigyromouse::decl = \
 "axis, axis, axis, axis, axis, axis, key [] = wiigyromouse(float smooth_factor=.75, int buffer_size=4, float xscale=1, float yscale=1, float min_offset=0.0616205, float max_offset=60, float deadzone = .01, bool use_accels=true, float dampen_factor = 0, int dampen_period = 10)";
wiigyromouse::wiigyromouse(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_FLOAT(smooth_factor);
  READ_INT(buffer_size);
  READ_FLOAT(xscale);
  READ_FLOAT(yscale);
  READ_FLOAT(min_offset);
  READ_FLOAT(max_offset);
  READ_FLOAT(deadzone);
  READ_BOOL(use_accels);
  READ_FLOAT(dampen_factor);
  READ_INT(dampen_period);
  if (buffer_size < 0)
    buffer_size = 1;
  if (buffer_size > 1000)
    buffer_size = 1000;
  if (deadzone < 0)
    deadzone = 0;
  if (deadzone > 1)
    deadzone = 1;
  if (smooth_factor < 0)
    smooth_factor = 0;
  if (smooth_factor > 1)
    smooth_factor = 1;
  dampen_period = 10*(dampen_period/10);
  if (dampen_period < 10)
    dampen_period = 10;
}

bool wiigyromouse::set_mapped_events(const std::vector<source_event>& listened) {
  if (listened.size() >= 6) {
    accels[0] = listened[0].value;
    accels[1] = listened[1].value;
    accels[2] = listened[2].value;
    current_gyros[0] = listened[3].value;
    current_gyros[1] = listened[4].value;
    current_gyros[2] = listened[5].value;
  }
  if (listened.size() >= 7) {
    ratchet_active = listened[6].value;
  }
  if (listened.size() >= 8) {
    dampening_buttons.resize(listened.size()-7);
    for (int i = 7; i < listened.size(); i++)
      dampening_buttons.push_back(!!listened[i].value);
  }
}
  
void wiigyromouse::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("wiigyromouse");
  FILL_DEF_FLOAT(smooth_factor);
  FILL_DEF_INT(buffer_size);
  FILL_DEF_FLOAT(xscale);
  FILL_DEF_FLOAT(yscale);
  FILL_DEF_FLOAT(min_offset);
  FILL_DEF_FLOAT(max_offset);
  FILL_DEF_FLOAT(deadzone);
  FILL_DEF_BOOL(use_accels);
  FILL_DEF_FLOAT(dampen_factor);
  FILL_DEF_INT(dampen_period);
}

void wiigyromouse::init(input_source* source) {
  this->owner = source;
  //allocation done in init, since init doesn't block the device thread.
  if (buffer_size < 0)
    buffer_size = 1;
  if (buffer_size > 1000)
    buffer_size = 1000;
  gyros = new float[buffer_size][3];
  for (int i = 0; i < buffer_size; i++)
    for (int j = 0; j < 3; j++)
      gyros[i][j] = 0;
  scaled_sq_deadzone = deadzone*deadzone*ABS_RANGE*ABS_RANGE;
};

void wiigyromouse::attach(input_source* source) {
  this->owner = source;
};

wiigyromouse::~wiigyromouse() {
  if (gyros)
    delete[] gyros;
}

bool wiigyromouse::claim_event(int id, mg_ev event) {
  if (id < 3) {
    accels[id] = event.value;
    return false;
  }
  if (id < 6) {
    current_gyros[id-3] = event.value;
    return false;
  }
  if (id == 6) {
    ratchet_active = event.value;
  } else {
    int d_id = id-7;
    if (d_id < 0 || d_id >= dampening_buttons.size())
      return false;
    if (event.value && !dampening_buttons[d_id])
      dampening_ticks = dampen_period/10;
    dampening_buttons[d_id] = !!event.value;
  }
  return false;
};

void wiigyromouse::process_recurring(output_slot* out) const {
  //newX and newY are going to be the two newly compute X/Y movements.
  //The final output will be a smoothed version of these new values.
  float newX = 0;
  float newY = 0;
  //Step One: Rotate our gyro data based off of the accelerometers
  //(accels can give a loose view into the controller's "roll")
  //Only rotate two of the axes... the non-roll gyroscopes ([0] and [2]).
  float rotation = atan2(accels[0], accels[2]);
  if (use_accels == 0 || (accels[0] == 0 && accels[2] == 0)) {
    //Wiimote in free fall? Held entirely on the wrong axis?
    //Take the lazy way out...
    rotation = 0;
  }
  //printf("rotation %6.2f : %7d %7d %7d \n", rotation*180/3.14159, accels[0], accels[1], accels[2]);
  float rotCos = cos(rotation);
  float rotSin = sin(rotation);
  float rotated_gyros[3] = \
  { current_gyros[0] * rotCos - current_gyros[2] * rotSin,\
    (float)current_gyros[1],\
    current_gyros[0] * rotSin  + current_gyros[2] * rotCos,\
  };
  //Step Two: apply xscale/yscale/dampening to the rotated values
  //(hand movements left/right and up/down tend to be different...)
  rotated_gyros[0] *= xscale;
  rotated_gyros[2] *= yscale;

  if (dampening_ticks > 0) {
    //printf("dampening...\n");
    rotated_gyros[0] *= dampen_factor;
    rotated_gyros[2] *= dampen_factor;
    dampening_ticks--;
  }

  //If we are doing the ratchet, act like we saw no movement.
  if (ratchet_active) {
    //printf("ratchet...\n");
    rotated_gyros[0] = 0;
    rotated_gyros[2] = 0;
  }

  //Step Three: Deadzone calculations
  //There is a circular deadzone where we send zero output/clear remainders.
  //Outside the deadzone, we interpolate from min_offset to max_offset
  float sqradius = rotated_gyros[0]*rotated_gyros[0] + rotated_gyros[2]*rotated_gyros[2];
  if (sqradius < scaled_sq_deadzone) {
    newX = remainders[0];
    newY = remainders[1];
    remainders[0] = 0;
    remainders[1] = 0;
    //keep newX and newY at 0
    
  } else {
    //find the ratio of where we are between the deadzone and the max output
    float scaled_radius = (sqrt(sqradius) - deadzone*ABS_RANGE)/(ABS_RANGE-deadzone*ABS_RANGE);
    //use that ratio to interpolate
    scaled_radius = (max_offset - min_offset)*scaled_radius + min_offset;
    float angle = atan2(rotated_gyros[0], rotated_gyros[2]);
    newX = -scaled_radius*sin(angle);
    newY = scaled_radius*cos(angle);
  }
    
  //Step Four: Smooth the new values.
  //If buffer_size is one, we keep a running average, that might accumulate errors.
  //If buffer_size > 1, we store all our values and compute the average as needed.
  float smoothX = 0;
  float smoothY = 0;
  if (buffer_size == 1) {
    gyros[0][0] = (smooth_factor)*gyros[0][0] + (1-smooth_factor)*newX;
    gyros[0][2] = (smooth_factor)*gyros[0][2] + (1-smooth_factor)*newY;
    smoothX = gyros[0][0];
    smoothY = gyros[0][2];
  } else {
    gyros[gyro_buffer_index][0] = newX;
    gyros[gyro_buffer_index][2] = newY;
    int index = gyro_buffer_index;
    gyro_buffer_index++;
    gyro_buffer_index = (gyro_buffer_index >= buffer_size) ? 0 : gyro_buffer_index;

    float current_weight = (1-smooth_factor);
    float total_weight = 0;
    int start_index = index;
    do {
      smoothX += current_weight*gyros[index][0];
      smoothY += current_weight*gyros[index][2];
      index--;
      index = (index < 0) ? buffer_size-1 : index;
      total_weight += current_weight;
      current_weight *= smooth_factor;
    } while (index != start_index);

    smoothX /= total_weight;
    smoothY /= total_weight;
  }


  //Catch floating point odditites
  if (!( smoothX > -2*max_offset && smoothX < 2*max_offset && smoothY > -2*max_offset && smoothY < 2*max_offset)) {
    printf("invalid calculations?\n");
    return;
  }
    
  //Step Six: Add in remainders only if going in same direction.
  if ( (remainders[0] >= 0) == (smoothX >= 0) )
    smoothX += remainders[0];
  if ((remainders[1] >= 0) == (smoothY >= 0) )
    smoothY += remainders[1];
  
  //Step Seven: Truncate to integers, update remainders;
  int32_t x,y;
  x = (int32_t)trunc(smoothX);
  remainders[0] = smoothX - x;
  y = (int32_t)trunc(smoothY);
  remainders[1] = smoothY - y;

  //Step Eight: Generate output events
  if (out && (x != 0 || y != 0)) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_REL;
    ev.code = REL_X;
    ev.value = x;
    out->take_event(ev);

    ev.code = REL_Y;
    ev.value = y;
    out->take_event(ev);

    //syn report will be sent by input_source
  }
    
  
}

void wiigyromouse::process_syn_report(output_slot* out) {
  //don't need to do anything: we work on the recurring_events, not the incoming events.
};
  

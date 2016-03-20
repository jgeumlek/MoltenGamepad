#ifndef WII_EVENTS_H
#define WII_EVENTS_H


//NOTE: Code abuses the fact wiimote buttons in nk_* follow/align with the orignal wm_* events
enum wii_keys {
  wm_a,
  wm_b,
  wm_plus,
  wm_minus,
  wm_1,
  wm_2,
  wm_home,
  wm_left,
  wm_right,
  wm_up,
  wm_down,

  nk_a,
  nk_b,
  nk_plus,
  nk_minus,
  nk_1,
  nk_2,
  nk_home,
  nk_left,
  nk_right,
  nk_up,
  nk_down,
  nk_c,
  nk_z,

  cc_a,
  cc_b,
  cc_x,
  cc_y,
  cc_plus,
  cc_minus,
  cc_home,
  cc_left,
  cc_right,
  cc_up,
  cc_down,
  cc_l,
  cc_zl,
  cc_r,
  cc_zr,
  cc_thumbl,
  cc_thumbr,

  wm_accel_x,
  wm_accel_y,
  wm_accel_z,
  wm_ir_x,
  wm_ir_y,

  nk_wm_accel_x,
  nk_wm_accel_y,
  nk_wm_accel_z,
  nk_ir_x,
  nk_ir_y,

  nk_accel_x,
  nk_accel_y,
  nk_accel_z,
  nk_stick_x,
  nk_stick_y,

  cc_left_x,
  cc_left_y,
  cc_right_x,
  cc_right_y,


  bal_x,
  bal_y,

  wii_event_max
};


extern const source_event wiimote_events[];



extern const source_option wiimote_options[];

int lookup_wii_event(const char* evname);

#endif

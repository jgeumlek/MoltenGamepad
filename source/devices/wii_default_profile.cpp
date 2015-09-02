#include "wiimote.h"
void wiimote::init_profile() {
  memset(key_trans, 0, sizeof(key_trans));
  key_trans[wm_a] =    new btn2btn(BTN_WEST);
  key_trans[wm_b] =    new btn2btn(BTN_NORTH);
  key_trans[wm_left] = new btn2btn(BTN_DPAD_DOWN);
  key_trans[wm_right]= new btn2btn(BTN_DPAD_UP);
  key_trans[wm_up] =   new btn2btn(BTN_DPAD_LEFT);
  key_trans[wm_down] = new btn2btn(BTN_DPAD_RIGHT);
  key_trans[wm_1] =    new btn2btn(BTN_EAST);
  key_trans[wm_2] =    new btn2btn(BTN_SOUTH);
  key_trans[wm_home] = new btn2btn(BTN_MODE);
  key_trans[wm_plus] = new btn2btn(BTN_START);
  key_trans[wm_minus]= new btn2btn(BTN_SELECT);
  
  key_trans[nk_a] =    new btn2btn(BTN_SOUTH);
  key_trans[nk_b] =    new btn2btn(BTN_EAST);
  key_trans[nk_left] = new btn2btn(BTN_DPAD_LEFT);
  key_trans[nk_right]= new btn2btn(BTN_DPAD_RIGHT);
  key_trans[nk_up] =   new btn2btn(BTN_DPAD_UP);
  key_trans[nk_down] = new btn2btn(BTN_DPAD_DOWN);
  key_trans[nk_1] =    new btn2btn(BTN_TL);
  key_trans[nk_2] =    new btn2btn(BTN_TR);
  key_trans[nk_home] = new btn2btn(BTN_MODE);
  key_trans[nk_plus] = new btn2btn(BTN_START);
  key_trans[nk_minus]= new btn2btn(BTN_SELECT);
  key_trans[nk_c] =    new btn2btn(BTN_NORTH);
  key_trans[nk_z] =    new btn2btn(BTN_WEST);
  
  key_trans[cc_a] =    new btn2btn(BTN_SOUTH);
  key_trans[cc_b] =    new btn2btn(BTN_EAST);
  key_trans[cc_x] =    new btn2btn(BTN_WEST);
  key_trans[cc_y] =    new btn2btn(BTN_NORTH);
  key_trans[cc_left] = new btn2btn(BTN_DPAD_LEFT);
  key_trans[cc_right]= new btn2btn(BTN_DPAD_RIGHT);
  key_trans[cc_up] =   new btn2btn(BTN_DPAD_UP);
  key_trans[cc_down] = new btn2btn(BTN_DPAD_DOWN);
  key_trans[cc_home] = new btn2btn(BTN_MODE);
  key_trans[cc_plus] = new btn2btn(BTN_START);
  key_trans[cc_minus]= new btn2btn(BTN_SELECT);
  key_trans[cc_l] =    new btn2btn(BTN_TL);
  key_trans[cc_r] =    new btn2btn(BTN_TR);
  key_trans[cc_zl] =   new btn2btn(BTN_TL2);
  key_trans[cc_zr] =   new btn2btn(BTN_TR2);
  
  for (int i = 0; i < wii_key_max; i++) {
    if (!key_trans[i]) key_trans[i] = new event_translator();
  }
  
  memset(abs_trans, 0, sizeof(abs_trans));
  
  abs_trans[cc_left_x] = new axis2axis(ABS_X,1);
  abs_trans[cc_left_y] = new axis2axis(ABS_Y,1);
  abs_trans[cc_right_x] = new axis2axis(ABS_RX,1);
  abs_trans[cc_right_y] = new axis2axis(ABS_RY,1);
  
  for (int i = 0; i < wii_abs_max; i++) {
    if (!abs_trans[i]) abs_trans[i] = new event_translator();
  }
}
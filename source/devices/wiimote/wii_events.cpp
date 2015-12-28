#include "wiimote.h"


#define EVNAME(X) X,#X
#define BTN DEV_KEY,0,nullptr
#define ABS DEV_AXIS,0,nullptr
#define OPT DEV_OPTION

const source_event wiimote_events[] = {
{EVNAME(wm_a),"Wiimote A button",BTN},
{EVNAME(wm_b),"Wiimote B button",BTN},
{EVNAME(wm_plus),"Wiimote + button",BTN},
{EVNAME(wm_minus),"Wiimote - button",BTN},
{EVNAME(wm_1),"Wiimote 1 button",BTN},
{EVNAME(wm_2),"Wiimote 2 button",BTN},
{EVNAME(wm_home),"Wiimote home button",BTN},
{EVNAME(wm_left),"Wiimote Dpad left",BTN},
{EVNAME(wm_right),"Wiimote Dpad right",BTN},
{EVNAME(wm_up),"Wiimote Dpad up",BTN},
{EVNAME(wm_down),"Wiimote Dpad down",BTN},

{EVNAME(nk_a),"Wiimote A button with Nunchuk",BTN},
{EVNAME(nk_b),"Wiimote B button with Nunchuk",BTN},
{EVNAME(nk_plus),"Wiimote + button with Nunchuk",BTN},
{EVNAME(nk_minus),"Wiimote - button with Nunchuk",BTN},
{EVNAME(nk_1),"Wiimote 1 button with Nunchuk",BTN},
{EVNAME(nk_2),"Wiimote 2 button with Nunchuk",BTN},
{EVNAME(nk_home),"Wiimote home button with Nunchuk",BTN},
{EVNAME(nk_left),"Wiimote Dpad left with Nunchuk",BTN},
{EVNAME(nk_right),"Wiimote Dpad right with Nunchuk",BTN},
{EVNAME(nk_up),"Wiimote Dpad up with Nunchuk",BTN},
{EVNAME(nk_down),"Wiimote Dpad down with Nunchuk",BTN},
{EVNAME(nk_c),"Nunchuk C button",BTN},
{EVNAME(nk_z),"Nunchuk Z button",BTN},

{EVNAME(cc_a),"Classic Controller A button",BTN},
{EVNAME(cc_b),"Classic Controller B button",BTN},
{EVNAME(cc_x),"Classic Controller X button",BTN},
{EVNAME(cc_y),"Classic Controller Y button",BTN},
{EVNAME(cc_plus),"Classic Controller + button",BTN},
{EVNAME(cc_minus),"Classic Controller - button",BTN},
{EVNAME(cc_home),"Classic Controller Home button",BTN},
{EVNAME(cc_left),"Classic Controller Dpad left",BTN},
{EVNAME(cc_right),"Classic Controller Dpad right",BTN},
{EVNAME(cc_up),"Classic Controller Dpad up",BTN},
{EVNAME(cc_down),"Classic Controller Dpad down",BTN},
{EVNAME(cc_l),"Classic Controller L button",BTN},
{EVNAME(cc_zl),"Classic Controller ZL button",BTN},
{EVNAME(cc_r),"Classic Controller R button",BTN},
{EVNAME(cc_zr),"Classic Controller ZR button",BTN},

{EVNAME(wm_accel_x),"Wiimote X acceleration ((-) <--> (+) axis)",ABS},
{EVNAME(wm_accel_y),"Wiimote Y acceleration (plug <--> pointer axis)",ABS},
{EVNAME(wm_accel_z),"Wiimote Z acceleration (top <--> bottom axis)",ABS},
{EVNAME(wm_ir_x),"Wiimote IR pointer X",ABS},
{EVNAME(wm_ir_y),"Wiimote IR pointer Y",ABS},


{EVNAME(nk_wm_accel_x),"Wiimote X acceleration with Nunchuk",ABS},
{EVNAME(nk_wm_accel_y),"Wiimote Y acceleration with Nunchuk",ABS},
{EVNAME(nk_wm_accel_z),"Wiimote Z acceleration with Nunchuk",ABS},
{EVNAME(nk_ir_x),"Wiimote IR pointer X with Nunchuk",ABS},
{EVNAME(nk_ir_y),"Wiimote IR pointer Y with Nunchuk",ABS},
{EVNAME(nk_accel_x),"Nunchuk X acceleration",ABS},
{EVNAME(nk_accel_y),"Nunchuk Y acceleration",ABS},
{EVNAME(nk_accel_z),"Nunchuk Z acceleration",ABS},
{EVNAME(nk_stick_x),"Nunchuk stick X",ABS},
{EVNAME(nk_stick_y),"Nunchuk stick Y",ABS},

{EVNAME(cc_left_x),"Classic Controller Left Stick X",ABS},
{EVNAME(cc_left_y),"Classic Controller Left Stick Y",ABS},
{EVNAME(cc_right_x),"Classic Controller Right Stick X",ABS},
{EVNAME(cc_right_y),"Classic Controller Right Stick Y",ABS},

{-1,nullptr, nullptr,NO_ENTRY,0,nullptr}
};

const source_option wiimote_options[] = {
{"wm_accel_active","Enable accelerometers when no extension","false"},
{"nk_accel_active","Enable accelerometers when nunchuk is present","false"},
{"wm_ir_active","Enable IR data when no extension","false"},
{"nk_ir_active","Enable IR data when nunchuk is present","false"},
{"","",""},
};


void wiimote::process(int type, int event_id, long long value) {
  
  send_value(event_id,value);
  
}
  

void wiimote::process_core() {
  struct input_event ev;
  int ret;
  while(ret = read(buttons.fd,&ev,sizeof(ev)) > 0) {
  int offset = 0;

  if (mode == NUNCHUK_EXT) offset = nk_a;
  if (ret > 0) {
    switch (ev.code) {
      case KEY_LEFT: process(EVENT_KEY, offset+wm_left,ev.value); break;
      case KEY_RIGHT:process(EVENT_KEY, offset+wm_right,ev.value); break;
      case KEY_UP:   process(EVENT_KEY, offset+wm_up,ev.value); break;
      case KEY_DOWN: process(EVENT_KEY, offset+wm_down,ev.value); break;
      case BTN_A:    process(EVENT_KEY, offset+wm_a,ev.value); break;
      case BTN_B:    process(EVENT_KEY, offset+wm_b,ev.value); break;
      case BTN_1:    process(EVENT_KEY, offset+wm_1,ev.value); break;
      case BTN_2:    process(EVENT_KEY, offset+wm_2,ev.value); break;
      case KEY_PREVIOUS: process(EVENT_KEY, offset+wm_minus,ev.value); break;
      case KEY_NEXT: process(EVENT_KEY, offset+wm_plus,ev.value); break;
      case BTN_MODE: process(EVENT_KEY, offset+wm_home,ev.value); break;
      case SYN_REPORT: if (out_dev) out_dev->take_event(ev);
    }
  }
  }
  if (ret < 0) perror("read core");
  
};

#define CLASSIC_STICK_SCALE ABS_RANGE/22
void wiimote::process_classic(int fd) {
  struct input_event ev;
  int ret = read(fd,&ev,sizeof(ev));
  if (ret > 0) {
    
    if (ev.type == EV_KEY) switch (ev.code) {
      case KEY_LEFT: process(EVENT_KEY, cc_left,ev.value); break;
      case KEY_RIGHT:process(EVENT_KEY, cc_right,ev.value); break;
      case KEY_UP:   process(EVENT_KEY, cc_up,ev.value); break;
      case KEY_DOWN: process(EVENT_KEY, cc_down,ev.value); break;
      case BTN_A:    process(EVENT_KEY, cc_a,ev.value); break;
      case BTN_B:    process(EVENT_KEY, cc_b,ev.value); break;
      case BTN_X:    process(EVENT_KEY, cc_x,ev.value); break;
      case BTN_Y:    process(EVENT_KEY, cc_y,ev.value); break;
      case KEY_PREVIOUS:
                     process(EVENT_KEY, cc_minus,ev.value); break;
      case KEY_NEXT: process(EVENT_KEY, cc_plus,ev.value); break;
      case BTN_MODE: process(EVENT_KEY, cc_home,ev.value); break;
      case BTN_TL:   process(EVENT_KEY, cc_l,ev.value); break;
      case BTN_TR:   process(EVENT_KEY, cc_r,ev.value); break;
      case BTN_TL2:  process(EVENT_KEY, cc_zl,ev.value); break;
      case BTN_TR2:  process(EVENT_KEY, cc_zr,ev.value); break;
    } else if (ev.type == EV_ABS) switch (ev.code) {
      case ABS_HAT1X:  process(EVENT_AXIS, cc_left_x,ev.value*CLASSIC_STICK_SCALE); break;
      case ABS_HAT1Y:  process(EVENT_AXIS, cc_left_y,-ev.value*CLASSIC_STICK_SCALE); break;
      case ABS_HAT2X:  process(EVENT_AXIS, cc_right_x,ev.value*CLASSIC_STICK_SCALE); break;
      case ABS_HAT2Y:  process(EVENT_AXIS, cc_right_y,-ev.value*CLASSIC_STICK_SCALE); break;
    } else {
    
      if (out_dev) out_dev->take_event(ev);

    }
      
  }
}

#define NUNCHUK_STICK_SCALE ABS_RANGE/23
#define NUNCHUK_ACCEL_SCALE ABS_RANGE/90
void wiimote::process_nunchuk(int fd) {
  struct input_event ev;
  int ret = read(fd,&ev,sizeof(ev));
  if (ret > 0) {
    
    if (ev.type == EV_KEY) switch (ev.code) {
      case BTN_C: process(EVENT_KEY, cc_left,ev.value); break;
      case BTN_Z: process(EVENT_KEY, cc_right,ev.value); break;
    } else if (ev.type == EV_ABS) switch (ev.code) {
      case ABS_HAT0X:  process(EVENT_AXIS, nk_stick_x,ev.value*NUNCHUK_STICK_SCALE); break;
      case ABS_HAT1Y:  process(EVENT_AXIS, nk_stick_y,ev.value*NUNCHUK_STICK_SCALE); break;
      case ABS_RX:     process(EVENT_AXIS, nk_accel_x,ev.value*NUNCHUK_ACCEL_SCALE); break;
      case ABS_RY:     process(EVENT_AXIS, nk_accel_y,ev.value*NUNCHUK_ACCEL_SCALE); break;
      case ABS_RZ:     process(EVENT_AXIS, nk_accel_z,ev.value*NUNCHUK_ACCEL_SCALE); break;
    } else {
      
      if (out_dev) out_dev->take_event(ev);
    }
  }
}

#define WIIMOTE_ACCEL_SCALE ABS_RANGE/90
void wiimote::process_accel(int fd) {
  struct input_event ev;
  int ret;
  while(ret = read(fd,&ev,sizeof(ev)) > 0) {
  int offset = 0;

  if (mode == NUNCHUK_EXT) {
    offset = nk_wm_accel_x;
  } else {
    offset = wm_accel_x;
  }
  if (ret > 0) {
    switch (ev.code) {
      case ABS_RX: process(EVENT_AXIS, offset+0,ev.value*WIIMOTE_ACCEL_SCALE); break;
      case ABS_RY: process(EVENT_AXIS, offset+1,ev.value*WIIMOTE_ACCEL_SCALE); break;
      case ABS_RZ: process(EVENT_AXIS, offset+2,ev.value*WIIMOTE_ACCEL_SCALE); break;
      case SYN_REPORT: if (out_dev) out_dev->take_event(ev);
    }
  }
  }
  if (ret < 0) perror("read accel");
}

#define IR_X_SCALE ABS_RANGE/500
#define IR_Y_SCALE ABS_RANGE/350
#define NO_IR_DATA 1023
void wiimote::process_ir(int fd) {
  struct input_event ev;
  int ret;
  while(ret = read(fd,&ev,sizeof(ev)) > 0) {
    switch (ev.code) {
      case ABS_HAT0X: ircache[0].x = ev.value; break;
      case ABS_HAT0Y: ircache[0].y = ev.value; break;
      case ABS_HAT1X: ircache[1].x = ev.value; break;
      case ABS_HAT1Y: ircache[1].y = ev.value; break;
      case ABS_HAT2X: ircache[2].x = ev.value; break;
      case ABS_HAT2Y: ircache[2].y = ev.value; break;
      case ABS_HAT3X: ircache[3].x = ev.value; break;
      case ABS_HAT3Y: ircache[3].y = ev.value; break;
      case SYN_REPORT: compute_ir(); if (out_dev) out_dev->take_event(ev); break;
    }
  }
  if (ret < 0) perror("read IR");
}

void wiimote::compute_ir() {
  int num = 0;
  int x = NO_IR_DATA;
  int y = NO_IR_DATA;
  int offset = 0;
  //Current logic: Take the leftmost visible IR source.
  //Not great, but better than nothing?
  if (mode == NUNCHUK_EXT) {
    offset = nk_ir_x;
  } else {
    offset = wm_ir_x;
  }
  
  for (int i = 0; i < 4; i++) {
    int ir_x = ircache[i].x;
    int ir_y = ircache[i].y;
    if (ir_x < x && ir_x != NO_IR_DATA && ir_x > 1) {
      x = ir_x;
      y = ir_y;
      num++;
    }
  }
  if (num != 0) {
    process(EVENT_AXIS, offset+0, x*IR_X_SCALE);
    process(EVENT_AXIS, offset+1, y*IR_Y_SCALE);
  }
  
}
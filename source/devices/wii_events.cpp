#include "wiimote.h"


#define EVNAME(X) X,#X

const wiimote_event wiimote_events_keys[] = {
{EVNAME(wm_a),"Wiimote A button"},
{EVNAME(wm_b),"Wiimote B button"},
{EVNAME(wm_plus),"Wiimote + button"},
{EVNAME(wm_minus),"Wiimote - button"},
{EVNAME(wm_1),"Wiimote 1 button"},
{EVNAME(wm_2),"Wiimote 2 button"},
{EVNAME(wm_home),"Wiimote home button"},
{EVNAME(wm_left),"Wiimote Dpad left"},
{EVNAME(wm_right),"Wiimote Dpad right"},
{EVNAME(wm_up),"Wiimote Dpad up"},
{EVNAME(wm_down),"Wiimote Dpad down"},

{EVNAME(nk_a),"Wiimote A button with Nunchuk"},
{EVNAME(nk_b),"Wiimote B button with Nunchuk"},
{EVNAME(nk_plus),"Wiimote + button with Nunchuk"},
{EVNAME(nk_minus),"Wiimote - button with Nunchuk"},
{EVNAME(nk_1),"Wiimote 1 button with Nunchuk"},
{EVNAME(nk_2),"Wiimote 2 button with Nunchuk"},
{EVNAME(nk_home),"Wiimote home button with Nunchuk"},
{EVNAME(nk_left),"Wiimote Dpad left with Nunchuk"},
{EVNAME(nk_right),"Wiimote Dpad right with Nunchuk"},
{EVNAME(nk_up),"Wiimote Dpad up with Nunchuk"},
{EVNAME(nk_down),"Wiimote Dpad down with Nunchuk"},
{EVNAME(nk_c),"Nunchuk C button"},
{EVNAME(nk_z),"Nunchuk Z button"},

{EVNAME(cc_a),"Classic Controller A button"},
{EVNAME(cc_b),"Classic Controller B button"},
{EVNAME(cc_x),"Classic Controller X button"},
{EVNAME(cc_y),"Classic Controller Y button"},
{EVNAME(cc_plus),"Classic Controller + button"},
{EVNAME(cc_minus),"Classic Controller - button"},
{EVNAME(cc_home),"Classic Controller Home button"},
{EVNAME(cc_left),"Classic Controller Dpad left"},
{EVNAME(cc_right),"Classic Controller Dpad right"},
{EVNAME(cc_up),"Classic Controller Dpad up"},
{EVNAME(cc_down),"Classic Controller Dpad down"},
{EVNAME(cc_l),"Classic Controller L button"},
{EVNAME(cc_zl),"Classic Controller ZL button"},
{EVNAME(cc_r),"Classic Controller R button"},
{EVNAME(cc_zr),"Classic Controller ZR button"},

{-1,nullptr, nullptr}
};

const wiimote_event wiimote_events_axes[] = {
{EVNAME(wm_accel_x),"Wiimote X acceleration (long axis)"},
{EVNAME(wm_accel_y),"Wiimote Y acceleration ((+) <--> (-) axis)"},
{EVNAME(wm_accel_z),"Wiimote Z acceleration (top <--> bottom axis)"},
{EVNAME(wm_ir_x),"Wiimote IR pointer X"},
{EVNAME(wm_ir_y),"Wiimote IR pointer Y"},


{EVNAME(nk_wm_accel_x),"Wiimote X acceleration with Nunchuk"},
{EVNAME(nk_wm_accel_y),"Wiimote Y acceleration with Nunchuk"},
{EVNAME(nk_wm_accel_z),"Wiimote Z acceleration with Nunchuk"},
{EVNAME(nk_ir_x),"Wiimote IR pointer X with Nunchuk"},
{EVNAME(nk_ir_y),"Wiimote IR pointer Y with Nunchuk"},
{EVNAME(nk_accel_x),"Nunchuk X acceleration"},
{EVNAME(nk_accel_y),"Nunchuk Y acceleration"},
{EVNAME(nk_accel_z),"Nunchuk Z acceleration"},
{EVNAME(nk_stick_x),"Nunchuk stick X"},
{EVNAME(nk_stick_y),"Nunchuk stick Y"},

{EVNAME(cc_left_x),"Classic Controller Left Stick X"},
{EVNAME(cc_left_y),"Classic Controller Left Stick Y"},
{EVNAME(cc_right_x),"Classic Controller Right Stick X"},
{EVNAME(cc_right_y),"Classic Controller Right Stick Y"},

{-1,nullptr, nullptr}
};

void wiimote::process(int type, int event_id, long long value) {
  if (!out_dev) return;
  if (type == EVENT_KEY) {
    key_trans[event_id]->process({type,value}, out_dev);
    std::cout << name <<"."<< wiimote_events_keys[event_id].name << " " <<  value << std::endl;
    return;
  }
  if (type == EVENT_AXIS) {
    abs_trans[event_id]->process({type,value}, out_dev);
    std::cout << name <<"."<< wiimote_events_axes[event_id].name << " " <<  value << std::endl;
    return;
  }
  
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
      case SYN_REPORT: out_dev->take_event(ev);
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
    
      out_dev->take_event(ev);

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
      
      out_dev->take_event(ev);
    }
  }
}
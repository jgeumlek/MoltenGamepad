#include "wiimote.h"


#define EVNAME(X) #X
#define BTN DEV_KEY
#define ABS DEV_AXIS
#define OPT DEV_OPTION

int lookup_wii_event(const char* evname) {
  if (evname == nullptr) return -1;
  for (int i = 0; i < wii_event_max; i++) {
    if (!strcmp(evname, wiimote_events[i].name))
      return i;
  }
  return -1;
}

const event_decl wiimote_events[] = {
  {EVNAME(wm_a), "Wiimote A button", BTN, "third"},
  {EVNAME(wm_b), "Wiimote B button", BTN, "fourth"},
  {EVNAME(wm_plus), "Wiimote + button", BTN, "start"},
  {EVNAME(wm_minus), "Wiimote - button", BTN, "select"},
  {EVNAME(wm_1), "Wiimote 1 button", BTN, "second"},
  {EVNAME(wm_2), "Wiimote 2 button", BTN, "first"},
  {EVNAME(wm_home), "Wiimote home button", BTN, "mode"},
  {EVNAME(wm_left), "Wiimote Dpad left", BTN, "down"}, //default mapping assumes wiimote is horizontal
  {EVNAME(wm_right), "Wiimote Dpad right", BTN, "up"}, //like a NES pad
  {EVNAME(wm_up), "Wiimote Dpad up", BTN, "left"},
  {EVNAME(wm_down), "Wiimote Dpad down", BTN, "right"},

  {EVNAME(nk_a), "Wiimote A button with Nunchuk", BTN, "first"},
  {EVNAME(nk_b), "Wiimote B button with Nunchuk", BTN, "second"},
  {EVNAME(nk_plus), "Wiimote + button with Nunchuk", BTN, "start"},
  {EVNAME(nk_minus), "Wiimote - button with Nunchuk", BTN, "select"},
  {EVNAME(nk_1), "Wiimote 1 button with Nunchuk", BTN, "tl"},
  {EVNAME(nk_2), "Wiimote 2 button with Nunchuk", BTN, "tr"},
  {EVNAME(nk_home), "Wiimote home button with Nunchuk", BTN, "mode"},
  {EVNAME(nk_left), "Wiimote Dpad left with Nunchuk", BTN, "left"},
  {EVNAME(nk_right), "Wiimote Dpad right with Nunchuk", BTN, "right"},
  {EVNAME(nk_up), "Wiimote Dpad up with Nunchuk", BTN, "up"},
  {EVNAME(nk_down), "Wiimote Dpad down with Nunchuk", BTN, "down"},
  {EVNAME(nk_c), "Nunchuk C button", BTN, "third"},
  {EVNAME(nk_z), "Nunchuk Z button", BTN, "fourth"},

  {EVNAME(cc_a), "Classic Controller A button", BTN, "first"},
  {EVNAME(cc_b), "Classic Controller B button", BTN, "second"},
  {EVNAME(cc_x), "Classic Controller X button", BTN, "third"},
  {EVNAME(cc_y), "Classic Controller Y button", BTN, "fourth"},
  {EVNAME(cc_plus), "Classic Controller + button", BTN, "start"},
  {EVNAME(cc_minus), "Classic Controller - button", BTN, "select"},
  {EVNAME(cc_home), "Classic Controller Home button", BTN, "mode"},
  {EVNAME(cc_left), "Classic Controller Dpad left", BTN, "left"},
  {EVNAME(cc_right), "Classic Controller Dpad right", BTN, "right"},
  {EVNAME(cc_up), "Classic Controller Dpad up", BTN, "up"},
  {EVNAME(cc_down), "Classic Controller Dpad down", BTN, "down"},
  {EVNAME(cc_l), "Classic Controller L button", BTN, "tl"},
  {EVNAME(cc_zl), "Classic Controller ZL button", BTN, "tl2"},
  {EVNAME(cc_r), "Classic Controller R button", BTN, "tr"},
  {EVNAME(cc_zr), "Classic Controller ZR button", BTN, "tr2"},

  {EVNAME(cc_thumbl), "Classic Controller Left Stick Click (Wii U Pro only)", BTN, "thumbl"},
  {EVNAME(cc_thumbr), "Classic Controller Right Stick Click  (Wii U Pro only)", BTN, "thumbr"},

  {EVNAME(cc_left_x), "Classic Controller Left Stick X", ABS, ""},
  {EVNAME(cc_left_y), "Classic Controller Left Stick Y", ABS, ""},
  {EVNAME(cc_right_x), "Classic Controller Right Stick X", ABS, ""},
  {EVNAME(cc_right_y), "Classic Controller Right Stick Y", ABS, ""},

  {EVNAME(wm_accel_x), "Wiimote X acceleration ((-) <--> (+) axis)", ABS, ""},
  {EVNAME(wm_accel_y), "Wiimote Y acceleration (plug <--> pointer axis)", ABS, ""},
  {EVNAME(wm_accel_z), "Wiimote Z acceleration (top <--> bottom axis)", ABS, ""},
  {EVNAME(wm_ir_x), "Wiimote IR pointer X", ABS, ""},
  {EVNAME(wm_ir_y), "Wiimote IR pointer Y", ABS, ""},


  {EVNAME(nk_wm_accel_x), "Wiimote X acceleration with Nunchuk", ABS, ""},
  {EVNAME(nk_wm_accel_y), "Wiimote Y acceleration with Nunchuk", ABS, ""},
  {EVNAME(nk_wm_accel_z), "Wiimote Z acceleration with Nunchuk", ABS, ""},
  {EVNAME(nk_ir_x), "Wiimote IR pointer X with Nunchuk", ABS, ""},
  {EVNAME(nk_ir_y), "Wiimote IR pointer Y with Nunchuk", ABS, ""},
  {EVNAME(nk_accel_x), "Nunchuk X acceleration", ABS, ""},
  {EVNAME(nk_accel_y), "Nunchuk Y acceleration", ABS, ""},
  {EVNAME(nk_accel_z), "Nunchuk Z acceleration", ABS, ""},
  {EVNAME(nk_stick_x), "Nunchuk stick X", ABS, ""},
  {EVNAME(nk_stick_y), "Nunchuk stick Y", ABS, ""},


  {EVNAME(bal_x), "Balance Board Center of Gravity X", ABS, ""},
  {EVNAME(bal_y), "Balance Board Center of Gravity Y", ABS, ""},
  {EVNAME(wm_gyro_x), "Wiimote Motion+ X Gyro", ABS, ""},
  {EVNAME(wm_gyro_y), "Wiimote Motion+ Y Gyro", ABS, ""},
  {EVNAME(wm_gyro_z), "Wiimote Motion+ Z Gyro", ABS, ""},
  {EVNAME(nk_wm_gyro_x), "Wiimote Motion+ X Gyro with Nunchuk", ABS, ""},
  {EVNAME(nk_wm_gyro_y), "Wiimote Motion+ Y Gyro with Nunchuk", ABS, ""},
  {EVNAME(nk_wm_gyro_z), "Wiimote Motion+ Z Gyro with Nunchuk", ABS, ""},
  {nullptr, nullptr, NO_ENTRY, nullptr}
};

const option_decl wiimote_options[] = {
  {"wm_accel_active", "Enable accelerometers when no extension is present", "false", MG_BOOL},
  {"nk_accel_active", "Enable accelerometers when nunchuk is present", "false", MG_BOOL},
  {"wm_ir_active", "Enable IR data when no extension is present", "false", MG_BOOL},
  {"nk_ir_active", "Enable IR data when nunchuk is present", "false", MG_BOOL},
  {"grab_exclusive", "Grab device events via ioctl EVIOCGRAB", "true", MG_BOOL},
  {"grab_permissions", "Grab device via blocking all read permissions", "false", MG_BOOL},
  {"grab_permissions", "Grab device via blocking all read permissions", "true", MG_BOOL},
  //grab_permissions = "true" only really needed for Wii U Pro, but it doesn't hurt the others.
  {"wm_gyro_active", "Enable Motion+ gyroscopes when no extension is present", "false", MG_BOOL},
  {"nk_gyro_active", "Enable Motion+ gyroscopes when nunchuk is present", "false", MG_BOOL},
  {nullptr, nullptr, nullptr, MG_NULL},
};




void wiimote::process_core() {
  struct input_event ev;
  int ret;
  while (ret = read(buttons.fd, &ev, sizeof(ev)) > 0) {
    int offset = 0;

    if (mode == NUNCHUK_EXT) offset = nk_a;
    if (ret > 0) {
      switch (ev.code) {
      case KEY_LEFT:
        send_value(offset + wm_left, ev.value);
        break;
      case KEY_RIGHT:
        send_value(offset + wm_right, ev.value);
        break;
      case KEY_UP:
        send_value(offset + wm_up, ev.value);
        break;
      case KEY_DOWN:
        send_value(offset + wm_down, ev.value);
        break;
      case BTN_A:
        send_value(offset + wm_a, ev.value);
        break;
      case BTN_B:
        send_value(offset + wm_b, ev.value);
        break;
      case BTN_1:
        send_value(offset + wm_1, ev.value);
        break;
      case BTN_2:
        send_value(offset + wm_2, ev.value);
        break;
      case KEY_PREVIOUS:
        send_value(offset + wm_minus, ev.value);
        break;
      case KEY_NEXT:
        send_value(offset + wm_plus, ev.value);
        break;
      case BTN_MODE:
        send_value(offset + wm_home, ev.value);
        break;
      case SYN_REPORT:
        methods.send_syn_report(ref);
      }
    }
  }
  if (ret < 0) perror("read core");

};

#define CLASSIC_STICK_SCALE ABS_RANGE/24
void wiimote::process_classic(int fd) {
  struct input_event ev;
  int ret = read(fd, &ev, sizeof(ev));
  if (ret > 0) {

    if (ev.type == EV_KEY) switch (ev.code) {
      case KEY_LEFT:
        send_value(cc_left, ev.value);
        break;
      case KEY_RIGHT:
        send_value(cc_right, ev.value);
        break;
      case KEY_UP:
        send_value(cc_up, ev.value);
        break;
      case KEY_DOWN:
        send_value(cc_down, ev.value);
        break;
      case BTN_A:
        send_value(cc_a, ev.value);
        break;
      case BTN_B:
        send_value(cc_b, ev.value);
        break;
      case BTN_X:
        send_value(cc_x, ev.value);
        break;
      case BTN_Y:
        send_value(cc_y, ev.value);
        break;
      case KEY_PREVIOUS:
        send_value(cc_minus, ev.value);
        break;
      case KEY_NEXT:
        send_value(cc_plus, ev.value);
        break;
      case BTN_MODE:
        send_value(cc_home, ev.value);
        break;
      case BTN_TL:
        send_value(cc_l, ev.value);
        break;
      case BTN_TR:
        send_value(cc_r, ev.value);
        break;
      case BTN_TL2:
        send_value(cc_zl, ev.value);
        break;
      case BTN_TR2:
        send_value(cc_zr, ev.value);
        break;
      }
    else if (ev.type == EV_ABS) switch (ev.code) {
      case ABS_HAT1X:
        send_value(cc_left_x, ev.value * CLASSIC_STICK_SCALE);
        break;
      case ABS_HAT1Y:
        send_value(cc_left_y, -ev.value * CLASSIC_STICK_SCALE);
        break;
      case ABS_HAT2X:
        send_value(cc_right_x, ev.value * CLASSIC_STICK_SCALE);
        break;
      case ABS_HAT2Y:
        send_value(cc_right_y, -ev.value * CLASSIC_STICK_SCALE);
        break;
      }
    else {
      methods.send_syn_report(ref);
    }

  }
  if (ret < 0 && errno == ENODEV) {
    close(fd);
    classic.fd = -1;
    return;
  }
  if (ret < 0) perror("read classic ext");
}

#define NUNCHUK_STICK_SCALE ABS_RANGE/85
#define NUNCHUK_ACCEL_SCALE ABS_RANGE/90
void wiimote::process_nunchuk(int fd) {
  struct input_event ev;
  int ret = read(fd, &ev, sizeof(ev));
  if (ret > 0) {

    if (ev.type == EV_KEY) switch (ev.code) {
      case BTN_C:
        send_value(nk_c, ev.value);
        break;
      case BTN_Z:
        send_value(nk_z, ev.value);
        break;
      }
    else if (ev.type == EV_ABS) switch (ev.code) {
      case ABS_HAT0X:
        send_value(nk_stick_x, ev.value * NUNCHUK_STICK_SCALE);
        break;
      case ABS_HAT0Y:
        send_value(nk_stick_y, -ev.value * NUNCHUK_STICK_SCALE);
        break;
      case ABS_RX:
        send_value(nk_accel_x, ev.value * NUNCHUK_ACCEL_SCALE);
        break;
      case ABS_RY:
        send_value(nk_accel_y, ev.value * NUNCHUK_ACCEL_SCALE);
        break;
      case ABS_RZ:
        send_value(nk_accel_z, ev.value * NUNCHUK_ACCEL_SCALE);
        break;
      }
    else {
      methods.send_syn_report(ref);
    }
  }
  if (ret < 0 && errno == ENODEV) {
    close(fd);
    nunchuk.fd = -1;
    return;
  }
  if (ret < 0) perror("read nunchuk");
}

#define WIIMOTE_ACCEL_SCALE ABS_RANGE/90
void wiimote::process_accel(int fd) {
  struct input_event ev;
  int ret;
  while (ret = read(fd, &ev, sizeof(ev)) > 0) {
    int offset = 0;

    if (mode == NUNCHUK_EXT) {
      offset = nk_wm_accel_x;
    } else {
      offset = wm_accel_x;
    }
    if (ret > 0) {
      switch (ev.code) {
      case ABS_RX:
        accelcache[0] = ev.value;
        send_value(offset + 0, (ev.value - accelcalibrations[0]) * WIIMOTE_ACCEL_SCALE);
        break;
      case ABS_RY:
        accelcache[1] = ev.value;
        send_value(offset + 1, (ev.value - accelcalibrations[1]) * WIIMOTE_ACCEL_SCALE);
        break;
      case ABS_RZ:
        accelcache[2] = ev.value;
        send_value(offset + 2, (ev.value - accelcalibrations[2]) * WIIMOTE_ACCEL_SCALE);
        break;
      case SYN_REPORT:
        methods.send_syn_report(ref);
      }
    }
  }
  if (ret < 0) perror("read accel");
}

#define IR_X_SCALE ABS_RANGE/450
#define IR_Y_SCALE ABS_RANGE/350
#define NO_IR_DATA 1023
void wiimote::process_ir(int fd) {
  struct input_event ev;
  int ret;
  while (ret = read(fd, &ev, sizeof(ev)) > 0) {
    switch (ev.code) {
    case ABS_HAT0X:
      ircache[0].x = ev.value;
      break;
    case ABS_HAT0Y:
      ircache[0].y = ev.value;
      break;
    case ABS_HAT1X:
      ircache[1].x = ev.value;
      break;
    case ABS_HAT1Y:
      ircache[1].y = ev.value;
      break;
    case ABS_HAT2X:
      ircache[2].x = ev.value;
      break;
    case ABS_HAT2Y:
      ircache[2].y = ev.value;
      break;
    case ABS_HAT3X:
      ircache[3].x = ev.value;
      break;
    case ABS_HAT3Y:
      ircache[3].y = ev.value;
      break;
    case SYN_REPORT:
      compute_ir();
      methods.send_syn_report(ref);
      break;
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
    //x axis appears to be 0-1022, y axis reported 0-800
    //invert x axis to match user movements: aiming left makes IR dots move right on camera
    //y-axis is already correct, likely due to y-axis/screen conventions (positive is lower)
    send_value(offset + 0, (511-x) * IR_X_SCALE);
    send_value(offset + 1, (y-400) * IR_Y_SCALE);
  }

}

void wiimote::process_motionplus(int fd) {
  struct input_event ev;
  int ret;
  while(ret = read(fd, &ev, sizeof(ev)) > 0) {
    switch (ev.code) {
      case ABS_RX:
        mpcache[0] = ev.value;
        break;
      case ABS_RY:
        mpcache[1] = ev.value;
        break;
      case ABS_RZ:
        mpcache[2] = ev.value;
        break;
      case SYN_REPORT:
        compute_motionplus();
        methods.send_syn_report(ref);
        break;
    }
  }
  if (ret < 0) perror("read motion+");
}

void wiimote::compute_motionplus() {
  if (!motionplus_calibrated)
    return;
  if (mode == NUNCHUK_EXT) {
    send_value(nk_wm_gyro_x, (int64_t)(mpcache[0] - mpcalibrations[0]));
    send_value(nk_wm_gyro_y, (int64_t)(mpcache[1] - mpcalibrations[1]));
    send_value(nk_wm_gyro_z, (int64_t)(mpcache[2] - mpcalibrations[2]));
  } else {
    send_value(wm_gyro_x, (int64_t)(mpcache[0] - mpcalibrations[0]));
    send_value(wm_gyro_y, (int64_t)(mpcache[1] - mpcalibrations[1]));
    send_value(wm_gyro_z, (int64_t)(mpcache[2] - mpcalibrations[2]));
  }
}

#define BAL_X_SCALE ABS_RANGE
#define BAL_Y_SCALE ABS_RANGE
void wiimote::process_balance(int fd) {
  struct input_event ev;
  int ret;
  while (ret = read(fd, &ev, sizeof(ev)) > 0) {
    switch (ev.code) {
    case ABS_HAT0X:
      balancecache[0] = ev.value;
      break;
    case ABS_HAT0Y:
      balancecache[1] = ev.value;
      break;
    case ABS_HAT1X:
      balancecache[2] = ev.value;
      break;
    case ABS_HAT1Y:
      balancecache[3] = ev.value;
      break;
    case SYN_REPORT:
      compute_balance();
      methods.send_syn_report(ref);
      break;
    }
  }
  if (ret < 0) perror("read balance board");
}

void wiimote::compute_balance() {
  int total = balancecache[0] + balancecache[1] + balancecache[2] + balancecache[3];
  int left = balancecache[2] + balancecache[3];
  int right = total - left;
  int front = balancecache[0] + balancecache[2];
  int back = total - front;

  /*Beware: lots of constants from experimentation below!*/

  float x = (right - left) / ((total + 1) * 0.7f);
  float y = (back - front) / ((total + 1) * 0.7f);

  if (total < 125) {
    x = 0;
    y = 0;
  }

  send_value(bal_x, (int)(x * BAL_X_SCALE));
  send_value(bal_y, (int)(y * BAL_Y_SCALE));

}

#define PRO_STICK_SCALE 32
void wiimote::process_pro(int fd) {
  struct input_event ev;
  int ret = read(fd, &ev, sizeof(ev));
  if (ret > 0) {

    if (ev.type == EV_KEY) switch (ev.code) {
      case BTN_DPAD_LEFT:
        send_value(cc_left, ev.value);
        break;
      case BTN_DPAD_RIGHT:
        send_value(cc_right, ev.value);
        break;
      case BTN_DPAD_UP:
        send_value(cc_up, ev.value);
        break;
      case BTN_DPAD_DOWN:
        send_value(cc_down, ev.value);
        break;
      case BTN_EAST:
        send_value(cc_a, ev.value);
        break;
      case BTN_SOUTH:
        send_value(cc_b, ev.value);
        break;
      case BTN_NORTH:
        send_value(cc_x, ev.value);
        break;
      case BTN_WEST:
        send_value(cc_y, ev.value);
        break;
      case BTN_SELECT:
        send_value(cc_minus, ev.value);
        break;
      case BTN_START:
        send_value(cc_plus, ev.value);
        break;
      case BTN_MODE:
        send_value(cc_home, ev.value);
        break;
      case BTN_TL:
        send_value(cc_l, ev.value);
        break;
      case BTN_TR:
        send_value(cc_r, ev.value);
        break;
      case BTN_TL2:
        send_value(cc_zl, ev.value);
        break;
      case BTN_TR2:
        send_value(cc_zr, ev.value);
        break;
      case BTN_THUMBL:
        send_value(cc_thumbl, ev.value);
        break;
      case BTN_THUMBR:
        send_value(cc_thumbr, ev.value);
        break;
      }
    else if (ev.type == EV_ABS) switch (ev.code) {
      case ABS_X:
        send_value(cc_left_x, ev.value * PRO_STICK_SCALE);
        break;
      case ABS_Y:
        send_value(cc_left_y, ev.value * PRO_STICK_SCALE);
        break;
      case ABS_RX:
        send_value(cc_right_x, ev.value * PRO_STICK_SCALE);
        break;
      case ABS_RY:
        send_value(cc_right_y, ev.value * PRO_STICK_SCALE);
        break;
      }
    else {
      methods.send_syn_report(ref);
    }

  }
}

#define CALIBRATION_FACTOR (0.05)
#define CALIBRATION_THRESHOLD 1100
#define CALIBRATION_RESET_THRESHOLD 2000
void wiimote::process_recurring_calibration() {
  if (motionplus_calibrated)
    return;
  double newvals[3];
  double delta = 0;
  for (int i = 0; i < 3; i++) {
    newvals[i] = CALIBRATION_FACTOR*mpcache[i] + (1-CALIBRATION_FACTOR)*mpcalibrations[i];
    delta += (newvals[i] - mpcache[i])*(newvals[i] - mpcache[i]);
    mpcalibrations[i] = newvals[i];
    accelcalibrations[i] = CALIBRATION_FACTOR*accelcalibrations[i] + (1-CALIBRATION_FACTOR)*accelcache[i];
  }
  accelcalibrations[2] = 0; //the axis aligned with gravity, we don't want to zero it out...
  mpvariance = CALIBRATION_FACTOR*delta + (1-CALIBRATION_FACTOR)*mpvariance;
  //printf("raw %08d %08d %08d delta %010.1f %3d\n", mpcache[0], mpcache[1], mpcache[2], delta, mp_required_samples);
  //printf("cal %07.2f %07.2f %07.2f delta %010.1f\n", newvals[0], newvals[1], newvals[2], mpvariance);
  if (mp_required_samples > 0) {
    mp_required_samples--;
    return;
  } else {
    if (mpvariance < CALIBRATION_THRESHOLD) {
      methods.print(ref,"calibration complete. You may now pick up the controller.");
      methods.request_recurring_events(ref, false);
      motionplus_calibrated = true;
    } else if (mpvariance > CALIBRATION_RESET_THRESHOLD) {
      mp_required_samples = 50;
    }
  }
}

int wiimote::upload_ff(const ff_effect* effect) {
  int fd = buttons.fd;
  if (fd < 0)
    fd = pro.fd;
  if (fd < 0)
    return -1;
  int ret = ioctl(fd, EVIOCSFF, effect);
  return effect->id;
}
int wiimote::erase_ff(int id) {
  int fd = buttons.fd;
  if (fd < 0)
    fd = pro.fd;
  if (fd < 0)
    return -1;
  int ret = ioctl(fd, EVIOCRMFF, id);
  return 0;
}
int wiimote::play_ff(int id, int repetitions) {
  int fd = buttons.fd;
  if (fd < 0)
    fd = pro.fd;
  if (fd < 0)
    return -1;
  input_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.type = EV_FF;
  ev.code = id;
  ev.value = repetitions;
  write(fd, &ev, sizeof(ev));
  return 0;
}

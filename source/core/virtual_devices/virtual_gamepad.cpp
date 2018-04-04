#include "virtual_gamepad.h"
#include "../devices/device.h"

virtual_gamepad::virtual_gamepad(std::string name, std::string descr, virtpad_settings settings,slot_manager* slot_man, uinput* ui) : virtual_device(name, descr, slot_man) {
  this->dpad_as_hat = settings.dpad_as_hat;
  this->analog_triggers = settings.analog_triggers;
  effects[0].id = -1;
  settings.u_ids.phys = "moltengamepad/" + name;
  uinput_fd = ui->make_gamepad(settings.u_ids, dpad_as_hat, analog_triggers, settings.rumble);
  if (uinput_fd < 0) throw std::runtime_error("No uinput node available.");
  this->padstyle = settings;
  this->ui = ui;
}

virtual_gamepad::~virtual_gamepad() {
  close_virt_device();
}

void virtual_gamepad::init() {
  if (padstyle.rumble) {
    auto ptr = shared_from_this();
    ui->watch_for_ff(uinput_fd, ptr);
  }
}

static int dpad_hat_axis[4] = {ABS_HAT0Y, ABS_HAT0Y, ABS_HAT0X, ABS_HAT0X};
static int dpad_hat_mult[4] = { -1,        1,         -1,        1        };

void virtual_gamepad::take_event(struct input_event in) {

  //translate DPAD buttons to hat events if needed.
  if (dpad_as_hat && in.type == EV_KEY && (in.code >= BTN_DPAD_UP && in.code <= BTN_DPAD_RIGHT)) {
    int index = in.code - BTN_DPAD_UP;
    in.type = EV_ABS;
    in.code = dpad_hat_axis[index];
    in.value *= dpad_hat_mult[index];
  }
  //follow our face_1234 mapping for the four face buttons... holdover from letting this layout be changed.
  if (in.type == EV_KEY && (in.code >= BTN_SOUTH && in.code <= BTN_WEST)) {
    if (in.code >= BTN_C) in.code--; //Skip BTN_C for computing the offset
    in.code = face_1234[in.code - BTN_SOUTH];
  }
  //rescale/recenter trigger axes to be 0-255
  if (in.type == EV_ABS && (in.code == ABS_Z || in.code == ABS_RZ)) {
    int64_t value = (in.value + 32768) * 255;
    value /= 2 * 32768l;
    in.value = value;
  }
  //these next two cases translate trigger buttons to trigger axes if needed.
  if (analog_triggers && in.type == EV_KEY && in.code == BTN_TR2) {
    in.type = EV_ABS;
    in.code = ABS_RZ, in.value *= 255;
  }
  if (analog_triggers && in.type == EV_KEY && in.code == BTN_TL2) {
    in.type = EV_ABS;
    in.code = ABS_Z,  in.value *= 255;
  }
  ssize_t res = write(uinput_fd, &in, sizeof(in));
  if (res < 0)
      perror("write to virt pad");
};


void virtual_gamepad::destroy_uinput_devs() {
  if (ui && uinput_fd >= 0) {
    ui->uinput_destroy(uinput_fd);
    uinput_fd = -1;
  }
}


void virtual_gamepad::clear_outputs() {
  //Could be optimized to only send gamepad events
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.value = 0;
  for (out_ev.code = 0; out_ev.code < KEY_MAX; out_ev.code++)
    take_event(out_ev);

  out_ev.type = EV_ABS;
  for (out_ev.code = 0; out_ev.code < ABS_MAX; out_ev.code++) {
    if (out_ev.code != ABS_Z && out_ev.code != ABS_RZ) {
      take_event(out_ev);
    } else {
      //have to handle triggers differently, since they are not
      //at 0 when neutral.
      input_event trigger = out_ev;
      trigger.value = -ABS_RANGE;
      take_event(trigger);
    }
  }


}

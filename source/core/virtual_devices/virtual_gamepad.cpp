#include "virtual_gamepad.h"
#include "../devices/device.h"

virtual_gamepad::virtual_gamepad(std::string name, std::string descr, virtpad_settings settings, uinput* ui) : virtual_device(name, descr) {
  this->dpad_as_hat = settings.dpad_as_hat;
  this->analog_triggers = settings.analog_triggers;
  effects[0].id = -1;
  settings.u_ids.phys = "moltengamepad/" + name;
  uinput_fd = ui->make_gamepad(settings.u_ids, dpad_as_hat, analog_triggers, settings.rumble);
  if (uinput_fd < 0) throw std::runtime_error("No uinput node available.");
  if (settings.rumble)
    ui->watch_for_ff(uinput_fd, this);
  this->padstyle = settings;
  this->ui = ui;
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

bool virtual_gamepad::accept_device(std::shared_ptr<input_source> dev) {
  std::lock_guard<std::mutex> guard(lock);
  
  //Accept unless we already have a device of this type.
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    if (ptr && ptr->get_type() == dev->get_type()) {
      return false;
    }
  }
  return true;
}

void virtual_gamepad::destroy_uinput_devs() {
  if (ui && uinput_fd >= 0) {
    ui->uinput_destroy(uinput_fd);
    uinput_fd = -1;
  }
}

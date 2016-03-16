#include "output_slot.h"
#include "uinput.h"

output_slot::~output_slot() {
  if (uinput_fd >= 0) uinput_destroy(uinput_fd);
}

static std::string boolstrings[2] = {"false", "true"};
virtual_gamepad::virtual_gamepad(std::string name, std::string descr, virtpad_settings settings, uinput* ui) : output_slot(name, descr) {
  this->dpad_as_hat = settings.dpad_as_hat;
  this->analog_triggers = settings.analog_triggers;
  set_face_map(settings.facemap_1234);
  uinput_fd = ui->make_gamepad(settings.u_ids, dpad_as_hat, analog_triggers);
  if (uinput_fd < 0) throw - 5;
  options["dpad_as_hat"] = boolstrings[dpad_as_hat];
  options["analog_triggers"] = boolstrings[analog_triggers];
  options["device_string"] = settings.u_ids.device_string;
  options["vendor_id"] = std::to_string(settings.u_ids.vendor_id);
  options["product_id"] = std::to_string(settings.u_ids.product_id);
  options["version_id"] = std::to_string(settings.u_ids.version_id);
  options["facemap_1234"] = get_face_map();
  options["acceptance"] = "singular";
  this->padstyle = settings;
}

virtual_keyboard::virtual_keyboard(std::string name, std::string descr, uinput_ids u_ids, uinput* ui) : output_slot(name, descr) {
  uinput_fd = ui->make_keyboard(u_ids);
  if (uinput_fd < 0) throw - 5;
  options["device_string"] = u_ids.device_string;
  options["vendor_id"] = std::to_string(u_ids.vendor_id);
  options["product_id"] = std::to_string(u_ids.product_id);
  options["version_id"] = std::to_string(u_ids.version_id);
  this->u_ids = u_ids;
}



static int dpad_hat_axis[4] = {ABS_HAT0Y, ABS_HAT0Y, ABS_HAT0X, ABS_HAT0X};
static int dpad_hat_mult[4] = { -1,        1,         -1,        1        };

void virtual_gamepad::take_event(struct input_event in) {

  if (dpad_as_hat && in.type == EV_KEY && (in.code >= BTN_DPAD_UP && in.code <= BTN_DPAD_RIGHT)) {
    int index = in.code - BTN_DPAD_UP;
    in.type = EV_ABS;
    in.code = dpad_hat_axis[index];
    in.value *= dpad_hat_mult[index];
  }
  if (in.type == EV_KEY && (in.code >= BTN_SOUTH && in.code <= BTN_WEST)) {
    if (in.code >= BTN_C) in.code--; //Skip BTN_C for computing the offset
    in.code = face_1234[in.code - BTN_SOUTH];
  }
  if (in.type == EV_ABS && (in.code == ABS_Z || in.code == ABS_RZ)) {
    long long value = (in.value + 32768) * 255;
    value /= 2 * 32768l;
    in.value = value;
  }
  if (analog_triggers && in.type == EV_KEY && in.code == BTN_TR2) {
    in.type = EV_ABS;
    in.code = ABS_RZ, in.value *= 255;
  }
  if (analog_triggers && in.type == EV_KEY && in.code == BTN_TL2) {
    in.type = EV_ABS;
    in.code = ABS_Z,  in.value *= 255;
  }
  write(uinput_fd, &in, sizeof(in));
};

bool virtual_gamepad::accepting() {
  if (acceptance == NONE)
    return false;
  if (acceptance == GREEDY)
    return true;
  return (pad_count == 0);
}

void virtual_gamepad::set_face_map(std::string map) {
  if (map.size() != 4) return;
  //Take a string like "SENW" and use it to map the four action buttons in order.
  //Letters are defined in terms of the evdev codes (ex. BTN_WEST == BTN_Y)
  for (int i = 0; i < 4; i++) {
    if (map[i] == 'a' || map[i] == 'A' || map[i] == 's' || map[i] == 'S') face_1234[i] = BTN_SOUTH;
    if (map[i] == 'b' || map[i] == 'B' || map[i] == 'e' || map[i] == 'E') face_1234[i] = BTN_EAST;
    if (map[i] == 'x' || map[i] == 'X' || map[i] == 'n' || map[i] == 'N') face_1234[i] = BTN_NORTH;
    if (map[i] == 'y' || map[i] == 'Y' || map[i] == 'w' || map[i] == 'W') face_1234[i] = BTN_WEST;
  }
}

std::string virtual_gamepad::get_face_map() {
  std::string out;
  for (int i = 0; i < 4; i++) {
    if (face_1234[i] == BTN_SOUTH) {
      out.push_back('S');
      continue;
    };
    if (face_1234[i] == BTN_EAST)  {
      out.push_back('E');
      continue;
    };
    if (face_1234[i] == BTN_NORTH) {
      out.push_back('N');
      continue;
    };
    if (face_1234[i] == BTN_WEST)  {
      out.push_back('W');
      continue;
    };
    out.push_back('?');
  }
  return out;
}


int virtual_gamepad::process_option(std::string name, std::string value) {
  if (name == "facemap_1234") {
    set_face_map(value);
    return OPTION_ACCEPTED;
  }
  if (name == "acceptance") {
    if (value == "singular") {
      acceptance = SINGULAR;
      return OPTION_ACCEPTED;
    }
    if (value == "none") {
      acceptance = NONE;
      return OPTION_ACCEPTED;
    }
    if (value == "greedy") {
      acceptance = GREEDY;
      return OPTION_ACCEPTED;
    }
  }
  /*all other options not handled yet*/
  return -1;
}

int virtual_keyboard::process_option(std::string name, std::string value) {
  /*all options not handled yet*/
  return -1;
}

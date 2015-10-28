#include "virtual_device.h"
#include "uinput.h"

virtual_device::~virtual_device() {
   if (uinput_fd >= 0) uinput_destroy(uinput_fd);
}

virtual_gamepad::virtual_gamepad(uinput* ui) {
   uinput_fd = ui->make_gamepad(false);
   if (uinput_fd < 0) throw -5;
}

virtual_keyboard::virtual_keyboard(uinput* ui) {
   uinput_fd = ui->make_keyboard();
   if (uinput_fd < 0) throw -5;
}

virtual_gamepad_dpad_as_hat::virtual_gamepad_dpad_as_hat(uinput* ui) : virtual_gamepad(ui) {
  uinput_fd = ui->make_gamepad(true);
   if (uinput_fd < 0) throw -5;
}

static int dpad_hat_axis[4] = {ABS_HAT0Y, ABS_HAT0Y, ABS_HAT0X, ABS_HAT0X};
static int dpad_hat_mult[4] = {-1,        1,         -1,        1        };

void virtual_gamepad_dpad_as_hat::take_event(struct input_event in) {
    
    if (in.type == EV_KEY && (in.code >= BTN_DPAD_UP && in.code <= BTN_DPAD_RIGHT)) {
      int index = in.code - BTN_DPAD_UP;
      in.type = EV_ABS;
      in.code = dpad_hat_axis[index];
      in.value *= dpad_hat_mult[index];
    }
    if(in.type != EV_SYN) std::cout << "virtpad: " << in.type << " " << in.code << " " << in.value << std::endl;
    write(uinput_fd,&in,sizeof(in));
};
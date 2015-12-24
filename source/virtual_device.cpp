#include "virtual_device.h"
#include "uinput.h"

virtual_device::~virtual_device() {
   if (uinput_fd >= 0) uinput_destroy(uinput_fd);
}

virtual_gamepad::virtual_gamepad(std::string name, bool dpad_as_hat, bool analog_triggers, uinput* ui) : virtual_device(name) {
   this->dpad_as_hat = dpad_as_hat;
   uinput_fd = ui->make_gamepad(dpad_as_hat,analog_triggers);
   if (uinput_fd < 0) throw -5;
   descr = "Virtual Gamepad";
   if (dpad_as_hat) descr += " (dpad as hat)";
   if (analog_triggers) descr += " (analog triggers)";
}

virtual_keyboard::virtual_keyboard(std::string name,uinput* ui) : virtual_device(name) {
   uinput_fd = ui->make_keyboard();
   descr = "Virtual Keyboard";
   if (uinput_fd < 0) throw -5;
}



static int dpad_hat_axis[4] = {ABS_HAT0Y, ABS_HAT0Y, ABS_HAT0X, ABS_HAT0X};
static int dpad_hat_mult[4] = {-1,        1,         -1,        1        };

void virtual_gamepad::take_event(struct input_event in) {
    
    if (dpad_as_hat && in.type == EV_KEY && (in.code >= BTN_DPAD_UP && in.code <= BTN_DPAD_RIGHT)) {
      int index = in.code - BTN_DPAD_UP;
      in.type = EV_ABS;
      in.code = dpad_hat_axis[index];
      in.value *= dpad_hat_mult[index];
    }
    if (in.type == EV_ABS && (in.code == ABS_Z || in.code == ABS_RZ)) {
      long long value = (in.value + 32768) * 255;
      value /= 2*32768l;
      in.value = value;
    }
    if(in.type != EV_SYN) std::cout << name<<": " << in.type << " " << in.code << " " << in.value << std::endl;
    write(uinput_fd,&in,sizeof(in));
};
#include "virtual_device.h"
#include "uinput.h"

virtual_device::~virtual_device() {
   uinput_destroy(uinput_fd);
}

virtual_gamepad::virtual_gamepad(uinput* ui) {
   uinput_fd = ui->make_gamepad();
   if (uinput_fd < 0) throw -5;
}

virtual_keyboard::virtual_keyboard(uinput* ui) {
   uinput_fd = ui->make_keyboard();
   if (uinput_fd < 0) throw -5;
}


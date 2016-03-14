#ifndef UINPUT_H
#define UINPUT_H

#include <linux/uinput.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdio.h>

void uinput_destroy(int fd);

struct uinput_ids {
  std::string device_string;
  int vendor_id;
  int product_id;
  int version_id;
};

class uinput {
public:
  uinput();
  ~uinput();
  int make_gamepad(const uinput_ids& ids, bool dpad_as_hat, bool analog_triggers);
  int make_keyboard(const uinput_ids& ids);
  int make_mouse(const uinput_ids& ids);
  int make_pointer(const uinput_ids& ids);

private:
  const char* filename;
};

#endif

#ifndef UINPUT_H
#define UINPUT_H

#include <linux/uinput.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void uinput_destroy(int fd);

class uinput {
public:
  uinput();
  ~uinput();
  int make_gamepad();
  int make_keyboard();
  int make_mouse();
  int make_pointer();

private:
  const char* filename;
  const char* gamepad_name;
  const char* keyboard_name;
  const char* mouse_name;
  const char* pointer_name;
};

#endif

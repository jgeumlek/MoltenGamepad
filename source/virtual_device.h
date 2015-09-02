#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include "uinput.h"
#include <linux/input.h>
#include <iostream>

#define MG_MAX_NAME_SIZE 64


class virtual_device {
public:
   char name[MG_MAX_NAME_SIZE];
   ~virtual_device();
   virtual void take_event(struct input_event in) {
   }
protected:
   int uinput_fd;
};

class virtual_gamepad : public virtual_device {
public:
  int key_cache[KEY_MAX];
  int abs_cache[ABS_MAX];
   
  virtual_gamepad(uinput* ui);
  virtual void take_event(struct input_event in) {
    std::cout << "virtpad: " << in.type << " " << in.code << " " << in.value << std::endl;
    write(uinput_fd,&in,sizeof(in));
  };
};

class virtual_keyboard : public virtual_device {
public:
   int keys[KEY_MAX];

   virtual_keyboard(uinput* ui);
};

#endif

#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include "uinput.h"
#include "eventlists/eventlist.h"
#include <iostream>

#define MG_MAX_NAME_SIZE 64


class virtual_device {
public:
   char name[MG_MAX_NAME_SIZE];
   ~virtual_device();
   virtual void take_event(struct input_event in) {
   }
  
   
   int pad_count = 0;
protected:
   int uinput_fd = -1;
};

class virtual_gamepad : public virtual_device {
public:
   
  virtual_gamepad(uinput* ui);
  virtual void take_event(struct input_event in) {
    if(in.type != EV_SYN) std::cout << "virtpad: " << in.type << " " << in.code << " " << in.value << std::endl;
    write(uinput_fd,&in,sizeof(in));
  };
};

class virtual_gamepad_dpad_as_hat : public virtual_gamepad {
public:
  //up, down, left, right
  
   
  virtual_gamepad_dpad_as_hat(uinput* ui);
  void take_event(struct input_event in);
};


class virtual_keyboard : public virtual_device {
public:
   

   virtual_keyboard(uinput* ui);
};

#endif

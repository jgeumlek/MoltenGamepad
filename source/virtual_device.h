#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include "uinput.h"
#include "eventlists/eventlist.h"
#include <iostream>
#include <string>



class virtual_device {
public:
   std::string name;
   virtual_device(std::string name) {
     this->name = name;
   }
   ~virtual_device();
   virtual void take_event(struct input_event in) {
   }
  
   
   int pad_count = 0;
protected:
   int uinput_fd = -1;
};

class virtual_gamepad : public virtual_device {
public:
  bool dpad_as_hat = false;
  virtual_gamepad(std::string name,bool dpad_as_hat, bool analog_triggers, uinput* ui);
  virtual void take_event(struct input_event in);
};




class virtual_keyboard : public virtual_device {
public:
   

   virtual_keyboard(std::string name,uinput* ui);
   virtual void take_event(struct input_event in) {
    if(in.type != EV_SYN) std::cout << "virtkey: " << in.type << " " << in.code << " " << in.value << std::endl;
    write(uinput_fd,&in,sizeof(in));
  };
};

#endif

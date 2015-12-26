#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include "uinput.h"
#include "eventlists/eventlist.h"
#include <iostream>
#include <string>



class virtual_device {
public:
   std::string name;
   std::string descr;
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
    write(uinput_fd,&in,sizeof(in));
  };
};

class debug_device : public virtual_device {
public:
  debug_device(std::string name, std::string descr) : virtual_device(name) {this->descr = descr;};
  virtual void take_event(struct input_event in) {
    if (in.type == EV_KEY) std::cout << name <<": " << in.code << " " << in.value << "(" << get_key_name(in.code) << ")" << std::endl; 
    if (in.type == EV_ABS) std::cout << name <<": " << in.code << " " << in.value << "(" << get_axis_name(in.code) << ")" << std::endl; 
    if (in.type == EV_REL) std::cout << name <<": " << in.code << " " << in.value << "(" << get_rel_name(in.code) << ")" << std::endl;
  };
};

#endif

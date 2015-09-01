#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include "uinput.h"

#define MG_MAX_NAME_SIZE 64


class virtual_device {
public:
   char name[MG_MAX_NAME_SIZE];
   ~virtual_device();
protected:
   int uinput_fd;
};

class virtual_gamepad : public virtual_device {
public:
   int hey[10];

   virtual_gamepad(uinput* ui);
};

class virtual_keyboard : public virtual_device {
public:
   int hey[200];

   virtual_keyboard(uinput* ui);
};

#endif

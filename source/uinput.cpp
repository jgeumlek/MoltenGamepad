#include "uinput.h"
#include "eventlists/eventlist.h"


const char* try_to_find_uinput() {
  static const char* paths[] = {
    "/dev/uinput",
    "/dev/input/uinput",
    "/dev/misc/uinput"
  };
  const int num_paths = 3;
  int i;

  for (i = 0; i < num_paths; i++) {
    if (access(paths[i],F_OK) == 0) {
      return paths[i];
    }
  }
  return nullptr;
}

void uinput_destroy(int fd) {
  ioctl(fd, UI_DEV_DESTROY);
}

uinput::uinput() {
  filename = try_to_find_uinput();
  if (filename == nullptr) throw -1;

  
}

int uinput::make_gamepad(const uinput_ids &ids, bool dpad_as_hat, bool analog_triggers) {
  static int abs[] = { ABS_X, ABS_Y, ABS_RX, ABS_RY};
  static int key[] = { BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST, BTN_SELECT, BTN_MODE, BTN_START, BTN_TL, BTN_TR, BTN_THUMBL, BTN_THUMBR, -1};
  struct uinput_user_dev uidev;
  int fd;
  int i;
  //TODO: Read from uinput for rumble events?
  fd = open(filename, O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    perror("open uinput");
    return -1;
  }
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, ids.device_string.c_str());
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = ids.vendor_id;
  uidev.id.product = ids.product_id;
  uidev.id.version = ids.version_id;

  ioctl(fd, UI_SET_EVBIT, EV_ABS);
  for (i = 0; i < 4; i++) {
    ioctl(fd, UI_SET_ABSBIT, abs[i]);
    uidev.absmin[abs[i]] = -32768;
    uidev.absmax[abs[i]] = 32768;
    uidev.absflat[abs[i]] = 1024;
  }
  
  if (analog_triggers) {
    ioctl(fd,UI_SET_ABSBIT, ABS_Z);
    uidev.absmin[ABS_Z] = 0;
    uidev.absmax[ABS_Z] = 255;
    uidev.absflat[ABS_Z] = 0;
    ioctl(fd,UI_SET_ABSBIT, ABS_RZ);
    uidev.absmin[ABS_RZ] = 0;
    uidev.absmax[ABS_RZ] = 255;
    uidev.absflat[ABS_RZ] = 0;
  }

  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  for (i = 0; key[i] >= 0; i++) {
    ioctl(fd, UI_SET_KEYBIT, key[i]);
  }
  
  if (dpad_as_hat) {
    ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);
    uidev.absmin[ABS_HAT0X] = -1;
    uidev.absmax[ABS_HAT0X] = 1;
    uidev.absflat[ABS_HAT0X] = 0;
    ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);
    uidev.absmin[ABS_HAT0Y] = -1;
    uidev.absmax[ABS_HAT0Y] = 1;
    uidev.absflat[ABS_HAT0Y] = 0;
  } else {
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
  }
  if (!analog_triggers) {
    ioctl(fd,UI_SET_KEYBIT, BTN_TL2);
    ioctl(fd,UI_SET_KEYBIT, BTN_TR2);
  }
    

  write(fd, &uidev, sizeof(uidev));
  if (ioctl(fd, UI_DEV_CREATE) < 0)
    perror("uinput device creation");
  return fd;
}


int uinput::make_keyboard(const uinput_ids &ids) {
  struct uinput_user_dev uidev;
  int fd;
  int i;

  //TODO: Read from uinput for rumble events?
  fd = open(filename, O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    perror("\nopen uinput");
    return -1;
  }
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, ids.device_string.c_str());
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor = ids.vendor_id;
  uidev.id.product = ids.product_id;
  uidev.id.version = ids.version_id;

  


  /*Just set all possible keys that come before BTN_MISC
   * This should cover all reasonable keyboard keys.*/
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  for (i = 2; i < BTN_MISC; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }
  for (i = KEY_OK; i < KEY_MAX; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }


  
  write(fd, &uidev, sizeof(uidev));
  if (ioctl(fd, UI_DEV_CREATE) < 0)
    perror("uinput device creation");
  return fd;
}

uinput::~uinput() {
}





#include "uinput.h"
#include "eventlists/eventlist.h"
#include "string.h"


const char* try_to_find_uinput() {
  static const char* paths[] = {
    "/dev/uinput",
    "/dev/input/uinput",
    "/dev/misc/uinput"
  };
  const int num_paths = 3;
  int i;

  for (i = 0; i < num_paths; i++) {
    if (access(paths[i], F_OK) == 0) {
      return paths[i];
    }
  }
  return nullptr;
}

std::string uinput_devnode(int fd) {
  char buffer[128];
  memset(buffer,0,sizeof(buffer));
  ioctl(fd, UI_GET_SYSNAME(127), &buffer);
  buffer[127] = '\0';
  return std::string("/sys/devices/virtual/input/") + std::string(buffer);
}

void uinput_destroy(int fd) {
  ioctl(fd, UI_DEV_DESTROY);
}

uinput::uinput() {
  filename = try_to_find_uinput();
  if (filename == nullptr) throw - 1;


}

int uinput::make_gamepad(const uinput_ids& ids, bool dpad_as_hat, bool analog_triggers) {
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
    uidev.absmax[abs[i]] = 32767;
    uidev.absflat[abs[i]] = 1024;
  }

  if (analog_triggers) {
    ioctl(fd, UI_SET_ABSBIT, ABS_Z);
    uidev.absmin[ABS_Z] = 0;
    uidev.absmax[ABS_Z] = 255;
    uidev.absflat[ABS_Z] = 0;
    ioctl(fd, UI_SET_ABSBIT, ABS_RZ);
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
    ioctl(fd, UI_SET_KEYBIT, BTN_TL2);
    ioctl(fd, UI_SET_KEYBIT, BTN_TR2);
  }


  write(fd, &uidev, sizeof(uidev));
  if (ioctl(fd, UI_DEV_CREATE) < 0)
    perror("uinput device creation");

  lock.lock();
  virtual_nodes.push_back(uinput_devnode(fd));
  lock.unlock();

  return fd;
}


int uinput::make_keyboard(const uinput_ids& ids) {
  struct uinput_user_dev uidev;
  int fd;
  int i;


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




  /*Just set all possible keys up to BTN_TASK
   * This should cover all reasonable keyboard and mouse keys.*/
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  for (i = KEY_ESC; i <= BTN_TASK; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }
  for (i = KEY_OK; i < KEY_KBDINPUTASSIST_CANCEL; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }

  /*Set basic mouse events*/
  static int abs[] = { ABS_X, ABS_Y};

  ioctl(fd, UI_SET_EVBIT, EV_ABS);
  for (i = 0; i < 2; i++) {
    ioctl(fd, UI_SET_ABSBIT, abs[i]);
    uidev.absmin[abs[i]] = -32768;
    uidev.absmax[abs[i]] = 32767;
  }

  ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

  write(fd, &uidev, sizeof(uidev));
  if (ioctl(fd, UI_DEV_CREATE) < 0)
    perror("uinput device creation");

  lock.lock();
  virtual_nodes.push_back(uinput_devnode(fd));
  lock.unlock();

  return fd;
}

int uinput::make_mouse(const uinput_ids& ids) {
  struct uinput_user_dev uidev;
  int fd;
  int i;


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




  //Set Mouse buttons
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  for (i = BTN_MOUSE; i <= BTN_MIDDLE; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }

  /*Set basic mouse events*/
  static int rel[] = { REL_X, REL_Y};

  ioctl(fd, UI_SET_EVBIT, EV_REL);
  for (i = 0; i < 2; i++) {
    ioctl(fd, UI_SET_RELBIT, rel[i]);
  }

  ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);

  write(fd, &uidev, sizeof(uidev));
  if (ioctl(fd, UI_DEV_CREATE) < 0)
    perror("uinput device creation");

  lock.lock();
  virtual_nodes.push_back(uinput_devnode(fd));
  lock.unlock();

  return fd;
}

uinput::~uinput() {
}

bool uinput::node_owned(const std::string& path) const {
  lock.lock();
  for (auto node : virtual_nodes) {
    //Check to see if node is a prefix of this path, if so the path belongs to us.
    auto comp = std::mismatch(node.begin(),node.end(),path.begin());
    if (comp.first == node.end()) {
      lock.unlock();
      return true;
    }
  }
  lock.unlock();
  return false;
}





#ifndef WIIMOTE_H
#define WIIMOTE_H
#include <vector>
#include <cstring>
#include <iostream>
#include "device.h"

#define WIIMOTE_NAME "Nintendo Wii Remote"
#define WIIMOTE_ACCEL_NAME  "Nintendo Wii Remote Accelerometer"
#define WIIMOTE_IR_NAME "Nintendo Wii Remote IR"
#define MOTIONPLUS_NAME "Nintendo Wii Remote Motion Plus"
#define NUNCHUK_NAME "Nintendo Wii Remote Nunchuk"
#define CLASSIC_NAME "Nintendo Wii Remote Classic Controller"
#define BALANCE_BOARD_NAME "Nintendo Wii Remote Balance Board"

struct dev_node {
  struct udev_device* dev = nullptr;
  int fd = -1;
};

enum ext_type {NUNCHUK, CLASSIC, GUITAR, DRUMS, UNKNOWN};
class wii_extension {
public:
  const char* descr = "unknown extension";
  struct dev_node node;
  struct wii_dev* parent;
  struct dev_node classic;

  ext_type type = UNKNOWN;
  virtual ~wii_extension() {
    if(node.dev) udev_device_unref(node.dev);
  }
};

class wii_dev : public input_source {
public:
  struct dev_node base;
  enum {WIIMOTE,  BALANCE_BOARD} wii_type;

  const char* name = "unnamed";
  const char* descr = "unidentified Wii device";

  virtual struct name_descr get_info() {
    struct name_descr info;
    info.name = name;
    info.descr = descr;
    return info;
  }
  
  virtual void handle_event(struct udev_device* dev) {
  } 

  virtual void list_events(name_list &list) {
  }
  virtual void list_options(name_list &list) {
  }
  virtual ~wii_dev() {
    if(base.dev) udev_device_unref(base.dev);
  };

};

struct wii_leds {
  int led_fd[4];
};

enum modes {NO_EXT, NUNCHUK_EXT, CLASSIC_EXT};
class wiimote : public wii_dev {
public:
  struct dev_node buttons;
  struct dev_node accel;
  struct dev_node ir;
  struct dev_node motionplus;
  wii_extension* extension = nullptr;
  struct wii_leds leds;

  modes mode = NO_EXT;

  char* name;

  ~wiimote() {
   if (extension) remove_extension();
   if (buttons.dev) udev_device_unref(buttons.dev);
   if (accel.dev) udev_device_unref(accel.dev);
   if (ir.dev) udev_device_unref(ir.dev);
   if (motionplus.dev) udev_device_unref(motionplus.dev);
   void *ptr = name;
   free (ptr);
  }
  virtual void handle_event(struct udev_device* dev);
  virtual struct name_descr get_info() {
    struct name_descr desc;
    desc.name = name;
    desc.descr = "TBD";
    return desc;
  }
  void enable_ir(bool enable);
  void enable_accel(bool enable);
  void enable_motionplus(bool enable);

  void remove_extension() {
    if(extension) {
      std::cout<< name << " lost its extension." << std::endl;
      delete extension;
    }
    extension = nullptr;
    mode = NO_EXT;
  }
  void store_node(struct udev_device* dev, const char* name);
  void remove_node(const char* name);

};



class ext_nunchuk : public wii_extension {
public:
struct dev_node nunchuk;
  bool accel_enabled;
  
  ext_nunchuk() {
    type = NUNCHUK;
    descr = "Nunchuk extension";
  }

  void enable_accel(bool enable);
};

class ext_classic : public wii_extension {
public:
struct dev_node classic;

  ext_classic() {
    type = CLASSIC;
    descr = "Classic Controller extension";
  }
};

class balance_board : public wii_dev {
public:
balance_board() {
    wii_type = BALANCE_BOARD;
    descr = "Balance Board";
  }
};



class wiimotes : public device_manager {
public:
  std::vector<wii_dev*> wii_devs;

  virtual int accept_device(struct udev* udev, struct udev_device* dev);

  virtual void list_devs(name_list &list) {
    for (auto it = wii_devs.begin(); it != wii_devs.end(); ++it) {
      list.push_back((*it)->get_info());
    }
  }

  ~wiimotes() {
    for (auto it = wii_devs.begin(); it != wii_devs.end(); ++it) {
      delete (*it);
    }
  }
private:
  int dev_counter = 0;
};

#endif

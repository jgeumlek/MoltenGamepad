#ifndef DEVICE_H
#define DEVICE_H

#include <libudev.h>
#include <vector>
#include <iostream>
#include "../event_change.h"
#include "../slot_manager.h"

#define ABS_RANGE 32000

typedef std::vector<struct name_descr> name_list;
struct category {
  const char* name;
  name_list entries;
};
typedef std::vector<category> cat_list;

struct name_descr {
  const char* name;
  const char* descr;
  int data;
};

struct source_event {
  const char* name;
  const char* descr;
  enum {ABSOLUTE, RELATIVE, BUTTON} type;
};

class input_source {
public:
  const char* name;
  virtual int set_player(int player_num) {
  }
  virtual void list_events(cat_list &list) {
  }
  virtual void list_options(name_list &list) {
  }
  virtual void set_slot(virtual_device* outdev) {
  }

};

class slot_manager;

class device_manager {
public:
  slot_manager* slot_man;
  virtual int accept_device(struct udev* udev, struct udev_device* dev) {
  return -1;
  }
  virtual void list_devs(name_list &list) {
  };
  
  device_manager(slot_manager* slot_man) : slot_man(slot_man) {
  }

  virtual ~device_manager() {
  }; 

};

#endif

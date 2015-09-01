#ifndef DEVICE_H
#define DEVICE_H

#include <libudev.h>
#include <vector>
#include <iostream>

typedef std::vector<struct name_descr> name_list; 

struct name_descr {
  const char* name;
  const char* descr;
};

struct source_event {
  const char* name;
  const char* descr;
  enum {ABSOLUTE, RELATIVE, BUTTON} type;
};

class input_source {
public:
  const char* name;
  int set_player(int player_num);
  virtual void list_events(name_list &list) {
  }
  virtual void list_options(name_list &list) {
  }

};

class device_manager {
public:
  virtual int accept_device(struct udev* udev, struct udev_device* dev) {
  return -1;
  }
  virtual void list_devs(name_list &list) = 0;

  virtual ~device_manager() {
  }; 

};

#endif

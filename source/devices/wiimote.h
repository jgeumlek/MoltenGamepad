#ifndef WIIMOTE_H
#define WIIMOTE_H
#include <vector>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/epoll.h>
#include "device.h"
#include "wii_events.h"

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
enum wmsg {ADD_FD, POKE};
struct wii_msg {
  wmsg msg;
  int body;
  int arg;
};

enum ext_type {NUNCHUK, CLASSIC, GUITAR, DRUMS, UNKNOWN};


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
  struct dev_node nunchuk;
  struct dev_node classic;
  struct wii_leds leds;

  modes mode = NO_EXT;

  char* name;
  event_translator* key_trans[wii_key_max];
  event_translator* abs_trans[wii_abs_max];
  virtual_device* out_dev;
  
  wiimote();

  ~wiimote();
  
  virtual void list_events(cat_list &list);
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
    if (mode != NO_EXT) std::cout<< name << " lost its extension." << std::endl;
    mode = NO_EXT;
  }
  
  virtual void set_slot(virtual_device* out_dev) {
    this->out_dev = out_dev;
  }
  
  void load_profile(profile* profile);
  void store_node(struct udev_device* dev, const char* name);
  void remove_node(const char* name);
  
  virtual void update_map(const char* evname, event_translator* trans);
  
  virtual enum entry_type entry_type(const char* name);
  
  void read_wiimote();
private:
  bool toggle_ir;
  bool toggle_accel;
  bool toggle_motionplus;
  bool toggle_nunchuk_accel;
  void listen_node(int type,int fd);
  void open_node(struct dev_node* node);
  void process_core();
  void process_classic(int fd);
  void process_nunchuk(int fd);
  void process(int type, int event_id, long long value);
  
  void clear_node(struct dev_node* node);
  
  int priv_pipe;
  
  
  

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
  
  virtual void update_maps(const char* evname, event_translator* trans);
  
  virtual input_source* find_device(const char* name);
  virtual enum entry_type entry_type(const char* name);
  
  void init_profile();

  wiimotes(slot_manager* slot_man) : device_manager(slot_man) {
    name = "wiimote";
    mapprofile.name = "wiimote";
    init_profile();
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

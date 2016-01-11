#ifndef WIIMOTE_H
#define WIIMOTE_H
#include <vector>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/epoll.h>
#include "../device.h"
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


enum ext_type {NUNCHUK, CLASSIC, GUITAR, DRUMS, UNKNOWN};


class wii_dev : public input_source {
public:
  struct dev_node base;

  
  const char* descr = "unidentified Wii device";
  
  wii_dev(slot_manager* slot_man) : input_source(slot_man) {};

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
struct irdata {
  int x = 1023;
  int y = 1023;
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

  char* nameptr;
  
  wiimote(slot_manager* slot_man);

  ~wiimote();
  
  virtual void list_events(cat_list &list);
  virtual void handle_event(struct udev_device* dev);
  virtual struct name_descr get_info() {
    struct name_descr desc;
    desc.name = name;
    desc.descr = "Wiimote";
    return desc;
  }
  void enable_ir(bool enable);
  void enable_accel(bool enable);
  void enable_motionplus(bool enable);

  void update_mode();
  void remove_extension() {
    if (mode != NO_EXT) std::cout<< name << " lost its extension." << std::endl;
    mode = NO_EXT;
    update_mode();
  }
  
 
  
  void store_node(struct udev_device* dev, const char* name);
  void remove_node(const char* name);
  
  
  virtual enum entry_type entry_type(const char* name);
  
  void read_wiimote();
protected:
  void process(void*);
  virtual int process_option(const char* opname, const char* value);

private:
  irdata ircache[4];
  bool wm_accel_active = false;
  bool nk_accel_active = false;
  bool wm_ir_active = false;
  bool nk_ir_active = false;
  
  bool active_ir = false;
  bool active_accel = false;
  bool toggle_motionplus;
  void listen_node(int type,int fd);
  void open_node(struct dev_node* node);
  void process_core();
  void process_classic(int fd);
  void process_nunchuk(int fd);
  void process_accel(int fd);
  void process_ir(int fd);
  void compute_ir();
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
  virtual void update_options(const char* evname, const char* value);
  virtual void update_chords(const char* ev1,const char* ev2, event_translator* trans);
  virtual void update_advanceds(const std::vector<std::string>& names, advanced_event_translator* trans);
  
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

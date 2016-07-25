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
#define WII_U_PRO_NAME "Nintendo Wii Remote Pro Controller"

struct dev_node {
  struct udev_device* dev = nullptr;
  int fd = -1;
  mode_t orig_mode;
};


enum ext_type {NUNCHUK, CLASSIC, GUITAR, DRUMS, UNKNOWN};




struct wii_leds {
  int led_fd[4];
};
struct irdata {
  int x = 1023;
  int y = 1023;
};

enum modes {NO_EXT, NUNCHUK_EXT, CLASSIC_EXT};


class wiimote : public input_source {
public:
  struct dev_node base;
  struct dev_node buttons;
  struct dev_node accel;
  struct dev_node ir;
  struct dev_node motionplus;
  struct dev_node nunchuk;
  struct dev_node classic;
  struct dev_node pro;
  struct dev_node balance;
  struct wii_leds leds;

  modes mode = NO_EXT;

  wiimote(slot_manager* slot_man, device_manager* manager);

  ~wiimote();
  
  virtual void handle_event(struct udev_device* dev);

  void enable_ir(bool enable);
  void enable_accel(bool enable);
  void enable_motionplus(bool enable);

  void update_mode();
  void remove_extension() {
    if (mode != NO_EXT) manager->log.take_message(name + " lost its extension.");
    mode = NO_EXT;
    update_mode();
  }

  virtual std::string get_description() const;
  virtual std::string get_type() const;

  void store_node(struct udev_device* dev, const char* name);
  void remove_node(const char* name);


  void read_wiimote();
protected:
  void process(void*);
  virtual int process_option(const char* opname, const char* value);

private:
  irdata ircache[4];
  int balancecache[4] = {0, 0, 0, 0};
  bool wm_accel_active = false;
  bool nk_accel_active = false;
  bool wm_ir_active = false;
  bool nk_ir_active = false;
  bool grab_exclusive = true;
  bool grab_permissions = false;

  bool active_ir = false;
  bool active_accel = false;
  bool toggle_motionplus;
  void listen_node(int type, int fd);
  void open_node(struct dev_node* node);
  void grab_ioctl_node(struct dev_node* node, bool grabbed);
  void grab_chmod_node(struct dev_node* node, bool grabbed);
  void grab_ioctl(bool grabbed);
  void grab_chmod(bool grabbed);


  void process_core();
  void process_classic(int fd);
  void process_nunchuk(int fd);
  void process_accel(int fd);
  void process_ir(int fd);
  void process_pro(int fd);
  void process_balance(int fd);
  void compute_ir();
  void compute_balance();
  void process(int type, int event_id, int64_t value);

  void clear_node(struct dev_node* node);

  int priv_pipe;

};




class wiimote_manager : public device_manager {
public:
  std::vector<wiimote*> wii_devs;

  virtual int accept_device(struct udev* udev, struct udev_device* dev);

  virtual void for_each_dev(std::function<void (const input_source*)> func) {
    std::lock_guard<std::mutex> lock(devlistlock);
    for (auto wm : wii_devs)
      func(wm);
  };

  void init_profile();

  wiimote_manager(moltengamepad* mg) : device_manager(mg,"wiimote") {
    init_profile();
  }

  ~wiimote_manager() {
    for (auto it = wii_devs.begin(); it != wii_devs.end(); ++it) {
      mg->remove_device(*it);
    }
  }
private:
  int dev_counter = 0;
  std::mutex devlistlock;
};

#endif

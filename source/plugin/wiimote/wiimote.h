#ifndef WIIMOTE_H
#define WIIMOTE_H
#include <vector>
#include <cstring>
#include <iostream>
#include <thread>
#include <sys/epoll.h>
#include <mutex>
#include <unistd.h>
#include <linux/input.h>
#include "../plugin.h"
#include "wii_events.h"

#define WIIMOTE_NAME "Nintendo Wii Remote"
#define WIIMOTE_ACCEL_NAME  "Nintendo Wii Remote Accelerometer"
#define WIIMOTE_IR_NAME "Nintendo Wii Remote IR"
#define MOTIONPLUS_NAME "Nintendo Wii Remote Motion Plus"
#define NUNCHUK_NAME "Nintendo Wii Remote Nunchuk"
#define CLASSIC_NAME "Nintendo Wii Remote Classic Controller"
#define BALANCE_BOARD_NAME "Nintendo Wii Remote Balance Board"
#define WII_U_PRO_NAME "Nintendo Wii Remote Pro Controller"

extern moltengamepad_methods wii_mg_methods;
extern device_plugin wiidev;

struct dev_node {
  struct udev_device* dev = nullptr;
  int fd = -1;
};



enum ext_type {NUNCHUK, CLASSIC, GUITAR, DRUMS, UNKNOWN};




struct wii_leds {
  int led_fd[4];
};
struct irdata {
  int x = 1023;
  int y = 1023;
};

enum modes {NO_EXT, NUNCHUK_EXT, CLASSIC_EXT, PRO_EXT, BALANCE_EXT, MODE_UNCERTAIN};
//MODE_UNCERTAIN is for the early stage where we don't know if this is a wiimote,
// a balance board, or a Wii U Pro controller.


class wiimote {
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

  modes mode = MODE_UNCERTAIN;

  ~wiimote();
  
  virtual void handle_event(struct udev_device* dev);

  void enable_ir(bool enable);
  void enable_accel(bool enable);
  void enable_motionplus(bool enable);

  void update_mode(modes mode);
  void remove_extension() {
    if (mode != NO_EXT)
      methods.print(ref, "removed extension");
    update_mode(NO_EXT);
  }

  const char* get_description() const;
  const char* get_type() const;
  int upload_ff(const ff_effect* effect);
  int erase_ff(int id);
  int play_ff(int id, int repetitions);

  void store_node(struct udev_device* dev, const char* name);
  void remove_node(const char* name);


  void read_wiimote();
  static device_methods methods;
  input_source* ref = nullptr;
  friend PLUGIN_INIT_FUNC(wiimote)(plugin_api api);
protected:
  void process(void*);
  virtual int process_option(const char* opname, const MGField value);

private:
  irdata ircache[4];
  int balancecache[4] = {0, 0, 0, 0};
  int mpcache[3] = {0,0,0};
  int accelcache[3] = {0,0,0};
  double mpcalibrations[3] = {0,0,0};
  double accelcalibrations[3] = {0,0,0};
  double mpvariance = 0;
  int mp_required_samples = 0;
  std::mutex mode_lock;
  bool wm_accel_active = false;
  bool nk_accel_active = false;
  bool wm_ir_active = false;
  bool nk_ir_active = false;
  bool wm_gyro_active = false;
  bool nk_gyro_active = false;
  bool grab_exclusive = true;
  bool grab_permissions = false;

  bool active_ir = false;
  bool active_accel = false;
  bool toggle_motionplus;
  bool motionplus_calibrated = false;
  void listen_node(int type, int fd);
  void open_node(struct dev_node* node);
  void grab_ioctl_node(struct dev_node* node, bool grabbed);
  void grab_chmod_node(struct dev_node* node, bool grabbed);
  void grab_ioctl(bool grabbed);
  void grab_chmod(bool grabbed);


  void send_value(int id, int64_t value) {
    methods.send_value(ref, id, value);
  };
  void process_core();
  void process_classic(int fd);
  void process_nunchuk(int fd);
  void process_accel(int fd);
  void process_ir(int fd);
  void process_pro(int fd);
  void process_balance(int fd);
  void process_motionplus(int fd);
  void process_recurring_calibration();
  void compute_ir();
  void compute_balance();
  void compute_motionplus();
  void process(int type, int event_id, int64_t value);

  void clear_node(struct dev_node* node);

  int priv_pipe;

};




class wiimote_manager {
public:
  std::vector<wiimote*> wii_devs;

  int accept_device(struct udev* udev, struct udev_device* dev);


  void init_profile();

  int init(device_manager* ref) {
    this->ref = ref;
    init_profile();
    return SUCCESS;
  }

  int process_manager_option(const char* name, MGField value);

  static manager_methods methods;
  static int (*request_slot) (input_source*);
  static int (*grab_permissions) (udev_device*, bool);
  static bool auto_assign_balance;

private:
  int dev_counter = 0;
  std::mutex devlistlock;
  device_manager* ref;
  wiimote* find_wii_dev_by_path(const char* syspath);
  int destroy_wii_dev_by_path(const char* syspath);
};

extern int wiimote_plugin_init(plugin_api api);

#endif

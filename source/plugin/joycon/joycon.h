#pragma once
#include "../plugin.h"
#include <linux/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <vector>
#include <mutex>
#include <time.h>

#define RIGHT_JOYCON_NAME "Joy-Con (R)"
#define LEFT_JOYCON_NAME "Joy-Con (L)"

extern device_plugin joycon_dev;

extern const event_decl joycon_events[];

extern const uint8_t request_state[9];

extern const uint8_t mystery_packet[9];

enum joycon_event_id {
  a,
  b,
  x,
  y,
  up,
  down,
  left,
  right,
  plus,
  minus,
  capture,
  home,
  l,
  zl,
  r,
  zr,
  thumbl,
  thumbr,
  l_sl,
  l_sr,
  r_sl,
  r_sr,
  left_x,
  left_y,
  right_x,
  right_y,
  solo_up,
  solo_down,
  solo_left,
  solo_right,
  solo_sl,
  solo_sr,
  solo_capplus,
  solo_homeminus,
  solo_lr,
  solo_zlzr,
  solo_thumbl,
  solo_x,
  solo_y,
};

enum JoyConMode {
  UNKNOWN,
  PARTNERED,
  SOLO,
};

enum JoyConSide {
  LEFT_JOYCON,
  RIGHT_JOYCON,
  BOTH_JOYCON,
  UNKNOWN_JOYCON,
};
  
class joycon_manager;

class joycon {
public:
  joycon(int fd1, int fd2, JoyConSide side1, JoyConSide side2, const char* syspath1, const char* syspath2, joycon_manager* manager);
  ~joycon();
  int init(input_source* ref);

  input_source* ref;

  friend PLUGIN_INIT_FUNC(joycon)(plugin_api api);
  static device_methods methods;

  constexpr static const char* name_stem = "jc";
  const char* get_description() const;

  void take_joycon_event(int id, uint64_t value);

  void process_recurring_event();
  void read_report(int index);
  std::string path[2];
  //sometimes we want to keep the fd open when deleting this joycon object.
  bool close_out_fd[2] = {true,true};
  int sent_cycle = 0;
  //booleans used for detecting JoyCon partnerships
  bool active_trigger[2] = {false, false}; //For each fd open, is their ZL/ZR pressed?
  bool active_solo_btns[2] = {false, false}; //For each fd open, is their SL+SR pressed?
  JoyConSide sides[2] = {UNKNOWN_JOYCON,UNKNOWN_JOYCON};
  int fds[2];
  bool activated = false;
  JoyConMode mode = UNKNOWN;
  int pending_reports[2] = {0,0};
protected:
  void process(void*);
  int process_option(const char* opname, const MGField value);
  uint8_t report[2][0x31];
  joycon_manager* manager;
};

//info used for detecting JoyCon partnerships.
struct joycon_info {
  joycon* jc = nullptr;
  bool active_trigger = false;
};

class joycon_manager {
public:

  void init_profile();

  int init(device_manager* ref);

  int start();

  int accept_device(struct udev* udev, struct udev_device* dev);

  int process_option(const char* name, const MGField value);

  joycon_manager();

  ~joycon_manager();

  friend PLUGIN_INIT_FUNC(joycon)(plugin_api api);
  static manager_methods methods;

  static int (*grab_permissions) (udev_device*, bool);

  constexpr static const char* name = "joycon";

  void check_partnership(joycon* jc);
private:
  device_manager* ref;
  std::vector<joycon_info> joycons;
  std::mutex lock;
};


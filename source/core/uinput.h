#ifndef UINPUT_H
#define UINPUT_H

#include <linux/uinput.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <map>
#include <memory>

struct uinput_ids {
  std::string device_string;
  std::string phys;
  int vendor_id;
  int product_id;
  int version_id;
};

class virtual_device;

class uinput {
public:
  uinput();
  ~uinput();
  int make_gamepad(const uinput_ids& ids, bool dpad_as_hat, bool analog_triggers, bool rumble);
  int make_keyboard(const uinput_ids& ids);
  int make_mouse(const uinput_ids& ids);
  int watch_for_ff(int fd, std::shared_ptr<virtual_device>& slot);
  void uinput_destroy(int fd);
  int start_ff_thread();

private:
  const char* filename;
  std::map<int, std::weak_ptr<virtual_device>> ff_slots;
  int epfd;
  std::thread* ff_thread;
  mutable std::mutex lock;
  volatile bool keep_looping;
  bool safe_to_close();

  int setup_epoll();
  void ff_thread_loop();
};


#endif

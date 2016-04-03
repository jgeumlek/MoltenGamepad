#ifndef UDEV_H
#define UDEV_H
#include <libudev.h>
#include <vector>
#include <thread>


class device_manager;
class uinput;

class udev_handler {
public:
  std::vector<device_manager*>* managers;
  struct udev* udev;
  struct udev_monitor* monitor;
  const uinput* ui = nullptr;
  std::thread* monitor_thread;
  volatile bool stop_thread = false;
  int pipe_fd;

  udev_handler();
  ~udev_handler();

  void set_managers(std::vector<device_manager*>* managers);
  void set_uinput(const uinput* ui);
  int enumerate();
  int start_monitor();
  int udev_fd();
  int read_monitor();
private:
  void pass_along_device(struct udev_device* new_dev);
};



#endif

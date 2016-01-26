#ifndef UDEV_H
#define UDEV_H
#include <libudev.h>
#include <vector>
#include <thread>


class device_manager;
class udev_handler {
public:
  std::vector<device_manager*>* managers;
  struct udev* udev;
  struct udev_monitor* monitor;
  std::thread* monitor_thread;
  volatile bool stop_thread = false;
  int pipe_fd;

  udev_handler();
  ~udev_handler();
  
  void set_managers(std::vector<device_manager*>* devs);
  int enumerate();
  int start_monitor();
  int udev_fd();
  int read_monitor();
};



#endif

#include "udev.h"
#include <iostream>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>

void pass_along_device(std::vector<device_manager*>* devs, struct udev* ud, struct udev_device* new_dev) {
  if (new_dev == nullptr) return;
  for(auto it = devs->begin(); it != devs->end(); ++it) {
    device_manager* what = *it;
    int ret = what->accept_device(ud,new_dev);
    if (ret == 0) break;
  }
}

int reads_monitor(udev_handler* uh);
  

udev_handler::udev_handler() {
  udev = udev_new();
  if (udev == nullptr) throw -11;

  monitor = nullptr;
  monitor_thread = nullptr;
}

udev_handler::~udev_handler() {
  if (monitor_thread) {
    stop_thread = true;
    monitor_thread->join();
    delete monitor_thread;
  }
  if (monitor) udev_monitor_unref(monitor);
  if (udev) udev_unref(udev);
}

void udev_handler::set_managers(std::vector<device_manager*> *devs) {
  managers = devs;
}

int udev_handler::start_monitor() {
  monitor = udev_monitor_new_from_netlink(udev,"udev");
  //udev_monitor_filter_add_match_subsystem_devtype(monitor,"hid",NULL);
  udev_monitor_filter_add_match_subsystem_devtype(monitor,"input",NULL);

  udev_monitor_enable_receiving(monitor);

  monitor_thread = new std::thread(&udev_handler::read_monitor, this);
  return 0;
}

int udev_handler::enumerate() {
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);
  //udev_enumerate_add_match_subsystem(enumerate,"hid");
  udev_enumerate_add_match_subsystem(enumerate,"input");

  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry* entry;

  udev_list_entry_foreach(entry, devices) {
    const char* path = udev_list_entry_get_name(entry);
    struct udev_device* dev = udev_device_new_from_syspath(udev,path);
    pass_along_device(managers, udev, dev);
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  return 0;
}


int udev_handler::read_monitor() {
  static const int EPOLL_MAX_EVENTS = 1;
  struct epoll_event event;
  struct epoll_event events[EPOLL_MAX_EVENTS];
  int epfd = epoll_create(EPOLL_MAX_EVENTS);
  memset(&events,0,sizeof(events));

  memset(&event,0,sizeof(event));

  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  event.data.ptr = this;
  epoll_ctl(epfd, EPOLL_CTL_ADD, udev_monitor_get_fd(monitor), &event);

  int pipes[2];
  pipe(pipes);
  event.data.ptr = nullptr;
  epoll_ctl(epfd, EPOLL_CTL_ADD, pipes[0], &event);

  pipe_fd = pipes[1];


  while (!stop_thread) {
    int n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, -1);
    if (!events[0].data.ptr) {
      char buffer[2];
      int ret = 1;
        ret = read(pipes[0],&buffer,sizeof(buffer));
    } else {
      struct udev_device *dev = udev_monitor_receive_device(monitor);
      if (dev) {
        pass_along_device(managers, udev, dev);
        udev_device_unref(dev);
      }
    }
  }
  std::cout << "stopping udev thread" << std::endl;

  return 0;
}

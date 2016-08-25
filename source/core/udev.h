#ifndef UDEV_H
#define UDEV_H
#include <libudev.h>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>


class device_manager;
class uinput;

struct node_permissions {
  udev_device* node;
  mode_t orig_mode;
};

//A node might have dependent nodes, like the jsX device for an eventX device.
struct grabbed_node {
  std::vector<node_permissions> children;
};

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
  int grab_permissions(udev_device* dev, bool grabbed);
private:
  void pass_along_device(struct udev_device* new_dev);
  std::mutex manager_lock;
  std::mutex grabbed_nodes_lock;
  std::unordered_map<std::string,grabbed_node> grabbed_nodes;
};



#endif

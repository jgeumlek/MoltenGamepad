#include "udev.h"
#include <iostream>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <glob.h>
#include <algorithm>
#include "devices/device.h"
#include "uinput.h"

struct deferred_claim {
  device_manager* manager;
  int order;
};

bool claim_cmp (const deferred_claim& a, const deferred_claim& b) {
  return a.order < b.order;
}

void udev_handler::pass_along_device(struct udev_device* new_dev) {
  if (new_dev == nullptr) return;
  std::vector<deferred_claim> deferred;
  std::lock_guard<std::mutex> lock(manager_lock);
  if (managers == nullptr) return;
  std::string path(udev_device_get_syspath(new_dev));
  if (ui && ui->node_owned(path))
    return; //Skip virtual devices we made

  //Give each manager a chance to claim the device.
  //Any deferred claims will be handled after going past every manager.
  for (auto it = managers->begin(); it != managers->end(); ++it) {
    device_manager* man = *it;
    int ret = man->accept_device(udev, new_dev);
    if (ret == DEVICE_CLAIMED) return;
    if (ret == DEVICE_UNCLAIMED || ret < 0) continue;
    deferred.push_back({man, ret});
  }
  if (deferred.empty())
    return; //no claims, no deferred claims.

  //use stable sort so that the normal ordering still applies.
  std::stable_sort(deferred.begin(), deferred.end(), claim_cmp);
  for (deferred_claim claim : deferred) {
    int ret = claim.manager->accept_deferred_device(udev, new_dev);
    if (ret == DEVICE_CLAIMED) break;
  }

}



udev_handler::udev_handler() {
  udev = udev_new();
  if (udev == nullptr) throw - 11;

  monitor = nullptr;
  monitor_thread = nullptr;
}

udev_handler::~udev_handler() {
  if (monitor_thread) {
    stop_thread = true;
    int signal = 0;
    write(pipe_fd, &signal, sizeof(signal));
    monitor_thread->join();
    delete monitor_thread;
  }
  for (auto entry : grabbed_nodes) {
    for (auto node : entry.second.children) {
      const char* devnode = udev_device_get_devnode(node.node);
      chmod(devnode, node.orig_mode);
      udev_device_unref(node.node);
    }
  }
  grabbed_nodes.clear();
  if (monitor) udev_monitor_unref(monitor);
  if (udev) udev_unref(udev);
}

void udev_handler::set_managers(std::vector<device_manager*>* managers) {
  std::lock_guard<std::mutex> lock(manager_lock);
  this->managers = managers;
}

void udev_handler::set_uinput(const uinput* ui) {
  this->ui = ui;
}

int udev_handler::start_monitor() {
  monitor = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "hid", NULL);
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL);

  udev_monitor_enable_receiving(monitor);

  monitor_thread = new std::thread(&udev_handler::read_monitor, this);
  return 0;
}

int udev_handler::enumerate() {
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "hid");
  udev_enumerate_add_match_subsystem(enumerate, "input");

  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry* entry;

  udev_list_entry_foreach(entry, devices) {
    const char* path = udev_list_entry_get_name(entry);
    struct udev_device* dev = udev_device_new_from_syspath(udev, path);
    pass_along_device(dev);
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
  memset(&events, 0, sizeof(events));

  memset(&event, 0, sizeof(event));

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
      ret = read(pipes[0], &buffer, sizeof(buffer));
    } else {
      struct udev_device* dev = udev_monitor_receive_device(monitor);
      if (dev) {
        pass_along_device(dev);
        const char* action = udev_device_get_action(dev);
        if (!strcmp(action, "remove")) {
          grab_permissions(dev, false);
        }
        udev_device_unref(dev);
      }
    }
  }
  std::cout << "stopping udev thread" << std::endl;

  return 0;
}

int udev_handler::grab_permissions(udev_device* dev, bool grabbed) {
  std::lock_guard<std::mutex> guard(grabbed_nodes_lock);
  const char* devnode = udev_device_get_devnode(dev);
  if (!devnode)
    return FAILURE;
  std::string devnodepath(devnode);
  if (grabbed) {
    //We need to do the grabbing!
    if (grabbed_nodes.find(devnodepath) != grabbed_nodes.end())
      return FAILURE;
    struct stat filestat;
    stat(devnode, &filestat);
    node_permissions base_node;
    base_node.node = udev_device_ref(dev);
    base_node.orig_mode = filestat.st_mode;
    grabbed_nodes[devnodepath].children.push_back(base_node);
    chmod(devnode, 0);

    //This device might have a js device we also want to grab...
    auto parent = udev_device_get_parent(dev);
    std::string parentpath(udev_device_get_syspath(parent));
    parentpath += "/js[0-9]*";
    glob_t globbuffer;
    glob(parentpath.c_str(), 0, nullptr, &globbuffer);

    for (int i = 0; i < globbuffer.gl_pathc; i++) {
      udev_device* subdev = udev_device_new_from_syspath(udev, globbuffer.gl_pathv[i]);
      if (!subdev)
        continue;
      devnode = udev_device_get_devnode(subdev);
      stat(devnode, &filestat);
      node_permissions sub_node;
      sub_node.node = udev_device_ref(subdev);
      sub_node.orig_mode = filestat.st_mode;
      grabbed_nodes[devnodepath].children.push_back(sub_node);
      chmod(devnode, 0);
      udev_device_unref(subdev);
    }

    globfree(&globbuffer);
    return SUCCESS;
  } else {
    //Undo the grabbing!
    auto it = grabbed_nodes.find(devnodepath);
    if (it == grabbed_nodes.end())
      return FAILURE;
    for (auto entry : it->second.children) {
      devnode = udev_device_get_devnode(entry.node);
      chmod(devnode, entry.orig_mode);
      udev_device_unref(entry.node);
    }
    grabbed_nodes.erase(devnodepath);
    return SUCCESS;
  }
  return FAILURE;
}

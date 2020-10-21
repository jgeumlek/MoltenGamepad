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
  const char* action = udev_device_get_action(new_dev);
  if (!action) action = "enumerated";
  debug_print(DEBUG_INFO, 4, "device ",action," ",path.c_str());
  struct udev_device* hidparent = udev_device_get_parent_with_subsystem_devtype(new_dev,"hid",NULL);
  struct udev_device* parent = udev_device_get_parent(new_dev);
  const char* phys = udev_device_get_sysattr_value(new_dev,"phys");

  if (!phys && hidparent) {
    phys = udev_device_get_property_value(hidparent, "HID_PHYS");
  }
  while (!phys && parent) {
    phys = udev_device_get_sysattr_value(parent,"phys");
    parent = udev_device_get_parent(parent);
  }
  if (phys && !strncmp(phys,"moltengamepad",13)) {
    debug_print(DEBUG_VERBOSE, 1, "\tskipped because it was made by MoltenGamepad");
    return; //Skip virtual devices we made
  }

  //Give each manager a chance to claim the device.
  //Any deferred claims will be handled after going past every manager.
  for (auto it = managers->begin(); it != managers->end(); ++it) {
    device_manager* man = *it;
    int ret = man->accept_device(udev, new_dev);
    if (ret == DEVICE_CLAIMED) {
      debug_print(DEBUG_INFO, 2, "\tclaimed by manager ",man->name.c_str());
      return;
    }
    if (ret == DEVICE_UNCLAIMED || ret < 0) continue;
    debug_print(DEBUG_INFO, 4, "\tclaimed by manager ",man->name.c_str(), ", order = ", std::to_string(ret+1).c_str());
    deferred.push_back({man, ret});
  }
  if (deferred.empty())
    return; //no claims, no deferred claims.

  //use stable sort so that the normal ordering still applies.
  std::stable_sort(deferred.begin(), deferred.end(), claim_cmp);
  for (deferred_claim claim : deferred) {
    int ret = claim.manager->accept_deferred_device(udev, new_dev);
    if (ret == DEVICE_CLAIMED)  {
      debug_print(DEBUG_INFO, 2, "\tultimately claimed by manager ", claim.manager->name.c_str());
      break;
    } else {
      debug_print(DEBUG_INFO, 3, "\trejected by manager ", claim.manager->name.c_str(), ", despite previous claim");
    }
  }

}



udev_handler::udev_handler() {
  udev = udev_new();
  if (udev == nullptr) throw std::runtime_error("udev failed.");

  monitor = nullptr;
  monitor_thread = nullptr;
}

udev_handler::~udev_handler() {
  if (monitor_thread) {
    stop_thread = true;
    int signal = 0;
    //just blindly write a byte to wake up the thread...
    ssize_t res = write(pipe_fd, &signal, sizeof(signal));
    if (res < 0)
      perror("write to wake udev monitor");
    try {
      monitor_thread->join();
    } catch (std::exception& e) {
    }
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
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw",NULL);
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL);

  udev_monitor_enable_receiving(monitor);

  monitor_thread = new std::thread(&udev_handler::read_monitor, this);
  return 0;
}

int udev_handler::enumerate() {
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "hid");
  udev_enumerate_add_match_subsystem(enumerate, "hidraw");
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
  int res = pipe(pipes);
  if (res != 0) 
    throw std::runtime_error("internal pipe creation failed.");
  event.data.ptr = nullptr;
  epoll_ctl(epfd, EPOLL_CTL_ADD, pipes[0], &event);

  pipe_fd = pipes[1];


  while (!stop_thread) {
    epoll_wait(epfd, events, EPOLL_MAX_EVENTS, -1);
    if (!events[0].data.ptr) {
      char buffer[2];
      int ret = read(pipes[0], &buffer, sizeof(buffer));
      if (ret < 0)
        perror("read to wake udev monitor");
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
  if (!dev)
    return FAILURE;
  if (!grabbed)
    return grab_permissions(dev, grabbed, 0); //flags don't matter when releasing.
  const char* subsystem = udev_device_get_subsystem(dev);
  if (subsystem && (!strcmp(subsystem, "hid") || !strcmp(subsystem, "hidraw")))
    return grab_permissions(dev, grabbed, GRAB_HID_NODE | GRAB_EVENT_AND_JS_NODE);

  return grab_permissions(dev, grabbed, GRAB_EVENT_AND_JS_NODE);
}

int udev_handler::grab_permissions(udev_device* dev, bool grabbed, int flags) {
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
    debug_print(DEBUG_NONE,2,"device grab: changing permissions of ",devnode);
    int ret = stat(devnode, &filestat);
    if (!ret)
      ret = chmod(devnode, 0);
    if (ret) {
      debug_print(DEBUG_NONE,3,"device hiding: changing permissions of ",devnode," failed.");
    } else {
      node_permissions base_node;
      base_node.node = udev_device_ref(dev);
      base_node.orig_mode = filestat.st_mode;
      grabbed_nodes[devnodepath].children.push_back(base_node);
    }
    

    udev_device* parent = udev_device_get_parent(dev);
    udev_device* hid_parent = udev_device_get_parent_with_subsystem_devtype(dev,"hid",nullptr);
    const char* subsystem = udev_device_get_subsystem(dev);

    //This device might have a js device we also want to grab...
    //go up a level to find siblings...
    std::string devpath(udev_device_get_syspath(dev));
    std::string parentpath(udev_device_get_syspath(parent));
    std::string hidparentpath(parentpath);
    hidparentpath += "/../..";
    if (hid_parent) hidparentpath = udev_device_get_syspath(hid_parent);
    std::string globstr = "";
    if (!subsystem)
      return SUCCESS; //we grabbed all that we know how to do!

    if (!strcmp(subsystem,"input") && (flags & GRAB_EVENT_AND_JS_NODE) && !(flags & GRAB_HID_NODE)) {
      globstr = parentpath + "/js*";
    }
    if (!strcmp(subsystem,"input") && (flags & GRAB_EVENT_AND_JS_NODE) && (flags & GRAB_HID_NODE)) {
      globstr = hidparentpath + "{,/hidraw/hidraw*,/input/input*/event*,/input/input*/js*}";
    }
    if (!strcmp(subsystem,"hid") && (flags & GRAB_EVENT_AND_JS_NODE)) {
      globstr = devpath + "{/hidraw/hidraw*,/input/input*/event*,/input/input*/js*}";
    }
    if (!strcmp(subsystem,"hid") && (flags & ~GRAB_EVENT_AND_JS_NODE)) {
      globstr = devpath + "{/hidraw/hidraw*}";
    }
    if (!strcmp(subsystem,"hidraw") && (flags & GRAB_EVENT_AND_JS_NODE)) {
      globstr = hidparentpath + "{input/input*/event*,/input/input*/js*}";
    }
    glob_t globbuffer;
    glob(globstr.c_str(), GLOB_BRACE, nullptr, &globbuffer);

    for (uint i = 0; i < globbuffer.gl_pathc; i++) {
      udev_device* subdev = udev_device_new_from_syspath(udev, globbuffer.gl_pathv[i]);
      if (!subdev)
        continue;
      devnode = udev_device_get_devnode(subdev);
      if (!devnode)
        continue;
      std::string subdevpath(devnode);
      if (subdevpath == devnodepath)
        continue;
      debug_print(DEBUG_NONE,2,"device hiding: changing permissions of ",devnode);
      ret = stat(devnode, &filestat);
      node_permissions sub_node;
      if(!ret)
        ret = chmod(devnode, 0);
      if (ret) {
        debug_print(DEBUG_NONE,3,"device hiding: changing permissions of ",devnode," failed.");
      } else {
        sub_node.node = udev_device_ref(subdev);
        sub_node.orig_mode = filestat.st_mode;
        grabbed_nodes[devnodepath].children.push_back(sub_node);
      }
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
      debug_print(DEBUG_NONE,2,"device unhiding: restoring permissions of ",devnode);
      int ret = chmod(devnode, entry.orig_mode);
      if (ret && errno != ENOENT) {
        perror("restore permissions");
        debug_print(DEBUG_NONE,3,"device unhiding: restoring permissions of ",devnode," failed.");
      }
      udev_device_unref(entry.node);
    }
    grabbed_nodes.erase(devnodepath);
    return SUCCESS;
  }
  return FAILURE;
}

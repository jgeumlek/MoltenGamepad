#include "generic.h"
#include <errno.h>


generic_file::generic_file(moltengamepad* mg, struct udev_device* node, bool grab_ioctl, bool grab_chmod, bool rumble) {
  this->mg = mg;
  this->rumble = rumble;
  struct udev_device* hidparent = udev_device_get_parent_with_subsystem_devtype(node,"hid",NULL);
  if (hidparent) {
    const char* uniq_id = udev_device_get_property_value(hidparent, "HID_UNIQ");
    if (uniq_id) 
      uniq = std::string(uniq_id);
    const char* phys_id = udev_device_get_property_value(hidparent, "HID_PHYS");
    if (phys_id)
      phys = std::string(phys_id);
  }
  epfd = epoll_create(1);
  if (epfd < 1) perror("epoll create");
  this->grab_ioctl = grab_ioctl;
  this->grab_chmod = grab_chmod;
  open_node(node);
  //set up a pipe so we can talk to out own thread.
  pipe(internal_pipe);
  
  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  event.data.u32 = internal_pipe[0];
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, internal_pipe[0], &event);
  if (ret < 0) perror("epoll add");
  
  if (fds.empty()) throw std::runtime_error("Generic device node opening failed.");

  thread = new std::thread(&generic_file::thread_loop, this);
}


generic_file::~generic_file() {
  keep_looping = false;
  for (auto node_it : nodes) {
    close_node(node_it.first, false); //TODO: Fix this repeated map look up...
  }
  if (thread) {
    int beep = 0;
    write(internal_pipe[1],&beep,sizeof(beep));
    try {
      thread->join();
    } catch (std::exception& e) {
    }
    delete thread;
  }
  for (auto dev : devices) {
    mg->remove_device(dev.get());
  }
  close(epfd);
  close(internal_pipe[0]);
  close(internal_pipe[1]);

}


void generic_file::open_node(struct udev_device* node) {
  std::lock_guard<std::mutex> guard(lock);
  std::string path(udev_device_get_devnode(node));
  if (nodes.find(path) == nodes.end()) {
    int mode = rumble ? O_RDWR : O_RDONLY;
    int fd = open(udev_device_get_devnode(node), mode | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0 && mode == O_RDWR && errno == EACCES) {
      fd = open(udev_device_get_devnode(node), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
      if (fd >= 0)
        mg->stdout->err(0,"Generic device could not get write permissions. Rumble effects are disabled.");
    }
    if (fd < 0) {
      perror("open subdevice:");
      return;
    }

    if (grab_ioctl) {
      ioctl(fd, EVIOCGRAB, 1);
    }

    if (grab_chmod) {
      //Remove all permissions. Other software will really ignore it.
      //Requires the device to be owned by the current user. (not merely have access)
      mg->udev.grab_permissions(node, true);

    }

    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    event.data.u32 = fd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    if (ret < 0) perror("epoll add");

    fds.push_back(fd);
    nodes[path] = {path, udev_device_ref(node), fd};
  }
}

void generic_file::close_node(struct udev_device* node, bool erase) {
  const char* path = udev_device_get_devnode(node);
  if (!path) return;

  close_node(std::string(path), erase);
}

void generic_file::close_node(const std::string& path, bool erase) {
  std::lock_guard<std::mutex> guard(lock);
  auto it = nodes.find(path);

  if (it == nodes.end()) return;

  close(it->second.fd);
  if (grab_chmod)
    mg->udev.grab_permissions(it->second.node, false);
  udev_device_unref(it->second.node);
  if (erase) nodes.erase(it);
}

void generic_file::add_dev(input_source* dev) {
  devices.push_back(dev->shared_from_this());
}

void generic_file::thread_loop() {
  struct epoll_event event;
  struct epoll_event events[1];
  memset(&event, 0, sizeof(event));
  while ((keep_looping)) {
    int n = epoll_wait(epfd, events, 1, -1);
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n < 0 && errno != EINTR) {
      perror("epoll wait:");
      break;
    }
    if (n == 0) continue;
    
    struct input_event ev;
    int file = events[0].data.u32;
    int ret = read(file, &ev, sizeof(ev));
    if (file == internal_pipe[0]) {
      continue; //Just a quick ping to ensure we aren't stuck in epoll_wait
    }
    if (ret == sizeof(ev)) {
      for (auto dev : devices) {
        write(((generic_device*)dev->plug_data)->pipe_write, &ev, sizeof(ev));
      }
    } else if (errno == ENODEV && keep_looping) {
      close(file);
      //TODO: possibly close out the stored node as well?
      //For now, rely on the generic manager telling us via udev events.
    }

  }
}

int generic_file::get_fd() {
  std::lock_guard<std::mutex> guard(lock);
  if (fds.size() == 0)
    return -1;
  return fds.front();
}


#ifndef GENERIC_H
#define GENERIC_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include "../device.h"
#include <sys/epoll.h>
#include <sys/stat.h>

class moltengamepad;

struct device_match {
  int vendor;
  int product;
  std::string name;
};
struct gen_source_event {
  int id;
  int split_id = 1;
  std::string name;
  std::string descr;
  enum entry_type type;
};

struct generic_driver_info {
  std::vector<device_match> matches;

  std::vector<gen_source_event> events;

  std::string name;
  std::string devname;
  bool grab_ioctl = false; //use ioctl EVIOCGRAB.
  bool grab_chmod = false; //Remove all permissions after we open device. Restore them on close.
  bool flatten = false;
  std::vector<std::pair<std::string,std::string>> aliases;
  int split = 1;
  std::vector<std::string> split_types;
};

int generic_config_loop(moltengamepad* mg, std::istream& in, std::string& path);


typedef std::pair<int, int> evcode;
typedef std::pair<int, input_absinfo> decodedevent;

class generic_device : public input_source {
public:
  char* nameptr = nullptr;
  const char* descr = "input device";
  int fd = -1;
  int pipe_read = -1;
  int pipe_write = -1;

  std::map<evcode, decodedevent> eventcodes;
  struct udev_device* node = nullptr;

  generic_device(std::vector<gen_source_event>& events, int fd, bool watch, slot_manager* slot_man, device_manager* manager, std::string type);
  ~generic_device();

  virtual int set_player(int player_num);
  virtual void list_events(cat_list& list);
  virtual void list_options(name_list& list);


  virtual void update_option(const char* opname, const char* value);

  virtual void process(void*);

  int get_pipe();

};

struct generic_node {
  std::string path;
  udev_device* node;
  int fd;
  mode_t orig_mode;
};


//Class to handle opening/closing/reading files.
//Because this driver handles splitting and flattening,
//it gets a bit more complicated.
//For example, if we want to EVIOCGRAB, 
//we need to have exactly one open fd for that node

//So here we are:
//a separate layer that can open the file and mux the events appropriately.
class generic_file {
public:
  int epfd = 0;
  std::thread* thread = nullptr;
  std::vector<generic_device*> devices;
  std::vector<int> fds;
  std::map<std::string, generic_node> nodes;
  bool grab_ioctl = false;
  bool grab_chmod = false;
  bool keep_looping = true;
  moltengamepad* mg;
  int internal_pipe[2];

  generic_file(struct udev_device* node, bool grab_ioctl, bool grab_chmod) {
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
    
    if (fds.empty()) throw - 1;

    thread = new std::thread(&generic_file::thread_loop, this);
  }


  ~generic_file() {
    keep_looping = false;
    for (auto node_it : nodes) {
      close_node(node_it.first, false); //TODO: Fix this repeated map look up...
    }
    if (thread) {
      int beep = 0;
      write(internal_pipe[1],&beep,sizeof(beep));
      thread->join();
      delete thread;
    }
    for (auto dev : devices) {
      mg->remove_device(dev);
    }
    close(epfd);
    close(internal_pipe[0]);
    close(internal_pipe[1]);

  }


  void open_node(struct udev_device* node) {
    std::string path(udev_device_get_devnode(node));
    if (nodes.find(path) == nodes.end()) {
      int fd = open(udev_device_get_devnode(node), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
      if (fd < 0) {
        perror("open subdevice:");
        return;
      }
      struct stat filestat;
      fstat(fd, &filestat);

      if (grab_ioctl) {
        ioctl(fd, EVIOCGRAB, 1);
      }

      if (grab_chmod) {
        //Remove all permissions. Other software will really ignore it.
        //Requires the device to be owned by the current user. (not merely have access)
        chmod(udev_device_get_devnode(node), 0);
      }

      struct epoll_event event;
      memset(&event, 0, sizeof(event));

      event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
      event.data.u32 = fd;
      int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
      if (ret < 0) perror("epoll add");

      fds.push_back(fd);
      nodes[path] = {path, udev_device_ref(node), fd, filestat.st_mode};
    }
  }

  void close_node(struct udev_device* node, bool erase) {
    const char* path = udev_device_get_devnode(node);
    if (!path) return;

    close_node(std::string(path), erase);
  }

  void close_node(const std::string& path, bool erase) {
    auto it = nodes.find(path);

    if (it == nodes.end()) return;

    close(it->second.fd);
    if (grab_chmod) chmod(path.c_str(), it->second.orig_mode);
    udev_device_unref(it->second.node);
    if (erase) nodes.erase(it);
  }

  void add_dev(generic_device* dev) {
    devices.push_back(dev);
  }

  void thread_loop() {
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
          write(dev->get_pipe(), &ev, sizeof(ev));
        }
      } else if (errno == ENODEV && keep_looping) {
        close(file);
        //TODO: possibly close out the stored node as well?
        //For now, rely on the generic manager telling us via udev events.
      }

    }
  }

};

class generic_manager : public device_manager {
public:
  generic_driver_info* descr = nullptr;
  int split = 1;
  bool flatten = false;


  generic_manager(moltengamepad* mg, generic_driver_info& descr);

  virtual ~generic_manager();

  virtual int accept_device(struct udev* udev, struct udev_device* dev);



  virtual void list_devs(name_list& list) {
    for (auto file : openfiles) {

      for (auto it = file->devices.begin(); it != file->devices.end(); ++it) {
        list.push_back({(*it)->name.c_str(), (*it)->descr, 0});
      }
    }
  }


protected:
  std::vector<generic_file*> openfiles;
  std::vector<std::vector<gen_source_event>> splitevents;
  int dev_counter = 0;
  std::string devname = "";
  int open_device(struct udev* udev, struct udev_device* dev);
  void create_inputs(generic_file* opened_file, int fd, bool watch);

};

#endif

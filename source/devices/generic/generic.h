#ifndef GENERIC_H
#define GENERIC_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include "../device.h"
#include <sys/epoll.h>

class moltengamepad;

struct device_match {
  int vendor;
  int product;
  std::string name;
};
struct gen_source_event {
  int id;
  std::string name;
  std::string descr;
  enum entry_type type;
};

struct generic_driver_info {
  std::vector<device_match> matches;
  
  std::vector<gen_source_event> events;
  
  std::string name;
  std::string devname;
  bool grab_exclusive = false;
  bool flatten = false;
  int split = 1;
};

int generic_config_loop(moltengamepad* mg, std::istream &in);


typedef std::pair<int,int> evcode;
typedef std::pair<int,int> slotevent;

class generic_device : public input_source {
public:
  char* nameptr = nullptr;
  const char* descr = "input device";
  int fd = -1;
  int pipe_read = -1;
  int pipe_write = -1;
  
  std::map<int,int> eventcodes;
  struct udev_device* node = nullptr;
  
  generic_device(std::vector<gen_source_event> &events, int fd);
  ~generic_device();
  
  virtual int set_player(int player_num);
  virtual void list_events(cat_list &list);
  virtual void list_options(name_list &list);
  
  
  virtual void update_option(const char* opname, const char* value);
  
  virtual enum entry_type entry_type(const char* name);
  
  virtual void process(void*);
  
  int get_pipe();
  
};


class generic_file {
public:
  int epfd = 0;
  std::thread* thread = nullptr;
  std::vector<generic_device*> devices;
  std::vector<int> fds;
  std::vector<struct udev_device*> nodes;
  bool grab = false;
  bool keep_looping = true;
  
  generic_file(struct udev_device* node, bool grab) {
    epfd = epoll_create(1);
    if (epfd < 1) perror("epoll create");
    open_node(node);
    
    thread = new std::thread(&generic_file::thread_loop,this);
  }
  
  generic_file() {
    
  }
  
  ~generic_file() {
    keep_looping = false;
    for (auto node : nodes) {
      if (node) udev_device_unref(node);
      node = nullptr;
    }
    for (auto dev : devices) {
      delete dev;
    }
    if(thread) {thread->join(); delete thread;}
    for (int fd : fds) close(fd);
  }
    
  
  void open_node(struct udev_device* node) {
    int fd = open(udev_device_get_devnode(node), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
      perror("open subdevice:");
    ioctl(fd, EVIOCGRAB, this);
  
    struct epoll_event event;
    memset(&event,0,sizeof(event));

    event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    event.data.u32 = fd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    if (ret< 0) perror("epoll add");
    
    fds.push_back(fd);
    nodes.push_back(udev_device_ref(node));
  }
  
  void add_dev(generic_device* dev) {
    devices.push_back(dev);
  }
  
  void thread_loop() {
    struct epoll_event event;
    struct epoll_event events[1];
    memset(&event,0,sizeof(event));
    while ((keep_looping)) {
      int n = epoll_wait(epfd, events, 1, 5);
      if (n < 0) {perror("epoll wait:");break;}
      if (n == 0) continue;
      struct input_event ev;
      int file = events[0].data.u32;
      int ret = read(file,&ev,sizeof(ev));
      if (ret > 0) {
        for (auto dev : devices) {
          write(dev->get_pipe(),&ev,sizeof(ev));
        }
      } else if (errno == ENODEV) {
        close(file);
        /*for (auto dev : devices) {
          delete dev;
        }
        keep_looping = false;*/
      }
      
    }
  }
  
};

class generic_manager : public device_manager {
public:
  generic_driver_info* descr = nullptr;
  int split = 1;
  bool flatten = false;
  
  
  generic_manager(slot_manager* slot_man, generic_driver_info &descr);

  ~generic_manager(); 
  
  virtual int accept_device(struct udev* udev, struct udev_device* dev);
  
  virtual void update_maps(const char* evname, event_translator* trans);
  
  virtual void update_option(const char* opname, const char* value);
  
  virtual input_source* find_device(const char* name);
  virtual enum entry_type entry_type(const char* name);
  
  
  virtual void list_devs(name_list &list) {
    for (auto file : openfiles) {
      
      for (auto it = file->devices.begin(); it != file->devices.end(); ++it) {
        list.push_back({(*it)->name,(*it)->descr,0});
      }
    }
  }
  
  
protected:
  std::vector<generic_file*> openfiles;
  int dev_counter = 0;
  std::string devname = "";
  int open_device(struct udev* udev, struct udev_device* dev);
  void create_inputs(generic_file* opened_file);
  
};

#endif
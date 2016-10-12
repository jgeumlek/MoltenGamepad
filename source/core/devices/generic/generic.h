#ifndef GENERIC_H
#define GENERIC_H

#include <vector>
#include <map>
#include <iostream>
#include <mutex>
#include <fstream>
#include <unistd.h>
#include <linux/input.h>
#include "../../../plugin/plugin.h"
#include "../../moltengamepad.h"
#include "../../plugin_loader.h"
#include <sys/epoll.h>

/*
 * The Generic driver functionality must be built in statically to moltengamepad.
 * (1) If no generic drivers are made, the impact of inclusion is minimal.
 * (2) The --config-path and --gendev-path options would overly-complicated to provide via plugin api
 * 
 * However, to avoid code redundancy, we still follow the plugin method of creating input_sources
 * and device_managers. Where reasonable, we still cheat and directly interact with moltengamepad objects.
 */

extern device_plugin genericdev;
extern manager_plugin genericman;
int init_generic_callbacks();

struct device_match {
  int vendor = -1;
  int product = -1;
  std::string name;
  std::string uniq;
  std::string phys;
  std::string driver;
  enum ev_match {EV_MATCH_IGNORED, EV_MATCH_SUBSET, EV_MATCH_EXACT, EV_MATCH_SUPERSET};
  ev_match events = EV_MATCH_IGNORED;
  int order = DEVICE_CLAIMED; //specifies the priority of this match.
};

int parse_hex(const std::string& text);

//Info on an event as read from the .cfg file
struct gen_source_event {
  int code; //e.g. BTN_SOUTH or ABS_X
  int split_id = 1;
  std::string name;
  std::string descr;
  enum entry_type type;
};

//Info on an event the input source needs. 
struct split_ev_info {
  int code; //The event count read from raw device, e.g. BTN_SOUTH
  entry_type type; //DEV_KEY, DEV_AXIS, etc.
  int id;       //The id of this event within the registered event list (MoltenGamepad assigned)
};

struct generic_driver_info {
  std::vector<device_match> matches;

  std::vector<gen_source_event> events;

  std::string name;
  std::string devname;
  bool grab_ioctl = false; //use ioctl EVIOCGRAB.
  bool grab_chmod = false; //Remove all permissions after we open device. Restore them on close.
  bool flatten = false;
  bool subscribe_to_gamepad = false;
  bool rumble = false;
  std::vector<std::pair<std::string,std::string>> aliases;
  int split = 1;
  std::vector<std::string> split_types;
};

int generic_config_loop(moltengamepad* mg, std::istream& in, std::string& path);
int add_generic_manager(moltengamepad* mg, generic_driver_info& info);

struct generic_node {
  std::string path;
  udev_device* node;
  int fd;
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
  std::vector<std::shared_ptr<input_source>> devices;
  std::vector<int> fds;
  std::map<std::string, generic_node> nodes;
  std::string uniq;
  std::string phys;
  std::mutex lock;
  bool grab_ioctl = false;
  bool grab_chmod = false;
  bool keep_looping = true;
  bool rumble = false;
  moltengamepad* mg;

  int internal_pipe[2];

  generic_file(moltengamepad* mg, struct udev_device* node, bool grab_ioctl, bool grab_chmod, bool rumble);

  ~generic_file();

  void open_node(struct udev_device* node);
  void close_node(struct udev_device* node, bool erase);
  void close_node(const std::string& path, bool erase);
  void add_dev(input_source* dev);
  void thread_loop();
  int get_fd();

};

typedef std::pair<int, int> evcode;
typedef std::pair<int, input_absinfo> decodedevent;

class generic_device {
public:
  int pipe_read = -1;
  int pipe_write = -1;
  int total_events;
  event_state* eventstates = nullptr;
  bool rumble = false;
  const std::string type;

  std::map<evcode, decodedevent> eventcodes;
  struct udev_device* node = nullptr;

  generic_device(std::vector<split_ev_info>& split_events, int total_events, generic_file* file, const std::string& type, bool rumble);
  ~generic_device();

  int init(input_source* ref);

  void process(void*);

  int get_pipe();
  input_source* ref = nullptr;
  generic_file* file = nullptr;
  static device_methods methods;

  int upload_ff(ff_effect* effect);
  int erase_ff(int id);
  int play_ff(int id, int repetitions);

};

class generic_manager {
public:
  generic_driver_info* descr = nullptr;
  int split = 1;
  bool flatten = false;
  device_manager* ref;
  static manager_methods methods;
  moltengamepad* mg;
  int event_count = 0;

  generic_manager(moltengamepad* mg, generic_driver_info& descr);
  manager_plugin get_plugin();

  virtual ~generic_manager();

  int accept_device(struct udev* udev, struct udev_device* dev);
  int accept_deferred_device(struct udev* udev, struct udev_device* dev);
  int init(device_manager* ref);

protected:
  std::mutex devlistlock;
  std::vector<generic_file*> openfiles;
  std::vector<std::vector<split_ev_info>> splitevents;
  int dev_counter = 0;
  std::string devname = "";
  int open_device(struct udev* udev, struct udev_device* dev);
  void create_inputs(generic_file* opened_file);

};

#endif

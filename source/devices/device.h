#ifndef DEVICE_H
#define DEVICE_H

#include <libudev.h>
#include <vector>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include "../event_change.h"
#include "../moltengamepad.h"
#include "../profile.h"
#include "../messages.h"

#define ABS_RANGE 32000

class event_translator;
class advanced_event_translator;
class moltengamepad;


class slot_manager;

class device_manager;


typedef std::vector<struct name_descr> name_list;
struct category {
  const char* name;
  name_list entries;
};
typedef std::vector<category> cat_list;

struct name_descr {
  const char* name;
  const char* descr;
  int data;
};

struct source_event {
  int id;
  const char* name;
  const char* descr;
  enum entry_type type;
  long long value;
  event_translator* trans;
  std::vector<advanced_event_translator*> attached;
};

struct source_option {
  std::string name;
  std::string descr;
  std::string value;
};

struct adv_entry {
  std::vector<std::string>* fields;
  advanced_event_translator* trans;
};

class input_source {
public:
  enum devtype { GAMEPAD, KEYBOARD, SPECIAL, UNKNOWN};
  input_source(slot_manager* slot_man, device_manager* manager, devtype type);
  virtual ~input_source();
  std::string name = "unnamed";
  virtual int set_player(int player_num) {
  }
  virtual void list_events(cat_list& list) {
  }
  void list_options(std::vector<source_option>& list);
  virtual void set_slot(output_slot* outdev) {
    this->out_dev = outdev;
  }

  void update_map(const char* evname, event_translator* trans);
  void update_option(const char* opname, const char* value);
  void update_advanced(const std::vector<std::string>& evnames, advanced_event_translator* trans);

  virtual enum entry_type entry_type(const char* name) {
  }

  void start_thread();
  void end_thread();

  const std::vector<source_event>& get_events() {
    return events;
  };

  void add_listener(int id, advanced_event_translator* trans);
  void remove_listener(int id, advanced_event_translator* trans);
  void force_value(int id, long long value);
  void send_value(int id, long long value);
  std::string get_alias(std::string event_name);
  const devtype getType() {return type;};
  static std::string type_name(devtype type) {
    if (type == GAMEPAD)
      return "gamepad";
    if (type == KEYBOARD)
      return "keyboard";
    if (type == SPECIAL)
      return "special";
    return "unknown";
  }
  std::shared_ptr<profile> get_profile() { return devprofile; };

  output_slot* out_dev = nullptr;
protected:
  slot_manager* slot_man;
  int epfd = 0;
  int priv_pipe = 0;
  int internalpipe = 0;
  std::vector<source_event> events;
  std::map<std::string, source_option> options;
  std::map<std::string, adv_entry> adv_trans;
  std::thread* thread = nullptr;
  volatile bool keep_looping = true;
  device_manager* manager;



  void register_event(source_event ev);
  void register_option(source_option ev);
  void watch_file(int fd, void* tag);
  void set_trans(int id, event_translator* trans);


  void thread_loop();

  virtual void process(void* tag) {};
  virtual int process_option(const char* opname, const char* value) {
    return 0;
  };
  devtype type = UNKNOWN;
  std::shared_ptr<profile> devprofile = std::make_shared<profile>();
};

class device_manager {
public:
  moltengamepad* mg;
  virtual int accept_device(struct udev* udev, struct udev_device* dev) {
    return -1;
  }
  virtual void list_devs(name_list& list) {
  };

  device_manager(moltengamepad* mg, std::string name) : mg(mg), name(name), log(name) {
    mapprofile->name = name;
    log.add_listener(1);
  }

  virtual ~device_manager() {
  };

  std::string name;
  simple_messenger log;
  std::shared_ptr<profile> mapprofile = std::make_shared<profile>();

};

#endif

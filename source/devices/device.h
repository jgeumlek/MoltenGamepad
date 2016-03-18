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

#define ABS_RANGE 32000

class event_translator;
class advanced_event_translator;
class moltengamepad;


class slot_manager;


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
  input_source(slot_manager* slot_man, devtype type);
  virtual ~input_source();
  const char* name = "unnamed";
  virtual int set_player(int player_num) {
  }
  virtual void list_events(cat_list& list) {
  }
  void list_options(std::vector<source_option>& list);
  virtual void set_slot(output_slot* outdev) {
    this->out_dev = outdev;
    if (outdev)  {
      std::cout << name << " assigned to slot " << outdev->name << std::endl;
    } else {
      std::cout << name << " not assigned to a slot" << std::endl;
    }
  }

  void update_map(const char* evname, event_translator* trans);
  void update_chord(const char* key1, const char* key2, event_translator* trans);
  void update_option(const char* opname, const char* value);
  void update_advanced(const std::vector<std::string>& evnames, advanced_event_translator* trans);

  virtual enum entry_type entry_type(const char* name) {
  }

  void start_thread();
  void end_thread();

  void load_profile(profile* profile);
  void export_profile(profile* profile);

  const std::vector<source_event>& get_events() {
    return events;
  };

  void add_listener(int id, advanced_event_translator* trans);
  void remove_listener(int id, advanced_event_translator* trans);
  void force_value(int id, long long value);
  void send_value(int id, long long value);
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

  output_slot* out_dev = nullptr;
protected:
  slot_manager* slot_man;
  int epfd = 0;
  int priv_pipe = 0;
  int internalpipe = 0;
  std::vector<source_event> events;
  std::map<std::string, source_option> options;
  std::unordered_map<std::string, std::string> aliases;
  std::map<std::string, adv_entry> adv_trans;
  std::thread* thread = nullptr;
  volatile bool keep_looping = true;



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
};

class device_manager {
public:
  moltengamepad* mg;
  virtual int accept_device(struct udev* udev, struct udev_device* dev) {
    return -1;
  }
  virtual void list_devs(name_list& list) {
  };

  device_manager(moltengamepad* mg) : mg(mg) {
  }

  virtual ~device_manager() {
  };



  virtual void update_maps(const char* evname, event_translator* trans) {
  }


  virtual void update_options(const char* opname, const char* value) {
  }

  virtual void update_advanceds(const std::vector<std::string>& names, advanced_event_translator* trans) {
  }

  virtual input_source* find_device(const char* name) {
  }

  virtual enum entry_type entry_type(const char* name) {
  }

  const char* name;
  profile mapprofile;

};

#endif

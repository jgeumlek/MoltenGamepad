#ifndef DEVICE_H
#define DEVICE_H

#include <libudev.h>
#include <vector>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <time.h>
#include "../event_translators/event_change.h"
#include "../moltengamepad.h"
#include "../profile.h"
#include "../messages.h"



class event_translator;
class advanced_event_translator;
class moltengamepad;


class slot_manager;

class device_manager;


struct source_event {
  int id;
  const char* name;
  const char* descr;
  enum entry_type type;
  int64_t value;
  event_translator* trans;
  std::vector<advanced_event_translator*> attached;
};

struct adv_entry {
  std::vector<std::string>* fields;
  advanced_event_translator* trans;
};

//Struct used internally, not designed for public consumption.
struct input_internal_msg {
  enum input_msg_type { IN_TRANS_MSG, IN_ADV_TRANS_MSG, IN_EVENT_MSG } type;
  int id;
  event_translator* trans;
  adv_entry adv;
  int64_t value;
  bool skip_adv_trans;
};

class input_source : public std::enable_shared_from_this<input_source> {
public:
  input_source(slot_manager* slot_man, device_manager* manager, std::string type);
  virtual ~input_source();
  
  
  virtual int set_player(int player_num) {
  }
  
  void list_options(std::vector<option_info>& list) const;
  
  virtual void set_slot(output_slot* outdev) {
    this->out_dev = outdev;
  }

  void update_map(const char* evname, event_translator* trans);
  void update_option(const char* opname, const char* value);
  void update_advanced(const std::vector<std::string>& evnames, advanced_event_translator* trans);

  void start_thread();
  void end_thread();

  const std::vector<source_event>& get_events() const {
    return events;
  };
  
  void inject_event(int id, int64_t value, bool skip_adv_trans);

  void add_listener(int id, advanced_event_translator* trans);
  void remove_listener(int id, advanced_event_translator* trans);
  

  std::string get_name() const { return name; };
  void set_name(std::string name) { this->name = name; };
  virtual std::string get_description() const { return descr; };
  virtual std::string get_type() const { return device_type; };
  std::string get_alias(std::string event_name) const;
  std::shared_ptr<profile> get_profile() const { return devprofile; };

  output_slot* out_dev = nullptr;
protected:
  slot_manager* slot_man;
  int epfd = 0;
  int priv_pipe = 0;
  int internalpipe = 0;
  std::string name = "unnamed";
  std::string descr = "No description available";
  std::string device_type = "gamepad";
  std::vector<source_event> events;
  std::map<std::string, option_info> options;
  std::map<std::string, adv_entry> adv_trans;
  std::shared_ptr<profile> devprofile = std::make_shared<profile>();
  std::thread* thread = nullptr;
  volatile bool keep_looping = true;
  device_manager* manager;
  
  std::vector<const event_translator*> recurring_events;
  bool do_recurring_events = false;
  timespec last_recurring_update;


  void register_event(source_event ev);
  void register_option(option_info ev);
  void watch_file(int fd, void* tag);
  void set_trans(int id, event_translator* trans);
  void force_value(int id, int64_t value);
  void send_value(int id, int64_t value);

  void thread_loop();

  virtual void process(void* tag) {};
  virtual int process_option(const char* opname, const char* value) {
    return 0;
  };
  
  
  void handle_internal_message(input_internal_msg &msg);

  void add_recurring_event(const event_translator* trans);
  void remove_recurring_event(const event_translator* trans);

  void process_recurring_events();
  int64_t ms_since_last_recurring_update();
};

class device_manager {
public:
  moltengamepad* mg;
  virtual int accept_device(struct udev* udev, struct udev_device* dev) {
    return -1;
  }

  device_manager(moltengamepad* mg, std::string name) : mg(mg), name(name), log(name) {
    mapprofile->name = name;
    log.add_listener(1);
  }

  virtual ~device_manager() {
  };
  
  virtual void for_each_dev(std::function<void (const input_source*)> func) {};

  std::string name;
  simple_messenger log;
  std::shared_ptr<profile> mapprofile = std::make_shared<profile>();

  

};

#endif

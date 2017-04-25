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
#include "../protocols/message_stream.h"
#include "../../plugin/plugin.h"



class event_translator;
class group_translator;
class moltengamepad;


class device_manager;

struct group_listener {
  group_translator* trans;
  int index;
  int8_t direction;
};

struct event_mapping {
  event_translator* trans;
  int8_t direction;
  std::vector<group_listener> attached;
};

struct recurring_info {
  const event_translator* trans;
  int id;
};

struct group_entry {
  std::string* key;
  std::vector<int>* fields;
  std::vector<int8_t>* directions;
  group_translator* trans;
};

//Struct used internally, not designed for public consumption.
struct input_internal_msg;

class input_source : public std::enable_shared_from_this<input_source> {
public:
  input_source(device_manager* manager, device_plugin plugin, void* plug_data);
  ~input_source();
  
  
  int set_player(int player_num) {
  }
  
  void list_options(std::vector<option_info>& list) const;
  
  void set_slot(output_slot* outdev);
  output_slot* get_slot();

  void update_map(const char* evname, int8_t direction, event_translator* trans);
  void update_option(const char* opname, const MGField field);
  void remove_option(std::string option_name);
  void update_group(const std::vector<std::string>& evnames, std::vector<int8_t> directions, group_translator* trans);

  void start_thread();
  void end_thread();

  const std::vector<source_event>& get_events() const {
    return events;
  };
  
  void inject_event(int id, int64_t value, bool skip_group_trans);

  void add_listener(int id, int8_t direction, group_translator* trans, int trans_index);
  void remove_listener(int id, group_translator* trans);

  int upload_ff(ff_effect effect);
  int erase_ff(int id);
  int play_ff(int id, int repetitions);

  int set_plugin_recurring(bool wants_recurring);
  

  std::string get_name() const { return name; };
  void set_name(std::string name) { this->name = name; };
  std::string get_manager_name() const;
  std::string get_description() const;
  std::string get_uniq() const { return uniq; };
  std::string get_phys() const { return phys; };
  std::string get_type() const;
  std::string get_alias(std::string event_name) const;
  std::shared_ptr<profile> get_profile() const { return devprofile; };


  void* const plug_data = nullptr;
  friend void init_plugin_api();
  friend moltengamepad;
protected:
  int epfd = 0;
  int priv_pipe = 0;
  int internalpipe = 0;
  std::string name = "unnamed";
  std::string descr = "No description available";
  std::string device_type = "gamepad";
  std::string uniq = ""; //A unique string for this input_source, if available
  std::string phys = ""; //A string describing how/where this device is connected, if available.
  std::vector<source_event> events;
  std::vector<event_mapping> ev_map;
  std::map<std::string, option_info> options;
  std::map<std::string, group_entry> group_trans;
  std::shared_ptr<profile> devprofile = std::make_shared<profile>();
  std::thread* thread = nullptr;
  volatile bool keep_looping = true;
  device_manager* manager;
  device_plugin plugin;
  std::mutex opt_lock;
  std::mutex slot_lock;
  output_slot* out_dev = nullptr;
  output_slot* assigned_slot = nullptr; //might differ from the above due to thread synchro.
  int ff_ids[1]; //Since the physical device might hand us different ids.

  std::vector<recurring_info> recurring_events;
  std::vector<const group_translator*> group_recurring_events;
  bool plugin_recurring = false;
  bool do_recurring_events = false;
  timespec last_recurring_update;


  void register_event(event_decl ev);
  void toggle_event(int id, event_state state);
  void register_option(option_info opt);
  void watch_file(int fd, void* tag);
  void set_trans(int id, int8_t direction, event_translator* trans);
  void force_value(int id, int64_t value);
  void send_value(int id, int64_t value);
  void send_syn_report();

  void thread_loop();

  void process(void* tag);
  int process_option(const char* opname, const MGField field);

  void print(std::string message);
  
  void handle_internal_message(input_internal_msg& msg);

  void add_recurring_event(const event_translator* trans, int id);
  void remove_recurring_event(const event_translator* trans);
  void add_group_recurring_event(const group_translator* trans);
  void remove_group_recurring_event(const group_translator* trans);

  void process_recurring_events();
  int64_t ms_since_last_recurring_update();
};

class device_manager {
public:
  moltengamepad* mg;
  void* const plug_data = nullptr;
  manager_plugin plugin;
  
  device_manager(moltengamepad* mg, manager_plugin plugin, void* plug_data);

  ~device_manager();
  
  int register_event(event_decl ev);
  int register_device_option(option_decl opt);
  int register_manager_option(option_decl opt);
  int register_alias(const char* external, const char* local);
  int register_event_group(event_group_decl decl);
  input_source* add_device(device_plugin dev, void* dev_plug_data);

  int accept_device(struct udev* udev, struct udev_device* dev);
  int accept_deferred_device(struct udev* udev, struct udev_device* dev);
  int process_manager_option(const std::string& name, MGField value);

  const std::vector<event_decl>& get_events() const {
    return events;
  };

  void for_all_devices(std::function<void (const input_source*)> func);

  std::string name;
  message_stream log;
  std::shared_ptr<profile> mapprofile = std::make_shared<profile>();

  std::vector<event_decl> events;

  options opts;
  bool has_options = false;

};

#endif

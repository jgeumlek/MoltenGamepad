#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include "options.h"

typedef std::pair<std::string, std::string> str_pair;

class event_translator;
class advanced_event_translator;
class input_source;

struct adv_map {
  std::vector<std::string> fields;
  advanced_event_translator* trans;
};

struct trans_map {
  event_translator* trans;
  entry_type type;
};


class profile : public std::enable_shared_from_this<profile> {
public:
  std::string name;
  std::unordered_map<std::string, trans_map> mapping;
  options opts;
  std::unordered_map<std::string, std::string> aliases;
  std::map<std::string, adv_map> adv_trans;
  mutable std::mutex lock;


  entry_type get_entry_type(std::string in_event_name);

  event_translator* copy_mapping(std::string in_event_name);

  void set_mapping(std::string in_event_name, event_translator* mapper, entry_type type, bool add_new);

  void set_advanced(std::vector<std::string> names, advanced_event_translator* trans);

  void remove_event(std::string event_name);
  void register_option(const option_info opt);
  void register_option(const option_decl opt);
  int set_option(std::string opname, std::string value);
  void remove_option(std::string option_name);

  void set_alias(std::string external, std::string local);
  std::string get_alias(std::string name);

  option_info get_option(std::string opname);
  void list_options(std::vector<option_info>& list) const;
  
  void subscribe_to(profile* parent);
  void remember_subscription(profile* parent); //similar to above, but we don't try to add ourselves to the parent.
  void add_listener(std::shared_ptr<profile> listener);
  void remove_listener(std::shared_ptr<profile> listener);
  void remove_listener(profile* listener);
  void add_device(std::shared_ptr<input_source> dev);
  void remove_device(input_source* dev);
  void copy_into(std::shared_ptr<profile> target, bool add_subscription,  bool add_new);
  std::shared_ptr<profile> get_shared_ptr();

  ~profile();
  profile();

  //Load up default gamepad event mappings onto this profile.
  void gamepad_defaults();

private:
  static profile default_gamepad_profile;
  static void build_default_gamepad_profile();
  std::vector<std::weak_ptr<profile>> subscriptions;
  std::vector<std::weak_ptr<profile>> subscribers;
  std::vector<std::weak_ptr<input_source>> devices;

  trans_map get_mapping(std::string in_event_name);

};

#endif

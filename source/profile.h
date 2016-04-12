#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>
#include <memory>

enum entry_type {DEV_OPTION, DEV_KEY, DEV_AXIS, DEV_REL, NO_ENTRY} ;

typedef std::pair<std::string, std::string> str_pair;

class event_translator;
class advanced_event_translator;

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
  std::unordered_map<std::string, std::string> options;
  std::unordered_map<std::string, std::string> aliases;
  std::map<std::string, adv_map> adv_trans;
  std::mutex lock;

  trans_map get_mapping(std::string in_event_name);

  event_translator* copy_mapping(std::string in_event_name);

  void set_mapping(std::string in_event_name, event_translator* mapper, entry_type type);

  void set_advanced(const std::vector<std::string>& names, advanced_event_translator* trans);

  void set_option(std::string opname, std::string value);

  void set_alias(std::string external, std::string local);
  std::string get_alias(std::string name);

  std::string get_option(std::string opname);
  
  void subscribe_to(profile* parent);
  void add_listener(std::shared_ptr<profile> listener);
  void remove_listener(std::shared_ptr<profile> listener);
  void remove_listener(profile* listener);
  std::shared_ptr<profile> get_shared_ptr();

  ~profile();
  profile();

  //Load up default gamepad event mappings onto this profile.
  void gamepad_defaults();

private:
  static profile default_gamepad_profile;
  static void build_default_gamepad_profile();
  std::vector<std::weak_ptr<profile> > subscriptions;
  std::vector<std::shared_ptr<profile> > subscribers;

};

#endif

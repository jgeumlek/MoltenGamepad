#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>

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

class profile {
public:
  const char* name;
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

  ~profile();

  //Load up default gamepad event mappings onto this profile.
  void gamepad_defaults();

private:
  static profile default_gamepad_profile;
  static void build_default_gamepad_profile();

};

#endif

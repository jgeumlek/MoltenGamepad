#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

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
  std::map<std::string, adv_map> adv_trans;

  trans_map get_mapping(std::string in_event_name);

  event_translator* copy_mapping(std::string in_event_name);

  void set_mapping(std::string in_event_name, event_translator* mapper, entry_type type);

  void set_advanced(const std::vector<std::string>& names, advanced_event_translator* trans);

  void set_option(std::string opname, std::string value);

  std::string get_option(std::string opname);

  ~profile();

};

#endif
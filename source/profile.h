#ifndef PROFILE_H
#define PROFILE_H
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

typedef std::pair<std::string,std::string> str_pair;

class event_translator;
class advanced_event_translator;

struct adv_map {
  std::vector<std::string> fields;
  advanced_event_translator* trans;
};

class profile {
public:
  const char* name;
  std::unordered_map<std::string, event_translator*> mapping;
  std::unordered_map<std::string, std::string> options;
  std::map<str_pair, event_translator*> chords;
  std::map<std::string,adv_map> adv_trans;
  
  event_translator* get_mapping(std::string in_event_name);
  
  event_translator* copy_mapping(std::string in_event_name);
  
  void set_mapping(std::string in_event_name, event_translator* mapper);
  
  void set_chord(std::string ev1, std::string ev2, event_translator* mapper);
  
  void set_advanced(const std::vector<std::string>& names, advanced_event_translator* trans);
  
  void set_option(std::string opname, std::string value);
  
  std::string get_option(std::string opname);
    
  
  ~profile();
    
};

#endif
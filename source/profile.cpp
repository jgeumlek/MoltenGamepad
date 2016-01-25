#include "profile.h"
#include "event_change.h"

  
trans_map profile::get_mapping(std::string in_event_name) {
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return {nullptr,NO_ENTRY};
  return (it->second);
}

event_translator* profile::copy_mapping(std::string in_event_name) {
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return new event_translator();
  return (it->second.trans->clone());
}

void profile::set_mapping(std::string in_event_name, event_translator* mapper, entry_type type) {
  trans_map oldmap = get_mapping(in_event_name);
  if (oldmap.trans) delete oldmap.trans;
  mapping.erase(in_event_name);
  mapping[in_event_name] = {mapper,type};
}



void profile::set_option(std::string opname, std::string value) {
  options.erase(opname);
  options[opname] = value;
}

void profile::set_advanced(const std::vector<std::string>& names, advanced_event_translator* trans) {
  if (names.empty()) return;
  auto it = names.begin();
  //TODO: Optimize key creation
  std::string key = *it;
  it++;
  for (; it != names.end(); it++) {
    key += "," + (*it);
  }
  
  auto stored = adv_trans.find(key);
  if (stored != adv_trans.end()) {
    delete stored->second.trans;
    adv_trans.erase(stored);
  }
  
  if (trans) {
    adv_map entry;
    entry.fields = names;
    entry.trans = trans;
    adv_trans[key] = entry;
  }
  
}

std::string profile::get_option(std::string opname) {
  auto it = options.find(opname);
  if (it == options.end()) return "";
  return it->second;
}
  

profile::~profile() {
  for (auto it = mapping.begin(); it != mapping.end(); it++) {
    if (it->second.trans) delete it->second.trans;
  }
  
  for (auto e : adv_trans) {
    if (e.second.trans) delete e.second.trans;
  }

  mapping.clear();
}
  



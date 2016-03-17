#include "profile.h"
#include "event_change.h"


trans_map profile::get_mapping(std::string in_event_name) {
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return {nullptr, NO_ENTRY};
  return (it->second);
}

event_translator* profile::copy_mapping(std::string in_event_name) {
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end())
    in_event_name = alias->second;
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return new event_translator();
  return (it->second.trans->clone());
}

void profile::set_mapping(std::string in_event_name, event_translator* mapper, entry_type type) {
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end())
    in_event_name = alias->second;
  trans_map oldmap = get_mapping(in_event_name);
  if (oldmap.trans) delete oldmap.trans;
  mapping.erase(in_event_name);
  mapping[in_event_name] = {mapper, type};
}



void profile::set_option(std::string opname, std::string value) {
  options.erase(opname);
  options[opname] = value;
}

void profile::set_advanced(const std::vector<std::string>& names, advanced_event_translator* trans) {
  if (names.empty()) return;
  auto it = names.begin();
  //this key creation is not ideal.
  std::string key = *it;
  it++;
  auto alias = aliases.find(key);
  if (alias != aliases.end())
    key = alias->second;
  for (; it != names.end(); it++) {
    alias = aliases.find(*it);
    key += "," + ((alias == aliases.end()) ? (*it) : alias->second);
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

void profile::set_alias(std::string external, std::string local) {
  if(local.empty()) {
    aliases.erase(external);
    return;
  }
  aliases[external] = local;
}

std::string profile::get_alias(std::string name) {
  auto alias = aliases.find(name);
  if (alias != aliases.end())
    return alias->second;
  return "";
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




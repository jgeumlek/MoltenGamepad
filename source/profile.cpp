#include "profile.h"
#include "event_change.h"
#include "devices/device.h"
profile::profile() {
}

profile::~profile() {
  for (auto prof : subscriptions) {
    if (auto profptr = prof.lock())
      profptr->remove_listener(this);
  }

  for (auto it = mapping.begin(); it != mapping.end(); it++) {
    if (it->second.trans) delete it->second.trans;
  }

  for (auto e : adv_trans) {
    if (e.second.trans) delete e.second.trans;
  }

  mapping.clear();
}

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
  lock.lock();
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end())
    in_event_name = alias->second;
  trans_map oldmap = get_mapping(in_event_name);
  if (oldmap.trans) delete oldmap.trans;
  mapping.erase(in_event_name);
  mapping[in_event_name] = {mapper, type};
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_mapping(in_event_name,mapper->clone(),type);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_map(in_event_name.c_str(),mapper);
  }
  lock.unlock();
}



void profile::set_option(std::string opname, std::string value) {
  lock.lock();
  options.erase(opname);
  options[opname] = value;
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_option(opname, value);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_option(opname.c_str(),value.c_str());
  }
  lock.unlock();
}

void profile::set_advanced(const std::vector<std::string>& names, advanced_event_translator* trans) {
  if (names.empty()) return;
  lock.lock();
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
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_advanced(names,trans->clone());
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_advanced(names,trans);
  }
  lock.unlock();
}

void profile::set_alias(std::string external, std::string local) {
  lock.lock();
  if (local.empty()) {
    aliases.erase(external);
  } else {
  aliases[external] = local;
  }
  lock.unlock();
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

void profile::subscribe_to(profile* parent) {
  if (parent == this) return;
  lock.lock();
  subscriptions.push_back(parent->get_shared_ptr());
  parent->add_listener(get_shared_ptr());
  lock.unlock();
}
void profile::remember_subscription(profile* parent) {
  if (parent == this) return;
  lock.lock();
  subscriptions.push_back(parent->get_shared_ptr());
  lock.unlock();
}

std::shared_ptr<profile> profile::get_shared_ptr() {
  return shared_from_this();
}

void profile::add_listener(std::shared_ptr<profile> listener) {
  lock.lock();
  subscribers.push_back(listener);
  lock.unlock();
}

void profile::remove_listener(std::shared_ptr<profile> listener) {
  remove_listener(listener.get());
}

void profile::remove_listener(profile* listener) {
  lock.lock();
  for (auto it = subscribers.begin(); it != subscribers.end(); it++) {
    std::shared_ptr<profile> ptr = it->lock();
    if (ptr.get() == listener) {
      subscribers.erase(it);
      break;
    }
  }
  lock.unlock();
}

void profile::add_device(std::shared_ptr<input_source> device) {
  lock.lock();
  devices.push_back(device);
  lock.unlock();
}

void profile::remove_device(input_source* dev) {
  lock.lock();
  for (auto it = devices.begin(); it != devices.end(); it++) {
    std::shared_ptr<input_source> ptr = it->lock();
    if (ptr.get() == dev) {
      devices.erase(it);
      break;
    }
  }
  lock.unlock();
}

void profile::copy_into(std::shared_ptr<profile> target, bool add_subscription) {
  lock.lock();
  for (auto entry : mapping)
    target->set_mapping(entry.first, entry.second.trans->clone(), entry.second.type);
  for (auto entry : adv_trans)
    target->set_advanced(entry.second.fields, entry.second.trans->clone());
  for (auto entry : options)
    target->set_option(entry.first,entry.second);
  for (auto entry : aliases)
    target->set_alias(entry.first,entry.second);
  if (add_subscription) {
    subscribers.push_back(target);
  }
    target->remember_subscription(this);
  lock.unlock();
}

void profile::build_default_gamepad_profile() {
  default_gamepad_profile.lock.lock();
  if (default_gamepad_profile.mapping.empty()) {
    auto map = &default_gamepad_profile.mapping;
    (*map)["primary"] =    {new btn2btn(BTN_SOUTH), DEV_KEY};
    (*map)["secondary"] =    {new btn2btn(BTN_EAST), DEV_KEY};
    (*map)["third"] =    {new btn2btn(BTN_WEST), DEV_KEY};
    (*map)["fourth"] =    {new btn2btn(BTN_NORTH), DEV_KEY};
    (*map)["left"] = {new btn2btn(BTN_DPAD_LEFT), DEV_KEY};
    (*map)["right"] = {new btn2btn(BTN_DPAD_RIGHT), DEV_KEY};
    (*map)["up"] =   {new btn2btn(BTN_DPAD_UP), DEV_KEY};
    (*map)["down"] = {new btn2btn(BTN_DPAD_DOWN), DEV_KEY};
    (*map)["mode"] = {new btn2btn(BTN_MODE), DEV_KEY};
    (*map)["start"] = {new btn2btn(BTN_START), DEV_KEY};
    (*map)["select"] = {new btn2btn(BTN_SELECT), DEV_KEY};
    (*map)["tl"] =    {new btn2btn(BTN_TL), DEV_KEY};
    (*map)["tr"] =    {new btn2btn(BTN_TR), DEV_KEY};
    (*map)["tl2"] =   {new btn2btn(BTN_TL2), DEV_KEY};
    (*map)["tr2"] =   {new btn2btn(BTN_TR2), DEV_KEY};
    (*map)["thumbl"] =   {new btn2btn(BTN_THUMBL), DEV_KEY};
    (*map)["thumbr"] =   {new btn2btn(BTN_THUMBR), DEV_KEY};

    (*map)["left_x"] = {new axis2axis(ABS_X, 1), DEV_AXIS};
    (*map)["left_y"] = {new axis2axis(ABS_Y, 1), DEV_AXIS};
    (*map)["right_x"] = {new axis2axis(ABS_RX, 1), DEV_AXIS};
    (*map)["right_y"] = {new axis2axis(ABS_RY, 1), DEV_AXIS};
    (*map)["tr2_axis"] = {new axis2axis(ABS_RZ, 1), DEV_AXIS};
    (*map)["tl2_axis"] = {new axis2axis(ABS_Z, 1), DEV_AXIS};

    //For devices with the dpad as a hat.
    (*map)["updown"] =   {new axis2btns(BTN_DPAD_UP,BTN_DPAD_DOWN), DEV_AXIS};
    (*map)["leftright"] =   {new axis2btns(BTN_DPAD_LEFT,BTN_DPAD_RIGHT), DEV_AXIS};
  }
  default_gamepad_profile.lock.unlock();
}

profile profile::default_gamepad_profile;

void profile::gamepad_defaults() {
  lock.lock();
  if (this != &default_gamepad_profile && default_gamepad_profile.mapping.empty())
    build_default_gamepad_profile();
  for (auto entry : default_gamepad_profile.mapping) {
    std::string alias = get_alias(entry.first);
    alias = alias.empty() ? entry.first : alias;
    mapping[alias] = {entry.second.trans->clone(), entry.second.type};
  }
  lock.unlock();
}


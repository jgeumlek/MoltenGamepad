#include "profile.h"
#include "event_translators/event_change.h"
#include "event_translators/translators.h"
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

//This is private, called only while locked.
trans_map profile::get_mapping(std::string in_event_name) {
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return {nullptr, NO_ENTRY};
  return (it->second);
}

entry_type profile::get_entry_type(std::string in_event_name) {
  std::lock_guard<std::mutex> guard(lock);
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end())
    in_event_name = alias->second;
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return NO_ENTRY;
  return (it->second.type);
}

event_translator* profile::copy_mapping(std::string in_event_name) {
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end())
    in_event_name = alias->second;
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return new event_translator();
  return (it->second.trans->clone());
}

void profile::set_mapping(std::string in_event_name, event_translator* mapper, entry_type type, bool add_new) {
  std::lock_guard<std::mutex> guard(lock);
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end())
    in_event_name = alias->second;
  trans_map oldmap = get_mapping(in_event_name);

  if (!add_new && oldmap.type == NO_ENTRY) {
    delete mapper;
    return; //We don't recognize this event, and we don't want to!
  }

  if (oldmap.trans) delete oldmap.trans;
  mapping.erase(in_event_name);
  mapping[in_event_name] = {mapper, type};
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_mapping(in_event_name,mapper->clone(), type, add_new);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_map(in_event_name.c_str(),mapper);
  }
}

void profile::remove_event(std::string event_name) {
  std::lock_guard<std::mutex> guard(lock);
  auto alias = aliases.find(event_name);
  if (alias != aliases.end())
    event_name = alias->second;
  trans_map oldmap = get_mapping(event_name);

  if (oldmap.type == NO_ENTRY) {
    return; //Nothing to do?
  }

  if (oldmap.trans) delete oldmap.trans;
  mapping.erase(event_name);

  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->remove_event(event_name);
  }
}

//The opt class can register from either an option_info or option_decl.
//Just go ahead and expose both...
void profile::register_option(const option_info opt) {
  std::lock_guard<std::mutex> guard(lock);
  opts.register_option(opt);

  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->register_option(opt);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_option(opt.name.c_str(),opt.value);
  }
}

void profile::register_option(const option_decl opt) {
  std::lock_guard<std::mutex> guard(lock);
  opts.register_option(opt);

  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->register_option(opt);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    auto optionval = opts.get_option(opt.name);
    if (optionval.value.type == MG_STRING)
      optionval.value.string = optionval.stringval.c_str();
    if (ptr) ptr->update_option(opt.name,optionval.value);
  }
}

int profile::set_option(std::string opname, std::string value) {
  std::lock_guard<std::mutex> guard(lock);

  int ret = opts.set(opname,value);
  if (ret != 0)
    return ret; //something went wrong... Don't propagate to subscribers.
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_option(opname, value);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    auto optionval = opts.get_option(opname);
    if (ptr) ptr->update_option(opname.c_str(),optionval.value);
  }
  return 0;
}

void profile::remove_option(std::string option_name) {
  std::lock_guard<std::mutex> guard(lock);
  

  int ret = opts.remove(option_name);
  if (ret != 0) return;

  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->remove_option(option_name);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->remove_option(option_name);
  }
}

void profile::set_advanced(std::vector<std::string> names, advanced_event_translator* trans) {
  if (names.empty()) return;
  std::lock_guard<std::mutex> guard(lock);
  for (int i = 0; i < names.size(); i++) {
    auto alias = aliases.find(names[i]);
    if (alias != aliases.end())
      names[i] = alias->second;
  }
  auto it = names.begin();
  //this key creation is not ideal.
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
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_advanced(names,trans->clone());
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_advanced(names,trans);
  }
}

void profile::set_alias(std::string external, std::string local) {
  std::lock_guard<std::mutex> guard(lock);
  if (local.empty()) {
    aliases.erase(external);
  } else {
    aliases[external] = local;
  }
}

std::string profile::get_alias(std::string name) {
  auto alias = aliases.find(name);
  if (alias != aliases.end())
    return alias->second;
  return "";
}

option_info profile::get_option(std::string opname) {
  std::lock_guard<std::mutex> guard(lock);
  return opts.get_option(opname);
}

void profile::list_options(std::vector<option_info>& list) const {
  opts.list_options(list);
}

void profile::subscribe_to(profile* parent) {
  if (parent == this) return;
  std::lock_guard<std::mutex> guard(lock);
  subscriptions.push_back(parent->get_shared_ptr());
  parent->add_listener(get_shared_ptr());
}
void profile::remember_subscription(profile* parent) {
  if (parent == this) return;
  std::lock_guard<std::mutex> guard(lock);
  subscriptions.push_back(parent->get_shared_ptr());
}

std::shared_ptr<profile> profile::get_shared_ptr() {
  return shared_from_this();
}

void profile::add_listener(std::shared_ptr<profile> listener) {
  std::lock_guard<std::mutex> guard(lock);
  subscribers.push_back(listener);
}

void profile::remove_listener(std::shared_ptr<profile> listener) {
  remove_listener(listener.get());
}

void profile::remove_listener(profile* listener) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto it = subscribers.begin(); it != subscribers.end(); it++) {
    std::shared_ptr<profile> ptr = it->lock();
    if (ptr.get() == listener) {
      subscribers.erase(it);
      break;
    }
  }
}

void profile::add_device(std::shared_ptr<input_source> device) {
  std::lock_guard<std::mutex> guard(lock);
  devices.push_back(device);
}

void profile::remove_device(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto it = devices.begin(); it != devices.end(); it++) {
    std::shared_ptr<input_source> ptr = it->lock();
    if (ptr.get() == dev) {
      devices.erase(it);
      break;
    }
  }
}

void profile::copy_into(std::shared_ptr<profile> target, bool add_subscription, bool add_new) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto entry : aliases)
    target->set_alias(entry.first,entry.second);
  for (auto entry : mapping)
    target->set_mapping(entry.first, entry.second.trans->clone(), entry.second.type, add_new);
  for (auto entry : adv_trans)
    target->set_advanced(entry.second.fields, entry.second.trans->clone());
  std::vector<option_info> optionlist;
  opts.list_options(optionlist);
  for (auto opt : optionlist) {
    if (add_new)  target->register_option(opt);
    if (!add_new) target->set_option(opt.name,opt.stringval);
  }
  if (add_subscription) {
    subscribers.push_back(target);
    target->remember_subscription(this);
  }
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
    (*map)["tl2_axis"] = {new axis2axis(ABS_Z, 1), DEV_AXIS};
    (*map)["tr2_axis"] = {new axis2axis(ABS_RZ, 1), DEV_AXIS};
    (*map)["tl2_axis_btn"] = {new event_translator(), DEV_KEY};
    (*map)["tr2_axis_btn"] = {new event_translator(), DEV_KEY};

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


#include "profile.h"
#include "event_translators/event_change.h"
#include "event_translators/translators.h"
#include "devices/device.h"
#include "parser.h"
#include <unordered_set>
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

  for (auto e : group_trans) {
    if (e.second.trans) delete e.second.trans;
  }

  mapping.clear();
}

event_translator* nullsafe_clone(event_translator* trans) {
  if (trans)
    return trans->clone();
  return nullptr;
}

//This is private, called only while locked.
trans_map* profile::get_mapping(std::string in_event_name) {
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return nullptr;
  return &(it->second);
}

entry_type profile::get_entry_type(std::string in_event_name) {
  std::lock_guard<std::mutex> guard(lock);
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end()) {
    if (alias->second.size() > 0 && alias->second.front() == ' ')
      return DEV_EVENT_GROUP;
    in_event_name = alias->second;
    //chop off any direction markers
    read_direction(in_event_name);
  }
  auto it = mapping.find(in_event_name);
  if (it == mapping.end()) return NO_ENTRY;
  return (it->second.type);
}

void profile::set_mapping(std::string in_event_name, int8_t direction, event_translator* mapper, entry_type type, bool add_new) {
  std::lock_guard<std::mutex> guard(lock);
  auto alias = aliases.find(in_event_name);
  if (alias != aliases.end()) {
    //we don't bother with checking for a group alias.
    //It's doomed to fail anyway.
    in_event_name = alias->second;
    direction *= read_direction(in_event_name);
  }
  trans_map* oldmap = get_mapping(in_event_name);

  if (!add_new && oldmap == nullptr) {
    if (mapper)
      delete mapper;
    return; //We don't recognize this event, and we don't want to!
  }

  if (oldmap && oldmap->trans) delete oldmap->trans;
  if (oldmap)
    clear_group_mapping(oldmap->active_group);
  mapping[in_event_name] = {mapper, type, direction, nullptr};
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_mapping(in_event_name, direction, mapper ? mapper->clone() : nullptr, type, add_new);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_map(in_event_name.c_str(), direction, mapper);
  }
}

void profile::remove_event(std::string event_name) {
  std::lock_guard<std::mutex> guard(lock);
  auto alias = aliases.find(event_name);
  if (alias != aliases.end()) {
    event_name = alias->second;
    //chop off any direction markers
    read_direction(event_name);
  }
  trans_map* oldmap = get_mapping(event_name);

  if (oldmap == nullptr) {
    return; //Nothing to do?
  }

  if (oldmap->trans) delete oldmap->trans;
  clear_group_mapping(oldmap->active_group);
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

void profile::set_group_mapping(std::vector<std::string> names, std::vector<int8_t> directions, group_translator* trans) {
  if (names.empty()) return;
  std::lock_guard<std::mutex> guard(lock);
  int total_op = 0; //a little counter to avoid infinite loops.
  for (uint i = 0; i < names.size(); i++) {
    total_op++;
    auto alias = aliases.find(names[i]);
    if (alias != aliases.end()) {
      const std::string& local = alias->second;
      if (local.size() > 0 && local.front() == ' ') {
        //this was a group alias! we need to insert the real events.
        //this is a bit obnoxious to do.
        //The 200 limit is to avoid silly infinite loops if someone is careless with aliases...
        std::vector<token> tokens = tokenize(local);
        if (tokens.empty() || tokens.size() + names.size() > 50 || total_op > 200)
          return; //abort
        //we need to delete this alias from our lists, and insert the new names in order at its old spot.
        //the first new name is pulled out of the loop to just overwrite the old alias spot.
        //We ignore the old alias direction value. What even is a negative event group?
        names[i] = tokens[0].value;
        directions[i] = read_direction(names[i]);
        auto name_it = names.begin()+i+1;
        auto dir_it = directions.begin()+i+1;
        //the last token is an end-of-line we should ignore.
        for (uint j = 1; j < tokens.size()-1; j++) {
          name_it = names.begin()+i+j;
          dir_it = directions.begin()+i+j;
          directions.insert(dir_it, read_direction(tokens[j].value));
          names.insert(name_it, tokens[j].value);
        }
        //decrement i to get us ready to reread the overwritten spot on the next loop.
        i--;
      } else {
        //whew. Just a simple alias!
        names[i] = alias->second;
        directions[i] *= read_direction(names[i]);
      }
    }
  }
  
  //two purposes here: loop over all event names and 
  // 1) build up the key used to index this translator by concatenating the vent names
  // 2) verify that this profile actually has the event being named.
  //this key creation is not ideal.
  std::string key;
  for (unsigned i = 0; i < names.size(); i++) {
    if (mapping.find(names[i]) == mapping.end())
      return; //abort! event not found.
    key += names[i] + ((directions[i] > 0)?"+":"-") + ",";
  }
  key.pop_back();

  auto stored = group_trans.find(key);
  if (stored != group_trans.end()) {
    clear_group_mapping(&(stored->second));
  }

  if (trans) {
    group_map entry;
    entry.fields = names;
    entry.trans = trans;
    entry.directions = directions;
    entry.clear_other_translations = trans->clear_other_translations();
    group_trans[key] = entry;
    if (entry.clear_other_translations) {
      //go through and clear the other translations!
      std::unordered_set<std::pair<const std::string,trans_map>*> uniq_maps;
      for (auto name : names) {
        //due to aliasing, do the unique set after lookup.
        auto lookup = mapping.find(name);
        
        if (lookup != mapping.end())
          uniq_maps.insert(&(*lookup));
      }
      group_map* this_map = &(group_trans.find(key)->second);
      //for each unique individual event involved:
      //clear any previous individual translation and
      //set this group translator as the active group
      for (auto map : uniq_maps) {
        clear_mapping(map);
        map->second.active_group = this_map;
      }
    }
  }
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_group_mapping(names, directions, trans ? trans->clone() : nullptr);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_group(names, directions, trans);
  }
}

//a group alias is denoted by a leading space on the stored local name.
//This was chosen as a group alias is a space-separated list
//that will be passed through a tokenizer that'll remove it and
//a non-group alias is useless with a leading space.

//It is also invalid anyways to have the same external name
//for two aliases, including both group and non-group aliases.
//Thus storing both in the same map makes sense.

void profile::set_alias(std::string external, std::string local) {
  std::lock_guard<std::mutex> guard(lock);
  while(local.size() > 0 && local.front() == ' ')
    local.erase(local.begin());
  if (local.empty()) {
    aliases.erase(external);
  } else {
    auto lookup = aliases.find(local);
    int total_aliasing = 0;
    while (lookup != aliases.end() && total_aliasing < 50) {
      local = lookup->second;
      lookup = aliases.find(local);
      total_aliasing++;
    }
    aliases[external] = local;
  }
}

std::string profile::get_alias(std::string name) {
  auto alias = aliases.find(name);
  if (alias != aliases.end() && alias->second.size() > 0 && alias->second.front() != ' ')
    return alias->second;
  return "";
}

void profile::set_group_alias(std::string external, std::string local) {
  std::lock_guard<std::mutex> guard(lock);
  if (local.empty()) {
    aliases.erase(external);
  } else {
    std::vector<token> tokens = tokenize(local);
    tokens.pop_back(); //end of line token ignored.
    for (auto token : tokens) {
      std::string alias = get_alias(token.value);
      if (!alias.empty())
        token.value = alias;
      if (mapping.find(token.value) == mapping.end())
        return; //abort! no sense including an alias we don't support.
    }
    if (local.front() != ' ')
      local.insert(local.begin(),' ');
    aliases[external] = local;
  }
}

std::string profile::get_group_alias(std::string name) {
  auto alias = aliases.find(name);
  if (alias != aliases.end() && alias->second.size() > 0 && alias->second.front() == ' ')
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

//These next two helper functions are only called while the profile is locked.
void profile::clear_mapping(std::pair<const std::string,trans_map>* event_mapping) {
  if (!event_mapping)
    return;
  clear_group_mapping(event_mapping->second.active_group);
  if (event_mapping->second.trans)
    delete event_mapping->second.trans;
  event_mapping->second.trans = nullptr;
  event_mapping->second.direction = 0;
  event_mapping->second.active_group = nullptr;
  //still need to propagate new mapping
  auto type = event_mapping->second.type;
  for (auto prof : subscribers) {
    auto ptr = prof.lock();
    if (ptr) ptr->set_mapping(event_mapping->first, 0, nullptr, type, false);
  }
  for (auto dev : devices) {
    auto ptr = dev.lock();
    if (ptr) ptr->update_map(event_mapping->first.c_str(), 0, nullptr);
  }
}
void profile::clear_group_mapping(group_map* group_mapping) {
  if (!group_mapping)
    return;
  //First, clear all back pointers
  for (auto field : group_mapping->fields) {
    trans_map* map = get_mapping(field);
    if (map)
      map->active_group = nullptr;
  }
  if (group_mapping->trans)
    delete group_mapping->trans;
  for (const auto& pair : group_trans) {
    if (&(pair.second) == group_mapping) {
      //propagate the change before deleting the metadata we need...
      for (auto prof : subscribers) {
        auto ptr = prof.lock();
        if (ptr) ptr->set_group_mapping(group_mapping->fields, group_mapping->directions, nullptr);
      }
      for (auto dev : devices) {
        auto ptr = dev.lock();
        if (ptr) ptr->update_group(group_mapping->fields, group_mapping->directions, nullptr);
      }
      group_trans.erase(pair.first);
      break;
    }
  }
  //still need to propagate new mapping.
  
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
  for (auto entry : aliases) {
    //aliases are needed for copying events along,
    //and it is mandatory for all subscribers to at least have their parents' aliases.
    //recall that we hackishly denote group aliases by a leading space.
    if (entry.second.size() > 0 && entry.second.front() != ' ')
      target->set_alias(entry.first,entry.second);
    //group aliases shouldn't be handled yet, as we wish to allow subscribers to potentially ignore them,
    //as they might refer to events the subscriber does not have.
  }
  for (auto entry : mapping)
    target->set_mapping(entry.first, entry.second.direction, nullsafe_clone(entry.second.trans), entry.second.type, add_new);
  for (auto entry : group_trans)
    target->set_group_mapping(entry.second.fields, entry.second.directions, entry.second.trans->clone());
  std::vector<option_info> optionlist;
  opts.list_options(optionlist);
  for (auto opt : optionlist) {
    if (add_new)  target->register_option(opt);
    if (!add_new) target->set_option(opt.name,opt.stringval);
  }
  for (auto entry : aliases) {
    //now is the time to set group aliases
    //the subscriber will ignore them if appropriate.
    if (entry.second.size() > 0 && entry.second.front() == ' ')
      target->set_group_alias(entry.first,entry.second);
  }
  if (add_subscription) {
    subscribers.push_back(target);
    target->remember_subscription(this);
  }
}

void profile::build_default_gamepad_profile() {
  bool do_group = false;

  default_gamepad_profile.lock.lock();
  if (default_gamepad_profile.mapping.empty()) {
    auto map = &default_gamepad_profile.mapping;
    (*map)["first"] =    {new btn2btn(BTN_SOUTH), DEV_KEY, 1, nullptr};
    (*map)["second"] =    {new btn2btn(BTN_EAST), DEV_KEY, 1, nullptr};
    (*map)["third"] =    {new btn2btn(BTN_WEST), DEV_KEY, 1, nullptr};
    (*map)["fourth"] =    {new btn2btn(BTN_NORTH), DEV_KEY, 1, nullptr};
    (*map)["left"] = {new btn2btn(BTN_DPAD_LEFT), DEV_KEY, 1, nullptr};
    (*map)["right"] = {new btn2btn(BTN_DPAD_RIGHT), DEV_KEY, 1, nullptr};
    (*map)["up"] =   {new btn2btn(BTN_DPAD_UP), DEV_KEY, 1, nullptr};
    (*map)["down"] = {new btn2btn(BTN_DPAD_DOWN), DEV_KEY, 1, nullptr};
    (*map)["mode"] = {new btn2btn(BTN_MODE), DEV_KEY, 1, nullptr};
    (*map)["start"] = {new btn2btn(BTN_START), DEV_KEY, 1, nullptr};
    (*map)["select"] = {new btn2btn(BTN_SELECT), DEV_KEY, 1, nullptr};
    (*map)["tl"] =    {new btn2btn(BTN_TL), DEV_KEY, 1, nullptr};
    (*map)["tr"] =    {new btn2btn(BTN_TR), DEV_KEY, 1, nullptr};
    (*map)["tl2"] =   {new btn2btn(BTN_TL2), DEV_KEY, 1, nullptr};
    (*map)["tr2"] =   {new btn2btn(BTN_TR2), DEV_KEY, 1, nullptr};
    (*map)["thumbl"] =   {new btn2btn(BTN_THUMBL), DEV_KEY, 1, nullptr};
    (*map)["thumbr"] =   {new btn2btn(BTN_THUMBR), DEV_KEY, 1, nullptr};

    (*map)["left_x"] = {nullptr, DEV_AXIS, 1, nullptr};
    (*map)["left_y"] = {nullptr, DEV_AXIS, 1, nullptr};
    (*map)["right_x"] = {nullptr, DEV_AXIS, 1, nullptr};
    (*map)["right_y"] = {nullptr, DEV_AXIS, 1, nullptr};
    (*map)["tl2_axis"] = {new axis2axis(ABS_Z, 1), DEV_AXIS, 1, nullptr};
    (*map)["tr2_axis"] = {new axis2axis(ABS_RZ, 1), DEV_AXIS, 1, nullptr};
    (*map)["tl2_axis_btn"] = {nullptr, DEV_KEY, 1, nullptr};
    (*map)["tr2_axis_btn"] = {nullptr, DEV_KEY, 1, nullptr};

    //For devices with the dpad as a hat.
    (*map)["updown"] =   {new axis2btns(BTN_DPAD_UP,BTN_DPAD_DOWN), DEV_AXIS, 1, nullptr};
    (*map)["leftright"] =   {new axis2btns(BTN_DPAD_LEFT,BTN_DPAD_RIGHT), DEV_AXIS, 1, nullptr};

    //for backwards compatability
    default_gamepad_profile.aliases["primary"] = "first";
    default_gamepad_profile.aliases["secondary"] = "second";

    do_group = true;
  }
  default_gamepad_profile.lock.unlock();
  if (do_group) {
    //easier to reuse code and do this with functions that will also affect the lock.
    std::vector<std::string> evnames({"left_x","left_y"});
    std::vector<int8_t> directions({1,1});
    default_gamepad_profile.set_group_mapping(evnames,directions,new thumb_stick(ABS_X,ABS_Y));
    evnames[0] = "right_x";
    evnames[1] = "right_y";
    default_gamepad_profile.set_group_mapping(evnames,directions,new thumb_stick(ABS_RX,ABS_RY));
  }
}

profile profile::default_gamepad_profile;

void profile::gamepad_defaults() {
  lock.lock();
  if (this != &default_gamepad_profile && default_gamepad_profile.mapping.empty())
    build_default_gamepad_profile();
  //for backwards compatability
  aliases["primary"] = "first";
  aliases["secondary"] = "second";
  for (auto entry : default_gamepad_profile.mapping) {
    std::string alias = get_alias(entry.first);
    int8_t dir = read_direction(alias)*entry.second.direction;
    alias = alias.empty() ? entry.first : alias;
    event_translator* mapper = entry.second.trans ? entry.second.trans->clone() : nullptr;
    mapping[alias] = {mapper, entry.second.type, dir, nullptr};
  }
  for (auto entry : default_gamepad_profile.group_trans) {
    std::vector<std::string> aliases;
    std::vector<int8_t> directions;
    for (uint i = 0; i < entry.second.fields.size(); i++) {
      std::string alias = get_alias(entry.second.fields[i]);
      int8_t dir = read_direction(alias);
      aliases.push_back(alias.empty() ? entry.second.fields[i] : alias);
      directions.push_back(entry.second.directions[i]*dir);
    }

    auto it = aliases.begin();
    //this key creation is still not ideal.
    std::string key = *it;
    it++;
    for (; it != aliases.end(); it++) {
      key += "," + (*it);
    }
    group_map new_entry;
    new_entry.fields = aliases;
    new_entry.trans = entry.second.trans->clone();
    new_entry.directions = directions;
    new_entry.clear_other_translations = new_entry.trans->clear_other_translations();
    group_trans[key] = new_entry;
    if (new_entry.clear_other_translations) {
      //go through and clear the other translations!
      std::unordered_set<std::pair<const std::string,trans_map>*> uniq_maps;
      for (auto name : new_entry.fields) {
        //due to aliasing, do the unique set after lookup.
        auto lookup = mapping.find(name);

        if (lookup != mapping.end())
          uniq_maps.insert(&(*lookup));
      }
      auto lookup = group_trans.find(key);
      if (lookup != group_trans.end()) {
        group_map* this_map = &(lookup->second);
        //for each unique individual event involved:
        //clear any previous individual translation and
        //set this group translator as the active group
        for (auto map : uniq_maps) {
          clear_mapping(map);
          map->second.active_group = this_map;
        }
      }
    }

  }
  lock.unlock();
}

//detects "left_x+" vs. "left_x-"
//modifies the passed in string to remove the +/- as well.
int8_t read_direction(std::string& name) {
  //positive direction (1) is the default!
  if (name.empty()) return 1;
  if (name.back() == '+') {
    name.pop_back();
    return 1;
  }
  if (name.back() == '-') {
    name.pop_back();
    return -1;
  }
  //backwards compat/liberal input acceptance: +/- at the beginning
  if (name.front() == '+') {
    name.erase(name.begin());
    return 1;
  }
  if (name.front() == '-') {
    name.erase(name.begin());
    return -1;
  }
  return 1;
}

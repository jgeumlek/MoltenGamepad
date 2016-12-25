#include "device.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <iostream>

input_source::input_source(device_manager* manager, device_plugin plugin, void* plug_data) 
      : manager(manager), plugin(plugin), plug_data(plug_data), uniq(plugin.uniq), phys(plugin.phys) {

  for (auto ev : manager->get_events())
    register_event(ev);
  std::vector<option_info> prof_opts;
  manager->mapprofile->list_options(prof_opts);
  for (auto opt : prof_opts)
    options[opt.name] = opt;

  epfd = epoll_create(1);
  if (epfd < 1) perror("epoll create");

  int internal[2];
  pipe(internal);
  watch_file(internal[0], this);

  priv_pipe = internal[1];
  internalpipe = internal[0];
  
  if (plugin.init)
    plugin.init(plug_data, this);

  ff_ids[0] = -1;
}
  

input_source::~input_source() {
  end_thread();
  close(internalpipe);
  close(priv_pipe);
  close(epfd);
  for (int i = 0; i < ev_map.size(); i++) {
    if (ev_map[i].trans) delete ev_map[i].trans;
  }



  for (auto it : adv_trans) {
    if (it.second.trans) delete it.second.trans;
    if (it.second.fields) delete it.second.fields;
  }

  if (assigned_slot) {
    assigned_slot->remove_device(this);
    assigned_slot = nullptr;
  };
  if (plugin.destroy)
    plugin.destroy(plug_data);
}

struct input_internal_msg {
  enum input_msg_type { IN_TRANS_MSG, IN_ADV_TRANS_MSG, IN_EVENT_MSG, IN_OPTION_MSG, IN_SLOT_MSG, IN_END_THREAD_MSG } type;
  int id;
  int64_t value;
  MGField field;
  adv_entry adv;
  bool skip_adv_trans;
  char* name;
};


void input_source::register_event(event_decl ev) {
  int id = events.size();
  source_event event = {
    .id = id,
    .name = ev.name,
    .descr = ev.descr,
    .type = ev.type,
    .value = 0,
    .state = EVENT_ACTIVE,
  };
  events.push_back(event);
  ev_map.resize(events.size());
}

void input_source::toggle_event(int id, event_state state) {
  if (id < 0 || id >= events.size() || events[id].state == EVENT_DISABLED)
    return;
  events[id].state = state;
  if (state == EVENT_DISABLED)
    devprofile->remove_event(std::string(events[id].name));
}

void input_source::register_option(option_info opt) {
  std::lock_guard<std::mutex> lock(opt_lock);
  options.insert(std::pair<std::string, option_info>(opt.name, opt));
}

void input_source::remove_option(std::string option_name) {
  std::lock_guard<std::mutex> lock(opt_lock);
  options.erase(option_name);
}


void input_source::watch_file(int fd, void* tag) {
  if (fd <= 0) return;
  struct epoll_event event;
  memset(&event, 0, sizeof(event));

  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  event.data.ptr = tag;
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
  if (ret < 0) perror("epoll add");
}


void input_source::update_map(const char* evname, int8_t direction, event_translator* trans) {
  std::string name(evname);
  auto alias = devprofile->aliases.find(name);
  if (alias != devprofile->aliases.end()) {
    direction *= read_direction(alias->second);
  }
  evname = name.c_str();
  for (int i = 0; i < events.size(); i++) {
    if (!strcmp(evname, events[i].name)) {
      set_trans(i, direction, trans->clone());
      return;
    }
  }
}

std::string field_to_string(const MGField field) {
  if (field.type == MG_STRING)
    return std::string(field.string);
  if (field.type == MG_INT)
    return std::to_string(field.integer);
  if (field.type == MG_BOOL)
    return field.boolean ? "true" : "false";
  return "error";
}

char* copy_str(const char* str) {
  int len = strlen(str) + 1;
  char* copy = (char*)calloc(len, sizeof(char));
  strncpy(copy, str, len);
  return copy;
}

void input_source::update_option(const char* name, const MGField value) {
  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.field = value;
  msg.name = copy_str(name);
  if (value.type == MG_STRING)
    msg.field.string = copy_str(value.string);
  msg.type = input_internal_msg::IN_OPTION_MSG;
  write(priv_pipe, &msg, sizeof(msg));
}

void input_source::set_slot(output_slot* slot) {
  std::lock_guard<std::mutex> guard(slot_lock);
  if (slot == assigned_slot) return;
  if (assigned_slot) {
    assigned_slot->remove_device(this);
    if (ff_ids[0] != -1) {
      play_ff(0,0);
      erase_ff(0);
    }
  }
  if (slot)
    slot->add_device(shared_from_this());
  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.field.slot = slot;
  msg.type = input_internal_msg::IN_SLOT_MSG;
  write(priv_pipe, &msg, sizeof(msg));
  assigned_slot = slot;
}

output_slot* input_source::get_slot() {
  std::lock_guard<std::mutex> guard(slot_lock);
  return assigned_slot;
}

void input_source::list_options(std::vector<option_info>& list) const {
  devprofile->list_options(list);
}



void input_source::update_advanced(const std::vector<std::string>& evnames, const std::vector<int8_t> directions, advanced_event_translator* trans) {
  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));

  std::vector<std::string> local_names = evnames;
  msg.adv.fields = new std::vector<int>();
  msg.adv.directions = new std::vector<int8_t>(directions);
  
  //First, translate event names using this device's aliases into the local names.
  for (int i = 0; i < local_names.size(); i++) {
    auto alias = devprofile->aliases.find(std::string(local_names.at(i)));
    if (alias != devprofile->aliases.end()) {
      std::string local_name = alias->second;
      (*msg.adv.directions)[i] *= read_direction(local_name);
      local_names[i] = local_name;
    }
  }
  //Next, check that all referenced events are present. Abort if not.
  for (std::string name : local_names) {
    bool found = false;
    for (int i = 0; i < events.size(); i++) {
      const source_event& ev = events[i];
      if (!strcmp(name.c_str(), ev.name)) {
        found = true;
        //store event index
        msg.adv.fields->push_back(i);
        break;
      }
    }
    if (!found) {
      delete msg.adv.fields;
      return; //Abort!
    }
  }

  //build the key to store this under.
  auto its = local_names.begin();
  std::string adv_name = *its;
  its++;
  for (; its != local_names.end(); its++) {
    adv_name += "," + (*its);
  }
  msg.adv.key = new std::string(adv_name);

  //Finally, instantiate the translator from it's prototype.
  msg.adv.trans = nullptr;
  if (trans) {
    msg.adv.trans = trans->clone();
    msg.adv.trans->init(this);
  }

  msg.id = -1;
  
  msg.type = input_internal_msg::IN_ADV_TRANS_MSG;

  write(priv_pipe, &msg, sizeof(msg));
}




void input_source::set_trans(int id, int8_t direction, event_translator* trans) {
  if (id < 0 || id >= events.size()) return;

  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.id = id;
  msg.field.trans = trans;
  msg.value = direction;
  msg.type = input_internal_msg::IN_TRANS_MSG;
  write(priv_pipe, &msg, sizeof(msg));
};

void input_source::inject_event(int id, int64_t value, bool skip_adv_trans) {
  if (id < 0 || id >= events.size()) return;

  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.id = id;
  msg.value = value;
  msg.skip_adv_trans = skip_adv_trans;
  msg.type = input_internal_msg::IN_EVENT_MSG;
  write(priv_pipe, &msg, sizeof(msg));

}

//Do we care enough to assign to a slot when this happens?
//Should be indicative of user input.
bool notable_event(const entry_type type, const int64_t value, const int64_t oldvalue) {
  if (type == DEV_KEY && value)
    return true; //key presses are always notable!
  if (type == DEV_AXIS) {
    int val_sign = -1*(value < -ABS_RANGE/2) + 1*(value > ABS_RANGE/2);
    int oldval_sign = -1*(oldvalue < -ABS_RANGE/2) + 1*(oldvalue > ABS_RANGE/2);
    //If oldvalue == 0, then this might be our first read! Better to assume
    //it is not notable. Most axes will have some jitter to make this
    //not a problem. Hats will be affected, but only at the very start.
    return (val_sign != oldval_sign) && oldvalue != 0;
  }
  return false;
}

int64_t apply_direction(entry_type type, int64_t value, int8_t direction) {
  if (type == DEV_AXIS && direction == -1)
    return -value;
  if (type == DEV_KEY && direction == -1)
    return !value;
  return value;
}

void input_source::send_value(int id, int64_t value) {
  if (id < 0 || id > events.size() || events[id].value == value)
    return;
  bool blocked = false;
  for (auto adv_trans : ev_map.at(id).attached) {
    
    if (adv_trans.trans->claim_event(adv_trans.index, {apply_direction(events[id].type,value,adv_trans.direction)}))
      blocked = true;
  }

  //On a notable event, try to claim a slot if we don't have one.
  if (!out_dev && notable_event(events[id].type, value, events[id].value)) {
    manager->mg->slots->request_slot(this);
    //Normally moving slots sends an event queued into our private pipe.
    //out_dev won't be updated until that event is read to ensure
    //no race condition with event processing. But right now we are
    //on the event loop thread. Skip the wait, set outdev now.
    //(The internal message will still be processed later, but
    // nothing will happen beyond assigning to out_dev again.)
    std::lock_guard<std::mutex> guard(slot_lock);
    out_dev = assigned_slot;
  }
  events.at(id).value = value;

  if (blocked) return;

  value = apply_direction(events[id].type, value, ev_map[id].direction);

  if (ev_map.at(id).trans && out_dev) ev_map.at(id).trans->process({value}, out_dev);
    

}

void input_source::send_syn_report() {
  if (out_dev) {
    for (auto pair : adv_trans) {
      pair.second.trans->process_syn_report(out_dev);
    }
    input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    out_dev->take_event(ev);
  }
}

void input_source::force_value(int id, int64_t value) {

  //On a notable event, try to claim a slot if we don't have one.
  if (!out_dev && notable_event(events[id].type, value, events[id].value)) {
    manager->mg->slots->request_slot(this);
    //Normally moving slots sends an event queued into our private pipe.
    //out_dev won't be updated until that event is read to ensure
    //no race condition with event processing. But right now we are
    //on the event loop thread. Skip the wait, set outdev now.
    //(The internal message will still be processed later, but
    // nothing will happen beyond assigning to out_dev again.)
    std::lock_guard<std::mutex> guard(slot_lock);
    out_dev = assigned_slot;
  }

  events.at(id).value = value;

  value = apply_direction(events[id].type, value, ev_map[id].direction);

  if (ev_map.at(id).trans && out_dev) ev_map.at(id).trans->process({value}, out_dev);

}




void input_source::thread_loop() {
  struct epoll_event event;
  struct epoll_event events[1];
  memset(&event, 0, sizeof(event));

  memset(&last_recurring_update,0,sizeof(timespec));

  while ((keep_looping)) {
    int timeout = -1;
    if (do_recurring_events) {
      //try to set epoll_wait to timeout at the next 10ms interval.
      timeout = 10 - ms_since_last_recurring_update();
      timeout = timeout < 0 ? 0 : timeout;
    }

    int n = epoll_wait(epfd, events, 1, timeout);
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n < 0 && errno != EINTR) {
      perror("epoll wait:");
      break;
    }
    if (do_recurring_events && (n == 0 || timeout == 0)) {
      process_recurring_events();
      continue;
    }
    if (events[0].data.ptr == this) {
      struct input_internal_msg msg;
      int ret = 1;
      ret = read(internalpipe, &msg, sizeof(msg));
      if (ret == sizeof(msg)) {
        handle_internal_message(msg);
      }
    } else {
      process(events[0].data.ptr);
    }
  }
}

void input_source::handle_internal_message(input_internal_msg& msg) {
  //Is it an advanced_event_translator message?
  if (msg.type == input_internal_msg::IN_ADV_TRANS_MSG) {
    if (!msg.adv.fields || msg.adv.fields->empty())
      return;
    
    std::string adv_name = *msg.adv.key;
    //If needed, erase the previous adv. trans. with this key.
    auto it = adv_trans.find(adv_name);
    if (it != adv_trans.end()) {
      remove_adv_recurring_event(it->second.trans);
      for (int ev_id : *(it->second.fields))
        remove_listener(ev_id, it->second.trans);
      delete it->second.fields;
      delete it->second.trans;
      delete it->second.key;
      adv_trans.erase(it);
    }
    //Attach and store the new one.
    if (msg.adv.trans) {
      msg.adv.trans->attach(this);
      std::vector<source_event> listened;
      for (int trans_index = 0; trans_index < msg.adv.fields->size(); trans_index++) {
        int ev_id = (*msg.adv.fields)[trans_index];
        int8_t direction = (*msg.adv.directions)[trans_index];
        add_listener(ev_id, direction, msg.adv.trans, trans_index);
        listened.push_back(events[ev_id]);
      }
      msg.adv.trans->set_mapped_events(listened);
      adv_trans[adv_name] = {msg.adv.key, msg.adv.fields, msg.adv.directions, msg.adv.trans};
      if (msg.adv.trans->wants_recurring_events()) {
        add_adv_recurring_event(msg.adv.trans);
      }
    } else {
      //setting to null, which is used to clear the old mapping.
      //no need to store this null value, so clean up these pointers.
      delete msg.adv.fields;
      delete msg.adv.directions;
      if (msg.adv.key) delete msg.adv.key;
    }
    do_recurring_events = recurring_events.size() + adv_recurring_events.size() > 0;
    return;
  }
  if (msg.type == input_internal_msg::IN_TRANS_MSG) {
    //Is it an event_translator message?
    if (!msg.field.trans) return;

    //Assumption: Every registered event must have a
    //valid event_translator at all times.
    //Assumption: This is called only from the unique thread
    //handling this device's events.
    event_translator** trans = &(this->ev_map.at(msg.id).trans);
    remove_recurring_event(*trans);
    delete *trans;
    *(trans) = msg.field.trans;
    ev_map.at(msg.id).direction = (int8_t) msg.value;
    msg.field.trans->attach(this);
    if (msg.field.trans->wants_recurring_events()) {
      add_recurring_event(msg.field.trans, msg.id);
    }
    do_recurring_events = recurring_events.size() + adv_recurring_events.size() > 0;
  }
  if (msg.type == input_internal_msg::IN_EVENT_MSG) {
    //Is it an event injection message?
    if (msg.id < 0) return;

    if (msg.skip_adv_trans) {
      force_value(msg.id, msg.value);
    } else {
      send_value(msg.id, msg.value);
    }
  }
  if (msg.type == input_internal_msg::IN_SLOT_MSG)
    out_dev = msg.field.slot;
  if (msg.type == input_internal_msg::IN_OPTION_MSG) {
    std::lock_guard<std::mutex> lock(opt_lock);
    std::string sname = std::string(msg.name);
    std::string svalue = field_to_string(msg.field);
    auto it = options.find(sname);
    if (it != options.end()) {
      if (!process_option(msg.name, msg.field)) {
        it->second.stringval = svalue;
      } else {
        //Device rejected the option.
      }
    }
    free (msg.name);
    if (msg.field.type == MG_STRING)
       free ((char*)msg.field.string);
  }

}

void input_source::process_recurring_events() {
  for (auto rec : recurring_events) {
    if (out_dev && events[rec.id].state == EVENT_ACTIVE) {
      rec.trans->process_recurring(out_dev);
    }
  }
  for (const advanced_event_translator* adv : adv_recurring_events) {
    adv->process_recurring(out_dev);
  }
  send_syn_report();
  clock_gettime(CLOCK_MONOTONIC, &last_recurring_update);
}

std::string input_source::get_alias(std::string event_name) const {
  auto alias = devprofile->aliases.find(event_name);
  if (alias != devprofile->aliases.end())
    return alias->second;
  return "";
}

void input_source::start_thread() {
  keep_looping = true;

  thread = new std::thread(&input_source::thread_loop, this);
}


void input_source::end_thread() {
  if (thread) {
    keep_looping = false;
    struct input_internal_msg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = input_internal_msg::IN_END_THREAD_MSG;
    write(priv_pipe, &msg, sizeof(msg));
    thread->join();
    delete thread;
    thread = nullptr;
  }
}

void input_source::add_listener(int id, int8_t direction, advanced_event_translator* trans, int trans_index) {
  if (id < 0 || id >= events.size()) return;
  ev_map[id].attached.push_back({trans, trans_index, direction});
}

void input_source::remove_listener(int id, advanced_event_translator* trans) {
  if (id < 0 || id >= ev_map.size()) return;
  for (auto it = ev_map[id].attached.begin(); it != ev_map[id].attached.end(); it++) {
    if (it->trans == trans) {
      ev_map[id].attached.erase(it);
      return;
    }
  }
}


void input_source::add_recurring_event(const event_translator* trans, int id) {
  recurring_events.push_back({trans, id});
}

void input_source::remove_recurring_event(const event_translator* trans) {
  for (auto it = recurring_events.begin(); it != recurring_events.end(); it++) {
    if (it->trans == trans) {
      recurring_events.erase(it);
      return;
    }
  }
}

void input_source::add_adv_recurring_event(const advanced_event_translator* trans) {
  adv_recurring_events.push_back(trans);
}

void input_source::remove_adv_recurring_event(const advanced_event_translator* trans) {
  for (auto it = adv_recurring_events.begin(); it != adv_recurring_events.end(); it++) {
    if (*it == trans) {
      adv_recurring_events.erase(it);
      return;
    }
  }
}

int64_t input_source::ms_since_last_recurring_update() {
  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  int64_t delta_sec = now.tv_sec - last_recurring_update.tv_sec;
  int64_t delta_nsec = now.tv_nsec - last_recurring_update.tv_nsec;
  return delta_sec*1000 + delta_nsec/1000000;
}

std::string input_source::get_manager_name() const {
  return manager->name;
}

void input_source::process(void *tag) {
  if (plugin.process_event)
    plugin.process_event(plug_data, tag);
}

int input_source::process_option(const char* opname, const MGField field) {
  if (plugin.process_option)
    return plugin.process_option(plug_data, opname, field);
  return -1;
}

std::string input_source::get_description() const {
  if (plugin.get_description)
    return std::string(plugin.get_description(plug_data));
  return descr;
}

std::string input_source::get_type() const {
  if (plugin.get_type)
    return std::string(plugin.get_type(plug_data));
  return device_type;
}
  
void input_source::print(std::string message) {
  manager->log.take_message(0,name + ": " + message);
}

int input_source::upload_ff(ff_effect effect) {
  if (plugin.upload_ff) {
    effect.id = ff_ids[0];
    ff_ids[0] = plugin.upload_ff(plug_data,&effect);
    if (ff_ids[0] < 0) ff_ids[0] = -1;
  }
  return -(ff_ids[0] < 0);
}

int input_source::erase_ff(int id) {
  if (plugin.erase_ff) {
    int ret = plugin.erase_ff(plug_data, ff_ids[id]);
    ff_ids[id] = -1;
    return ret;
  }
  return -1;
}

int input_source::play_ff(int id, int repetitions) {
  if (plugin.play_ff)
    return plugin.play_ff(plug_data, ff_ids[id], repetitions);
  return -1;
}

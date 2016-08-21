#include "device.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <iostream>

input_source::input_source(device_manager* manager, device_plugin plugin, void* plug_data) 
      : manager(manager), plugin(plugin), plug_data(plug_data), uniq(plugin.uniq) {

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

  if (out_dev) {
    manager->mg->slots->remove(this);
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
  source_event event = {
    .id = 0,
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


void input_source::update_map(const char* evname, event_translator* trans) {
  std::string name(evname);
  auto alias = devprofile->aliases.find(name);
  if (alias != devprofile->aliases.end())
    evname = alias->second.c_str();

  for (int i = 0; i < events.size(); i++) {
    if (!strcmp(evname, events[i].name)) {
      set_trans(i, trans->clone());
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
  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.field.slot = slot;
  msg.type = input_internal_msg::IN_SLOT_MSG;
  write(priv_pipe, &msg, sizeof(msg));
}

void input_source::list_options(std::vector<option_info>& list) const {
  devprofile->list_options(list);
}



void input_source::update_advanced(const std::vector<std::string>& evnames, advanced_event_translator* trans) {
  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));

  msg.adv.fields = new std::vector<std::string>();
  *msg.adv.fields = evnames;
  //First, translate event names using this device's aliases.
  for (int i = 0; i < msg.adv.fields->size(); i++) {
    auto alias = devprofile->aliases.find(std::string(evnames.at(i)));
    if (alias != devprofile->aliases.end())
      (*msg.adv.fields)[i] = alias->second.c_str();
  }
  //Next, check that all referenced events are present. Abort if not.
  for (std::string name : *msg.adv.fields) {
    bool found = false;
    for (auto ev : events) {
      if (!strcmp(name.c_str(), ev.name)) {
        found = true;
        break;
      }
    }
    if (!found) {
      delete msg.adv.fields;
      return; //Abort!
    }
  }

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




void input_source::set_trans(int id, event_translator* trans) {
  if (id < 0 || id >= events.size()) return;

  struct input_internal_msg msg;
  memset(&msg, 0, sizeof(msg));
  msg.id = id;
  msg.field.trans = trans;
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
  events.at(id).value = value;
  write(priv_pipe, &msg, sizeof(msg));

}

void input_source::send_value(int id, int64_t value) {
  if (id < 0 || id > events.size() || events[id].value == value)
    return;
  bool blocked = false;
  for (auto adv_trans : ev_map.at(id).attached)
    if (adv_trans->claim_event(id, {value})) blocked = true;

  
  events.at(id).value = value;

  if (blocked) return;

  if (ev_map.at(id).trans && out_dev) ev_map.at(id).trans->process({value}, out_dev);

  //On a key press, try to claim a slot if we don't have one.
  if (!out_dev && events.at(id).type == DEV_KEY)
    manager->mg->slots->request_slot(this);

}

void input_source::send_syn_report() {
  if (out_dev) {
    input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    out_dev->take_event(ev);
  }
}

void input_source::force_value(int id, int64_t value) {

  events.at(id).value = value;

  if (ev_map.at(id).trans && out_dev) ev_map.at(id).trans->process({value}, out_dev);

  //On a key press, try to claim a slot if we don't have one.
  if (!out_dev && events.at(id).type == DEV_KEY)
    manager->mg->slots->request_slot(this);

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
    //First, build the key to store this under.
    auto its = msg.adv.fields->begin();
    std::string adv_name = *its;
    its++;
    for (; its != msg.adv.fields->end(); its++) {
      adv_name += "," + (*its);
    }

    //If needed, erase the previous adv. trans. with this key.
    auto it = adv_trans.find(adv_name);
    if (it != adv_trans.end()) {
      delete it->second.fields;
      delete it->second.trans;
      adv_trans.erase(it);
    }
    //Attach and store the new one.
    if (msg.adv.trans) {
      msg.adv.trans->attach(this);
      adv_trans[adv_name] = {msg.adv.fields, msg.adv.trans};
    } else {
      delete msg.adv.fields;
    }

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
    msg.field.trans->attach(this);
    if (msg.field.trans->wants_recurring_events()) {
      add_recurring_event(msg.field.trans, msg.id);
    }
    do_recurring_events = recurring_events.size() > 0;
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

void input_source::add_listener(int id, advanced_event_translator* trans) {
  if (id < 0 || id >= events.size()) return;
  ev_map[id].attached.push_back(trans);
}

void input_source::remove_listener(int id, advanced_event_translator* trans) {
  if (id < 0 || id >= ev_map.size()) return;
  for (auto it = ev_map[id].attached.begin(); it != ev_map[id].attached.end(); it++) {
    if (*it == trans) {
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
  manager->log.take_message(name + ": " + message);
}

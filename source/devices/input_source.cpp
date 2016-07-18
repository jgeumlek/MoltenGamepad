#include "device.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <iostream>



input_source::input_source(slot_manager* slot_man, device_manager* manager, std::string type) : slot_man(slot_man), manager(manager), device_type(type) {
  epfd = epoll_create(1);
  if (epfd < 1) perror("epoll create");

  int internal[2];
  pipe(internal);
  watch_file(internal[0], this);

  priv_pipe = internal[1];
  internalpipe = internal[0];

}

input_source::~input_source() {
  end_thread();
  close(internalpipe);
  close(priv_pipe);
  close(epfd);
  for (int i = 0; i < events.size(); i++) {
    if (events[i].trans) delete events[i].trans;
  }



  for (auto it : adv_trans) {
    if (it.second.trans) delete it.second.trans;
    if (it.second.fields) delete it.second.fields;
  }

  if (out_dev) {
    slot_man->remove(this);
  };
}


void input_source::register_event(source_event ev) {
  if (!ev.trans) ev.trans = new event_translator();
  events.push_back(ev);
}

void input_source::register_option(source_option opt) {

  options.insert(std::pair<std::string, source_option>(opt.name, opt));
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

void input_source::update_option(const char* name, const char* value) {
  std::string sname = std::string(name);
  std::string svalue = std::string(value);
  auto it = options.find(sname);
  if (it != options.end()) {
    if (!process_option(name, value)) {
      it->second.value = svalue;
    } else {
      //Device rejected the option.
    }
  }
}

void input_source::list_options(std::vector<source_option>& list) {
  for (auto e : options) {
    list.push_back(e.second);
  }
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
  msg.trans = trans;
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
  bool blocked = false;
  for (auto adv_trans : events.at(id).attached)
    if (adv_trans->claim_event(id, {value})) blocked = true;

  
  events.at(id).value = value;

  if (blocked) return;

  

  if (events.at(id).trans && out_dev) events.at(id).trans->process({value}, out_dev);

  //On a key press, try to claim a slot if we don't have one.
  if (!out_dev && events.at(id).type == DEV_KEY)
    slot_man->request_slot(this);

}

void input_source::force_value(int id, int64_t value) {

  events.at(id).value = value;

  if (events.at(id).trans && out_dev) events.at(id).trans->process({value}, out_dev);

  //On a key press, try to claim a slot if we don't have one.
  if (!out_dev && events.at(id).type == DEV_KEY)
    slot_man->request_slot(this);

}




void input_source::thread_loop() {
  struct epoll_event event;
  struct epoll_event events[1];
  memset(&event, 0, sizeof(event));

  while ((keep_looping)) {
    int n = epoll_wait(epfd, events, 1, 10);
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n < 0 && errno != EINTR) {
      perror("epoll wait:");
      break;
    }
    if (n == 0) {
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

void input_source::handle_internal_message(input_internal_msg &msg) {
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
    if (msg.id < 0 || !msg.trans) return;

    //Assumption: Every registered event must have an
    //an event_translator at all times.
    //Assumption: This is called only from the unique thread
    //handling this device's events.
    event_translator** trans = &(this->events.at(msg.id).trans);
    delete *trans;
    *(trans) = msg.trans;
    msg.trans->attach(this);
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
}

void input_source::process_recurring_events() {
  std::lock_guard<std::mutex> guard(recurring_event_lock);
  for (auto trans : recurring_events) {
    if (out_dev) {
      trans->process_recurring(out_dev);
    }
  }
  if (out_dev) {
    input_event ev;
    memset(&ev,0,sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    out_dev->take_event(ev);
  }
}

std::string input_source::get_alias(std::string event_name) {
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
    write(priv_pipe, "h", sizeof(char));
    thread->join();
    delete thread;
    thread = nullptr;
  }
}

void input_source::add_listener(int id, advanced_event_translator* trans) {
  if (id < 0 || id >= events.size()) return;
  events[id].attached.push_back(trans);
}

void input_source::remove_listener(int id, advanced_event_translator* trans) {
  if (id < 0 || id >= events.size()) return;
  for (auto it = events[id].attached.begin(); it != events[id].attached.end(); it++) {
    if (*it == trans) {
      events[id].attached.erase(it);
      return;
    }
  }
}


void input_source::add_recurring_event(const event_translator* trans) {
  std::lock_guard<std::mutex> guard(recurring_event_lock);
  recurring_events.push_back(trans);
}

void input_source::remove_recurring_event(const event_translator* trans) {
  std::lock_guard<std::mutex> guard(recurring_event_lock);
  for (auto it = recurring_events.begin(); it != recurring_events.end(); it++) {
    if (*it == trans) {
      recurring_events.erase(it);
      return;
    }
  }
}

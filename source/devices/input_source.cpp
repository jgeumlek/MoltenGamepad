#include "device.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <iostream>



input_source::input_source(slot_manager* slot_man, device_manager* manager, devtype type) : slot_man(slot_man), manager(manager), type(type) {
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
  auto alias = aliases.find(name);
  if (alias != aliases.end())
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

struct pass_trans {
  int id;
  event_translator* trans;
  adv_entry adv;
};



void input_source::update_advanced(const std::vector<std::string>& evnames, advanced_event_translator* trans) {
  struct pass_trans msg;
  memset(&msg, 0, sizeof(msg));

  msg.adv.fields = new std::vector<std::string>();
  *msg.adv.fields = evnames;
  for (int i = 0; i < msg.adv.fields->size(); i++) {
    auto alias = aliases.find(std::string(evnames.at(i)));
    if (alias != aliases.end())
      (*msg.adv.fields)[i] = alias->second.c_str();
  }
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

  msg.adv.trans = nullptr;
  if (trans) {
    msg.adv.trans = trans->clone();
    msg.adv.trans->init(this);
  }

  msg.id = -1;

  write(priv_pipe, &msg, sizeof(msg));
}




void input_source::set_trans(int id, event_translator* trans) {
  if (id < 0 || id >= events.size()) return;

  struct pass_trans msg;
  memset(&msg, 0, sizeof(msg));
  msg.id = id;
  msg.trans = trans;
  write(priv_pipe, &msg, sizeof(msg));
};


void input_source::send_value(int id, long long value) {
  bool blocked = false;
  for (auto adv_trans : events.at(id).attached)
    if (adv_trans->claim_event(id, {value})) blocked = true;

  if (blocked) return;

  events.at(id).value = value;

  if (events.at(id).trans && out_dev) events.at(id).trans->process({value}, out_dev);

  //On a key press, try to claim a slot if we don't have one.
  if (!out_dev && events.at(id).type == DEV_KEY)
    slot_man->request_slot(this);

}

void input_source::force_value(int id, long long value) {

  events.at(id).value = value;

  if (events.at(id).trans && out_dev) events.at(id).trans->process({value}, out_dev);

  //On a key press, try to claim a slot if we don't have one.
  if (!out_dev && events.at(id).type == DEV_KEY)
    slot_man->request_slot(this);

}

void input_source::load_profile(profile* profile) {
  for (auto ev : events) {
    int id = ev.id;
    event_translator* trans = profile->copy_mapping(ev.name);
    if (trans) {
      set_trans(id, trans);
    }
  }

  for (auto opt : options) {
    std::string value = profile->get_option(opt.first);
    if (!value.empty()) {
      update_option(opt.first.c_str(), value.c_str());
    }
  }
  for (auto entry : profile->adv_trans) {
    update_advanced(entry.second.fields, entry.second.trans);
  }
  for (auto entry : profile->aliases) {
    aliases[entry.first] = entry.second;
  }

}

void input_source::export_profile(profile* profile) {
  for (auto ev : events) {
    event_translator* trans = ev.trans->clone();
    profile->set_mapping(ev.name, trans, ev.type);
  }

  for (auto opt : options) {
    profile->set_option(opt.first, opt.second.value);
  }

  for (auto entry : adv_trans) {
    profile->set_advanced(*entry.second.fields, entry.second.trans->clone());
  }
}


void input_source::thread_loop() {
  struct epoll_event event;
  struct epoll_event events[1];
  memset(&event, 0, sizeof(event));

  while ((keep_looping)) {
    int n = epoll_wait(epfd, events, 1, -1);
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n < 0 && errno != EINTR) {
      perror("epoll wait:");
      break;
    }
    if (events[0].data.ptr == this) {
      struct pass_trans msg;
      int ret = 1;
      ret = read(internalpipe, &msg, sizeof(msg));
      if (ret == sizeof(msg)) {
        if (msg.adv.fields && !msg.adv.fields->empty()) {
          auto its = msg.adv.fields->begin();
          std::string adv_name = *its;
          its++;
          for (; its != msg.adv.fields->end(); its++) {
            adv_name += "," + (*its);
          }


          auto it = adv_trans.find(adv_name);
          if (it != adv_trans.end()) {
            delete it->second.fields;
            delete it->second.trans;
            adv_trans.erase(it);
          }
          if (msg.adv.trans) {
            msg.adv.trans->attach(this);
            adv_trans[adv_name] = {msg.adv.fields, msg.adv.trans};
          } else {
            delete msg.adv.fields;
          }

          continue;
        }
        if (msg.id < 0) continue;

        event_translator** trans = &(this->events.at(msg.id).trans);
        delete *trans;
        *(trans) = msg.trans;

      }
    } else {
      process(events[0].data.ptr);
    }
  }
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

#include "device.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <iostream>

std::mutex device_delete_lock;


input_source::input_source(slot_manager* slot_man) : slot_man(slot_man){
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
  
  for (auto it : chords) {
    if (it.second) delete it.second;
  }
  
  if (out_dev) { slot_man->remove(this); };
}


void input_source::register_event(source_event ev) {
  if (!ev.trans) ev.trans = new event_translator();
  events.push_back(ev);
}

void input_source::register_option(source_option opt) {
  
  options.insert(std::pair<std::string,source_option>(opt.name,opt));
}


void input_source::watch_file(int fd, void* tag) {
  if (fd <= 0) return;
  struct epoll_event event;
  memset(&event,0,sizeof(event));

  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  event.data.ptr = tag;
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
  if (ret< 0) perror("epoll add");
}


void input_source::update_map(const char* evname, event_translator* trans) {
  for (int i = 0; i < events.size(); i++) {
    if (!strcmp(evname,events[i].name)) {
      set_trans(i,trans->clone());
      return;
    }
  }
}

void input_source::update_option(const char* name, const char* value) {
  std::string sname = std::string(name);
  std::string svalue = std::string(value);
  auto it = options.find(sname);
  if (it != options.end()) {
    if (!process_option(name,value)) {
      it->second.value = svalue;
    } else {
      //Device rejected the option.
    }
  }
}

void input_source::list_options(std::vector<source_option> &list) {
  for (auto e : options) {
    list.push_back(e.second);
  }
}

struct pass_trans {
  int id;
  int id2; //for chords.
  event_translator* trans;
};

void input_source::update_chord(const char* evname1,const char* evname2, event_translator* trans) {
  int id1 = -1;
  int id2 = -1;
  for (int i = 0; i < events.size(); i++) {
    if (!strcmp(evname1,events[i].name)) {
      id1 = i;
      break;
    }
  }
  for (int i = 0; i < events.size(); i++) {
    if (!strcmp(evname2,events[i].name)) {
      id2 = i;
      break;
    }
  }
  
  if (id1 != -1 && id2 != -1 && id1 != id2) {
    if (trans) {
      std::cout << name << " doing chord " << evname1 << "+" << evname2 << "->" << trans->to_string() << std::endl;
    } else {
      std::cout << name << " clearing chord " << evname1 << "+" << evname2 << std::endl;
    }
    struct pass_trans msg;
    memset(&msg,0,sizeof(msg));
    msg.id = id1;
    msg.id2 = id2;
    msg.trans = nullptr;
    if (trans) msg.trans = trans->clone();
    write(priv_pipe,&msg,sizeof(msg));
  }
    
}



void input_source::set_trans(int id, event_translator* trans) {
  if (id < 0 || id >= events.size()) return;
  
  struct pass_trans msg;
  memset(&msg,0,sizeof(msg));
  msg.id = id;
  msg.id2 = -1;
  msg.trans = trans;
  write(priv_pipe,&msg,sizeof(msg));
};


void input_source::send_value(int id, long long value) {
  for (auto adv_trans : events.at(id).attached)
    if (adv_trans->claim_event(id,{value})) return;
  events.at(id).value = value;
  
  if (events.at(id).trans && out_dev) events.at(id).trans->process({value},out_dev);
  
  //On a key press, try to claim a slot if we don't have one. 
  if (!out_dev && events.at(id).type == DEV_KEY) { slot_man->request_slot(this); };
  process_chords();
  
}

void input_source::load_profile(profile* profile) {
  for (auto ev : events) {
    int id = ev.id;
    event_translator* trans = profile->copy_mapping(ev.name);
    if (trans) {
      set_trans(id,trans);
    }
  }
  for (auto chord : profile->chords) {
    std::string first = chord.first.first;
    std::string second = chord.first.second;
    update_chord(first.c_str(),second.c_str(),chord.second->clone());
  }
  for (auto opt : options) {
    std::string value = profile->get_option(opt.first);
    if (!value.empty()) {
      update_option(opt.first.c_str(),value.c_str());
    }
  }
    
}

void input_source::export_profile(profile* profile) {
  for (auto ev : events) {
    event_translator* trans = ev.trans->clone();
    profile->set_mapping(ev.name,trans);
  }
  for (auto chord : chords) {
    int id1 = chord.first.first;
    int id2 = chord.first.second;
    event_translator* trans = chord.second->clone();
    profile->set_chord(events[id1].name,events[id2].name,trans);
  }
  for (auto opt : options) {
    profile->set_option(opt.first,opt.second.value);
  }
}

  
void input_source::thread_loop() {
  struct epoll_event event;
  struct epoll_event events[1];
  memset(&event,0,sizeof(event));

  while ((keep_looping)) {
    int n = epoll_wait(epfd, events, 1, -1);
    if (n < 0 && errno == EINTR) { continue;}
    if (n < 0 && errno != EINTR) {perror("epoll wait:");break;}
    if (events[0].data.ptr == this) {
      struct pass_trans msg;
      int ret = 1;
      ret = read(internalpipe,&msg,sizeof(msg));
      if (ret == sizeof(msg)) {
        if (msg.id < 0 ) continue;
        if (msg.id2 < 0) {
          event_translator** trans = &(this->events.at(msg.id).trans);
          delete *trans;
          *(trans) = msg.trans;
        } else {
          std::pair<int,int> keypair(msg.id,msg.id2);
          auto it = chords.find(std::pair<int,int>(msg.id,msg.id2));
          if (it != chords.end()) {
            delete (*it).second;
            chords.erase(it);
          }
          if (msg.trans)
            chords.insert(std::pair<std::pair<int,int>,event_translator*>(keypair,msg.trans));
        }
        
      }
    } else {
      process(events[0].data.ptr);
    }
  }
}

void input_source::process_chords() {
  for (auto chord : chords) {
    int id1 = chord.first.first;
    int id2 = chord.first.second;
    event_translator* trans = chord.second;
    bool chordvalue = (events.at(id1).value && events.at(id2).value);
    trans->process({chordvalue},out_dev);
  }
}
  
void input_source::start_thread() {
  keep_looping = true;
  
  thread = new std::thread(&input_source::thread_loop, this);
}


void input_source::end_thread() {
  if(thread) {
    keep_looping = false;
    write(priv_pipe,"h",sizeof(char));
    thread->join();
    delete thread;
    thread = nullptr;
  }
}
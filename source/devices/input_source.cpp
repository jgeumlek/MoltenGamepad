#include "device.h"
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <iostream>

input_source::input_source(){
    epfd = epoll_create(1);
    if (epfd < 1) perror("epoll create");
    
}

input_source::~input_source() {
  end_thread();
}


void input_source::register_event(source_event ev) {
  events.push_back(ev);
}


void input_source::watch_file(int fd, void* tag) {
  struct epoll_event event;
  memset(&event,0,sizeof(event));

  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  event.data.ptr = tag;
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
  if (ret< 0) perror("epoll add");
}


void input_source::update_map(const char* evname, event_translator* trans) {
  for (int i = 0; i < events.size(); i++) {
    if (!strcmp(evname,events[i].name)) set_trans(i,trans);
  }
}

struct pass_trans {
  int id;
  event_translator* trans;
};

void input_source::set_trans(int id, event_translator* trans) {
  if (id < 0 || id >= events.size()) return;
  
  struct pass_trans msg;
  memset(&msg,0,sizeof(msg));
  msg.id = id;
  msg.trans = trans;
  write(priv_pipe,&msg,sizeof(msg));
};


void input_source::send_value(int id, long long value) {
  events.at(id).value = value;
  
  if (events.at(id).trans && out_dev) events.at(id).trans->process({value},out_dev);
  
}
  
void input_source::thread_loop() {
  struct epoll_event event;
  struct epoll_event events[1];
  memset(&event,0,sizeof(event));

  
  int internal[2];
  pipe(internal);
  event.data.ptr = this;
  watch_file(internal[0], this);
  
  priv_pipe = internal[1];
  

  while ((keep_looping)) {
    int n = epoll_wait(epfd, events, 1, -1);
    if (n < 0) {perror("epoll wait:");break;}
    if (events[0].data.ptr == this) {
      struct pass_trans msg;
      int ret = 1;
      ret = read(internal[0],&msg,sizeof(msg));
      if (ret == sizeof(msg)) {
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
  if(thread) {
    keep_looping = false;
    write(priv_pipe,"h",sizeof(char));
    thread->join();
    delete thread;
    thread = nullptr;
  }
}
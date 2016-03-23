#ifndef MESSAGES_H
#define MESSAGES_H
#include <sstream>
#include <vector>
#include <string>
#include <mutex>
#include "oscpkt/oscpkt.hh"

class message_stream {
public:
  enum listen_type {PLAINTEXT, OSC};
  std::string name;
  message_stream(std::string name) : name(name) {};
  virtual void take_message(std::string text) = 0;
  virtual void add_listener(int fd) = 0;
  virtual void add_listener(int fd, listen_type type) = 0;
  virtual void remove_listener(int fd) = 0;
};

class simple_messenger : message_stream {
  std::vector<int> text_fds;
  std::vector<int> osc_fds;
  std::ostringstream buffer;
  std::mutex lock;
public:
  simple_messenger(std::string name) : message_stream(name) {};
  virtual void take_message(std::string text);
  virtual void add_listener(int fd);
  virtual void add_listener(int fd, listen_type type);
  
  virtual void remove_listener(int fd);
};
#endif

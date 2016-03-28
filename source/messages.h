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
  virtual void text(const std::string& text) = 0;
  virtual void message(const std::string& text, oscpkt::PacketWriter& packet) = 0;
  virtual void add_listener(int fd) = 0;
  virtual void add_listener(int fd, listen_type type) = 0;
  virtual void remove_listener(int fd) = 0;
};

class simple_messenger : public message_stream {
protected:
  std::vector<int> text_fds;
  std::vector<int> osc_fds;
  std::mutex lock;
public:
  simple_messenger(std::string name) : message_stream(name) {};
  virtual void text(const std::string& text);
  virtual void message(const std::string& text, oscpkt::PacketWriter& packet);
  virtual void add_listener(int fd);
  virtual void add_listener(int fd, listen_type type);
  
  virtual void remove_listener(int fd);
};
#endif

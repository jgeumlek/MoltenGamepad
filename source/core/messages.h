#ifndef MESSAGES_H
#define MESSAGES_H
#include <sstream>
#include <vector>
#include <string>
#include <mutex>

class message_stream {
public:
  std::string name;
  message_stream(std::string name) : name(name) {};
  virtual void take_message(std::string text) = 0;
  virtual void add_listener(int fd) = 0;
  virtual void remove_listener(int fd) = 0;
  virtual void flush() = 0;
};

class simple_messenger : message_stream {
  std::vector<int> fds;
  std::ostringstream buffer;
  std::mutex lock;
public:
  simple_messenger(std::string name) : message_stream(name) {};
  virtual void take_message(std::string text);
  virtual void add_listener(int fd);
  virtual void remove_listener(int fd);
  virtual void flush();
private:
  void flush_buffer();
};
#endif

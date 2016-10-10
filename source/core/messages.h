#ifndef MESSAGES_H
#define MESSAGES_H
#include <sstream>
#include <vector>
#include <string>
#include <mutex>

class message_protocol;

class message_stream {
  std::vector<message_protocol*> listeners;
  std::mutex lock;
public:
  std::string name;
  message_stream(std::string name) : name(name) {};
  virtual void add_listener(message_protocol* listener);
  virtual void remove_listener(message_protocol* listener);
  virtual void flush();

  virtual void take_message(std::string text);
  virtual void print(std::string text);
  virtual void err(std::string text, std::string path, int line_number);
  virtual void err(std::string text);

};

#endif

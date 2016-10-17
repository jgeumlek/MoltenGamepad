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

#define DEBUG_NONE 0
#define DEBUG_INFO 5
#define DEBUG_VERBOSE 10
extern int DEBUG_LEVELS[];
extern int* DEBUG_LEVEL;
//pass the number of args, and then that number of strings aftwerwards to print.
//No newline is necessary.
void debug_print(int level, int num_args...);

#endif

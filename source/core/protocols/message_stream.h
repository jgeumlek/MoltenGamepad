#ifndef MESSAGES_H
#define MESSAGES_H
#include <sstream>
#include <vector>
#include <string>
#include <mutex>
#include "debug_messages.h"

//For connections to another process, the client gives us a response id
//with every request. Output related to that request is sent back with the same response id.

//For output generated without a controlling client request (i.e. it has no provided response
// id), a response id of 0 is used.

class message_protocol;

class response_stream;
class input_source;
class output_slot;

class message_stream {
  std::vector<message_protocol*> listeners;
  std::mutex lock;
public:
  std::string name;
  message_stream(std::string name) : name(name) {};
  virtual void add_listener(message_protocol* listener);
  virtual void remove_listener(message_protocol* listener);
  virtual void flush();

  virtual void take_message(int resp_id, std::string text);
  virtual void print(int resp_id, std::string text);
  virtual void err(int resp_id, std::string text, std::string path, int line_number);
  virtual void err(int resp_id, std::string text);
  virtual void device_slot(int resp_id, input_source* device, output_slot* slot);
  virtual void device_plug(int resp_id, input_source* device, std::string action);

  virtual void end_response(int resp_id, int ret_val);
};


class response_stream {
  int response_id;
  message_stream* stream;
public:
  response_stream(int id, message_stream* stream) : response_id(id), stream(stream) {};
  void take_message(std::string text);
  void print(std::string text);
  void err(std::string text, std::string path, int line_number);
  void err(std::string text);
  void device_slot(input_source* device, output_slot* slot);
  void device_plug(input_source* device, std::string action);

  void end_response(int ret_val);
};

#endif

#pragma once

#include "../protocols.h"
class socket_out : public message_protocol {
public:
  int fd;
  socket_out(int fd) : fd(fd) {};
  virtual void text_message(int resp_id, const std::string& text);
  virtual void err(int resp_id, const std::string& text, const std::string& path, int line_number);
  virtual void err(int resp_id, const std::string& text);

  virtual void end_response(int resp_id, int ret_val);
  virtual ~socket_out() {};
};

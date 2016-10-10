#pragma once
#include <string>

class message_protocol {
public:
  virtual void text_message(int resp_id, const std::string& text) = 0;
  virtual void err(int resp_id, const std::string& text, const std::string& path, int line_number) = 0;
  virtual void err(int resp_id, const std::string& text) = 0;
  virtual ~message_protocol() {};
};

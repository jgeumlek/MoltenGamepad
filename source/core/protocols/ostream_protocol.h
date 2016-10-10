#pragma once
#include <iostream>
#include "../protocols.h"
class ostream_protocol : public message_protocol {
public:
  std::ostream& out;
  std::ostream& stderr;
  ostream_protocol(std::ostream& out, std::ostream& err) : out(out), stderr(err) {};
  virtual void text_message(int resp_id, const std::string& text);
  virtual void err(int resp_id, const std::string& text, const std::string& path, int line_number);
  virtual void err(int resp_id, const std::string& text);
  virtual ~ostream_protocol() {};
};

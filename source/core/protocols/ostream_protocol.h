#pragma once
#include <iostream>
#include "protocols.h"
class ostream_protocol : public message_protocol {
public:
  std::ostream& out;
  std::ostream& stderr;
  ostream_protocol(std::ostream& out, std::ostream& err) : out(out), stderr(err) {};
  virtual void text_message(int resp_id, const std::string& text);
  virtual void err(int resp_id, const std::string& text, const std::string& path, int line_number);
  virtual void err(int resp_id, const std::string& text);
  virtual void device_slot(int resp_id, input_source* device, output_slot* slot);
  virtual void device_plug(int resp_id, input_source* device, std::string action);

  virtual void end_response(int resp_id, int ret_val) {}; //ignored for ostream.
  virtual ~ostream_protocol() {};
};

#pragma once
#include <string>

class input_source;
class virtual_device;
class output_slot;

class message_protocol {
public:
  virtual void text_message(int resp_id, const std::string& text) {};
  virtual void err(int resp_id, const std::string& text, const std::string& path, int line_number) {};
  virtual void err(int resp_id, const std::string& text) {};
  virtual void device_slot(int resp_id, input_source* device, virtual_device* slot) {};
  virtual void device_plug(int resp_id, input_source* device, std::string action) {};
  virtual void slot_event(int resp_id, output_slot* slot, std::string action) {};
  virtual void end_response(int resp_id, int ret_val) {};
  virtual ~message_protocol() {};
};

#include "ostream_protocol.h"
#include "../devices/device.h"
#include "../virtual_devices/virtual_device.h"

void ostream_protocol::text_message(int resp_id, const std::string& text) {
  //This protocol does not use response ids.
  out << text << std::endl;
}

void ostream_protocol::err(int resp_id, const std::string& text, const std::string& path, int line_number) {
  stderr << text << " (" << path << ":" << std::to_string(line_number) << ")" << std::endl;
}
void ostream_protocol::err(int resp_id, const std::string& text) {
  stderr << text << std::endl;
}

void ostream_protocol::device_slot(int resp_id, input_source* device, virtual_device* slot) {
  if (slot) {
    out << "slot: " << device->get_name() << " assigned to slot " << slot->name << std::endl;
  } else {
    out << "slot: " << device->get_name() << " not assigned to any slot" << std::endl;
  }
}

void ostream_protocol::device_plug(int resp_id, input_source* device, std::string action) {
  if (action == "add") {
    out << "plug: " << device->get_name() << " added" << std::endl;
  } else if (action == "remove") {
    out << "plug: " << device->get_name() << " removed" << std::endl;
  }
}

void ostream_protocol::slot_event(int resp_id, output_slot* slot, std::string action) {
  if (action == "open") {
    out << "slot: " << slot->name << " opened virtual device" << std::endl;
  } else if (action == "close") {
    out << "slot: " << slot->name << " closed virtual device" << std::endl;
  }
};

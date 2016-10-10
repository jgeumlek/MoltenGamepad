#include "ostream_protocol.h"

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

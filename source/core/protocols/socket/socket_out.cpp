#include "socket_out.h"
#include "../../../oscpkt/oscpkt.hh"
#include <unistd.h>

void socket_out::text_message(int resp_id, const std::string& text) {
  oscpkt::PacketWriter pw;
  oscpkt::Message msg;
  msg.init("/text").pushInt32(resp_id).pushStr(text);
  pw.init().addMessage(msg);
  uint32_t size = pw.packetSize();
  std::lock_guard<std::mutex> guard(write_lock);
  write(fd, &size, sizeof(size));
  write(fd, pw.packetData(), size);
}

void socket_out::err(int resp_id, const std::string& text, const std::string& path, int line_number) {
  oscpkt::PacketWriter pw;
  oscpkt::Message msg;
  msg.init("/error").pushInt32(resp_id).pushStr(text).pushStr(path).pushInt32(line_number);
  pw.init().addMessage(msg);
  uint32_t size = pw.packetSize();
  std::lock_guard<std::mutex> guard(write_lock);
  write(fd, &size, sizeof(size));
  write(fd, pw.packetData(), size);
}
void socket_out::err(int resp_id, const std::string& text) {
  oscpkt::PacketWriter pw;
  oscpkt::Message msg;
  msg.init("/error").pushInt32(resp_id).pushStr(text).pushStr("").pushInt32(-1);
  pw.init().addMessage(msg);
  uint32_t size = pw.packetSize();
  std::lock_guard<std::mutex> guard(write_lock);
  write(fd, &size, sizeof(size));
  write(fd, pw.packetData(), size);
}

void socket_out::end_response(int resp_id, int ret_val) {
  oscpkt::PacketWriter pw;
  oscpkt::Message msg;
  msg.init("/done").pushInt32(resp_id).pushInt32(ret_val);
  pw.init().addMessage(msg);
  uint32_t size = pw.packetSize();
  std::lock_guard<std::mutex> guard(write_lock);
  write(fd, &size, sizeof(size));
  write(fd, pw.packetData(), size);
}

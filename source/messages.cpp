#include "messages.h"
#include <unistd.h>
void simple_messenger::text(const std::string& text) {
  if (text.empty()) return;
  oscpkt::PacketWriter pw;
  std::stringstream buffer;
  buffer << name << ": " << text << std::endl;

  if (!osc_fds.empty()) {
    oscpkt::Message msg;
    msg.init("/text").pushStr(name).pushStr(text);
    pw.init().addMessage(msg);
  }
  message(buffer.str(), pw);
}

void simple_messenger::message(const std::string& text, oscpkt::PacketWriter& packet) {
  lock.lock();
  int len = text.size();

  for (int fd : text_fds) {
    write(fd, text.c_str(), len);
  }

  if (!osc_fds.empty() && packet.isOk()) {
    uint32_t size = packet.packetSize();
    for (int fd : osc_fds) {
      //TODO: Make this size+packet write atomic.
      write(fd,&size,sizeof(size));
      write(fd,packet.packetData(),size);
    }
  }
  lock.unlock();
}

void simple_messenger::add_listener(int fd) {
  lock.lock();
  text_fds.push_back(fd);
  lock.unlock();
}

void simple_messenger::add_listener(int fd, listen_type type) {
  lock.lock();
  if (type == PLAINTEXT)
    text_fds.push_back(fd);
  if (type == OSC)
    osc_fds.push_back(fd);
  lock.unlock();
}

void simple_messenger::remove_listener(int fd) {
  lock.lock();
  for (auto it = text_fds.begin(); it != text_fds.end(); it++) {
    while (it != text_fds.end() && *it == fd) {
      text_fds.erase(it);
    }
  }
  for (auto it = osc_fds.begin(); it != osc_fds.end(); it++) {
    while (it != osc_fds.end() && *it == fd) {
      osc_fds.erase(it);
    }
  }
  lock.unlock();
}

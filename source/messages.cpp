#include "messages.h"
#include <unistd.h>
void simple_messenger::take_message(std::string text) {
  if (text.empty()) return;
  lock.lock();
  buffer << name << ": " << text << std::endl;
  std::string str = buffer.str();
  int len = str.size();

  for (int fd : text_fds) {
    write(fd, str.c_str(), len);
  }

  if (!osc_fds.empty()) {
    oscpkt::PacketWriter pw;
    oscpkt::Message msg;
    msg.init("/text").pushStr(name).pushStr(text);
    pw.init().addMessage(msg);
    uint32_t size = pw.packetSize();
    for (int fd : osc_fds) {
      write(fd,&size,sizeof(size));
      write(fd,pw.packetData(),size);
    }
  }
  buffer.str("");
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

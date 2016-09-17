#include "messages.h"
#include <unistd.h>
void simple_messenger::take_message(std::string text) {
  lock.lock();
  buffer << name << ": " << text << std::endl;
  flush_buffer();
  lock.unlock();
}

void simple_messenger::add_listener(int fd) {
  lock.lock();
  fds.push_back(fd);
  lock.unlock();
}

void simple_messenger::remove_listener(int fd) {
  lock.lock();
  for (auto it = fds.begin(); it != fds.end(); it++) {
    while (it != fds.end() && *it == fd) {
      fds.erase(it);
    }
  }
  lock.unlock();
}
void simple_messenger::flush() {
  lock.lock();
  flush_buffer();
  lock.unlock();
}

void simple_messenger::flush_buffer() {
 std::string str = buffer.str();
 int len = str.size();
 if (len == 0)
   return;

 for (int fd : fds) {
   write(fd, str.c_str(), len);
 }
 buffer.str("");
}

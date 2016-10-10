#include "messages.h"
#include "protocols.h"
#include <unistd.h>

void message_stream::add_listener(message_protocol* listener) {
  lock.lock();
  listeners.push_back(listener);
  lock.unlock();
}

void message_stream::remove_listener(message_protocol* listener) {
  lock.lock();
  for (auto it = listeners.begin(); it != listeners.end(); it++) {
    while (it != listeners.end() && *it == listener) {
      listeners.erase(it);
    }
  }
  lock.unlock();
}
void message_stream::flush() {
}

void message_stream::take_message(std::string text) {
  lock.lock();
  std::ostringstream buffer;
  buffer << name << ": " << text;
  for (auto listener : listeners)
    listener->text_message(0,buffer.str());
  lock.unlock();
}

void message_stream::print(std::string text) {
  lock.lock();
  for (auto listener : listeners)
    listener->text_message(0,text);
  lock.unlock();
}

void message_stream::err(std::string text, std::string path, int line_number) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->err(0, text, path, line_number);
}

void message_stream::err(std::string text) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->err(0, text);
}

#include "message_stream.h"
#include "protocols.h"
#include <unistd.h>
#include <cstdarg>
#include <iostream>

void message_stream::add_listener(message_protocol* listener) {
  lock.lock();
  listeners.push_back(listener);
  lock.unlock();
}

void message_stream::remove_listener(message_protocol* listener) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto it = listeners.begin(); it != listeners.end(); it++) {
    if (*it == listener) {
      listeners.erase(it);
      return;
    }
  }
}
void message_stream::flush() {
}

void message_stream::take_message(int resp_id, std::string text) {
  lock.lock();
  std::ostringstream buffer;
  buffer << name << ": " << text;
  for (auto listener : listeners)
    listener->text_message(resp_id,buffer.str());
  lock.unlock();
}

void message_stream::print(int resp_id, std::string text) {
  lock.lock();
  for (auto listener : listeners)
    listener->text_message(resp_id,text);
  lock.unlock();
}

void message_stream::err(int resp_id, std::string text, std::string path, int line_number) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->err(resp_id, (name.empty() ? "" : name + ": ") + text, path, line_number);
}

void message_stream::err(int resp_id, std::string text) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->err(resp_id, (name.empty() ? "" : name + ": ") + text);
}

void message_stream::device_slot(int resp_id, input_source* device, output_slot* slot) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->device_slot(resp_id, device, slot);
}

void message_stream::device_plug(int resp_id, input_source* device, std::string action) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->device_plug(resp_id, device, action);
}

void message_stream::end_response(int resp_id, int ret_val) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto listener : listeners)
    listener->end_response(resp_id, ret_val);
}

void response_stream::take_message(std::string text) {
  stream->take_message(response_id,text);
}

void response_stream::print(std::string text) {
  stream->print(response_id,text);
}

void response_stream::err(std::string text, std::string path, int line_number) {
  stream->err(response_id,text,path,line_number);
}

void response_stream::device_slot(input_source* device, output_slot* slot) {
  stream->device_slot(response_id,device,slot);
}

void response_stream::device_plug(input_source* device, std::string action) {
  stream->device_plug(response_id,device,action);
}

void response_stream::err(std::string text) {
  stream->err(response_id,text);
}

void response_stream::end_response(int ret_val) {
  stream->end_response(response_id,ret_val);
}

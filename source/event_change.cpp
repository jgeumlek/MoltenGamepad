#include "event_change.h"
#include "devices/device.h"

#include <iostream>
#include <unistd.h>

void simple_chord::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();
  for (auto name : event_names) {
    event_ids.push_back(-1);
    event_vals.push_back(0);
  }
  for (auto event : events) {
    for (int i = 0; i < event_names.size(); i++) {
      if (!strcmp(event.name,event_names[i].c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
      }
    }
  }
  
  out_dev_ptr = &(source->out_dev);
};

void simple_chord::attach(input_source* source) {
  
  for (int id : event_ids)
    source->add_listener(id,this);
  
  this->source = source;
};

simple_chord::~simple_chord() {
  if (source) {
    for (int id : event_ids) {
      source->remove_listener(id,this);
    }
  }
  if (out_trans) delete out_trans;
}

bool simple_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  for (int i = 0; i < event_ids.size(); i++) {
    if (id == event_ids[i]) {
      event_vals[i] = event.value;
    }
    output = output && (event_vals[i]);
  }
  if (output != output_cache) {
    out_trans->process({output},*out_dev_ptr);
    output_cache = output;
  }

  return false;
};

bool exclusive_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  int index;
  int old_val;
  

  for (int i = 0; i < event_ids.size(); i++) {
    if (id == event_ids[i]) {
      index = i;
      old_val = event_vals[i];
      event_vals[i] = event.value;
      chord_hits[i] = event.value;
    }
    output = output && (chord_hits[i]);
  }
  //TODO: Fix success->one off-> false success. (rename: chord_hits...)
  
   //if not thread, start thread.
  if (!thread && event.value && !old_val) {
    thread_active = true;
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    chord_hits[index] = event.value;
    thread = new std::thread(&exclusive_chord::thread_func,this);
    thread->detach();
  }
  
  if (output && output != output_cache) {
    //chord succeeded. Send event only if thread hasn't timed out.
    thread_active = false;
    if (thread) out_trans->process({output},*out_dev_ptr);
 
    output_cache = output;
  }
  if (!output && output != output_cache) {
    //chord released. clear out everything.
    out_trans->process({output},*out_dev_ptr);
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    output_cache = output;
  }
  //If key up, let it pass.
  //If we have no thread and our output says we failed, let it pass.
  if (!event.value || (!output_cache && !thread_active)) return false; //Pass along key up events.
  
  //We have a thread still going, or we hit a chord claiming this event.
  
  return true;
};

void exclusive_chord::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();
  for (auto name : event_names) {
    event_ids.push_back(-1);
    event_vals.push_back(0);
    chord_hits.push_back(0);
  }
  for (auto event : events) {
    for (int i = 0; i < event_names.size(); i++) {
      if (!strcmp(event.name,event_names[i].c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
      }
    }
  }
  
  out_dev_ptr = &(source->out_dev);
};

void exclusive_chord::thread_func() {
  //sleep
  usleep(20000);
  //If we are still active, fire the event.
  if (thread_active) {
    //send out events
    thread_active = false;
    for (int i = 0; i < event_ids.size(); i++) {
      if (chord_hits[i]) source->send_value(event_ids[i], event_vals[i]);
      chord_hits[i] = 0;
    }
  }
    delete thread;
    thread = nullptr;
}

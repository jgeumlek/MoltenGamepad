#include "virtual_device.h"
#include "../uinput.h"
#include "../devices/device.h"
#include <linux/uinput.h>


virtual_device::~virtual_device() {
  // close_virt_device() must be called in derived classes! Unavailble within base.
  // also, the base class has no actual virt devices to close...
}

bool virtual_device::remove_device(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);

  uint num_devices = devices.size();
  bool found_it = false;
  for (auto it = devices.begin(); it != devices.end();) {
    auto ptr = it->lock();
    while (!ptr) {
      it = devices.erase(it);
      if (it == devices.end())
        break;
      ptr = it->lock();
    }
    if (it == devices.end())
      break;
    if (ptr.get() == dev) {
      it = devices.erase(it);
      found_it = true;
    } else {
      it++;
    }
  }
  if (num_devices > 0 && devices.size() == 0)
        process_becoming_empty();
  return found_it;
}

bool virtual_device::accept_device(std::shared_ptr<input_source> dev) {
  std::lock_guard<std::mutex> guard(lock);

  //Accept unless we already have a device of this type.
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    if (ptr && ptr->get_type() == dev->get_type()) {
      return false;
    }
  }
  return true;
}

bool virtual_device::add_device(std::shared_ptr<input_source> dev) {
  std::lock_guard<std::mutex> guard(lock);
  devices.push_back(dev);
  if (effects[0].id != -1)
    dev->upload_ff(effects[0]);
  return true;
}

void virtual_device::clear_outputs() {
  //Could be optimized to only send events relevant to a device.
  //This would require devices to keep lists of their relevant events.
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.value = 0;
  for (out_ev.code = 0; out_ev.code < KEY_MAX; out_ev.code++)
    take_event(out_ev);

  out_ev.type = EV_ABS;
  for (out_ev.code = 0; out_ev.code < ABS_MAX; out_ev.code++)
    take_event(out_ev);

  out_ev.type = EV_REL;
  for (out_ev.code = 0; out_ev.code < REL_MAX; out_ev.code++)
    take_event(out_ev);
}

bool virtual_device::close_virt_device() {
  bool success = true;
  lock.lock();
  //check for ff effect
  if (effects[0].id != -1) {
    //Bad stuff might happen with uinput module.
    success = false;
  } else {
    //close/destroy the uinput device -- device type specific.
    destroy_uinput_devs();
  }

  lock.unlock();

  return success;
}

int virtual_device::upload_ff(const ff_effect& effect) {
  std::lock_guard<std::mutex> guard(lock);
  if (effect.id == -1 && effects[0].id != -1)
    return -1;
  effects[0] = effect;
  self_ref_to_prevent_deletion_with_ff = this->shared_from_this();
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    ptr->upload_ff(effect);
  }
  return 0; //return an id for this event.
}

int virtual_device::erase_ff(int id) {
  std::lock_guard<std::mutex> guard(lock);
  if (id != 0)
    return FAILURE;
  effects[0].id = -1;
  self_ref_to_prevent_deletion_with_ff = nullptr;
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    ptr->erase_ff(id);
  }
  return SUCCESS; //return an id for this event.
}

int virtual_device::play_ff(int id, int repetitions) {
  std::lock_guard<std::mutex> guard(lock);
  if (id != 0 || effects[0].id == -1)
    return FAILURE;
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    ptr->play_ff(id, repetitions);
  }
  return SUCCESS;
}

void virtual_device::send_start_press(int milliseconds) {
  input_event in;
  memset(&in, 0, sizeof(input_event));
  in.type = EV_KEY;
  in.code = BTN_START;
  in.value = 1;
  take_event(in);
  in.value = 0;
  take_delayed_event(in, milliseconds);
  in.type = EV_SYN;
  in.code = SYN_REPORT;
  take_event(in);
}

void virtual_device::process_becoming_empty() {
  //clear the output events
  clear_outputs();
  //check if we should send a fake start press.
  if (slot_man->press_start_on_any_disconnect) {
    send_start_press(slot_man->start_press_milliseconds);
  }
  //we don't handle press_start_on_last_disconnect here.
  //That requires info we don't have here. (e.g. is this input source just moving slots?)

}

uint virtual_device::input_source_count() {
  return devices.size();
}

void virtual_device::take_delayed_event(struct input_event in, int milliseconds) {
  timespec now;
  memset(&now,0,sizeof(timespec));
  if (milliseconds < 0)
    milliseconds = 0;
  if (milliseconds > 10000)
    milliseconds = 10000;
  std::lock_guard<std::mutex> guard(delayed_events_lock);
  clock_gettime(CLOCK_MONOTONIC, &now);
  in.time.tv_sec = now.tv_sec;
  in.time.tv_usec = now.tv_nsec/1000 + milliseconds*1000;
  while (in.time.tv_usec > 1000000) {
    in.time.tv_usec -= 1000000;
    in.time.tv_sec++;
  }
  delayed_events.push_back(in);
}

void virtual_device::check_delayed_events() {
  std::lock_guard<std::mutex> guard(delayed_events_lock);
  if (delayed_events.empty())
    return;
  timespec now;
  memset(&now,0,sizeof(timespec));
  clock_gettime(CLOCK_MONOTONIC, &now);
  bool had_event = false;
  for (uint i = 0; i < delayed_events.size(); i++) {
    const input_event& event = delayed_events[i];
    if (now.tv_sec > event.time.tv_sec || ( (now.tv_sec == event.time.tv_sec) && (now.tv_nsec/1000 <= event.time.tv_usec))) {
      take_event(event);
      delayed_events.erase(delayed_events.begin() + i);
      i--;
      had_event = true;
    }
  }
  if (had_event) {
    input_event report;
    memset(&report, 0, sizeof(report));
    report.type = EV_SYN;
    report.code = SYN_REPORT;
    take_event(report);
  }
}

void print_effect(ff_effect& effect) {
  std::cout << " type:"<< effect.type;
  std::cout << " id:"<< effect.id;
  std::cout << " d:"<< effect.direction;
  std::cout << " type:"<< effect.type;
  std::cout << " trigger:"<< effect.trigger.button << "," << effect.trigger.interval;
  std::cout << " play:"<< effect.replay.length << "," << effect.replay.delay << std::endl;
}

void virtual_device::ref() {
  std::lock_guard<std::mutex> guard(lock);
  extra_refs.push_back(shared_from_this());
}

void virtual_device::unref() {
  std::lock_guard<std::mutex> guard(lock);
  if (!extra_refs.empty())
    extra_refs.pop_back();
}


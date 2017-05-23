#include "virtual_device.h"
#include "../uinput.h"
#include "../devices/device.h"
#include <linux/uinput.h>
#include <csignal>

virtual_device::~virtual_device() {
}

bool virtual_device::remove_device(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    while (!ptr) {
      it = devices.erase(it);
      if (it == devices.end()) return false;
      ptr = it->lock();
    }
    if (ptr.get() == dev) {
      devices.erase(it);
      return true;
    }
  }
  return false;
}

bool virtual_device::accept_device(std::shared_ptr<input_source> dev) {
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
void noop_signal_handler(int signum) {
  //HACK: Assumes we only close_virt_device at the very end of process lifespan.
  return;
}
void virtual_device::close_virt_device() {
  do {
    signal(SIGINT, noop_signal_handler);
    signal(SIGTERM, noop_signal_handler);
    lock.lock();
    //check for ff effect
    if (effects[0].id != -1) {
      std::cerr << "A virtual device cannot be closed until its force-feedback effects have been erased." << std::endl;
      lock.unlock();
      sleep(2);
      continue;
      //Bad stuff might happen with uinput module. Just keep looping.
    }
    //close/destroy the uinput device -- device type specific.
    destroy_uinput_devs();
    
    lock.unlock();
    return;  //Everything is fine, break the loop.
  } while (true);
}

int virtual_device::upload_ff(const ff_effect& effect) {
  std::lock_guard<std::mutex> guard(lock);
  if (effect.id == -1 && effects[0].id != -1)
    return -1;
  effects[0] = effect;
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






void print_effect(ff_effect& effect) {
  std::cout << " type:"<< effect.type;
  std::cout << " id:"<< effect.id;
  std::cout << " d:"<< effect.direction;
  std::cout << " type:"<< effect.type;
  std::cout << " trigger:"<< effect.trigger.button << "," << effect.trigger.interval;
  std::cout << " play:"<< effect.replay.length << "," << effect.replay.delay << std::endl;
}



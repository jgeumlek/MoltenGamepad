#include "output_slot.h"
#include "uinput.h"
#include "devices/device.h"
#include <linux/uinput.h>
#include <csignal>

output_slot::~output_slot() {
}

bool output_slot::remove_device(input_source* dev) {
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

bool output_slot::accept_device(std::shared_ptr<input_source> dev) {
  return true;
}

bool output_slot::add_device(std::shared_ptr<input_source> dev) {
  std::lock_guard<std::mutex> guard(lock);
  devices.push_back(dev);
  if (effects[0].id != -1)
    dev->upload_ff(effects[0]);
  return true;
}

void output_slot::clear_outputs() {
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
void output_slot::close_virt_device() {
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
    //close/destroy the uinput device.
    if (uinput_fd >= 0) {
      if (ui)
        ui->uinput_destroy(uinput_fd);
    }
    lock.unlock();
    return;  //Everything is fine, break the loop.
  } while (true);
}

int output_slot::upload_ff(const ff_effect& effect) {
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

int output_slot::erase_ff(int id) {
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

int output_slot::play_ff(int id, int repetitions) {
  std::lock_guard<std::mutex> guard(lock);
  if (id != 0 || effects[0].id == -1)
    return FAILURE;
  for (auto it = devices.begin(); it != devices.end(); it++) {
    auto ptr = it->lock();
    ptr->play_ff(id, repetitions);
  }
  return SUCCESS;
}

static std::string boolstrings[2] = {"false", "true"};
virtual_gamepad::virtual_gamepad(std::string name, std::string descr, virtpad_settings settings, uinput* ui) : output_slot(name, descr) {
  this->dpad_as_hat = settings.dpad_as_hat;
  this->analog_triggers = settings.analog_triggers;
  effects[0].id = -1;
  settings.u_ids.phys = "moltengamepad/" + name;
  uinput_fd = ui->make_gamepad(settings.u_ids, dpad_as_hat, analog_triggers, settings.rumble);
  if (uinput_fd < 0) throw std::runtime_error("No uinput node available.");
  if (settings.rumble)
    ui->watch_for_ff(uinput_fd, this);
  this->padstyle = settings;
  this->ui = ui;
}

virtual_keyboard::virtual_keyboard(std::string name, std::string descr, uinput_ids keyboard_ids, uinput_ids mouse_ids, uinput* ui) : output_slot(name, descr) {
  uinput_fd = ui->make_keyboard(keyboard_ids);
  if (uinput_fd < 0) throw std::runtime_error("No uinput node available.");
  
  mouse_fd = ui->make_mouse(mouse_ids);
  if (mouse_fd < 0) throw std::runtime_error("Making uinput mouse failed.");
  
  this->u_ids = keyboard_ids;
  this->ui = ui;
}

void virtual_keyboard::take_event(struct input_event in) {
  //Relative events go to a separate mouse device.
  //SYN events should go to both!
  if (in.type == EV_REL || in.type == EV_SYN) {
    write(mouse_fd, &in, sizeof(in));
    if (in.type == EV_REL) return;
  }
  write(uinput_fd, &in, sizeof(in));
};



static int dpad_hat_axis[4] = {ABS_HAT0Y, ABS_HAT0Y, ABS_HAT0X, ABS_HAT0X};
static int dpad_hat_mult[4] = { -1,        1,         -1,        1        };

void virtual_gamepad::take_event(struct input_event in) {

  //translate DPAD buttons to hat events if needed.
  if (dpad_as_hat && in.type == EV_KEY && (in.code >= BTN_DPAD_UP && in.code <= BTN_DPAD_RIGHT)) {
    int index = in.code - BTN_DPAD_UP;
    in.type = EV_ABS;
    in.code = dpad_hat_axis[index];
    in.value *= dpad_hat_mult[index];
  }
  //follow our face_1234 mapping for the four face buttons... holdover from letting this layout be changed.
  if (in.type == EV_KEY && (in.code >= BTN_SOUTH && in.code <= BTN_WEST)) {
    if (in.code >= BTN_C) in.code--; //Skip BTN_C for computing the offset
    in.code = face_1234[in.code - BTN_SOUTH];
  }
  //rescale/recenter trigger axes to be 0-255
  if (in.type == EV_ABS && (in.code == ABS_Z || in.code == ABS_RZ)) {
    int64_t value = (in.value + 32768) * 255;
    value /= 2 * 32768l;
    in.value = value;
  }
  //these next two cases translate trigger buttons to trigger axes if needed.
  if (analog_triggers && in.type == EV_KEY && in.code == BTN_TR2) {
    in.type = EV_ABS;
    in.code = ABS_RZ, in.value *= 255;
  }
  if (analog_triggers && in.type == EV_KEY && in.code == BTN_TL2) {
    in.type = EV_ABS;
    in.code = ABS_Z,  in.value *= 255;
  }
  write(uinput_fd, &in, sizeof(in));
};

bool virtual_gamepad::accept_device(std::shared_ptr<input_source> dev) {
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

void print_effect(ff_effect& effect) {
  std::cout << " type:"<< effect.type;
  std::cout << " id:"<< effect.id;
  std::cout << " d:"<< effect.direction;
  std::cout << " type:"<< effect.type;
  std::cout << " trigger:"<< effect.trigger.button << "," << effect.trigger.interval;
  std::cout << " play:"<< effect.replay.length << "," << effect.replay.delay << std::endl;
}



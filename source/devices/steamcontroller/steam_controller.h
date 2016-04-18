#ifdef BUILD_STEAM_CONTROLLER_DRIVER
#ifndef STEAM_CONTROLLER_H
#define STEAM_CONTROLLER_H
#include <scrawpp/context.hpp>
#include <scrawpp/controller.hpp>
#include <thread>
#include <functional>
#include "../device.h"


enum sc_keys {
  sc_a,
  sc_b,
  sc_x,
  sc_y,
  sc_forward,
  sc_back,
  sc_mode,
  sc_tl,
  sc_tl2,
  sc_tr,
  sc_tr2,
  sc_lgrip,
  sc_rgrip,
  sc_stick_click,
  sc_left_pad_click,
  sc_right_pad_click,
  sc_left_pad_touch,
  sc_right_pad_touch,

  sc_stick_x,
  sc_stick_y,
  sc_left_pad_x,
  sc_left_pad_y,
  sc_right_pad_x,
  sc_right_pad_y,
  sc_tl2_axis,
  sc_tr2_axis,

  //TODO: gyroscopes

  steamcont_event_max
};


extern const source_event steamcont_events[];

extern const source_option steamcont_options[];

int lookup_steamcont_event(const char* evname);


class steam_controller : public input_source {
public:
  const char* descr = "Steam Controller";

  steam_controller(scraw::controller* sc, slot_manager* slot_man, device_manager* manager);
  ~steam_controller();

  virtual void list_events(cat_list& list);
  virtual struct name_descr get_info() {
    struct name_descr desc;
    desc.name = name.c_str();
    desc.descr = descr;
    return desc;
  }
  
  virtual enum entry_type entry_type(const char* name);


protected:
  void process(void*);
  virtual int process_option(const char* opname, const char* value) { return -1; }; //TODO: options
private:
  bool automouse = false;
  bool autobuttons = false;
  bool padcentering = true;
  scraw::controller* sc;
  int statepipe[2];
  void on_state_change(const scraw_controller_state_t& state);
};

class steam_controller_manager : public device_manager {
public:
  std::unordered_map<scraw::controller, steam_controller*> sc_devs;

  //We don't handle any udev events at all!
  virtual int accept_device(struct udev* udev, struct udev_device* dev) { return -2;};

  virtual void list_devs(name_list& list) {
    for (auto dev : sc_devs) {
      list.push_back(dev.second->get_info());
    }
  }

  virtual input_source* find_device(const char* name);
  virtual enum entry_type entry_type(const char* name);

  void init_profile();

  steam_controller_manager(moltengamepad* mg);

  ~steam_controller_manager() {
    keep_scanning = false;
    sc_context_thread->join();
    delete sc_context_thread;
  }
private:
  scraw::context sc_context;
  std::thread* sc_context_thread;
  volatile bool keep_scanning;

  void on_controller_gained(scraw::controller sc);
  void on_controller_lost(scraw::controller sc);
};
#endif
#endif

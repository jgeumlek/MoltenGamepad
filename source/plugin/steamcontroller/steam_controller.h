#pragma once
#include <scrawpp/context.hpp>
#include <scrawpp/controller.hpp>
#include <thread>
#include <functional>
#include <unistd.h>
#include <linux/input.h>
#include <unordered_map>
#include <mutex>
#include "../plugin.h"

extern device_plugin scdev;

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


extern const event_decl steamcont_events[];

extern const option_decl steamcont_options[];

int lookup_steamcont_event(const char* evname);


class steam_controller {
public:
  steam_controller(scraw::controller* sc);
  ~steam_controller();

  input_source* ref;
  friend PLUGIN_INIT_FUNC(steamcontroller)(plugin_api api);
  static device_methods methods;

protected:
  void process(void*);
  int process_option(const char* opname, const MGField value) { return -1; }; //TODO: options
private:
  bool automouse = false;
  bool autobuttons = false;
  bool padcentering = true;
  scraw::controller* sc;
  int statepipe[2];
  void on_state_change(const scraw_controller_state_t& state);
};

class steam_controller_manager {
public:
  std::unordered_map<scraw::controller, steam_controller*> sc_devs;

  void init_profile();

  int init(device_manager* ref) {
    this->ref = ref;
    init_profile();
  }

  int start();

  steam_controller_manager();

  ~steam_controller_manager() {
    keep_scanning = false;
    if (sc_context_thread) {
      try {
        sc_context_thread->join();
      } catch (std::exception& e) {
      }
      delete sc_context_thread;
    }
  }

  friend int sc_plugin_init(plugin_api api);
  friend int plugin_init(plugin_api api);
  static manager_methods methods;
private:
  scraw::context sc_context;
  std::thread* sc_context_thread = nullptr;
  volatile bool keep_scanning;
  std::mutex devlistlock;
  device_manager* ref;


  void on_controller_gained(scraw::controller sc);
  void on_controller_lost(scraw::controller sc);
};


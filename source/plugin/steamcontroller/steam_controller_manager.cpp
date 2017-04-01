#include "steam_controller.h"

manager_methods steam_controller_manager::methods;

steam_controller_manager::steam_controller_manager() : sc_context(std::bind(&steam_controller_manager::on_controller_gained, this, std::placeholders::_1), std::bind(&steam_controller_manager::on_controller_lost, this, std::placeholders::_1)) {

}

int steam_controller_manager::start() {
  //need to do a thread for hot plugs, since these don't go through udev.
  keep_scanning = true;

  sc_context_thread = new std::thread([&] () {
    while (keep_scanning) {
      sc_context.handle_events(1000);
    }
  });
  return 0;
}

void steam_controller_manager::on_controller_gained(scraw::controller sc) {
  scraw::controller* scrawref = new scraw::controller(sc);
  steam_controller* steamcont = new steam_controller(scrawref);
  sc_devs[sc] = steamcont;
  device_plugin plug = scdev;

  methods.add_device(ref, plug, steamcont);
}
void steam_controller_manager::on_controller_lost(scraw::controller sc) {
  auto dev = sc_devs.find(sc);
  if (dev != sc_devs.end()) {
    methods.remove_device(ref,dev->second->ref);
    sc_devs.erase(dev);
  }
}


void steam_controller_manager::init_profile() {
  //Init some event translators

  const event_decl* ev = &steamcont_events[0];
  for (int i = 0; ev->name && *ev->name; ev = &steamcont_events[++i]) {
    methods.register_event(ref, *ev);
  }

  auto set_alias = [&] (const char* external, const char* internal) {
    methods.register_alias(ref, external, internal);
  };

  //Init some aliases to act like a standardized game pad
  set_alias("first","a");
  set_alias("second","b");
  set_alias("third","x");
  set_alias("fourth","y");
  set_alias("leftright","left_pad_x");
  set_alias("updown","left_pad_y");
  set_alias("start","forward");
  set_alias("select","back");
  set_alias("thumbl","stick_click");
  set_alias("thumbr","right_pad_click");
  set_alias("left_x","stick_x");
  set_alias("left_y","stick_y");
  set_alias("right_x","right_pad_x");
  set_alias("right_y","right_pad_y");

  methods.register_event_group(ref, {"left_pad","left_pad_x,left_pad_y","Left Touch Pad","dpad()"});
  methods.register_event_group(ref, {"stick","stick_x,stick_y","Thumb stick","stick(left_x,left_y)"});
  methods.register_event_group(ref, {"right_pad","right_pad_x,right_pad_y","Right Touch Pad","stick(right_x,right_y)"});

};


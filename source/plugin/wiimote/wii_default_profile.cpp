#include "wiimote.h"

void wiimote_manager::init_profile() {
  //Init some event translators
  const event_decl* ev = &wiimote_events[0];
  for (int i = 0; ev->name && *ev->name; ev = &wiimote_events[++i]) {
    methods.register_event(ref, *ev);
  }

  auto set_alias = [&] (const char* external, const char* internal) {
    methods.register_alias(ref, external, internal);
  };
  //Init some aliases to act like a standardized game pad
  set_alias("primary","cc_a");
  set_alias("secondary","cc_b");
  set_alias("third","cc_x");
  set_alias("fourth","cc_y");
  set_alias("left","cc_left");
  set_alias("right","cc_right");
  set_alias("up","cc_up");
  set_alias("down","cc_down");
  set_alias("mode","cc_home");
  set_alias("start","cc_plus");
  set_alias("select","cc_minus");
  set_alias("tl","cc_l");
  set_alias("tr","cc_r");
  set_alias("tl2","cc_zl");
  set_alias("tr2","cc_zr");
  set_alias("thumbl","cc_thumbl");
  set_alias("thumbr","cc_thumbr");
  set_alias("left_x","cc_left_x");
  set_alias("left_y","cc_left_y");
  set_alias("right_x","cc_right_x");
  set_alias("right_y","cc_right_y");
  

  //set default group translators by declaring event groups
  methods.register_event_group(ref, {"nk_stick","nk_stick_x,nk_stick_y","Nunchuk stick","stick(left_x,left_y)"});

  const option_decl* opt = &wiimote_options[0];
  for (int i = 0; opt->name && *opt->name; opt = &wiimote_options[++i]) {
    methods.register_dev_option(ref, *opt);
  }
  
  methods.register_manager_option(ref, {"auto_assign_balance","Assign balance boards to an output slot upon connection","true",MG_BOOL});
};


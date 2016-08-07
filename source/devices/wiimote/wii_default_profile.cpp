#include "wiimote.h"
#include "../../profile.h"
#include "../../event_translators/translators.h"

void wiimote_manager::init_profile() {
  //Init some event translators
  const event_decl* ev = &wiimote_events[0];
  for (int i = 0; ev->name && *ev->name; ev = &wiimote_events[++i]) {
    register_event(*ev);
  }

  //Init some aliases to act like a standardized game pad
  mapprofile->set_alias("primary","cc_a");
  mapprofile->set_alias("secondary","cc_b");
  mapprofile->set_alias("third","cc_x");
  mapprofile->set_alias("fourth","cc_y");
  mapprofile->set_alias("left","cc_left");
  mapprofile->set_alias("right","cc_right");
  mapprofile->set_alias("up","cc_up");
  mapprofile->set_alias("down","cc_down");
  mapprofile->set_alias("mode","cc_home");
  mapprofile->set_alias("start","cc_plus");
  mapprofile->set_alias("select","cc_minus");
  mapprofile->set_alias("tl","cc_l");
  mapprofile->set_alias("tr","cc_r");
  mapprofile->set_alias("tl2","cc_zl");
  mapprofile->set_alias("tr2","cc_zr");
  mapprofile->set_alias("thumbl","cc_thumbl");
  mapprofile->set_alias("thumbr","cc_thumbr");
  mapprofile->set_alias("left_x","cc_left_x");
  mapprofile->set_alias("left_y","cc_left_y");
  mapprofile->set_alias("right_x","cc_right_x");
  mapprofile->set_alias("right_y","cc_right_y");
  

  const option_decl* opt = &wiimote_options[0];
  for (int i = 0; opt->name && *opt->name; opt = &wiimote_options[++i]) {
    register_option(*opt);
  }
  
  mg->gamepad->copy_into(mapprofile, true, false);

};


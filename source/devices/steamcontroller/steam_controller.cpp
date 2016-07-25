#ifdef BUILD_STEAM_CONTROLLER_DRIVER
#include "steam_controller.h"


#define BTN DEV_KEY,0,nullptr
#define ABS DEV_AXIS,0,nullptr
#define OPT DEV_OPTION
const source_event steamcont_events[] = {
  {sc_a,"a","A Button",BTN},
  {sc_b,"b","B Button",BTN},
  {sc_x,"x","X Button",BTN},
  {sc_y,"y","Y Button",BTN},
  {sc_forward,"forward","Forward Button (right middle button)",BTN},
  {sc_back,"back","Back Button (left middle button)",BTN},
  {sc_mode,"mode","Mode Button (middle middle button)",BTN},
  {sc_tl,"tl","Left Bumper",BTN},
  {sc_tl2,"tl2","Left Trigger",BTN},
  {sc_tr,"tr","Right Bumper",BTN},
  {sc_tr2,"tr2","Right Trigger",BTN},
  {sc_lgrip,"lgrip","Left Grip",BTN},
  {sc_rgrip,"rgrip","Right Grip",BTN},
  {sc_stick_click,"stick_click","Thumb Stick Click",BTN},
  {sc_left_pad_click,"left_pad_click","Left Touch Pad Click",BTN},
  {sc_right_pad_click,"right_pad_click","Right Touch Pad Click",BTN},
  {sc_left_pad_touch,"left_pad_touch","Left Touch Pad Touch Detected",BTN},
  {sc_right_pad_touch,"right_pad_touch","Right Touch Pad Touch Detected",BTN},

  {sc_stick_x,"stick_x","Thumb Stick X-axis", ABS},
  {sc_stick_y,"stick_y","Thumb Stick Y-axis", ABS},
  {sc_left_pad_x,"left_pad_x","Left Touch Pad X-axis", ABS},
  {sc_left_pad_y,"left_pad_y","Left Touch Pad Y-axis", ABS},
  {sc_right_pad_x,"right_pad_x","Right Touch Pad X-axis", ABS},
  {sc_right_pad_y,"right_pad_y","Right Touch Pad Y-axis", ABS},
  {sc_tl2_axis,"tl2_axis","Right Trigger Analog Values", ABS},
  {sc_tr2_axis,"tr2_axis","Right Trigger Analog Values", ABS},
  { -1, nullptr, nullptr, NO_ENTRY, 0, nullptr}
};

const source_option steamcont_options[] = {
  {"automouse", "Enable built in mouse movement emulation (\"Lizard\" mode)", "false"},
  {"autobuttons", "Enable built in keyboard/mouse button emulation (\"Lizard\" mode)", "false"},
  {"", "", ""},
};

int lookup_steamcont_event(const char* evname) {
  const source_event* event = &steamcont_events[0];
  for ( ; event->id >= 0; event++) {
     if (!strcmp(event->name, evname))
       return event->id;
  }
  return -1;
}

steam_controller::steam_controller(scraw::controller* sc, slot_manager* slot_man, device_manager* manager) : sc(sc), input_source(slot_man, manager, "gamepad") {
  for (int i = 0; i < steamcont_event_max; i++) {
    register_event(steamcont_events[i]);
  }
  for (int i = 0; !steamcont_options[i].name.empty(); i++) {
    register_option(steamcont_options[i]);
  }
  pipe(statepipe);
  watch_file(statepipe[0],statepipe);
  scraw::controller_config ctrl_cfg;
  ctrl_cfg.idle_timeout = 300;
  ctrl_cfg.imu = false;
  sc->configure(ctrl_cfg);
  sc->lizard_buttons(autobuttons);
  sc->on_state_change() = std::bind(&steam_controller::on_state_change, this, std::placeholders::_2);
}
steam_controller::~steam_controller() {
 delete sc;
 close(statepipe[0]);
 close(statepipe[1]);
}


void steam_controller::on_state_change(const scraw_controller_state_t& state) {
  //turn this call back into an epoll event.
  write(statepipe[1], &state, sizeof(state));
}

//A lot of boiler plate if-statements to check each event one-by-one.
//So let's make some macros to give clearer semantics.

#define CHECK_BTN(SCRAW_ID,MG_ID) if (!!(buttons & SCRAW_ID) != events[MG_ID].value) send_value(MG_ID,!!(buttons & SCRAW_ID));
#define CHECK_AXIS(SCRAW_ID,MG_ID) if (state.SCRAW_ID != events[MG_ID].value) send_value(MG_ID,state.SCRAW_ID);

//Play it safe, since changing sign can overflow.
#define CHECK_AXIS_FLIP(SCRAW_ID,MG_ID) do { \
  uint64_t new_val = state.SCRAW_ID;\
  if (-new_val != events[MG_ID].value) send_value(MG_ID,-new_val);\
} while(0);

//Need to to rescale the positive-only trigger values to span the whole range.
#define CHECK_AXIS_TRIGGER(SCRAW_ID,MG_ID) do { \
  uint64_t new_val = state.SCRAW_ID;\
  new_val *= 2;\
  new_val -= RANGE;\
  if (new_val != events[MG_ID].value) send_value(MG_ID,new_val);\
} while(0);

void steam_controller::process(void* tag) {
  scraw_controller_state_t state;
  int ret = read(statepipe[0],&state,sizeof(state));
  if (ret < sizeof(state))
    return; //abort.
  //start checking events for new values to report...
  auto buttons = state.buttons;
  CHECK_BTN(SCRAW_BTN_A,sc_a);
  CHECK_BTN(SCRAW_BTN_B,sc_b);
  CHECK_BTN(SCRAW_BTN_X,sc_x);
  CHECK_BTN(SCRAW_BTN_Y,sc_y);

  CHECK_BTN(SCRAW_BTN_START,sc_forward);
  CHECK_BTN(SCRAW_BTN_SELECT,sc_back);
  CHECK_BTN(SCRAW_BTN_STEAM,sc_mode);

  CHECK_BTN(SCRAW_BTN_LEFT_SHOULDER,sc_tl);
  CHECK_BTN(SCRAW_BTN_LEFT_TRIGGER,sc_tl2);
  CHECK_BTN(SCRAW_BTN_RIGHT_SHOULDER,sc_tr);
  CHECK_BTN(SCRAW_BTN_RIGHT_TRIGGER,sc_tr2);
  CHECK_BTN(SCRAW_BTN_LEFT_GRIPPER,sc_lgrip);
  CHECK_BTN(SCRAW_BTN_RIGHT_GRIPPER,sc_rgrip);

  CHECK_BTN(SCRAW_BTN_JOYSTICK,sc_stick_click);
  CHECK_BTN(SCRAW_BTN_LEFT_TP,sc_left_pad_click);
  CHECK_BTN(SCRAW_BTN_RIGHT_TP,sc_right_pad_click);
  CHECK_BTN(SCRAW_BTN_LEFT_TP_TOUCH,sc_left_pad_touch);
  CHECK_BTN(SCRAW_BTN_RIGHT_TP_TOUCH,sc_right_pad_touch);
  
  CHECK_AXIS(left_trackpad_x,sc_left_pad_x);
  CHECK_AXIS_FLIP(left_trackpad_y,sc_left_pad_y);
  CHECK_AXIS(right_trackpad_x,sc_right_pad_x);
  CHECK_AXIS_FLIP(right_trackpad_y,sc_right_pad_y);
  CHECK_AXIS(joystick_x,sc_stick_x);
  CHECK_AXIS_FLIP(joystick_y,sc_stick_y);

  CHECK_AXIS_TRIGGER(left_trigger,sc_tl2_axis);
  CHECK_AXIS_TRIGGER(right_trigger,sc_tr2_axis);

  struct input_event syn_ev;
  memset(&syn_ev, 0, sizeof(syn_ev));
  syn_ev.type = EV_SYN;
  syn_ev.code = SYN_REPORT;
  if (out_dev) out_dev->take_event(syn_ev);
}

#endif

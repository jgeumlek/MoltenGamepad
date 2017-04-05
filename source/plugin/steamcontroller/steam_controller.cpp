#include "steam_controller.h"


#define BTN DEV_KEY
#define ABS DEV_AXIS
#define OPT DEV_OPTION

device_methods steam_controller::methods;

const event_decl steamcont_events[] = {
  {"a","A Button",BTN,"first"},
  {"b","B Button",BTN,"second"},
  {"x","X Button",BTN,"third"},
  {"y","Y Button",BTN,"fourth"},
  {"forward","Forward Button (right middle button)",BTN,"start"},
  {"back","Back Button (left middle button)",BTN,"select"},
  {"mode","Mode Button (middle middle button)",BTN,"mode"},
  {"tl","Left Bumper",BTN,"tl"},
  {"tl2_axis_btn","Left Trigger",BTN,"nothing"},
  {"tr","Right Bumper",BTN,"tr"},
  {"tr2_axis_btn","Right Trigger",BTN,"nothing"},
  {"lgrip","Left Grip",BTN,""},
  {"rgrip","Right Grip",BTN,""},
  {"stick_click","Thumb Stick Click",BTN,"thumbl"},
  {"left_pad_click","Left Touch Pad Click",BTN,"thumbl"},
  {"right_pad_click","Right Touch Pad Click",BTN,"thumbr"},
  {"left_pad_touch","Left Touch Pad Touch Detected",BTN,""},
  {"right_pad_touch","Right Touch Pad Touch Detected",BTN,""},

  {"stick_x","Thumb Stick X-axis", ABS,""},
  {"stick_y","Thumb Stick Y-axis", ABS,""},
  {"left_pad_x","Left Touch Pad X-axis", ABS,""},
  {"left_pad_y","Left Touch Pad Y-axis", ABS,""},
  {"right_pad_x","Right Touch Pad X-axis", ABS,""},
  {"right_pad_y","Right Touch Pad Y-axis", ABS,""},
  {"tl2_axis","Right Trigger Analog Values", ABS,"tl2_axis"},
  {"tr2_axis","Right Trigger Analog Values", ABS,"tr2_axis"},
  {nullptr, nullptr, NO_ENTRY, nullptr}
};

const option_decl steamcont_options[] = {
  {"automouse", "Enable built in mouse movement emulation (\"Lizard\" mode)", "false", MG_BOOL},
  {"autobuttons", "Enable built in keyboard/mouse button emulation (\"Lizard\" mode)", "false", MG_BOOL},
  {nullptr, nullptr, nullptr},
};

steam_controller::steam_controller(scraw::controller* sc) : sc(sc) {
  pipe(statepipe);
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

#define CHECK_BTN(SCRAW_ID,MG_ID) methods.send_value(ref,MG_ID,!!(buttons & SCRAW_ID));
#define CHECK_AXIS(SCRAW_ID,MG_ID) methods.send_value(ref,MG_ID,state.SCRAW_ID);

//Play it safe, since changing sign can overflow.
#define CHECK_AXIS_FLIP(SCRAW_ID,MG_ID) do { \
  uint64_t new_val = state.SCRAW_ID;\
  methods.send_value(ref,MG_ID,-new_val);\
} while(0);

//Need to to rescale the positive-only trigger values to span the whole range.
#define CHECK_AXIS_TRIGGER(SCRAW_ID,MG_ID) do { \
  uint64_t new_val = state.SCRAW_ID;\
  new_val *= 2;\
  new_val -= ABS_RANGE;\
  methods.send_value(ref,MG_ID,new_val);\
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

  methods.send_syn_report(ref);
}


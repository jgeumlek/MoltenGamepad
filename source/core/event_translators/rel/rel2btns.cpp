#include "rel2btns.h"
#include "../event_translator_macros.h"

bool rel2btns::wants_recurring_events() {
  return true;
}

// press down buttons if input is positive or negative
// edge cases:
// - release button if currently pressed so we can press it again
// - release button immediately afterwards if btn_hold_time is 0
void rel2btns::process(struct mg_ev ev, virtual_device* out) {
  int val = ev.value;
  int btn_code;

  // get current time
  struct timeval cur_time;
  gettimeofday(&cur_time, NULL);

  // check which button should be pressed / not pressed
  bool currently_pressed = false;
  if (val == 0) {
    // 0 value: no button pressed
    return;
  } else if (val > 0) {
    // positive value: positive button pressed
    btn_code = pos_btn;

    // check and update whether the button is currently pressed
    if (btn_hold_time > 0) {
      if (pos_btn_pressed.tv_sec != 0) {currently_pressed = true;}
      pos_btn_pressed = cur_time;
    }
  } else {
    // negative value: negative button pressed
    btn_code = neg_btn;

    // check and update whether the button is currently pressed
    if (btn_hold_time > 0) {
      if (neg_btn_pressed.tv_sec != 0) {currently_pressed = true;}
      neg_btn_pressed = cur_time;
    }
  }

  // initialize output event(s)
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.code = btn_code;

  // if the button we want to press is currently pressed, release it so we can press it again
  if (currently_pressed) {
    out_ev.value = 0;
    write_out(out_ev, out);
  }

  // press button
  out_ev.value = 1;
  write_out(out_ev, out);

  // if no btn_hold_time, immediately release button
  if (btn_hold_time == 0) {
    out_ev.value = 0;
    write_out(out_ev, out);
  }
}

// release buttons after a set amount of time
void rel2btns::process_recurring(virtual_device* out) const {
  // if no buttons currently being held down, return
  if (pos_btn_pressed.tv_sec == 0 && neg_btn_pressed.tv_sec == 0) {return;}

  // get current time
  struct timeval cur_time;
  gettimeofday(&cur_time, NULL);

  // initialize output event
  struct input_event out_ev;
  memset(&out_ev, 0, sizeof(out_ev));
  out_ev.type = EV_KEY;
  out_ev.value = 0;

  struct timeval zero_time = {0, 0};

  // release positive button after a certain amount of time
  if (pos_btn_pressed.tv_sec != 0) { // if positive button being pressed
    // get time since pressed
    int time_diff_usec = (cur_time.tv_sec - pos_btn_pressed.tv_sec) * 1000000 + (cur_time.tv_usec - pos_btn_pressed.tv_usec);
    // if time since pressed > btn_hold_time
    if (time_diff_usec > btn_hold_time * 1000000) {
      // release button
      out_ev.code = pos_btn;
      write_out(out_ev, out);
      pos_btn_pressed = zero_time;
    }
  }

  // release negative button after a certain amount of time
  if (neg_btn_pressed.tv_sec != 0) { // if positive button being pressed
    // get time since pressed
    int time_diff_usec = (cur_time.tv_sec - neg_btn_pressed.tv_sec) * 1000000 + (cur_time.tv_usec - neg_btn_pressed.tv_usec);
    // if time since pressed > btn_hold_time
    if (time_diff_usec > btn_hold_time * 1000000) {
      // release button
      out_ev.code = neg_btn;
      write_out(out_ev, out);
      neg_btn_pressed = zero_time;
    }
  }
}

const char* rel2btns::decl = "rel = rel2btns(key_code, key_code, float btn_hold_time=0.05)";
rel2btns::rel2btns(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_KEY(neg_btn);
  READ_KEY(pos_btn);
  READ_FLOAT(btn_hold_time);
}
void rel2btns::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("rel2btns");
  FILL_DEF_KEY(neg_btn);
  FILL_DEF_KEY(pos_btn);
  FILL_DEF_FLOAT(btn_hold_time);
}

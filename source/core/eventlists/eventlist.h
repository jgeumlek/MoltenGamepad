#ifndef EVENTLIST_H
#define EVENTLIST_H

#include <linux/input.h>


#ifndef BTN_DPAD_UP
#define BTN_DPAD_UP 544
#define DPAD_AS_HAT
#endif

#ifndef BTN_DPAD_DOWN
#define BTN_DPAD_DOWN 545
#define DPAD_AS_HAT
#endif


#ifndef BTN_DPAD_LEFT
#define BTN_DPAD_LEFT 546
#define DPAD_AS_HAT
#endif

#ifndef BTN_DPAD_RIGHT
#define BTN_DPAD_UP 547
#define DPAD_AS_HAT
#endif

struct event_listing {
  int id;
  const char* name;
  const char* descr;
};

enum out_type { OUT_ABS, OUT_KEY, OUT_REL, OUT_NONE };
struct event_info {
  enum out_type type;
  int value;
};


extern const event_listing key_list[];
extern const event_listing axis_list[];
extern const event_listing rel_list[];
extern const event_listing gamepad_btn_list[];
extern const event_listing gamepad_axis_list[];

const char* get_key_name(int key_id);
const char* get_axis_name(int axis_id);
const char* get_rel_name(int rel_id);

int get_key_id(const char* keyname);
int get_axis_id(const char* axisname);
int get_rel_id(const char* relname);

bool is_keyboard_key(int key_id);


event_info lookup_event(const char* eventname);

#endif

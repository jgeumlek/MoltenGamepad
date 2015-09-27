#ifndef EVENTLiST_H
#define EVENTLIST_H

#include <linux/input.h>

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


event_info lookup_event(const char* eventname);

#endif

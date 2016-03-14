#include "eventlist.h"
#include <cstring>

const char* id2name(const event_listing list[], int id) {
  for (int i = 0; list[i].name != nullptr; i++) {
    if (list[i].id == id) return list[i].name;
  }
  return nullptr;
}

int name2id(const event_listing list[], const char* name) {
  for (int i = 0; list[i].name != nullptr; i++) {
    if (!strcmp(list[i].name, name)) return list[i].id;
  }
  return -1;
}

const char* get_key_name(int key_id) {
  const char* name = id2name(gamepad_btn_list, key_id);
  if (name) return name;

  return id2name(key_list, key_id);
}

const char* get_axis_name(int axis_id) {
  const char* name = id2name(gamepad_axis_list, axis_id);
  if (name) return name;

  return id2name(axis_list, axis_id);
}

const char* get_rel_name(int rel_id) {
  return id2name(rel_list, rel_id);
}

int get_key_id(const char* keyname) {
  int id = name2id(gamepad_btn_list, keyname);
  if (id != -1) return id;

  return name2id(key_list, keyname);
}

int get_axis_id(const char* axisname) {
  int id = name2id(gamepad_axis_list, axisname);
  if (id != -1) return id;

  return name2id(axis_list, axisname);
}

int get_rel_id(const char* relname) {
  return name2id(rel_list, relname);
}

event_info lookup_event(const char* eventname) {
  event_info out;

  out.type = OUT_KEY;
  out.value = get_key_id(eventname);
  if (out.value != -1) return out;

  out.type = OUT_ABS;
  out.value = get_axis_id(eventname);
  if (out.value != -1) return out;

  out.type = OUT_REL;
  out.value = get_rel_id(eventname);
  if (out.value != -1) return out;

  out.type = OUT_NONE;
  return out;
}

bool is_keyboard_key(int key_id) {
  const char* name = get_key_name(key_id);
  return (name[0] == 'k');
}
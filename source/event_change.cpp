#include "event_change.h"
#include "devices/device.h"

#include <iostream>
#include <unistd.h>

#define BEGIN_FILL_DEF(X) def.identifier = X; MGField field;
#define FILL_DEF(X,TYPE,LOC) field.type = TYPE; field.LOC = X; def.fields.push_back(field);
#define FILL_DEF_KEY(X) FILL_DEF(X,MG_KEY,key)
#define FILL_DEF_AXIS(X) FILL_DEF(X,MG_AXIS,axis)
#define FILL_DEF_REL(X) FILL_DEF(X,MG_REL,rel)
#define FILL_DEF_INT(X) FILL_DEF(X,MG_INT,integer)
#define FILL_DEF_TRANS(X,TYPE) FILL_DEF(X,TYPE,trans)
#define FILL_DEF_SLOT(X) FILL_DEF(X,MG_SLOT,slot)
#define FILL_DEF_KEYBOARD(X) FILL_DEF(X,MG_KEYBOARD_SLOT,slot)

const MGType btn2btn::fields[] = { MG_KEY, MG_NULL };
btn2btn::btn2btn(std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_KEY)
    out_button = fields.front().key;
}
void btn2btn::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("btn2btn");
  FILL_DEF_KEY(out_button);
}

const MGType btn2axis::fields[] = { MG_AXIS, MG_INT, MG_NULL };
btn2axis::btn2axis(std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_AXIS)
    out_axis = fields.front().axis;
  if (fields.size() > 1 && fields[1].type == MG_INT)
    direction = fields[1].integer;
}
void btn2axis::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("btn2axis");
  FILL_DEF_AXIS(out_axis);
  FILL_DEF_INT(direction);
}

const MGType axis2axis::fields[] = { MG_AXIS, MG_INT, MG_NULL };
axis2axis::axis2axis(std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_AXIS)
    out_axis = fields.front().axis;
  if (fields.size() > 1 && fields[1].type == MG_INT)
    direction = fields[1].integer;
}
void axis2axis::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("axis2axis");
  FILL_DEF_AXIS(out_axis);
  FILL_DEF_INT(direction);
}

const MGType axis2btns::fields[] = { MG_KEY, MG_KEY, MG_NULL };
axis2btns::axis2btns(std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_KEY)
    neg_btn = fields.front().key;
  if (fields.size() > 1 && fields[1].type == MG_KEY)
    pos_btn = fields[1].key;
}
void axis2btns::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("axis2btns");
  FILL_DEF_KEY(neg_btn);
  FILL_DEF_KEY(pos_btn);
}
const MGType redirect_trans::fields[] = { MG_TRANS, MG_SLOT, MG_NULL };
redirect_trans::redirect_trans(std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_TRANS)
    this->trans = fields.front().trans->clone();
  if (fields.size() > 1 && fields[1].type == MG_SLOT)
    this->redirected = fields[1].slot;
}
void redirect_trans::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("redirect");
  FILL_DEF_TRANS(trans,MG_TRANS);
  FILL_DEF_SLOT(redirected);
}
const MGType keyboard_redirect::fields[] = { MG_KEY, MG_KEYBOARD_SLOT, MG_NULL };
keyboard_redirect::keyboard_redirect(std::vector<MGField>& fields)  : redirect_trans() {
  if (fields.size() > 0 && fields.front().type == MG_KEY) {
    this->trans = new btn2btn(fields[0].key);
    key_code = fields[0].key;
  }
  if (fields.size() > 1 && fields[1].type == MG_KEYBOARD_SLOT)
    this->redirected = fields[1].slot;
}
void keyboard_redirect::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("key");
  FILL_DEF_KEY(key_code);
  FILL_DEF_KEYBOARD(redirected);
}
const MGType multitrans::fields[] = { MG_TRANS, MG_TRANS, MG_NULL };
multitrans::multitrans(std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_TRANS) {
    translist.push_back(fields.at(0).trans->clone());
  }
  if (fields.size() > 1 && fields.at(1).type == MG_TRANS) {
    translist.push_back(fields.at(1).trans->clone());
  }
}
void multitrans::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("multi");
  for (auto trans : translist) {
    FILL_DEF_TRANS(trans,MG_TRANS);
  }
}


const MGType simple_chord::fields[] = { MG_KEY_TRANS, MG_NULL };
simple_chord::simple_chord(std::vector<std::string> event_names, std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_KEY_TRANS)
    this->out_trans = fields.front().trans->clone();

  this->event_names = event_names;
}
void simple_chord::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("simple");
  FILL_DEF_TRANS(out_trans,MG_KEY_TRANS);
}

void simple_chord::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();
  for (auto name : event_names) {
    event_ids.push_back(-1);
    event_vals.push_back(0);
  }
  for (auto event : events) {
    for (int i = 0; i < event_names.size(); i++) {
      if (!strcmp(event.name, event_names[i].c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
      }
    }
  }

  out_dev_ptr = &(source->out_dev);
};

void simple_chord::attach(input_source* source) {

  for (int id : event_ids)
    source->add_listener(id, this);

  this->source = source;
};

simple_chord::~simple_chord() {
  if (source) {
    for (int id : event_ids) {
      source->remove_listener(id, this);
    }
  }
  if (out_trans) delete out_trans;
}

bool simple_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  for (int i = 0; i < event_ids.size(); i++) {
    if (id == event_ids[i]) {
      event_vals[i] = event.value;
    }
    output = output && (event_vals[i]);
  }
  if (output != output_cache) {
    out_trans->process({output}, *out_dev_ptr);
    output_cache = output;
  }

  return false;
};


const MGType exclusive_chord::fields[] = { MG_KEY_TRANS, MG_NULL };
exclusive_chord::exclusive_chord(std::vector<std::string> event_names, std::vector<MGField>& fields) {
  if (fields.size() > 0 && fields.front().type == MG_KEY_TRANS)
    this->out_trans = fields.front().trans->clone();
  this->event_names = event_names;
}
void exclusive_chord::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("exclusive");
  FILL_DEF_TRANS(out_trans,MG_KEY_TRANS);
}

bool exclusive_chord::claim_event(int id, mg_ev event) {
  bool output = true;
  int index;
  int old_val;


  for (int i = 0; i < event_ids.size(); i++) {
    if (id == event_ids[i]) {
      index = i;
      old_val = event_vals[i];
      event_vals[i] = event.value;
      chord_hits[i] = event.value;
    }
    output = output && (chord_hits[i]);
  }

  //if not thread, start thread.
  if (!thread && event.value && !old_val) {
    thread_active = true;
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    chord_hits[index] = event.value;
    thread = new std::thread(&exclusive_chord::thread_func, this);
    thread->detach();
  }

  if (output && output != output_cache) {
    //chord succeeded. Send event only if thread hasn't timed out.
    thread_active = false;
    if (thread) out_trans->process({output}, *out_dev_ptr);

    output_cache = output;
  }
  if (!output && output != output_cache) {
    //chord released. clear out everything.
    out_trans->process({output}, *out_dev_ptr);
    for (int i = 0; i < event_vals.size(); i++) {
      chord_hits[i] = 0;
    }
    output_cache = output;
  }
  //If key up, let it pass.
  //If we have no thread and our output says we failed, let it pass.
  if (!event.value || (!output_cache && !thread_active)) return false; //Pass along key up events.

  //We have a thread still going, or we hit a chord claiming this event.

  return true;
};

void exclusive_chord::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();
  for (auto name : event_names) {
    event_ids.push_back(-1);
    event_vals.push_back(0);
    chord_hits.push_back(0);
  }
  for (auto event : events) {
    for (int i = 0; i < event_names.size(); i++) {
      if (!strcmp(event.name, event_names[i].c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
      }
    }
  }

  out_dev_ptr = &(source->out_dev);
};

void exclusive_chord::thread_func() {
  //sleep
  usleep(20000);
  //If we are still active, fire the event.
  if (thread_active) {
    //send out events
    thread_active = false;
    for (int i = 0; i < event_ids.size(); i++) {
      if (chord_hits[i]) source->send_value(event_ids[i], event_vals[i]);
      chord_hits[i] = 0;
    }
  }
  delete thread;
  thread = nullptr;
}

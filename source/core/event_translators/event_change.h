#ifndef EVENT_CHANGE_H
#define EVENT_CHANGE_H
#include "../output_slot.h"
#include <linux/input.h>
#include <string>
#include "../eventlists/eventlist.h"
#include "../devices/device.h"
#include "../moltengamepad.h"
#include "../mg_types.h"


#define EVENT_KEY 0
#define EVENT_AXIS 1


struct mg_ev {
  int64_t value;
};

class input_source;
class event_translator;
class group_translator;

struct MGTransDef {
  std::string identifier;
  std::vector<MGField> fields;
};

//A simple event translator. Takes one input event, and translates it. Essentially just a "pipe".
class event_translator {
public:
  //called on a device event, such as a button or axis movement
  virtual void process(struct mg_ev ev, output_slot* out) {
  }
  
  //called regularly on a tick event; a certain amount of time has elapsed.
  virtual void process_recurring(output_slot* out) const {
  }

  void write_out(struct input_event ev, output_slot* out) const {
    out->take_event(ev);
  }

  //event translators are passed around via cloning.
  //This isn't just for memory management, but also
  //lets the profile store a translator as a "prototype"
  virtual event_translator* clone() {
    return new event_translator(*this);
  }
  
  //Called when the device's thread is ready for attaching.
  virtual void attach(input_source* source) {};
  
  //Do we want the input_source to send recurring "ticks" for processing?
  virtual bool wants_recurring_events() { return false; };


  virtual ~event_translator() {};

  event_translator(std::vector<MGField>& fields) {};
  virtual void fill_def(MGTransDef& def) {
    def.identifier = "nothing";
  }
  event_translator() {};

  //vector of parameter meta-data. The macros will deal with it.
  std::vector<int16_t> field_flags;
};
  

//A more complicated event translator. It can request to listen to multiple events.
class group_translator {
public:
  //Initialize any values needed with this input source
  virtual void init(input_source* source) {};
  //Take in a list of event types that this translator will be listening to.
  virtual bool set_mapped_events(const std::vector<source_event>& listened_events) {};
  //Called when the device's thread is ready for attaching.
  virtual void attach(input_source* source) {};
  //Return true to block the input source's native handling of this event.
  //index is with respect to this translators list of mapped inputs.
  //This function should be where we handle any incoming events.
  virtual bool claim_event(int index, mg_ev event) {
    return false;
  };
  //called regularly on a tick event; a certain amount of time has elapsed.
  virtual void process_recurring(output_slot* out) const {};
  //called on a SYN_REPORT, to allow processing multiple events at an appropriate time.
  virtual void process_syn_report(output_slot* out) {};
  //Similar to event_translator::clone(), acts as a prototype method.
  virtual group_translator* clone() {
    return new group_translator(*this);
  }
  virtual ~group_translator() {};

  //Do we want the input_source to send recurring "ticks" for processing?
  virtual bool wants_recurring_events() { return false; };
  //Do we want to clear other translators for our events?
  virtual bool clear_other_translations() { return true; };

  group_translator(std::vector<MGField>& fields) {};
  group_translator() {};
  virtual void fill_def(MGTransDef& def) {
    def.identifier = "nothing";
  }

  //vector of parameter meta-data. The macros will deal with it.
  std::vector<int16_t> field_flags;
};

#endif

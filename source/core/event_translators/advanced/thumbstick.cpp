#include "thumbstick.h"
#include "../event_translator_macros.h"

const char* thumb_stick::decl = "axis, axis = stick(string stick_type, float deadzone=.1, float outzone=.01, float angle_snap=0)";
thumb_stick::thumb_stick(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  const char* stick_type;
  READ_STRING(stick_type);
  if (!strncmp(stick_type,"right",5)) {
    outputs[0] = ABS_RX;
    outputs[1] = ABS_RY;
  } else {
    outputs[0] = ABS_X;
    outputs[1] = ABS_Y;
  }

  READ_FLOAT(deadzone);
  READ_FLOAT(outzone);
  READ_FLOAT(angle_snap);
}

bool thumb_stick::set_mapped_events(const std::vector<std::string>& event_names) {
  this->event_names = event_names;
}
  
void thumb_stick::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("stick");
  field.type = MG_STRING;
  field.string = (outputs[0] == ABS_X) ? "left" : "right";
  def.fields.push_back(field);
  FILL_DEF_FLOAT(deadzone);
  FILL_DEF_FLOAT(outzone);
  FILL_DEF_FLOAT(angle_snap);
}

void thumb_stick::init(input_source* source) {
  //Stash the actual event ids this device has for the names we are interested in.
  auto events = source->get_events();

  for (int i = 0; i < 2; i++) {
    std::string looking_for = event_names[i];
    std::string alias = source->get_alias(looking_for);
    if (!alias.empty()) looking_for = alias;
    for (auto event : events) {
      if (!strcmp(event.name, looking_for.c_str())) {
        event_ids[i] = event.id;
        event_vals[i] = event.value;
        break;
      }
    }
  }

};

void thumb_stick::attach(input_source* source) {

  source->add_listener(event_ids[0], this);
  source->add_listener(event_ids[1], this);

  this->owner = source;
};

thumb_stick::~thumb_stick() {
  if (owner) {
    owner->remove_listener(event_ids[0], this);
    owner->remove_listener(event_ids[0], this);
  }
}

bool thumb_stick::claim_event(int id, mg_ev event) {

  return false;
};

#include "calibratable.h"
#include "../event_translator_macros.h"
#include <cmath>

typedef enum {
    ID_EVENT_RECALIBRATE,
    ID_EVENT_AXIS,
    ID_EVENT_SET,
}IDEventCalibratable;

const char* calibratable::decl = "key, axis, key = calibratable(axis_code a)";
calibratable::calibratable(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_AXIS(axis);
}

bool calibratable::set_mapped_events(const std::vector<source_event>& listened) {
  if (listened.size() != 3) {
      return false;
  }
  cached_input_axes = listened[ID_EVENT_AXIS].value;
  cached_input_set = listened[ID_EVENT_SET].value;
  return true;

}
  
void calibratable::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("calibratable");
  FILL_DEF_AXIS(axis);
}

void calibratable::init(input_source* source) {
  this->owner = source;
};

void calibratable::attach(input_source* source) {
  this->owner = source;
};



bool calibratable::claim_event(int id, mg_ev event) {
    switch (id){
        case ID_EVENT_RECALIBRATE:
            if(event.value == 1){
                // if not pressed reset calibration
                calibration->start_calibration(cached_input_set == 0);
            }
            break;
        case ID_EVENT_SET:
            cached_input_set = event.value;
            if(cached_input_set == 1  && calibration ->is_calibrating()){
               calibration->save_calibrate_data(cached_input_axes); 
            }
            break;
        case ID_EVENT_AXIS:
            cached_input_axes = event.value;
            break;
        default:
            //TODO: notify error
            break;
    }
    return false;
};

void calibratable::process_syn_report(virtual_device* out){
    struct input_event out_ev;
    output_event_val = calibration->get_game_value(cached_input_axes);
    if (cached_output_event_val == output_event_val){
        return;
    }
    cached_output_event_val = output_event_val;
    memset(&out_ev, 0, sizeof(out_ev));
    out_ev.type = EV_ABS;
    out_ev.code = axis;
    out_ev.value = output_event_val;
    out->take_event(out_ev);
}
		

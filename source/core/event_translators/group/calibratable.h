#pragma once
#include "../event_change.h"
#include "calibration.h"

class calibratable : public group_translator {
public:
  int64_t cached_input_set;
  int64_t cached_input_axes;
  int64_t output_event_val;
  int64_t cached_output_event_val;
  int recalibrate_event; // event that help us to take the photo of the axis
  int axis;
  int set_event;
  input_source* owner = nullptr;
  //setting field_flags is optional, but this surpresses printing out those default values later.
  //note that this constructor is only called in hard-coded situations, not during parsing.
  calibratable(int axis) :axis(axis), is_calibrating(false) {
    field_flags.push_back(0); //axis
    calibration = std::make_shared<LinearCalibration>();
  };

  virtual ~calibratable(){
        calibration = nullptr;
  }
  virtual bool clear_other_translations() { return false; };
  virtual void process_syn_report(virtual_device* out);

  virtual void init(input_source* source);
  virtual void attach(input_source* source);
  virtual bool set_mapped_events(const std::vector<source_event>& listened);

  virtual bool claim_event(int id, mg_ev event);

  virtual group_translator* clone() {
      calibratable * cloned = new calibratable(axis);
      if(calibration){
          cloned->calibration =  calibration->clone();
      }
      return cloned;
  }

  static const char* decl;
  calibratable(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
protected:
  std::shared_ptr<Calibration> calibration = nullptr;
  std::vector<int64_t> input_samples;
  int number_samples;
  int index_sample;
  bool is_calibrating;
};

#pragma once
#include "../event_change.h"

class btn2rel : public event_translator {
public:
  int out_rel;
  int speed = 1;
  volatile int value = 0;
  input_source* source = nullptr;
  btn2rel(int out, int speed) : out_rel(out), speed(speed) {
  }
  
  virtual ~btn2rel();

  virtual void process(struct mg_ev ev, output_slot* out);
  virtual void process_recurring(output_slot* out) const;
  virtual void attach(input_source* source);

  virtual btn2rel* clone() {
    return new btn2rel(*this);
  }

  static const MGType fields[];
  btn2rel(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

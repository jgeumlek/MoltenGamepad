#pragma once
#include "../event_change.h"
#include "btn2btn.h"
#include "../general/redirect_trans.h"

class keyboard_redirect : public redirect_trans {
public:
  int key_code;
  keyboard_redirect(int key_code, event_translator* trans, output_slot* redirected) : redirect_trans(trans, redirected) {
    this->key_code = key_code;
  }

  virtual keyboard_redirect* clone() {
    return new keyboard_redirect(key_code, this->trans, redirected);
  }

  static const MGType fields[];
  keyboard_redirect(std::vector<MGField>& fields);
  virtual void fill_def(MGTransDef& def);
};

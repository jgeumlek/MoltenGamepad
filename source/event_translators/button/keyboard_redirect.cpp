#include "keyboard_redirect.h"
#include "../event_translator_macros.h"

const MGType keyboard_redirect::fields[] = { MG_KEY, MG_KEYBOARD_SLOT, MG_NULL };
keyboard_redirect::keyboard_redirect(std::vector<MGField>& fields)  : redirect_trans() {
  BEGIN_READ_DEF;
  READ_KEY(key_code);
  READ_KEYBOARD(redirected);
  trans = new btn2btn(key_code);
}
void keyboard_redirect::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("key");
  FILL_DEF_KEY(key_code);
  FILL_DEF_KEYBOARD(redirected);
}

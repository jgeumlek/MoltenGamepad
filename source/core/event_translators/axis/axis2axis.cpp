#include "axis2axis.h"
#include "../event_translator_macros.h"

const char* axis2axis::decl = "axis = axis2axis(axis_code, int direction=1, int min=-32768, int middle=0, int max=32768)";
axis2axis::axis2axis(std::vector<MGField>& fields) {
  BEGIN_READ_DEF;
  READ_AXIS(out_axis);
  READ_INT(direction);
  READ_INT(min);
  READ_INT(middle);
  READ_INT(max);
}
void axis2axis::fill_def(MGTransDef& def) {
  BEGIN_FILL_DEF("axis2axis");
  FILL_DEF_AXIS(out_axis);
  FILL_DEF_INT(direction);
  FILL_DEF_INT(min);
  FILL_DEF_INT(middle);
  FILL_DEF_INT(max);
}

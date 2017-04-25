#include "../devices/device.h"

#include <iostream>
#include <unistd.h>

#define BEGIN_FILL_DEF(X) def.identifier = X; MGField field;
#define FILL_DEF(X,TYPE,LOC) do {\
  field.type = TYPE; field.LOC = X; \
  field.flags = (field_flags.size() > def.fields.size()) ? field_flags[def.fields.size()] : 0;\
  def.fields.push_back(field); } while(0);
#define FILL_DEF_KEY(X) FILL_DEF(X,MG_KEY,key)
#define FILL_DEF_AXIS(X) FILL_DEF(X,MG_AXIS,axis)
#define FILL_DEF_REL(X) FILL_DEF(X,MG_REL,rel)
#define FILL_DEF_INT(X) FILL_DEF(X,MG_INT,integer)
#define FILL_DEF_BOOL(X) FILL_DEF(X,MG_BOOL,boolean)
#define FILL_DEF_TRANS(X,TYPE) FILL_DEF(X,TYPE,trans)
#define FILL_DEF_SLOT(X) FILL_DEF(X,MG_SLOT,slot)
#define FILL_DEF_KEYBOARD(X) FILL_DEF(X,MG_KEYBOARD_SLOT,slot)
#define FILL_DEF_FLOAT(X) FILL_DEF(X,MG_FLOAT,real)

// BEGIN declares some local variables to allow the other macro magic.
#define BEGIN_READ_DEF \
  int __index = 0; std::vector<event_translator*> __localclones;\
  field_flags.resize(fields.size(),0);
// On failure we need to destroy our local event_translator clones.
#define TRANS_FAIL \
  for (auto trans : __localclones) { delete trans; };\
  throw std::runtime_error("translator construction failed.");

#define READ_DEF(X,TYPE,LOC)  \
  if (fields.size() > __index && fields.at(__index).type == TYPE) { \
    X = fields.at(__index).LOC; \
    field_flags[__index] = fields.at(__index).flags;\
    __index++; \
  } else { \
    TRANS_FAIL \
  } 
#define HAS_NEXT (fields.size() > __index)
#define READ_KEY(X) READ_DEF(X,MG_KEY,key)
#define READ_AXIS(X) READ_DEF(X,MG_AXIS,axis)
#define READ_REL(X) READ_DEF(X,MG_REL,rel)
#define READ_INT(X) READ_DEF(X,MG_INT,integer)
#define READ_BOOL(X) READ_DEF(X,MG_BOOL,boolean)
#define READ_TRANS(X,TYPE) READ_DEF(X,TYPE,trans); if (!X) { TRANS_FAIL }; X = X->clone(); __localclones.push_back(X);
#define READ_SLOT(X) READ_DEF(X,MG_SLOT,slot)
#define READ_KEYBOARD(X) READ_DEF(X,MG_KEYBOARD_SLOT,slot)
#define READ_STRING(X) READ_DEF(X,MG_STRING,string)
//TODO: COPY_STRING(X) if we want to actually allocate...
#define READ_FLOAT(X) READ_DEF(X,MG_FLOAT,real)

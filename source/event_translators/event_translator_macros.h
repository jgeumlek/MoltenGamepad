#include "../devices/device.h"

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

//TODO: TRANS_FAIL doesn't clean up any cloned translators.
//      potential memory leak, but low priority
//      since the parser should be preventing any of these exceptions.
#define BEGIN_READ_DEF int __index = 0;
#define TRANS_FAIL throw -5;
#define READ_DEF(X,TYPE,LOC)  \
  if (fields.size() > __index && fields.at(__index).type == TYPE) { \
    X = fields.at(__index).LOC; \
    __index++; \
  } else { \
    TRANS_FAIL \
  } 
#define READ_KEY(X) READ_DEF(X,MG_KEY,key)
#define READ_AXIS(X) READ_DEF(X,MG_AXIS,axis)
#define READ_REL(X) READ_DEF(X,MG_REL,rel)
#define READ_INT(X) READ_DEF(X,MG_INT,integer)
#define READ_TRANS(X,TYPE) READ_DEF(X,TYPE,trans); X = X->clone();
#define READ_SLOT(X) READ_DEF(X,MG_SLOT,slot)
#define READ_KEYBOARD(X) READ_DEF(X,MG_KEYBOARD_SLOT,slot)

#pragma once


enum MGType {
  MG_NULL,
  MG_KEY_TRANS,
  MG_REL_TRANS,
  MG_AXIS_TRANS,
  MG_TRANS,
  MG_GROUP_TRANS,
  MG_KEY,
  MG_AXIS,
  MG_REL,
  MG_STRING,
  MG_INT,
  MG_FLOAT,
  MG_BOOL,
  MG_SLOT,
  MG_KEYBOARD_SLOT,
  MG_AXIS_DIR,
};

#define NEGATIVE_AXIS_DIR (1<<12)

#define EXTRACT_AXIS(axis_dir) (axis_dir & (NEGATIVE_AXIS_DIR - 1))

#define EXTRACT_DIR(axis_dir) ( (axis_dir & NEGATIVE_AXIS_DIR) ? -1 : 1)

enum event_state { 
  EVENT_ACTIVE, //Device can currently send this event.
  EVENT_INACTIVE, //Device might be able to send this event, but not right now.
  EVENT_DISABLED, //Device will NEVER be able to send this event.
  //This last one is because all events of a manager are inherited, but a device might not support all.
};

enum entry_type {
  NO_ENTRY,
  DEV_OPTION,
  DEV_KEY,
  DEV_AXIS,
  DEV_REL,
  DEV_ANY,
  DEV_EVENT_GROUP,
};

enum device_claim {
  DEVICE_CLAIMED = 0,
  DEVICE_UNCLAIMED = -1,
};

//Allow us to report a provisional claim.
//a later driver might have a stronger claim.
#define DEVICE_CLAIMED_DEFERRED(X)  (X >= 0 ? X : DEVICE_UNCLAIMED)

enum mg_result_codes {
  SUCCESS = 0,
  FAILURE = -1,
};
  

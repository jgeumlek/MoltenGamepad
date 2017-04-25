#pragma once
#include <libudev.h>
#include <linux/input.h>
#include "plugin_constants.h"

#define ABS_RANGE 32768
class input_source;
class device_manager;
class event_translator;
class group_translator;
class output_slot;





struct MGField {
  MGType type;
  union {
    event_translator* trans;
    group_translator* group_trans;
    int key;
    int axis;
    int rel;
    const char* string;
    int integer;
    bool boolean;
    float real;
    output_slot* slot;
  };
  int16_t flags; //a set of flags, that may be contextual. Field meta-data.
};

#define FLAG_NAMED 1
#define FLAG_DEFAULT 2
#define FLAG_INVERT 4

//These declaration structs will be used to register events and options to be exposed.

struct event_decl {
  //This event will be assigned a numeric id based on the order it was registered.
  //Event ids start at 0.
  const char* name;
  const char* descr;
  enum entry_type type;  //DEV_KEY or DEV_AXIS or DEV_REL
  const char* default_mapping; //will be read by the parser to generate an event translator
};

struct event_group_decl {
  //a name for this event group. It'll be used as an alias for assignment purposes to refer to the events in namelist.
  const char* group_name;
  //the namelist argument is simply a comma separated string of previously registered event names (not aliases).
  //The listed names may include a direction modifier.
  // Ex: "touch_x,touch_y" or "tilt_y,tilt_x-" or "l,r,select,start"
  const char* namelist;
  const char* descr;
  const char* default_mapping; //will be read by the parser to generate a group translator
};

struct option_decl {
  const char* name;
  const char* descr;
  const char* value;  //if type is MG_INT or MG_BOOL, this string will be parsed appropriately.
  MGType type;
};



//These are methods of an input_source that the plugin may call.
struct device_methods {
  size_t size;
  //retrieve the driver-supplied pointer
  void* (*plug_data) (const input_source* dev);
  //Watch the given file descriptor, with a tag to identify it.
  //The tag MUST NOT be equal to dev
  int (*watch_file) (input_source* dev, int fd, void* tag);
  //Set an event's state to be EVENT_ACTIVE, EVENT_INACTIVE, or EVENT_DISABLED.
  //Marking an event as disabled is permanent and cannot be changed later.
  int (*toggle_event) (input_source* dev, int id, event_state state);
  //Send a value for the event of the given id.
  //DEV_KEY events should be 0 or 1 (unpressed or pressed).
  //DEV_AXIS events should be scaled to fill the range -ABS_RANGE to ABS_RANGE
  //This function should ONLY be called from  the context of the process_event callback.
  int (*send_value) (input_source* dev, int id, int64_t value);
  //Send a SYN_REPORT event.
  //Events generated via send_value might not be honored until a SYN_REPORT is sent.
  //This function should ONLY be called from  the context of the process_event callback.
  int (*send_syn_report) (input_source* dev);
  //Permanently remove a registered option from this input source
  int (*remove_option) (input_source* dev, const char* opname);
  //Print a message labelled as coming from this device.
  int (*print) (input_source*, const char* message);
  //Request to have a callback used every 10ms or so.
  int (*request_recurring_events) (input_source* dev, bool wants_recurring);
};

struct device_plugin {
  size_t size;
  //The name of this device and its profile.
  //An appropriate number will be appended to this name.
  const char* name_stem;
  //A unique string to identify this device if available.
  const char* uniq;
  //A string identifying how this device is connected to the user's machine.
  const char* phys;
  //Store the input_source* pointer and perform whatever initialization is required.
  //e.q. watching a certain file.
  int (*init) (void* plug_data, input_source* dev);
  //Free whatever resources are claimed by the plug_data
  int (*destroy) (void* plug_data);
  //Return an informative description of this device.
  const char* (*get_description) (const void* plug_data);
  //Return a string identifying what type of device this is.
  //Two special types are recognized:
  // -"gamepad" for gamepad-like devices.
  // -"keyboard" guarantees this device will be assigned to the keyboard slot
  //Any other string will be treated as a separate type.
  //When requesting a slot, a slot will be avoided if it already has a device of that type.
  const char* (*get_type) (const void* plug_data);
  //Called when the file descriptor provided via watch_file has data to be read.
  int (*process_event) (void* plug_data, void* tag);
  //called when a device option is changed.
  int (*process_option) (void* plug_data, const char* opname, MGField opvalue);
  //Called to upload a force feedback effect. (A.K.A. Rumble vibration)
  //The return value should be a non-negative id that will represent this effect.
  int (*upload_ff) (void* plug_data, ff_effect* effect);
  //Called to erase the specified effect.
  int (*erase_ff) (void* plug_data, int id);
  //Called to activate a previously uploaded effect.
  int (*play_ff) (void* plug_data, int id, int repeats);
  //Called every 10ms or so if request_recurring_events
  int (*process_recurring_event) (void* plug_data);
};

struct manager_methods {
  size_t size;
  //retrieve the driver-supplied pointer
  void* (*plug_data) (const device_manager*);
  //expose an event
  int (*register_event) (device_manager*, event_decl ev);
  //expose an option that is independent for each device.
  int (*register_dev_option) (device_manager*, option_decl opt);
  //register an event alias, such as "primary" -> "a"
  int (*register_alias) (device_manager*, const char* external, const char* local);
  //register an option that applies to the device manager as a whole.
  int (*register_manager_option) (device_manager*, option_decl opt);
  //Adds a new input source using the device_plugin callbacks and set its plug_data.
  input_source* (*add_device) (device_manager*, device_plugin, void* dev_plug_data);
  //Remove a device. The device's plug_data object should be assumed destroyed.
  int (*remove_device) (device_manager*, input_source*);
  //Print a message coming from this manager.
  int (*print) (device_manager*, const char* message);
  //register a group of events and provide a default group translator for them.
  int (*register_event_group) (device_manager*, event_group_decl);
};

struct manager_plugin {
  size_t size;
  //A name for this manager and its profile
  const char* name;
  //set to true if the manager's profile should listen for changes to the gamepad profile.
  bool subscribe_to_gamepad_profile;
  //Store the device_manager* reference
  //This init callback should register all events and options.
  //No input sources should be created during this callback
  int (*init) (void* plug_data, device_manager*);
  //free the resources of the plug_data
  int (*destroy) (void* plug_data);
  //If this manager creates input sources independently of udev events, 
  //any such processes should be started via this callback.
  //Any functionality that depends on manager options should be done in start
  int (*start) (void* plug_data);
  //If the udev device is relevant to this manager:
  //  add or remove input_sources as appropriate
  //  return DEVICE_CLAIMED
  //If the device is relevant, but it is a low-priority match:
  //  do nothing
  //  return DEVICE_CLAIMED_DEFERRED(X)
  //    where X is a positive integer.
  //
  //  this deferred claim MUST NOT be used on device remove events.
  //
  //otherwise:
  //  return DEVICE_UNCLAIMED
  //This udev_device reference will be freed after this callback end.
  //The manager should create its own reference if needed later.
  int (*process_udev_event) (void* plug_data, struct udev* udev, struct udev_device* dev);
  //UNIMPLEMENTED
  int (*process_manager_option) (void* plug_data, const char* opname, MGField opvalue);
  //This is called if DEVICE_CLAIMED_DEFERRED() was used, and this driver ultimately
  //had the strongest claim to the device.
  //This should add an input_source and return DEVICE_CLAIMED.
  //
  //If the driver never uses DEVICE_CLAIMED_DEFERRED(), this function is not used.
  int (*process_deferred_udev_event) (void* plug_data, struct udev* udev, struct udev_device* dev);
};
// init is called first
// Then process_manager_option for each option registered in init
// Finally start is called


struct moltengamepad_methods {
  size_t size;
  //Add a device manager with the given callbacks and plug_data
  device_manager* (*add_manager) (manager_plugin, void* manager_plug_data);
  //Request that this input source be assigned a slot.
  int (*request_slot) (input_source*);
  //If grabbed == true
  //  perform appropriate file permission shuffling to hide this device
  //If grabbed == false
  //  undo the above
  int (*grab_permissions) (udev_device* dev, bool grabbed);
};

//A struct of pointers all the methods the plugin will be provided.
struct plugin_api_header {
  size_t size;
  struct moltengamepad_methods* mg;
  struct manager_methods* manager;
  struct device_methods* device;
};

//Use the header to get offsets to the relevant sections rather than
//accessing directly, as sizes of these structs may grow.
struct plugin_api {
  plugin_api_header head;
  struct moltengamepad_methods mg;
  struct manager_methods manager;
  struct device_methods device;
};

//A quick helper macro to check all sizes are in agreement
#define API_EXACT(X) \
  (X.head.size == sizeof(plugin_api_header) && \
    X.head.mg->size == sizeof(moltengamepad_methods) && \
    X.head.manager->size == sizeof(manager_methods) && \
    X.head.device->size == sizeof(device_methods))
//Another helper macro to check sizes are compatible.
//Care must be taken to still initialize all values!
#define API_COMPAT(X) \
  (X.head.size >= sizeof(plugin_api_header) && \
    X.head.mg->size >= sizeof(moltengamepad_methods) && \
    X.head.manager->size >= sizeof(manager_methods) && \
    X.head.device->size >= sizeof(device_methods))

//We define two helper macros PLUGIN_INIT and PLUGIN_INIT_FUNC.
//PLUGIN_INIT should be used when defining your plugin init function,
//like so:
//
//   PLUGIN_INIT(plugin_name)(plugin_api api) {
//     ...
//   }
//
//The "(plugin_api api)" portion is simply the parameter section for your init function.
//This section was excluded from the macro for clarity.
//Note that this macro handles defining the return type of the function.
//
//When compiling as an external plugin, this resolves to the appropriate
//standard entry point name.
//
//When compiling as a static plugin, this uses the given plugin_name and
//resolves to a unique name for this init function to avoid conflicts.
//It also causes the plugin to be loaded automatically by statically calling
//a function to register the plugin.
//
//PLUGIN_INIT_FUNC simply resolves to the function name and return type, without
//adding any extern qualifiers or auto-registration features.
//It is useful if you need to refer to this function as a friend elsewhere.
//
//    friend PLUGIN_INIT_FUNC(plugin_name)(plugin_api api);
//
//It includes the return type but excludes the parameters so as to match
//the behaviour of the PLUGIN_INIT macro.

#ifdef PLUGIN

#define PLUGIN_INIT(X) extern "C" int plugin_init
#define PLUGIN_INIT_FUNC(X) int plugin_init

#else


#define PLUGIN_INIT(X) \
int X##_plugin_init(plugin_api);\
int X##_loaded = register_plugin(&X##_plugin_init);\
int X##_plugin_init

#define PLUGIN_INIT_FUNC(X)  int X##_plugin_init
#endif



//Leave this alone, it will be handled automatically. It is the entry point for loading dynamic plugins.
extern "C" int plugin_init(plugin_api);
extern int register_plugin( int (*init) (plugin_api));

#pragma once
#include <libudev.h>
#include "plugin_constants.h"

#define ABS_RANGE 32768
class input_source;
class device_manager;
class event_translator;
class advanced_event_translator;
class output_slot;





struct MGField {
  MGType type;
  union {
    event_translator* trans;
    advanced_event_translator* adv_trans;
    int key;
    int axis;
    int rel;
    const char* string;
    int integer;
    bool boolean;
    float real;
    output_slot* slot;
  };
};

//These declaration structs will be used to register events and options to be exposed.

struct event_decl {
  //This event will be assigned a numeric id based on the order it was registered.
  //Event ids start at 0.
  const char* name;
  const char* descr;
  enum entry_type type;  //DEV_KEY or DEV_AXIS or DEV_REL
  const char* default_mapping = ""; //will be read by the parser to generate an event translator
};

struct option_decl {
  const char* name;
  const char* descr;
  const char* value;  //if type is MG_INT or MG_BOOL, this string will be parsed appropriately.
  MGType type;
};



//These are methods of an input_source that the plugin may call.
struct device_methods {
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
};

struct device_plugin {
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
};

struct manager_methods {
  //retrieve the driver-supplied pointer
  void* (*plug_data) (const device_manager*);
  //expose an event
  int (*register_event) (device_manager*, event_decl ev);
  //expose an option that is independent for each device.
  int (*register_dev_option) (device_manager*, option_decl opt);
  //register an event alias, such as "primary" -> "a"
  int (*register_alias) (device_manager*, const char* external, const char* local);
  //(UNIMPLEMENTED) register an option that applies to the device manager as a whole.
  int (*register_manager_option) (device_manager*, option_decl opt);
  //Adds a new input source using the device_plugin callbacks and set its plug_data.
  input_source* (*add_device) (device_manager*, device_plugin, void* dev_plug_data);
  //Remove a device. The device's plug_data object should be assumed destroyed.
  int (*remove_device) (device_manager*, input_source*);
  //Print a message coming from this manager.
  int (*print) (device_manager*, const char* message);
};

struct manager_plugin {
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
  int (*start) (void* plug_data);
  //If the udev_device is relevant to this manager:
  //  add or remove input_sources as appropriate
  //  return DEVICE_CLAIMED
  //otherwise:
  //  return DEVICE_UNCLAIMED
  //This udev_device reference will be freed after this callback end.
  //The manager should create its own reference if needed later.
  int (*process_udev_event) (void* plug_data, struct udev* udev, struct udev_device* dev);
  //UNIMPLEMENTED
  int (*process_manager_option) (void* plug_data, const char* opname, MGField opvalue);
};


struct moltengamepad_methods {
  //Add a device manager with the given callbacks and plug_data
  device_manager* (*add_manager) (manager_plugin, void* manager_plug_data);
  //Request that this input source be assigned a slot.
  int (*request_slot) (input_source*);
};

//A struct of all the methods the plugin will be provided.
struct plugin_api {
  struct moltengamepad_methods mg;
  struct manager_methods manager;
  struct device_methods device;
};

//call this with a function pointer to be ran when loading this plugin.
//This function should call add_manager as appropriate.
int register_plugin( int (*init) (plugin_api));
//If you include a line like
//     int loaded = register_plugin(my_init_function)
//that declares a static global int, then the my_init_function
//will be called automatically when MoltenGamepad starts up.
//Otherwise, nothing at all will happen!

//Leave this alone, it will be handled automatically
extern int (*plugin_init) (plugin_api);
  
  
  

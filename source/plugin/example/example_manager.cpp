#include "example_plugin.h"

manager_methods example_manager::methods;

example_manager::example_manager() {
  //Any basic initialization can be done here.
  //It is INVALID to create any devices here.
}

int example_manager::init(device_manager* ref) {
  //This function serves to provides us with the reference
  //used to called the various functions of the Plugin API.
  this->ref = ref;

  //Now that we have the ref, we can start initializing our profiles
  //and registering events and options.
  init_profile();

  //It is still INVALID to create any devices here.
  return 0;
}

int example_manager::start() {
  //This is when any real work should be started,
  //such as a separate thread waiting for connections.

    //start_listening_thread()...

  //Devices can now be created.

  //If driver-level option values are loaded from a file,
  //it is guaranteed all will have been processed by the
  //time start() is called.

  return 0;
}

example_manager::~example_manager() {
  //as of now, this is only called as MoltenGamepad exits.
  //Now would be a good time to tidy any files or sockets.
}

void example_manager::init_profile() {
  //Init some event translators

  //register events from some convenient static array.
  const event_decl* ev = &example_events[0];
  for (int i = 0; ev->name && *ev->name; ev = &example_events[++i]) {
    methods.register_event(ref, *ev);
  }
  //register device-level options
  methods.register_dev_option(ref,{"foo","simple test option","false",MG_BOOL});

  //create a convenience function...
  auto set_alias = [&] (const char* external, const char* internal) {
    methods.register_alias(ref, external, internal);
  };

  //Init some aliases...
  set_alias("first","one");
  set_alias("second","two");
  set_alias("third","three");
  set_alias("fourth","four");
  //... as needed.

  //Now when the user sets gamepad.third, it will
  //resolve to updating the event "three" exposed on these devices.

  //Register some driver-level options.
  methods.register_manager_option(ref, {"test_option","a simple option to demonstrate plugin features","false",MG_BOOL});
};

int example_manager::accept_device(struct udev* udev, struct udev_device* dev) {
  //using the udev references, do whatever is needed to determine whether the device is relevant to this driver.
  //These references will be freed after this function ends! Use udev to store references if needed.

  //the action from udev_device_get_action() might be null. In this case it is a device
  //already connected when MoltenGamepad started. Treat it like an "add".

  //If it is an "add" action and relevant, create a new device object.
  //  example_device exdev = new example_device();
  //  device_plugin implementation = example_dev;
  //    (set fields like uniq or phys specific to this device if desired...)
  //  methods.add_device(ref, implementation, exdev);
  //  return DEVICE_CLAIMED;

  //If it is a "remove" action and relevant, destroy the device object.
  //  ... find the appropriate example_device exdev...
  //  methods.remove_device(ref, exdev->ref);
  //  return DEVICE_CLAIMED;

  //Otherwise...
  //  return DEVICE_UNCLAIMED;
  return DEVICE_UNCLAIMED;
}

int example_manager::process_option(const char* name, const MGField value) {
  //This is called whenever a driver-level option is set, and it provides the value.

  //the value is already stored somewhere in MoltenGamepad, but the plugin does
  //not have access to it. If other code needs access to this value, store a copy somewhere
  //accessible.

  //If there are multiple options, use the name to distinguish them.

  //in the future it should be possible to signal errors, such as invalid values.
  //For now, might as well signal success by returning 0.
  return 0;
}


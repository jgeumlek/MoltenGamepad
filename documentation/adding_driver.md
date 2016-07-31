
See the steam controller driver for a simple driver.

A driver has to provide instances of `input_source` and `device_manager`. Both are declared in `devices/device.h`

This documentation is a work-in-progress, and the internals are unstable.

A future change to further encapsulate drivers into a plug-in framework is being considered.

#Device Manager

The `device_manager` serves two purposes: recognize when relevant devices are added/removed, and keeping track of its own devices.

##Constructor

Along with whatever initialization is required, it is recommended that the contstructor also set up the appropriate event translators in the driver's profile

##Overrides

You need to override two methods.

    int accept_device(struct udev* udev, struct udev_device* dev)

This method will be called with devices as they are discovered by udev. If the passed in device is not valid for this driver, return a negative value.

If the device is valid for this driver, then create an appropriate instance of `input_source` and return `0`.

    void for_each_dev(std::function<void (const input_source*)> func) {};
    
This method should loop over all devices owned by this driver and call the provided function upon them. This method should hide the details of whatever data structures and locking is required.

##adding/removing a device

This is done via `add_device(...)` and `remove_device(...)` on the `moltengamepad` object. These both deal with shared pointers, and the `input_source` will likely be deleted when the latter is called.

When adding the device, it is the driver's responsibility to ensure the `input_source` has its profile subscribed to the driver's profile. This is easily done with the `copy_into` method of profiles.

#Input Source

The `input_source` class should represent one logical device.

##Constructor

It is important that the constructor use `register_event` and `register_option` to declare all events and options. Further, these must match the events/options registered by the profile of the driver (though they may be a subset).

Keep track of the order events are registered. They are assinged numeric ids in this order to avoid repeated string comparisons.

##Waiting for events

Use `watch_file` to add a device node or file to wait for events. This method takes a `void* tag` that should be a unique value to identify the file to read events from. It CANNOT be a pointer to the `input_source` itself.

##Processing events

When a watched file has information to be read, `process(...)` will be called with the `void* tag` provided. This method should use the tag to identify the file to be read and generate events as needed.

When an event is detected, call `send_value(int id, int64_t value)`. The `id` is a number representing the event, it corresponds to the order events were registered. 

For a button, the `value` should be 0 or 1, for unpressed or pressed.

For an axis, the `value` should lie in -32768 to 32768. The values should be scaled to best fit this range.

Conventions:

- Any thumbstick should have negative values in the up and left directions
- Any analog trigger should be at (or close to) -32768 when unpressed and 32768 when fully pressed


##SYN_REPORT

It is the input_sources responsibility to call `take_event` on the output source with a `SYN_REPORT` event when appropriate. This should generally be done after sending any events.

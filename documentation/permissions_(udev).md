MoltenGamepad requires file permissions to function, and you are unlikely to have these permissions by default. Udev rules help to automate the setup of these permissions.

# Intro to udev rules

Udev rules match constraints with `==`, update and set values with `=`, and use plurals like `DRIVERS` rather than `DRIVER` to check values of parent devices.

Rules can be placed in files in `/etc/udev/rules.d/##-<name>.rules` where `<name>` is a short identifier and `##` is a two digit number enforcing the order rules are applied.

All rules below can go in a file named `99-moltengamepad.rules`. Since they require some customization on user name or group names, you'll need to make this file yourself.

# Basic Requirements

In order to create virtual devices, MoltenGamepad requires write access to `uinput`. Below is an example udev rule for this.

    KERNEL=="uinput", MODE="0660", GROUP="uinput", OPTIONS+="static_node=uinput"
    
    # Note the "uinput" group that must exist, and be sure to place yourself in that group. You might need to re-log in.

In order to read events from an event device, MoltenGamepad requires read access to the event device. Devices resembling a joystick/game pad are automatically tagged, and this tag is later used to ensure the current active user has access to these devices. Other devices, such as mice, keyboards, or Wii remotes (since they do not resemble game pads as far as the kernel knows), will not have the appropriate read permissions.

Tagging a device as a joystick when it is not is not recommended, as some software might search for these tags and get confused.

Below is an example udev rule that makes all Wii devices world-readable.

    DRIVERS=="wiimote", MODE="666"

# Advanced Requirements

## Rumble

Should MG ever support rumble, write permissions will be needed on the event devices. (and read permission on uninput)

## Allow MoltenGamepad to change permissions

This requires MoltenGamepad to be the owner of the device node. The following rule does this for all joystick-like devices, and the above wiimote rule can be updated with its own `OWNER=` clause.

    SUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="?*", OWNER="<username>", MODE="660"

Note the need to specify a user. If you are the sole user on your computer, you can just set this to your user account. If you have multiple users, this may require a specific moltengamepad user to be created (which will need access to its own configuration files)

Why would one want MoltenGamepad to have owner rights? It allows for the device to have its permissions changed. This allows MoltenGamepad to open the device, and then remove all permissions, preventing other applications from opening this device. This can prevent software from seeing both the original device and the translated virtual device. Software that already does hotplugging might be able to open the original device before MoltenGamepad gets a chance to change the permissions.

### Aside: uaccess

Many distros have game pads owned by root, yet user software needs to read them. The devices resembling joysticks get tagged with uaccess, which later creates file access controls such that the current user (the one logged into the display server) inherits all permissions of the owner of the device. This means only the active seated user can read these game pads. This is pretty slick, but gets in our way. MoltenGamepad needs read permissions to read the device, but the owner can't have read permissions else uaccess will pass this permission along.

## LEDs and sysfs attributes

I am unaware of a clean way of granting permissions to sub-files of a device node, such as the `brightness` attribute exposed in sysfs. The following rule shows how the LED brightnesses for Wii remotes can be set in udev, using some subshells.


    SUBSYSTEM=="leds", ACTION=="add", DRIVERS=="wiimote", RUN+="/bin/sh -c 'chgrp <groupname> /sys%p/brightness'", RUN+="/bin/sh -c 'chmod g+w /sys%p/brightness'"

Note again the group name required.

# Security Considerations

Allowing the current user access to `uinput` means malicious software covertly running as the current user can create/spoof input devices, such as injecting key strokes or clicks. I think current X sessions suffer this regardless.

Allowing read access to devices means malicious software could read them. The privacy/sensitivity of game pad inputs is likely to be minimal, though, as long as you avoid typing in credentials with your controller. For joystick like devices, this access already exists. For other devices like Wii remotes, adding this access should be negligible. (Though malicious software could open the accelerometers to drain your batteries)

Access to LEDs can let them also change your LEDs, which should also be minor. The `/bin/sh` is run as root, so be careful what commands are run in a udev `RUN` clause.

Running MoltenGamepad as root is not recommended.

Adding your user to the input group is not ideal, as it gives access to raw keyboard devices.

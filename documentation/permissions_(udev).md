MoltenGamepad requires file permissions to function, and you are unlikely to have these permissions by default. Udev rules help to automate the setup of these permissions.

See the `installation` directory for some pre-made udev rules files.

This documentation is only for those who wish to write their own udev rules, which may be needed for strange devices that do not fall under the bundled udev rules.

# Intro to udev rules

Udev rules match constraints with `==`, update and set values with `=`, and use plurals like `DRIVERS` rather than `DRIVER` to check values of parent devices.

Rules can be placed in files in `/etc/udev/rules.d/##-<name>.rules` where `<name>` is a short identifier and `##` is a two digit number enforcing the order rules are applied.

All rules below can go in a file named `99-moltengamepad.rules`. Since they require some customization on user name or group names, you'll need to make this file yourself.

# Basic Requirements

In order to create virtual devices, MoltenGamepad requires write access to `uinput`. Below is an example udev rule for this.

    KERNEL=="uinput", MODE="0660", GROUP="uinput", OPTIONS+="static_node=uinput"
    
    # Note the "uinput" group that must exist, and be sure to place yourself in that group. You might need to re-log in.

In order to read events from an event device, MoltenGamepad requires read access to the event device. Devices resembling a joystick/game pad are automatically tagged, and this tag is later used to ensure the current active user has access to these devices. Other devices, such as mice, keyboards, or Wii remotes (since they do not resemble game pads as far as the kernel knows), will not have the appropriate read permissions.

It is not recommended to forcefully tag a device as a joystick when it is not, as some software might search for these tags and get confused.

Below is an example udev rule that makes all Wii devices world-readable.

    DRIVERS=="wiimote", MODE="0666"

# Advanced Requirements

## Rumble

In order to send rumble events to a device, MoltenGamepad requires write access to the event device.

## Allow MoltenGamepad to change permissions (Hide the original device)

The `change_permissions` feature of MoltenGamepad is designed to achieve the following:

1. allow MoltenGamepad to read the event device
2. prevent all other programs from reading this event device

Accomplishing this can be done by setting up MoltenGamepad as a separate user with unique privileges to read these devices, but this is cumbersome both in setting up this new user and removing the existing uaccess granting permission to the current user.

As a slightly simpler method, the `change_permissions` feature removes all file permissions after opening the device, thereby preventing all other software from opening the original device. This requires MoltenGamepad to be the owner of the device node. The following rule does this for all joystick-like devices, and the above wiimote rule can be updated with its own `OWNER=` clause.

    SUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="?*", OWNER="<username>", MODE="0660"

Note the need to specify a user. If you are the sole user on your computer, you can just set this to your user account. If you have multiple users, this may require a specific moltengamepad user to be created (which will need access to its own configuration files)

Why would one want MoltenGamepad to have owner rights? It allows for the device to have its permissions changed. This allows MoltenGamepad to open the device, and then remove all permissions, preventing other applications from opening this device. This can prevent software from seeing both the original device. Software that already does hotplugging might be able to open the original device before MoltenGamepad gets a chance to change the permissions.

### Aside: uaccess

Most are familiar with file permissions where read/write/execute permission is specified separately for users who own the file, users in the file's group, and all other users. File access control lists further refine this, allowing more complicated ways of granting permissions to specific users.

Many distros want input devices to be owned by root, and the display server is trusted to handle reading mice and keyboards. But what about gamepads? They are owned by root, but games wish to read them without super-user privileges. The uaccess tag uses file access control lists to grant permissions to the current "active" user (the one logged into the display server). This means a user physically at the machine can run games and can read these devices, but other users logged in remotely cannot. Pretty reasonable.



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

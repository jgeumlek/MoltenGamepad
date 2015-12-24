#MoltenGamepad
*Flexible input device remapper, geared towards gamepads*

(Tested only on Arch Linux, 64-bit)

##Building

    make

If you get undefined KEY_* errors, you'll need to remove those lines from the eventlists. (Sorry, no script to generate these just yet)

It requires udev and uinput.



##Running

    ./moltengamepad

MoltenGamepad will start up, search existing devices, and wait for devices to show up. MoltenGamepad will read commands from its standard input.

    ./moltengamepad --help

to see the available command line arguments.

You'll need appropriate permissions on the uinput device node, as well as any device nodes you wish MoltenGamepad to work with.

You might need to use --uinput-path to tell MoltenGamepad where the uinput device is on your system. (You might even need to modprobe uinput)



##Motivation

You have input devices. You have software waiting for input events. Unfortunately, for one reason or another, they aren't speaking the same language of events. Perhaps you recently swapped out your input device. Perhaps the software has hardcoded inputs. MoltenGamepad acts as an translation layer by reading the input device and outputting new events.

Possible use cases include:

* Making a wide variety of input devices all appear to be the same, allowing software configurations to be agnostic of the currently used device.
* Getting around software limits, such as using a keyboard in a game that requires gamepads, or vice versa.
* Handling split event devices, allowing multiple input sources to act as one synthetic device.
* Handling combined event devices, allowing a single input device to appear as multiple input sources.
* Empowering unorthodox input solutions and behaviors, such as for an art installation.


##Features

* Create virtual gamepads that match the expectations of most modern games.
* Persistent virtual devices, allowing hotswapping of controllers in software that doesn't natively.
* Specialized wiimote support for such an unusual input device.
* Flexible generic device driver for translating most input devices.
* Human readable/editable configuration and mapping files.
* Easy loading and saving of event mapping profiles.
* Supports a command FIFO for controlling a running instance of MoltenGamepad via scripting.


##Documentation

See this README, the various files in the `documentation` folder, the output of `./moltengamepad --help`, and the output of the `help` command while running MoltenGamepad.

##Default Locations

By default, configuration files are placed in `$XDG_CONFIG_HOME/moltengamepad`, defaulting to `$HOME/.config/moltengamepad`

Profiles are located in a `profiles` subdirectory of the configuration directory.

Generic driver specifications are in a `gendevices` subdirectory of the configuration directory.

The command FIFO is placed at `$XDG_RUNTIME_DIR/moltengamepad` by default.

##Requirements

You'll need write permission to your uinput device. You'll also need read permissions on any event device you wish to translate with MoltenGamepad.

udev rules are the best way to make these permissions persistent/automatic.

The wiimote driver expects you to already be able to connect your wiimote via bluetooth. MoltenGamepad uses the wiimote support of the Linux kernel.

##Known Issues

* Changing input mappings does not clear out previous values, potentially leading to stuck inputs.
* Multiple inputs mapped to the same output event clobber each other. Desired behavior uncertain.
* Framework needs some clean up before it can truely be called "extendable".
* Software may attempt to read both the virtual and original devices, leading to duplicated events.
* No rumble support.
* Will likely add some amount of input latency, though it hasn't been measured beyond playtests.
* Automagic configuration can lead to minor undesired behavior with aliases for events.



##Troubleshooting FAQ-ish Section

###What's this about file permissions for the devices?
MoltenGamepad will fail if you don't have the right permissions, and you likely won't have the right permissions unless you do some extra work. Though not recommended for regular use, running MoltenGamepad as a super-user can be a useful way to try it out before you have the permissions sorted out.

You need write access to uinput to create the virtual gamepads. MoltenGamepad assumes uinput is located at /dev/uinput which might not be true on other distros. If you know where uinput is on your system, use the --uinput-path flag.

You need read access to the various event devices. Most systems automatically tag event devices that look like joysticks/gamepads to be readable by the current user. Unorthodox devices like a wiimote will need special udev rules. See the udev rules of the xwiimote project for an example for wiimotes.

###How do I connect a wiimote?

That is outside the scope of MoltenGamepad. Your bluetooth system handles this. This software assumes your bluetooth stack and kernel wiimote driver are already working and usable.

See https://wiki.archlinux.org/index.php/XWiimote for more information on connecting wiimotes.

Note that this uses the kernel driver, not one of the various wiimote libraries like cwiid that do handle connections, so the info on https://wiki.archlinux.org/index.php/Wiimote is not applicable. To use MoltenGamepad with wiimotes, do not use cwiid and wminput.

Aside from seeing the device entries created by the kernel driver, a successful connection can be verified by the Wiimote LEDs switching to having just the left-most one lit. Prior to that, all 4 LEDs will be blinking while the wiimote is in sync mode.

I have had some confusing experience of the wiimote connections sometimes consistently failing then magically working when I try later. I've also seen an unrelated issue when repeatedly and quickly disconnecting and reconnecting a controller. These are once again, outside the scope of MoltenGamepad. Some of this might be my bluetooth hardware being flakey.

In the former case, the bluetooth connection fails. In the latter case, the connection succeeds, but the no kernel devices are created. It seemed like the wiimote was connected and was on, but all 4 of its LEDs were off since the kernel driver never set them.

###What is a MoltenGamepad driver?

A driver handles a certain class of input devices. Its responsibilities include identifying appropriate devices and knowing when they are removed. A driver also includes an implementation of an input source, providing the code to actually read and process input events.

MoltenGamepad has a wiimote driver built in, handling the gritty details of the Linux kernel event devices made by a wiimote. It supports swapping events when a wiimote extension is changed, along with combining the extension inputs with the wiimote inputs.

MoltenGamepad also contains support to read special configuration files to create generic drivers. These drivers can identify input devices by their reported name, and can provide meaningful names to their event codes. 

###How does setting a mapping work?

The general syntax is

    <profile>.<in event> = <out event>

For example,

    wiimote.wm_a = primary

Every driver has a profile, as does every device. Their profiles are named the same. (Ex. the `wiimote` driver has a a profile named `wiimote`). Changing a mapping in a driver's profile will change that mapping in all connected devices of that driver, along with any future connected devices. Changing a mapping in a device's profile changes it for just that device, and may be overwritten by a future change to the driver's profile.

###Slots?

Slots refer to the virtual output devices, so named to echo the "player slots" seen on game consoles as well as avoiding using the word "device" everywhere in every context. By default, MoltenGamepad creates 4 virtual gamepad slots, one virtual keyboard slot and one blank dummy slot. Input sources are assigned to the first virtual gamepad that has no connected devices. If none are available, the input source is placed onto the dummy slot.

Note that slots can only process appropriate events. Sending a keyboard key press to a virtual gamepad will lead to it being silently ignored.

Why not create a virtual device that can send all events? Some software expects gamepads to only have certain event codes, and not others. Keeping separate virtual devices greatly aids autodetection magic.

To move a device to a different slot, use this syntax:

    move <device> to <slot>

Need to find a device name or slot name?

    print slots
    print devices



###Primary, Secondary, Third, Fourth? What are those?

These are the face buttons on a controller. Commonly labelled A,B,X,Y. At the moment, this assignment is hardcoded, but the goal is to allow swapping this at the output layer rather than requiring all profiles to be altered when a change is needed by the user:

* primary == BTN_SOUTH (BTN_A)
* secondary == BTN_EAST (BTN_B)
* third == BTN_WEST (BTN_Y)
* fourth == BTN_NORTH (BTN_X)

Names in parentheses are deprecated event names for these event codes, and do not necessarily correlate with the printed labels seen on controllers. They were chosen to reflect intuitive purposes for the buttons, and avoid the quagmire of the inconsistent/ambiguous labels A,B,X,Y.

Since all event codes are recognized, feel free to use `btn_south` instead of `primary` in your profiles. (But no guarantees offered on how that setting will be affected by the future capability to swap the meaning of `primary`)




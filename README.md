#MoltenGamepad
*Flexible input device remapper, geared towards gamepads*

(Tested only on Arch Linux, 64-bit)
##Motivation

You have input devices. You have software waiting for input events. Unfortunately, for one reason or another, they aren't speaking the same language of events. Perhaps you recently swapped out your input device. Perhaps the software has hardcoded inputs. MoltenGamepad acts as an translation layer by reading the input device and outputting new events.

Possible use cases include:

* Making a wide variety of input devices all appear to be the same, allowing software configurations to be agnostic of the currently used device.
* Getting around software limits, such as using a keyboard in a game that requires gamepads, or vice versa.
* Handling split event devices, allowing multiple input sources to act as one synthetic device.
* Handling combined event devices, allowing a single input device to appear as multiple input sources.
* Empowering unorthodox input solutions and behaviors, such as for an art installation.

For example, perhaps you have a Wii remote and you want to play some games. Unfortunately, the games expect to see controllers that look like an XBox 360 controller, not the multitude of separate event devices created by the Wii remote driver in the linux kernel. MoltenGamepad can translate that Wii remote's events, making it work. By using a special MoltenGamepad driver, the translation can better utilize the wiimote features, such as extensions.


##Features

* Create virtual gamepads that match the expectations of most modern games.
* Persistent virtual devices, allowing hotswapping of controllers in software that doesn't natively.
* Specialized wiimote support for such an unusual input device, making wiimotes incredibly easy to use for gaming.
* Flexible generic device driver for translating most input devices, with very easy configuration.
* All configuration files are designed to be human-readable and editable.
* Easy loading and saving of event mapping profiles.
* Profiles are changeable at run time, of course.
* Supports a command FIFO for controlling a running instance of MoltenGamepad via scripting.

MoltenGamepad targets a set-it-and-forget-it daemon-like usage pattern,  where devices can connect or disconnect freely. Its main purpose is letting a user "standardize" their software to expect just one type of controller, and then automagically transform connected input devices to match that standardized abstraction. 

##Building

    make

If you get undefined KEY_* errors, you'll need to remove those lines from the eventlists. (Sorry, no script to generate these just yet)

It requires udev and uinput.

If you also have the [scraw](https://gitlab.com/dennis-hamester/scraw) and [scrawpp](https://gitlab.com/dennis-hamester/scrawpp) libraries installed, there is some steam controller support available, but it is not included in the default make target.

    make steam
    
This make target will build and include the steam controller support.

##Running

    ./moltengamepad

MoltenGamepad will start up, search existing devices, and wait for devices to show up. MoltenGamepad will read commands from its standard input.

    ./moltengamepad --help

to see the available command line arguments.


You'll need appropriate permissions on the uinput device node, as well as any device nodes you wish MoltenGamepad to work with.

You might need to use `--uinput-path` to tell MoltenGamepad where the uinput device is on your system. (You might even need to modprobe uinput)


##Getting Started

When started, MoltenGamepad will create its virtual outputs, wait for input sources it recognizes, and will assign them to a slot upon their first key press.

WARNING: Out of the box, MoltenGamepad will only have the included wiimote driver. Unless you are using wiimotes, you'll need to create some config files describing a generic driver for your device before MoltenGamepad will do anything useful. (See `documentation/gendev.md` for details.)

MoltenGamepad will also listen on stdin for user commands, such as changing a mapping or moving an input source to a different virtual output.

For use with most games, use the `--mimic-xpad` option to make the virtual outputs appear as a wired 360 controller, which often has premade support in games (and notably Steam)

    ./moltengamepad --mimic-xpad

Another useful option is `--make-fifo`, which creates a named pipe so that the running instance can be controlled from scripts and such.



##Additional Documentation

See this README, the various files in the `documentation` folder, the output of `./moltengamepad --help`, and the output of the `help` command while running MoltenGamepad.

##Default Locations

By default, configuration files are placed in `$XDG_CONFIG_HOME/moltengamepad`, defaulting to `$HOME/.config/moltengamepad`

Profiles are located in a `profiles` subdirectory of the configuration directory.

Generic driver specifications are in a `gendevices` subdirectory of the configuration directory.

The command FIFO is placed at `$XDG_RUNTIME_DIR/moltengamepad` by default.

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

You need write access to uinput to create the virtual gamepads.

You need read access to the various event devices you wish to read. Most systems automatically tag event devices that look like joysticks/gamepads to be readable by the current user. Unorthodox devices like a wiimote will need special udev rules.

###What is a MoltenGamepad driver?

A driver handles a certain class of input devices. Its responsibilities include identifying appropriate devices and knowing when they are removed. A driver also includes an implementation of an input source, providing the code to actually read and process input events.

MoltenGamepad has a wiimote driver built in, handling the gritty details of the Linux kernel event devices made by a wiimote. It supports swapping events when a wiimote extension is changed, along with combining the extension inputs with the wiimote inputs.

MoltenGamepad also contains support to read special configuration files to create generic drivers. These drivers can identify input devices by their reported name, and can provide meaningful names to their event codes.

The dream here is to have a variety of drivers, enabling interesting features of certain hardware or gathering unorthodox input sources. Perhaps one might read controller inputs off the network/chatroom. Or expose controller inputs onto the file system.

REMINDER: At the moment, only the wiimote driver will be active on a bare installation of MoltenGamepad.

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

These are the face buttons on a controller. Commonly labelled A,B,X,Y. Each output slot maintains an assignment of these four events onto the four standard evdev events for a controller's face buttons. The default mapping looks like:

* primary == BTN_SOUTH (BTN_A)
* secondary == BTN_EAST (BTN_B)
* third == BTN_NORTH (BTN_X)
* fourth == BTN_WEST (BTN_Y)

Names in parentheses are deprecated event names for these event codes, and do not necessarily correlate with the printed labels seen on controllers. They were chosen to reflect intuitive purposes for the buttons, and avoid the quagmire of the inconsistent/ambiguous labels A,B,X,Y.

Since all event codes are recognized, one may use `btn_south` instead of `primary` in one's profiles. (But no guarantees offered on how that setting is affected by swapping the meaning of `primary`)

###How do I connect a wiimote?

That is outside the scope of MoltenGamepad. Your bluetooth system handles this. This software assumes your bluetooth stack and kernel wiimote driver are already working and usable. A simple session with `bluetoothctl` works well, and wiimotes can be paired to connect automatically.

See https://wiki.archlinux.org/index.php/XWiimote for more information on connecting wiimotes. (Do not install the X.Org wiimote driver, it is not needed, and would conflict somewhat with MoltenGamepad. The xwiimote library is not needed, but its utilities can be useful for inspecting wiimotes)

Note that this uses the kernel driver, not one of the various wiimote libraries like cwiid that do handle connections, so the info on https://wiki.archlinux.org/index.php/Wiimote is not applicable. To use MoltenGamepad with wiimotes, do not use cwiid and wminput.

Aside from seeing the device entries created by the kernel driver, a successful connection can be verified by the Wiimote LEDs switching to having just the left-most one lit. Prior to that, all 4 LEDs will be blinking while the wiimote is in sync mode.



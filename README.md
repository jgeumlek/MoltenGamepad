# MoltenGamepad
*Flexible input device remapper, geared towards gamepads*

(Tested mainly on Arch Linux, 64-bit. Debian packaging contributed by mbenkmann)

## Motivation

Do you have input devices that you wish sent different events? MoltenGamepad is a daemon that can replace input devices with virtual ones while providing a way to translate or remap events.

Its main focus is on game controllers. They come in many shapes with many features, but most games make a lot of assumptions about what a controller should look like. MoltenGamepad allows for the diverse real input devices to all appear virtually as a "standard" game pad that most games understand.

The goal is to make any controller "just work", even as they are inserted or removed from the system. For complicated input devices, this can even involve writing specialized support to fully exploit their features.


## Features

* Create virtual gamepads that almost all games can use.
* Virtual gamepads are persistent, which fixes the numerous games that break when controllers are disconnected.
* Flexible generic device driver framework, where only a text file is needed to support input sources that don't need special features.
* All configuration files are designed to be human-readable and editable.
* Easy loading and saving of event mappings that can be changed at run time.
* Specialized userspace driver for Wii remotes that make full use of extension controller hotswapping.
* Supports a command FIFO or socket for controlling a running instance of MoltenGamepad via scripting.
* Can handle cases where one event device represents multiple logical controllers (like an arcade stick), or vice versa.
* Virtual devices can process rumble events (but this is disabled by default. See `documentation/rumble.md`.)

MoltenGamepad targets a set-it-and-forget-it daemon-like usage pattern,  where devices can connect or disconnect freely. Its main purpose is letting a user "standardize" their software to expect just one type of controller, and then automagically transform connected input devices to match that standardized abstraction. 

## Building

    make

If you get undefined KEY_* errors, you'll need to remove those lines from the eventlists. The following command should rebuild these eventlists to match your system. Afterwards you can try running `make` again. If this fails with `not found` you may need to update a variable in the Makefile to tell it where your key codes are defined.

    make eventlists

The only linked libraries under this default target are libudev, libpthread, and libdl.

Currently three plugins can optionally be built into MoltenGamepad when compiling, `wiimote`, `joycon`, and `steamcontroller`. By default, only the wiimote plugin is set to be built. Modify the lines at the beginning of the Makefile to control whether these plugins are included.

Plugins can also be set to be built as external plugins. These plugins will need to be moved to a `plugins` folder inside MG's config directory in order to be found and loaded. (Plugins will only be loaded if MG is started with the `--load-plugins` option).

Note that the Steam Controller plugin requires the [scraw](https://gitlab.com/dennis-hamester/scraw) and [scrawpp](https://gitlab.com/dennis-hamester/scrawpp) libraries.

## Installing

The `installation` directory has files and scripts to help you get MoltenGamepad up and running quickly. Look in there for the udev rules required to give MoltenGamepad the appropriate permissions.

The installation above is bare-bones, and MoltenGamepad won't know very many devices. See the [MG-Files repo](https://github.com/jgeumlek/MG-Files) and get the crowd-sourced device configs to fully enjoy the MoltenGamepad experience.

## Running

    ./moltengamepad

MoltenGamepad will start up, search existing devices, and wait for devices to show up. MoltenGamepad will read commands from its standard input.

    ./moltengamepad --help

to see the available command line arguments.

Type `help` into MG to see the available runtime commands.

You'll need appropriate permissions on the uinput device node, as well as any device nodes you wish MoltenGamepad to work with. See the `installation` folder for more details.

You might need to use `--uinput-path` to tell MoltenGamepad where the uinput device is on your system. (You might even need to modprobe uinput.)

## Sending Commands

As MoltenGamepad might be running as a background task, there are two additional ways to send commands to MoltenGamepad.

A FIFO can be made with `--make-fifo`. It acts as a special file where commands written to it will be read by MG. There is not bidirectional; MG cannot send any information back through the FIFO.

A socket can be made with `--make-socket`. This also creates a special file, one that supports sending and receiving data. You'll need a client such as [moltengamepadctl](https://github.com/jgeumlek/moltengamepadctl) that knows the appropriate way to communicate with MG.

## Configuration Locations

Configuration files follow the XDG specification.  `$XDG_CONFIG_HOME` is checked first before falling back to check the listed directories in `$XDG_CONFIG_DIRS`. A `moltengamepad` folder is used within these directories.
 
With the default values for these XDG variables the following behavior applies:

* User specific files are located in `~/.config/moltengamepad/`
* Systemwide files are located in `/etc/xdg/moltengamepad/`

Profiles are located in a `profiles` subdirectory of a configuration directory. 

Generic driver specifications are in a `gendevices` subdirectory of a configuration directory.

## Quick Summary: The Big Picture

MoltenGamepad creates virtual game pad devices, known as output slots. These output slots are what will ultimately be read by your other software.

MoltenGamepad also listens for input sources it recognizes, such as Wii remotes. Each input source declares a set of events it exposes.

The mapping from input source events to output slot events is stored in a profile. Every device maintains its own profile.

Input sources can be assigned to and moved from output slots freely.

This is the big picture of MoltenGamepad: input sources being translated according to profiles into the output slots they have been assigned. The profiles and the slot assignments can be changed while MoltenGamepad is running.

## Getting Started

This is just a very quick introduction to get you on your feet and a little more able to discover what MG can do for you.

When started, MoltenGamepad will create its virtual outputs, wait for input sources it recognizes, and will assign them to a slot upon their first event.

WARNING: Out of the box, MoltenGamepad will only have the included wiimote driver. Unless you are using wiimotes, you'll need to create some config files describing a generic driver for your device before MoltenGamepad will do anything useful. (See `documentation/gendev.md` for details.) Community contributed configuration files can be found in the [MG-Files repo](https://github.com/jgeumlek/MG-Files).

MoltenGamepad will also listen on stdin for user commands, such as changing a mapping or moving an input source to a different virtual output.

Useful command options:

* `--mimic-xpad` Make the virtual controllers appear to be wired Xbox 360 controllers, which most games (and Steam) expect.
* `--rumble` if you want rumble and understand the risks
* `--load-plugins` if you have any external plugins you wish to load.


When MoltenGamepad starts, take a moment to look over what is printed to get an idea of how MG initializes.

Run `print drivers` to see the drivers you have loaded. Pick one and try printing its profile, ex `print profiles wiimote`.

Next, go a head and connect a recognized input device. You should see it be identified and given a name.

Try pressing a button on your input device, and you should see it get assigned an output slot.

Try `print profiles`, and notice how there are profiles for each driver and each device. Changes to a driver profile will propagate to all connected devices and future connected devices! The `gamepad` profile will propagate changes to the appropriate driver profiles. 

Try changing an input mapping

    wiimote.wm_a = start

or 

    wiimote.wm_a = key(key_a)

Try changing the output slot assignment:

    move <device name> to virtpad2 

Try printing out the event name aliases of a device or driver

    print aliases wiimote

Or print the events to see their descriptions

    print events wiimote



## Additional Documentation

See this README, the various files in the `documentation` folder, the output of `./moltengamepad --help`, and the output of the `help` command while running MoltenGamepad.

Documentation for the plugins is located in their source directory. (e.g. `source/plugins/wiimote/`).



## Known Issues

* Changing input mappings does not clear out previous values, potentially leading to stuck inputs.
* Multiple inputs mapped to the same output event clobber each other. Desired behavior uncertain.
* Will likely add some amount of input latency, though it hasn't been measured beyond playtests.

## Troubleshooting FAQ-ish Section

### What's this about file permissions for the devices?
MoltenGamepad will fail if you don't have the right permissions, and you likely won't have the right permissions unless you do some extra work. Though not recommended for regular use, running MoltenGamepad as a super-user can be a useful way to try it out before you have the permissions sorted out.

You need write access to uinput to create the virtual gamepads.

You need read access to the various event devices you wish to read. Most systems automatically tag event devices that look like joysticks/gamepads to be readable by the current user. Unorthodox devices like a wiimote will need special udev rules.

If you enable rumble support, you need write access to the various event devices in order to send the rumble events.

See the `udev.rules` files in the `installation` directory for more information.

### What is a MoltenGamepad driver?

A driver handles a certain class of input devices. Its responsibilities include identifying appropriate devices and knowing when they are removed. A driver also includes an implementation of an input source, providing the code to actually read and process input events.

One included driver is the Wiimote driver. It handles the gritty details of the Linux kernel event devices made by a wiimote. Extra features include swapping active events when a wiimote extension is changed, along with combining the extension inputs with the wiimote inputs.

MoltenGamepad also contains support to read special configuration files to create generic drivers. These drivers can identify input devices by their reported name or vendor/product ids, and can provide meaningful names to their event codes. This `gendevices` functionality can support any reasonable device recognized by your kernel.

The dream here is to have a variety of drivers, enabling interesting features of certain hardware or gathering unorthodox input sources. Perhaps one might read controller inputs off the network/chatroom. Or expose controller inputs onto the file system.

REMINDER: At the moment, only the wiimote driver will be active on a default installation of MoltenGamepad.

### How does setting a mapping work?

The general syntax is

    <profile>.<in event> = <out event>

For example,

    wiimote.wm_a = primary

Every driver has a profile, as does every device. Their profiles are named the same. (Ex. the `wiimote` driver has a a profile named `wiimote`). Changing a mapping in a driver's profile will change that mapping in all connected devices of that driver, along with any future connected devices. Changing a mapping in a device's profile changes it for just that device, and may be overwritten by a future change to the driver's profile.

The profiles form a tree-shaped hierarchy, where a change to a mapping in one is propagated to its subordinates. It looks roughly like the following.

    gamepad
     +<driver profile>
     |  +<device profile>
     |  +<device profile>
     |
     +<driver profile>
       +<device profile>

### Slots?

Slots refer to the virtual output devices, so named to echo the "player slots" seen on game consoles as well as avoiding using the word "device" everywhere in every context. By default, MoltenGamepad creates 4 virtual gamepad slots, one virtual keyboard slot and one blank dummy slot. Input sources are assigned to the first virtual gamepad that has no connected devices. If none are available, the input source is placed onto the dummy slot.

Note that slots can only process appropriate events. Sending a keyboard key press to a virtual gamepad will lead to it being silently ignored.

Why not create a virtual device that can send all events? Some software expects gamepads to only have certain event codes, and not others. Keeping separate virtual devices greatly aids autodetection magic.

To move a device to a different slot, use this syntax:

    move <device> to <slot>

Need to find a device name or slot name?

    print slots
    print devices



### First, Second, Third, Fourth? What are those?

These are the face buttons on a controller. Commonly labelled A,B,X,Y. The default mapping looks like:

* first == BTN_SOUTH (BTN_A)
* second == BTN_EAST (BTN_B)
* third == BTN_NORTH (BTN_X)
* fourth == BTN_WEST (BTN_Y)

Names in parentheses are deprecated event names for these event codes, and do not necessarily correlate with the printed labels seen on controllers. The names in MoltenGamepad were chosen to reflect intuitive purposes for the buttons, and avoid the quagmire of the inconsistent/ambiguous labels A,B,X,Y.

Since all event codes are recognized, one may use `btn_south` instead of `first` in one's profiles.

### How do I connect a wiimote?

That is outside the scope of MoltenGamepad. Your bluetooth system handles this. This software assumes your bluetooth stack and kernel wiimote driver are already working and usable. A simple session with `bluetoothctl` works well. It is possible to pair wiimotes such that they remember your bluetooth adapter and will attempt to connect to it when any button is pressed.

See https://wiki.archlinux.org/index.php/XWiimote for more information on connecting wiimotes. (Do not install the X.Org wiimote driver, it is not needed, and would conflict somewhat with MoltenGamepad. The xwiimote library is not needed, but its utilities can be useful for inspecting wiimotes)

Note that this uses the kernel driver, not one of the various wiimote libraries like cwiid that do handle connections, so the info on https://wiki.archlinux.org/index.php/Wiimote is not applicable. To use MoltenGamepad with wiimotes, do not use cwiid and wminput.

Aside from seeing the device entries created by the kernel driver, a successful connection can be verified by the Wiimote LEDs switching to having just the left-most one lit. Prior to that, all 4 LEDs will be blinking while the wiimote is in sync mode.



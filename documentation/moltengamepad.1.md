moltengamepad 1 "May 2020" moltengamepad "User Manual"
==================================================

# NAME
  moltengamepad - Make using game controllers painless

# SYNOPSIS
  moltengamepad [options]

# Description

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


There are two separate types of global options for MoltenGamepad one might wish to configure:

1. Start-up options that cannot be changed later.
2. Dynamic global options normally accessible via the `set` command.

# Start-up Options: `moltengamepad.cfg`
An optional configuration file `moltengamepad.cfg` can be created in the config directory. This file is used to store information about how MoltenGamepad should behave. The contents are similar to the various command line arguments that can be passed to MoltenGamepad, and are unable to be changed after MoltenGamepad has started.

Comments can be included in the file via `#`.

A sample .cfg can be printed out using the `--print-cfg` option. This sample .cfg contains all available options to be set.

## Location

The XDG spec is followed, using `$XDG_CONFIG_HOME` and `$XDG_CONFIG_DIRS`. Only the first `moltengamepad.cfg` discovered is used.

By default this means the following locations ordered by preference:

1. `~/.config/moltengamepad/moltengamepad.cfg`
2. `/etc/xdg/moltengamepad/moltengamepad.cfg`

## Setting options

Most long-form command line arguments can be specified in this file instead. Compared to the commandline arguments, the following changes are required:

* Instead of hyphens, underscores are used
* True or false values are assigned instead of negation prefixes (i.e. `enumerate=false` instead of `no_enumerate`)

The `daemon`,`pidfile`, and `stay-alive` settings can not be specified in this file. They are exclusively commandline arguments.

The full list of available options can be seen by running

    moltengamepad --print-cfg

## Loading Profiles at Start-up

Profile files can be loaded at the start of MoltenGamepad by specifying them in the config file.

They are loaded after drivers are initialized but before any devices are added. As such, these profiles can only affect driver-level profiles.

    load profiles from "<filename>"

File paths are relative to the profiles directory.

## Command Line Arguments

When an option is expressed both in the config file and in the arguments, the value implied in the command line arguments takes precedence.

## Example

    #useful settings
    mimic_xpad = true
    make_fifo = true
    
    #load preferred default mappings
    load profiles from default_map

This of course requires a file `default_map` in the profiles directory

# Global Options: `options/*.cfg`

MoltenGamepad has a second notion of global options, those that can be changed while MoltenGamepad is running. They are separated into categories, and these categories can be displayed with the `print options` command.

These option categories are initialized with a matching `.cfg` file in the `/options/` subdirectory of the config directory. Similar to the above, `$XDG_CONFIG_DIRS` is respected.

## Example

For this example, we wish to set the option `auto_assign` in the `slots` category to `true`. Normally this would require the command `set slots auto_assign = true` to be entered after MoltenGamepad has started.

Instead, one can create the file `options/slots.cfg` with the following contents:

    auto_assign = true

Note how the file name matches the category of the options being set.
# MoltenGamepad Profile Documentation

This file documents the behavior and use of MoltenGamepad profiles. These profiles contain event mappings and device-level options. Profiles do not include global/driver-level options such as those reached by the `set` command.

This document has a few key sections:

1. An overview into the semantics of a profile
2. A guided tour on profile features and how to use them
3. A description of the root "gamepad" profile
4. Extra details/advanced features


## Overview

A Profile contains all the information one might want to configure about a device: the event mappings and the device options.

Naturally every input source carries its own profile, where the profile has the same name as the device.

Each driver also carries a profile. This profile is inherited by all the devices arising from that driver. Changes to the driver profile are propagated to the relevant devices. The driver profile is very useful for setting up devices that haven't been connected yet. Remember that driver-level options are stored in a profile; the driver profile is specifically for device-level information!

There is also a special profile, "gamepad", that acts like a root profile. Drivers can optionally subscribe to the gamepad profile. Thus changes to the gamepad profile can propagate to all gamepad devices in MoltenGamepad, even though they may belong to different drivers.

For demonstration purposes, this file will deal with `wiimote` driver, and will assume two wiimotes are connected (`wm1` and `wm2`).

## Tour of Features

This section is both a rough "getting started" guide and a collection of reference information. 


### Changing a profile

A profile can be altered by issuing a command of this form:

    <profile>.<event name> = <out event>

For example,

    wiimote.wm_a = select

would set all wiimotes to emit a select button event when their "a" button is pressed.

    wm2.wm_a = start

Would set `wm2` to emit a start button event when its "a" button is pressed, while `wm1`'s profile would be unmodified.

An event's mapping can be cleared by setting it to `nothing`.

    wiimote.wm_a = nothing

### Listing Profiles

    print profiles

will list all profiles currently in use. At the moment, this will simply be a list of all drivers and their devices, plus the special gamepad profile.

    print profiles <profile name>

will print out the mappings in that profile, in the same format as the commands used to set them.

    print profiles wiimote

This will display the already populated wiimote default profile, which gives a nice example.

### Listing Input Events

    print events <device or driver>

Unfortunately this does not work for the gamepad profile, but it does work for the devices and drivers. It will print out the events along with their descriptions.

    print devices <device>

This prints out information of a device, which also includes its event list.


### Possible Output Events

The following are specially recognized as gamepad button events for output. The entries are in this order (event code, MoltenGamepad name, description):

*  {BTN_SOUTH, "first", "Primary face button (Confirm)"},
*  {BTN_EAST, "second", "Second face button (Go Back)"},
*  {BTN_WEST, "third", "Third face button"},
*  {BTN_NORTH, "fourth", "Fourth face button"},
*  {BTN_START, "start", "Start button"},
*  {BTN_SELECT, "select", "Select button"},
*  {BTN_MODE, "mode", "Special button, often with a logo"},
*  {BTN_TL, "lt", "Upper left trigger"},
*  {BTN_TL2,"lt2", "Lower left trigger"},
*  {BTN_TR, "tr", "Upper right trigger"},
*  {BTN_TR2, "tr2", "Lower left trigger"},
*  {BTN_THUMBL, "thumbl", "Left thumb stick click"},
*  {BTN_THUMBR, "thumbr", "Right thumb sitck click"},
*  {BTN_DPAD_UP,"up", "Up on the dpad"},
*  {BTN_DPAD_DOWN, "down", "Down on the dpad"},
*  {BTN_DPAD_LEFT,"left", "Left on the dpad"},
*  {BTN_DPAD_RIGHT,"right","Right on the dpad"},

Those last four aren't available if your system is using an old event list in your kernel.

The following are specially recognized as gamepad axis events for output:


*  {ABS_X, "left_x", "Left stick X-axis"},
*  {ABS_Y, "left_y", "Left stick Y-axis"},
*  {ABS_RX, "right_x", "Right stick X-axis"},
*  {ABS_RY, "right_y", "Right stick Y-axis"},
*  {ABS_HAT2Y, "tl2_axis", "Analog lower left trigger"},
*  {ABS_HAT2X, "tr2_axis", "Analog lower right trigger"},

If the four dpad event codes were not available on your system, two extra axis events are added:

* {ABS_HAT0X,"leftright", "left/right on the dpad"},
* {ABS_HAT0Y, "updown", "up/own on the dpad"},

(The `--dpad-as-hat` option does the appropriate mapping of four dpad events to a hat when your system has the four dpad event codes available. There is not much need for these two extra axes on such systems, but they are still available as `abs_hat0x` if truly needed.)

In addition, the full range of evdev events (of type KEY or ABS) are also available, using lower case identifiers. Here are a subset just to demonstrate:

* key_a
* key_space
* key_esc
* btn_south
* abs_x
* key_playpause
* key_nextsong
* key_previoussong
* key_volumeup

### Mapping a button to a button

    wiimote.wm_a = primary

That's it.

### Mapping an axis to an axis

    wiimote.cc_left_x = left_x
    wiimote.cc_left_x = left_x+
    wiimote.cc_left_x = left_x-

The first two are equivalent. The last one inverts the axis direction.

### Mapping a button to an axis

    wiimote.wm_a = left_x+
    wiimote.wm_a = left_x-

The +/- represents whether the button should output in the positive or negative direction. When pressed, the button maxes out that axis in that direction. When not released, the button sets that axis to zero.

### Mapping a button to a relative event

Unlike an axis that represents absolute values, relative events express only changes. They are seen from mice, which have no idea where the mouse is, only how fast it is moving.

    wiimote.wm_a = rel_x+
    wiimote.wm_a = rel_x-

The +/- represents whether the button should output in the positive or negative direction. 

While pressed, a relative event will be generate at a regular rate.


### Mapping an axis to buttons

    wiimote.cc_left_x = left,right

The first output button is pressed when the axis gets sufficiently negative. The second output button is pressed when the axis gets sufficiently positive. When the axis is not at either extreme, both buttons are released.

### Mapping an axis to a relative event.

    wiimote.cc_left_x = rel_x
    wiimote.cc_left_x = rel_x+
    wiimote.cc_left_x = rel_x-

The first two are equivalent. The last one inverts the direction.

Similar to the button-to-relative mapping, these events are generated at a fixed rate. Unlike the button mapping, an axis can express a range of speeds for smoother control.

### Mapping a thumb stick

To generate events for a thumb stick, one generally wants to consider the two axes simultaneously to make a decision. This requires using "group translators" that can read from multiple events.

    wiimote.(cc_left_x,cc_left_y) = stick(left_x,left_y)
    wiimote.(cc_right_x,cc_right_y) = dpad

The former maps to the left stick of the virtual output device. The latter maps to the dpad of the virtual output device.
This more-complicated dpad-mapping is only needed for analog axes like a thumb stick. For an input device with the dpad represented as a hat, the section "mapping an axis to buttons" suffices.

Note how the multiple input events are simply listed inside parentheses. To clear a "group translator", just set its input combination to "nothing". The mapping is ordering-sensitive!

    wiimote.(cc_left_x,cc_left_y) = nothing

Aliases can also map to input event lists, making this easier. The following two lines are equivalent due to the built in alias "left_stick":

    wiimote.(cc_left_x,cc_left_y) = dpad
    wiimote.left_stick = dpad

When using these combined translators, the individual translators on the axes should be set to "nothing". Certain group translators, such as `stick` and `dpad`, will enforce this automatically (setting the group to `dpad` clears the individual mappings, and setting any individual mapping will clear out the group `dpad` mapping.)

### Inverted mapping

The input event on the left side can have an optional `-` added to the end to invert the events sent. For an axis, this is negation, while a button is inverted logically.

    wiimote.wm_accel_x- = left_x

### Multiple outputs

An event can be sent to multiple translators with `multi`:

    wiimote.wm_a = multi(start,select)

### Keyboard Redirect

Recall that the virtual gamepad output slots cannot emit keyboard events. However, a special translator can redirect these events to the correct keyboard slot.

    wiimote.wm_a = key(key_a)
    
This maps the wiimote a button to `key_a` on the keyboard slot, regardless of what slot the wiimote is currently in.

The device still must be assigned to slot for these events to occur.

### Mouse Redirect

    wiimote.cc_left_x = mouse(rel_x)
    
Similar to the above.

## The Gamepad profile

There is a special profile named `gamepad`. Drivers can subscribe to this profile, such that any changes to the gamepad profile apply to the driver (and thus the driver's devices as well).

For example

    gamepad.select = start

This sets the select button of all game pad devices to send start-button events. This is achieved by each driver internally having a table of aliases, mapping the gamepad event names to the event names of the driver. There is a limitation here, in that each alias can only have one result (e.g. each driver has at most one "select" event).

For Wii devices, all such mappings affect only the classic controller control scheme.

It has the following events:

* left_x,left_y: The two axes of the left stick. Right and Down are the positive directions.
* right_x, right_y: The two axes of the right stick. Same directions apply.
* primary,secondary,third,fourth: The four action buttons, or "face" buttons. The exact arrangement is left to the driver.
* up,down,left,right: The four digital events of a dpad.
* updown, leftright: The two axes of a dpad that is represented as a hat.
* start, select: the conventional extra buttons, often used for  in-game menus.
* mode: The additional meta button common on modern game pads, like the Wii Home button or the Xbox Guide button.
* tr, tl: The two upper shoulder buttons, or "bumpers".
* tr2, tl2: The digital two lower shoulder buttons or trigger. (ONLY FOR DEVICES WITHOUT ANALOG TRIGGERS)
* tr2_axis, tl2_axis: The analog axes for the two analog triggers, if present.
* tr2_axis_btn, tl2_axis_btn: The generally superfluous digital events emitted by analog triggers. (ONLY FOR DEVICES WITH ANALOG TRIGGERS)

Why the concern over "tr2_axis_btn"? When we have analog values, we generally want to ignore these events. But when we don't have analog values to read, we want to pay attention to the digital events.

Thus "tr2" is an event that is generally mapped to do something, while "tr2_axis_btn" is generally mapped to be ignored.

Currently MoltenGamepad only supports output devices with just digital or just analog triggers. Listening to both "tr2" and "tr2_axis" would lead to the two events clobbering each other.

If MoltenGamepad supports output devices with combined digital/analog triggers, then we'd probably want to make the digital value respect the conventions of the output device and activate at certain levels. This would still involve ignoring the "tr2_axis_btn".

Why have an event we almost always want to ignore? To standardize it and to give explicit guidelines to tell driver writers to not map these events to tr2/tl2.

## Extra Details

MoltenGamepad keeps track of whether a input event is a key/button (has only two values: pressed or not), or an axis/absolute (range of values). Similarly, MoltenGamepad uses output event names rather than numeric codes to know whether the output is a key or axis. Then an appropriate event translator is chosen to make this match. In general, MoltenGamepad attempts to do "the right thing".

These event translators can be specified directly.

    wiimote.wm_a = start
    #IS EQUIVALENT TO
    wiimote.wm_a = btn2btn(btn_start)
    
The following are available:

* btn2btn(event code) maps a button to the specified event code.
* axis2axis(event code, direction) maps an axis to the specified event code, where direction is +1 or -1
* axis2btns(neg event code, pos event code) maps an axis to the two specified buttons
* btn2axis(event code, direction) maps a button to the specified event code, where direction is +1 or -1
* btn2rel(event code, speed) maps a button to a relative event, generating events periodically while held
* axis2rel(event code, speed) maps an axis to a relative event, generating events periodically

See `print translators` for more details. It will show translator declarations like the following:

    key = btn2axis(axis_code, int direction=1)

where the `key` left of the `=` denotes this should be mapped to an input event that is a key (or button) press. The first argument should be an axis code like `abs_x` or `0`. The second argument is an integer that defaults to 1, and the argument is named "direction".

The arguments follow this pattern: `type [name] [= default value]`. Arguments that aren't named must be specified positionally. Arguments without default values must be provided.

A `[]` after an argument represents that it is variadic. It is thus a place holder for an argument list of arbitrary size. This is seen in

     event = multi(trans [])

where `multi(btn_start)`, `multi(btn_start,btn_select)`, `multi(btn_start,btn_select,btn_mode)` are all valid.

The following are all equivalent, noting that `ABS_X` is axis 0 and is the same as `left_x`:

    btn2axis(0)
    btn2axis(abs_x,1)
    btn2axis(direction=1,abs_x)
    btn2axis(0,direction=1)
    btn2axis(left_x)

## Group Translators

As mentioned in the section about mapping sticks, some translations really need to look at multiple events.



`chord(key_trans)` fires its internal event whenever all of its inputs are pressed. ex. `wiimote.(wm_a,wm_b) = chord(tr)` will send a `tr` press when both the A and B buttons are held. The original events `wm_a` and `wm_b` are also fired. The chord is released when any of the involved buttons are released. 

`exclusive(key_trans)` is an exclusive chord action, where all involved buttons must be pressed down at the same time. It exclusively fires its internal event, as if the involved buttons weren't pressed at all. For example, `wiimote.(wm_a,wm_b) = exclusive(tr)` will send the `wm_a` or `wm_b` events only if they are pressed separately. MoltenGamepad does not support creating complicated layers of exclusive chords, the behavior for two simultaneous overlapping exclusive chords is not well defined.

`stick` and `dpad` were described in the mapping a thumb stick section.

## Saving

    save profiles to <filename>
    
Will save all currently used driver profiles (not device profiles!) to the specified file, placed in the `profiles` config directory.

You'll likely want to put your filename in quotes.

You'll also likely want to open up this file later in your favorite text editor and clean it up.

## Loading

    load profiles from <filename>
    
Will load profile mappings from the specified file. No concern is taken over whether this affects driver or device profiles, and any commands referencing currently nonexistent profiles will be ignored.

Sometimes these files will be referred to as a "profile", but this is inacurrate. These files can contain information for many of the profiles in MG.

## Headers

Specifying a profile name in square brackets will set the implicit profile name for all following commands

    [wiimote]
    wm_a = start
    wm_b = select
    
Note how `wm_a` sufficed, instead of `wiimote.wm_a`


## EXPERIMENTAL FEATURES. USE AT YOUR OWN RISK

These features are in development, and the syntax is subject to change, and full functionality not guaranteed:

### Recursive load
 Profile files are allowed to use the load command, allowing for a form of inheritance, along with a bag of worms. This will likely be disabled in the future. 
# MoltenGamepad Generic Device Driver Documentation

## Intro

MoltenGamepad can translate button and axis events from any evdev device, such those in `/dev/input/event#`. To do this, MoltenGamepad needs some basic information on how to handle these devices. Creating a Generic Driver is an easy way to get enable basic functionality of MoltenGamepad for a device.

A configuration file has four main parts/purposes:

1. Specify what criteria a device must meet for this generic driver to apply.
2. Specify information about this generic driver, including options
3. Specify raw events read from the device and appropriate names to be exposed for these events.
4. Specify any event name aliases the device needs.

Default event mappings are NOT handled by these configs. That is to be handled by loading the desired profile at start up.

The format for these config files is designed to be simple yet flexible. There is a lot of flexibility required for generic drivers, so the full spec can be daunting. Look at the example files to get a good idea of how this all works.

## Generic Driver Config File

A generic driver can be specified by creating a `<filename>.cfg` file in a `gendevices` config directory.

Such a file has the following form, where `#` starts comments. All `<values>` MUST be placed in quotes when spaces/punctuation are present. (Limited escaping applies, `\"` for a literal quote, `\\` for a literal backslash)

This is the full spec. See the examples for a simpler view.

    # 1. Device Matching
    #specify devices details to match against. If any match, the device will be claimed by this driver.
    #see the section "Matching Devices" for more details
    [<Device Match>]
    [<Additional Device Match>]
    [<Additional Device Match>]
    
    # 2. Driver Info
    name= <driver name> #set the name of this driver seen in MoltenGamepad
    devname= <device name prefix> #set the name assigned to identified devices
    
    #    Driver Settings (if omitted, the default values shown are used)
    
    ##Should we grab the device exclusively, preventing events from being seen by others?
    ##This hides incoming events, but it DOES NOT hide the device node. It merely makes it appear silent.
    #exclusive="false" 
    
    ##Should we block all permissions after opening, preventing others from even opening the device?
    ##This is generally effective at making software ignore the original device entirely.
    ##Note: requires active user to be the device node owner.
    #change_permissions="false"
    
    ##Assuming change_permissions="true", should we go even further to hide any hidraw nodes
    ##in addition to the event device we read from? This is handy when MG needs to speak
    ##with the event device, but still wishes to hide the hidraw nodes from other software.
    ##Note: requires active user to be the hidraw node owner.
    #change_hid_permissions="false" 
    
    ##Should we coalesce all identified devices into one virtual input source?
    ##(Helpful for annoying devices that create a dead duplicate node,
    ## or if you know you want to combine all devices of this type)
    #flatten="false"

    ##Should we try to forward rumble (force-feedback) events?
    ##This setting has no effect unless MoltenGamepad was run with rumble enabled.
    ##You cannot enable both rumble and flatten.
    #rumble="false"
    
    
    ##How many input sources should we generate for this device?
    ##Useful for device nodes that may represent two or more controllers together.
    #split = 1
    
    ##Specify the device types. This is used for allocating output slots.
    ##Can be an any identifier string, but the following are special:
    ## "gamepad" - a normal gamepad device. (default device type)
    ## "keyboard" - forces the keyboard output slot to be used.
    ## Anything else leads to MG attempting to allocate no more than one of that
    ## type in each slot.
    #device_type = "gamepad"
    #
    ##If split is greater than one, each split can be given a different device type.
    #1.device_type = "gamepad"
    #2.device_type = "gamepad"
    #...

    ##Should we forcefully listen to the root "gamepad" profile to get our event mappings?
    ##By default, we listen non-forcefully: only when the driver has "gamepad" type devices.
    ##Reminder: The default device type is "gamepad"!
    #gamepad_subscription = false
    
    # 3. Specify Events
    
    #begin event identification, repeat for each event
    <event code name> = <name>,<description>
    
    #<event code name> is the evdev event name, such as btn_left or key_esc
    # NOTE: If you need to specify an event by number, use the following notation:
    #       key(306)          [the same as btn_c]
    #       abs(1)            [the same as abs_y]
    #<name> is the name of the event seen in MoltenGamepad
    #<description> is the description of the event seen in MoltenGamepad
    
    #If split is greater than one, prefix the <event code name> with the desired subdevice number
    #followed by a dot ( . ), 
    1.<event code name> = <name>,<description>
    
    # 4. Specify aliases
    
    #alias external_name local_name
    
    #local_name must be one the declared event <name>s
    #Aliases are for convenience, and for handling being subscribed to a different profile.
    

If multiple `[<Device Match>]` declarations are in a row, they are presumed to be alternative devices that should be grouped under the same driver. If they are not in a row, it is assumed that the user is beginning a new generic driver specification. (Yes, multiple generic drivers can be specified in one file.)

# Matching Devices

The most basic way to match a device is via it's reported name string. Putting it in quotes is recommended.

    ["Sony Computer Entertainment Wireless Controller"]

Other traits can be specified in match declaration using `<field>=` notation. Available fields to match against are 

* `name` : the reported name string
* `vendor` : the hexadecimal vendor id
* `product` : the hexadecimal product id
* `uniq` : a (potentially missing) uniquely identifying string for the device
* `driver` : the name of the linux driver for this event device
* `events` : can be one of `superset`, `subset`, or `exact`. Superset matches if the device contains all the events of this generic driver. Subset matches if the device has no reported events not listed in this driver, but it must have at least one event in common with this driver. Exact requires both of the conditions of superset and subset to hold. That is, the device has exactly the events of this driver; no more, no less.
* `min_common_events` : An integer that specifies extra information for the `events=subset` constraint. By default, the `subset` match requires, at minimum, one event in common. Setting `min_common_events` to an integer greater than one changes this requirement. For example, `events=subset min_common_events=8` will require the exposed events to be a subset of the ones in the driver, and further require the device exposes at least 8 of them.
* `order` : A positive integer indicating the order to consider matches. When a device has multiple matches, the one with the lowest `order` is taken. If not specified, a match has an `order` of `1`, which is the lowest allowed value. Ties are broken based on whichever match is read first.

Putting this together results in a match line that may resemble the following.

    [name="Microsoft X-Box 360 pad" vendor=045e product=028e]

The first field is assumed to be the name. Thus the following is valid as long as ambiguity is avoided.

    ["Microsoft X-Box 360 pad" vendor=045e product=028e]

When a match line specifies multiple fields, a device is considered a match only if ALL specified fields are matched.

Remember that multiple match lines can be used, in which a device matches the driver if ANY individual match line is matched.

The `order` property of matches is for cases where a device might match two separate generic drivers.

## Finding Event Codes and Name Strings

The `evtest` utility (not included with MoltenGamepad) is incredibly useful for this. Run it to see all devices you currently have read access to. Select a device, and it will print out events as they happen. Interact with your input device, and make note of the events generated.

Near the top of the `evtest` output will be the vendor and product ids as well.


## Example 1
    
Here is a short example using the split functionality:

    ["AlpsPS/2 ALPS GlidePoint"]
    
    name = "touchpad"
    devname = "pad"
    exclusive = "true"
    flatten = "true"
    split = 2

    1.btn_left = "hit","Player 1 action."
    2.btn_right = "hit","Player 2 action."
    
This creates a driver named `touchpad`. When an `AlpsPS/2 ALPS GlidePoint` device is found, two input sources are made, `pad1` and `pad2`. Each will have an event named `hit`, mappable directly (such as `pad1.hit = ...`) or via the driver profile to affect both (`touchpad.hit = ...`).

Splitting is useful for arcade panels and some controller hubs, which can appear as a single event device.

## Example 2

Here is a longer file, showing a configuration for a Dualshock 4 controller.


    ["Sony Computer Entertainment Wireless Controller"]
    
    name = "dualshock4"
    devname = "ds_"
    exclusive = "true"
    #since the original device is also a gamepad by most standards, we need change_permissions in
    #order to hide the original DS4 devices.
    #Reminder: change_permissions requires a udev rule to make the current user the owner of the device
    change_permissions = "true"
    flatten = "false"
    rumble = "true"
    
    
    btn_tl2 = "share", "Share Button"
    btn_tr2 = "options", "Options Button"
    
    btn_thumbl = "touchpad_press", "Touchpad click action"
    btn_select = "l3", "Left stick click"
    btn_start = "r3", "Right stick click"

    abs_hat0x = "leftright", "D-pad left/right axis"
    abs_hat0y = "updown", "D-pad up/down axis"
    
    btn_east = "cross", "The cross (X) button"
    btn_c = "circle", "The circle button"
    btn_north = "triangle", "The triangle button"
    btn_south = "square", "The square button"
    
    btn_west = "l1", "The left top trigger"
    btn_z = "r1", "The right top trigger"
    
    btn_tl = "l2", "The left lower trigger"
    abs_rx = "l2_axis", "The left lower trigger analog values"
    
    btn_tr = "r2", "The right lower trigger"
    abs_ry = "r2_axis", "The right lower trigger analog values"
    
    abs_x = "left_x", "The left stick x axis"
    abs_y = "left_y", "The left stick y axis"
    
    abs_z = "right_x", "The right stick x axis"
    abs_rz = "right_y", "The right stick y axis"
    
    #The default device type is "gamepad", and
    #this leads to the driver subscribing to the gamepad profile.
    #But the gamepad profile uses some different event names.
    #So these aliases let us inherit the appropriate event mappings.
    
    alias first cross
    alias second circle
    alias third square
    alias fourth triangle
    
    alias thumbl l3
    alias thumbr r3
    
    alias start options
    alias select share
    
    alias tr r1
    alias tr2_axis_btn r2
    alias tl l1
    alias tl2_axis_btn l2
    
    alias tr2_axis r2_axis
    alias tl2_axis l2_axis

## More Examples

See the [MG-Files repo](https://github.com/jgeumlek/MG-Files) for more contributed generic driver cfgs.
# FURTHER READING

More information can be found here:

https://github.com/mbenkmann/MoltenGamepad/tree/devel/documentation

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

#MoltenGamepad Profile Documentation

##Overview

Profiles contain a mapping from MoltenGamepad event names to output events.

Currently a profile exists for each driver and each device. The driver profile is used by newly connected devices to initialize their profile, and changes to a driver profile propagate to devices of that driver.

For demonstration purposes, this file will deal with `wiimote` driver, and assume two wiimotes are connected (`wm1` and `wm2`).

A profile can be altered by issuing a command of this form:

    <profile>.<event name> = <out event>

For example,

    wiimote.wm_a = select

would set all wiimotes to emit a select button event when their "a" button is pressed.

    wm2.wm_a = start

Would set `wm2` to emit a start button event when its "a" button is pressed, while `wm1` would still send a primary event.

##Listing Profiles

    print profiles

will list all profiles currently in use. At the moment, this will simply be a list of all drivers and their devices.

    print profiles <profile name>

will print out the mappings in that profile, in the same format as the commands used to set them.

    print profiles wiimote

This is handy, as it will display the already populated wiimote default profile, which gives a nice example.

##Listing Input Events

    print devices <device>

This will print out all events this device might emit, along with a short description of what that event represents. Note that this must be a device, not a driver!

##Possible Output Events

The following are specially recognized as gamepad button events for output (event code, MoltenGamepad name, description):

*  {BTN_SOUTH, "primary", "Primary face button (Confirm)"},
*  {BTN_EAST, "secondary", "Second face button (Go Back)"},
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

(The `--dpad-as-hat` option does the appropriate mapping of four dpad events to a hat when your system has the four dpad event codes available, their is no need for these two extra axes)

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

##Mapping a button to a button

    wiimote.wm_a = primary

That's it.

##Mapping an axis to an axis

    wiimote.cc_left_x = left_x
    wiimote.cc_left_x = +left_x
    wiimote.cc_left_x = -left_x

The first two are equivalent. The last one inverts the axis direction.

##Mapping a button to an axis

    wiimote.wm_a = +left_x
    wiimote.wm_a = -left_x

The +/- represents whether the button should output in the positive or negative direction. When pressed, the button maxes out that axis in that direction. When not released, the button sets that axis to zero.

##Mapping an axis to buttons

    wiimote.cc_left_x = left,right

The first output button is pressed when the axis gets sufficiently negative. The second output button is pressed when the axis gets sufficiently positive. When the axis is not at either extreme, both buttons are released.

##The Gamepad profile

There is a special profile named `gamepad`. Drivers can subscribe to this profile, such that any changes to the gamepad profile apply to the driver (and thus the driver's devices as well).

For example

    gamepad.select = start

This sets the select button of all game pad devices to send start-button events. This is achieved by each driver internally having a table of aliases, mapping the gamepad event names to the event names of the driver. There is a limitation here, in that the mapping can only have one result (e.g. each driver has at most one "select" event).

For Wii devices, all such mappings affect only the classic controller control scheme.

##Extra Details

MoltenGamepad keeps track of whether a input event is a key/button (0 or 1), or an axis/absolute (range of values). Similarly, MoltenGamepad uses output event names rather than numeric codes to know whether the output is a key or axis. Then an appropriate event translator is chosen to make this match. In general, MoltenGamepad attempts to do "the right thing".

These event translators can be specified directly.

    wiimote.wm_a = start
    #IS EQUIVALENT TO
    wiimote.wm_a = btn2btn(btn_start)
    
The following are available:

* btn2btn(event code) maps a button to the specified event code.
* axis2axis(event code, direction) maps an axis to the specified event code, where direction is +1 or -1
* axis2btns(neg event code, pos event code) maps an axis to the two specified buttons
* btn2axis(event code, direction) maps a button to the specified event code, where direction is +1 or -1


##Saving

    save profiles to <filename>
    
Will save all currently used driver profiles (not device profiles!) to the specified file, placed in the `profiles` config directory.

You'll likely want to put your filename in quotes.

You'll also likely want to open up this file later in your favorite text editor and clean it up.

##Loading

    load profiles from <filename>
    
Will load profile mappings from the specified file. No concern is taken over whether this affects driver or device profiles, and any commands referencing currently nonexistent profiles will be ifnored.

##Headers

Specifying a profile name in square brackets will set the implicit profile name for all following commands

    [wiimote]
    wm_a = start
    wm_b = select
    
Note how `wm_a` sufficed, instead of `wiimote.wm_a`

##EXPERIMENTAL FEATURES. USE AT YOUR OWN RISK

These features are in development, and the syntax is subject to change, and full functionality not guaranteed:

###Recursive load
 Profile files are allowed to use the load command, allowing for a form of inheritance, along with a bag of worms. Might be disabled in the future.

###Keyboard Redirect

    wiimote.wm_a = key(key_a)
    
This maps the wiimote a button to `key_a` on the keyboard slot, regardless of what slot the wiimote is currently in.

###Multi event

    wiimote.wm_a = multi(primary,third)

This takes two event translators, and performs both of them.

###Chords

    wiimote.(wm_a,wm_b) = thumbl

This makes it so that a `thumbl` event is generated everytime both `wm_a` and `wm_b` are pressed, and the appropriate release event is fired whenever they are not both pressed. 

Chords must use events located on the same input source. Chords DO NOT prevent the original events from firing. (ex. pressing both `wm_a` and `wm_b` would lead to 3 presses, each individually plus the chord event).

If you want a chord that prevents the original events, a very experimental implementation is offered via specifying a full `exclusive` advanced translator.

    wiimote.(wm_a,wm_b) = exclusive(thumbl)

Exclusive chords DO NOT support creating a complicated hierarchy of chords, as two exclusive chords sharing a button or overlapping will not be able to prevent each other's events. Exclusive chords inherhently add some input latency, as to do otherwise would require being able to see the future. Further, making many chords rely on the same button is not recommended.

# MoltenGamepad Wiimote Driver Documentation

The MoltenGamepad wiimote driver is based off of the ideas of the WiimoteGlue project. The central ideas are:

* Combine the multiple kernel event devices into one.
* Support changing the mapping when extensions are swapped.

Like WiimoteGlue, it will never support the Wii U Gamepad, and does not yet support the drums or guitar controllers. LEDs are not changed.

Accel and IR values are reported very simply, with a fair bit of noise and jitter as a result.

A wiimote provides the following events, though only a subset will be reachable at any one time:

* wm_a (Wiimote A button)
* wm_b (Wiimote B button)
* wm_plus (Wiimote + button)
* wm_minus (Wiimote - button)
* wm_1 (Wiimote 1 button)
* wm_2 (Wiimote 2 button)
* wm_home (Wiimote home button)
* wm_left (Wiimote Dpad left)
* wm_right (Wiimote Dpad right)
* wm_up (Wiimote Dpad up)
* wm_down (Wiimote Dpad down)
* nk_a (Wiimote A button with Nunchuk)
* nk_b (Wiimote B button with Nunchuk)
* nk_plus (Wiimote + button with Nunchuk)
* nk_minus (Wiimote - button with Nunchuk)
* nk_1 (Wiimote 1 button with Nunchuk)
* nk_2 (Wiimote 2 button with Nunchuk)
* nk_home (Wiimote home button with Nunchuk)
* nk_left (Wiimote Dpad left with Nunchuk)
* nk_right (Wiimote Dpad right with Nunchuk)
* nk_up (Wiimote Dpad up with Nunchuk)
* nk_down (Wiimote Dpad down with Nunchuk)
* nk_c (Nunchuk C button)
* nk_z (Nunchuk Z button)
* cc_a (Classic Controller A button)
* cc_b (Classic Controller B button)
* cc_x (Classic Controller X button)
* cc_y (Classic Controller Y button)
* cc_plus (Classic Controller + button)
* cc_minus (Classic Controller - button)
* cc_home (Classic Controller Home button)
* cc_left (Classic Controller Dpad left)
* cc_right (Classic Controller Dpad right)
* cc_up (Classic Controller Dpad up)
* cc_down (Classic Controller Dpad down)
* cc_l (Classic Controller L button)
* cc_zl (Classic Controller ZL button)
* cc_r (Classic Controller R button)
* cc_zr (Classic Controller ZR button)
* cc_thumbl (Left stick click, Wii U Pro Only)
* cc_thumbr (Right stick click, Wii U Pro Only)
* wm_accel_x (Wiimote X acceleration (long axis))
* wm_accel_y (Wiimote Y acceleration ((+) <--> (-) axis))
* wm_accel_z (Wiimote Z acceleration (top <--> bottom axis))
* wm_ir_x (Wiimote IR pointer X)
* wm_ir_y (Wiimote IR pointer Y)
* nk_wm_accel_x (Wiimote X acceleration with Nunchuk)
* nk_wm_accel_y (Wiimote Y acceleration with Nunchuk)
* nk_wm_accel_z (Wiimote Z acceleration with Nunchuk)
* nk_ir_x (Wiimote IR pointer X with Nunchuk)
* nk_ir_y (Wiimote IR pointer Y with Nunchuk)
* nk_accel_x (Nunchuk X acceleration)
* nk_accel_y (Nunchuk Y acceleration)
* nk_accel_z (Nunchuk Z acceleration)
* nk_stick_x (Nunchuk stick X)
* nk_stick_y (Nunchuk stick Y)
* cc_left_x (Classic Controller Left Stick X)
* cc_left_y (Classic Controller Left Stick Y)
* cc_right_x (Classic Controller Right Stick X)
* cc_right_y (Classic Controller Right Stick Y)
* bal_x (Balance Board Center of Gravity X)
* bal_y (Balance Board Center of Gravity Y)

Instead of the "modes" of WiimoteGlue, MoltenGamepad has the slightly less flexible approach of creating appropriate separate events for each mode. In other words, if no extension is present, the wiimote "a" button fires the "wm_a" event. When a nunchuk is present, it fires the "nk_a" event instead. (Do note that the nunchuk has no "a" button of its own!)

A Wii U Pro controller will use the "cc_" events like a classic controller.

# Motion Sensors

Various sensors on the wiimote are off by default: The accelerometers, the IR camera, and the motion+ gyroscopes. You must explicitly enable them, otherwise no events will be generated.

These sensors are toggled independently in the "no nunchuk" and "nunchuk present" mode.

    wiimote.?wm_accel_active = true  # activate accels when no nunchuk is present.
    wiimote.?nk_accel_active = true  # activates accels when nunchuk is present
    
    wiimote.?wm_ir_active = true
    wiimote.?nk_ir_active = true



# Pairing

Connecting a wiimote is done outside of MoltenGamepad.

A wiimote, if initially paired/connected/trusted in quick succession, might be set to automatically connect to one's PC whenever a button is pressed. This appears to only occur if the Linux system is connecting for the "first" time (i.e. you might need to "forget" the device and pair again from scratch to cause this process to happen again).

# EXPERIMENTAL: wiigyromouse

The contents of this section are not intended to be stable, and may change dramatically.

The motion+ gyroscopes are now exposed, but these events are not intended for direct translation. Similar to the other sensors, they must be enabled prior to use (using the `?wm_gyro_active` and `?nk_gyro_active` device options).

When first opened, these gyroscope sensors need some calibration, and no events will be generated until calibration is complete. The calibration is designed to not complete until the values are steady.

You are expected to set the Wii remote down on a steady surface for a while in order to complete the calibration process.

An experimental group event translator is included that can use these gyroscopes to produce relative mouse movements. As a reminder, these generated mouse movements will only work if the MoltenGamepad wiimote device is set to the `keyboard` output slot (the virtual gamepads do nothing with mouse events).

The wiigyromouse translator expects to see many events: 3 axes of accels, 3 axes of gyros, an optional ratchet button, and optionally further buttons that will temporarily "dampen" the generated mouse movements. (Dampening is intended to help prevent the mouse from moving just because you pressed that button and slightly moved the Wii remote while doing so.)

Some event group aliases have been written to help shorten this: `wm_accels, wm_gyros, nk_wm_accels, nk_wm_gyros`.

Thus you can write a mapping such as

    wiimote.(wm_accels,wm_gyros,wm_a) = wiigyromouse

where the A button is used as the ratchet (think: lifting a mouse off a mousepad, allows controller to move without generating mouse movements.)

You might like to also try a reverse ratchet, where mouse movements are made only while the button is held. Simply invert the ratchet button in the mapping.

    wiimote.(wm_accels,wm_gyros,wm_a-) = wiigyromouse

Any button events listed after the ratchet will be treated as dampening buttons.

The event translator exposes many options, and fine tuning of the defaults is still needed. See issue #29 for more details.

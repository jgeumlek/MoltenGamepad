#MoltenGamepad Wiimote Driver Documentation

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

Connecting a wiimote is done outside of MoltenGamepad.
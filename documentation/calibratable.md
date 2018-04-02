
Now we have a group that allow "hot calibration or online calibration". Yes, you can calibrate your gamepad while you are playing.

I design this group thinking in using the wiimote like light gun in MAME or nestopia. The syntax is the following:

The calibratable group translator permits a single axis to be calibrated with the help of some extra buttons.

It allows the axis to be calibrated at run time as needed. It is nice for emulating a light gun.

The basic format is:

```
<gamepad>.(<key,<axes>,<key>) = calibratable(<axes_code>)
```

To be more concrete, the above format applied to a wiimote might resemble:

```
wiimote.(wm_home,wm_ir_x,wm_b) = calibratable(abs_x)
```

We want to calibrate the wiimote infrared X axis, and map it to `abs_x` on the virtual gamepad.
 - The first key event triggers the calibration. After we press `wm_home`, we start calibrating.
 - The second is the input axis we want to calibrate.
 - The third is the key event that we use to record data to calibrate. When we press `wm_b`, the data value of wiimote is stored for use in the calibration process.

The calibration process involves collecting four measurements. They are, in order: The minimum desired input, the minimum desired output, the maximum desired input, the maximum desired output. The calibration process will end once these four measurements have been recorded.

To explain this process in detail, we will consider a case where the wiimote IR data is being used to control an absolute mouse cursor like a light gun. The profile syntax will be:

```
wiimote.(wm_home,wm_ir_x,wm_b) = calibratable(abs_x)
wiimote.(wm_home,wm_ir_y,wm_b) = calibratable(abs_y)
```

Notice that both calibratable translators are using the same buttons to collect data. This means both axes can be calibrated at once.

The full calibration process:

 1. Press `wm_home` to start collecting data.
 2. Physically aim the wiimote at the top left corner of the game screen, and press `wm_b`. This locks in the furthest left and furthest up physical position of the wiimote.
 3. Now move the wiimote until the in-game cross hair is located at the top left, and press `wm_b`. This records the output values we desire to emit.
 4. Repeat Step 2, but physically aim at the bottom right corner of the game screen. This locks in the furthest right and furthest down the wiimote will physically point.
 5. Repeat Step 3, but move the in-game cross hair to the bottom right of the game screen. This locks in the maximum values that should be emitted. 
 6. Calibration is complete; move the wiimote around to verify the new calibrated mapping.

---

Notice that steps 3 and 5 depending on getting good values from what is currently being emitted. If the raw data is really bad, these steps can be quite hard!

When you press `wm_home` is the above example, it restarts the calibration process, including reseting the calibration data.

The calibratable translator also supports composing its calibrations. This allows you to use the previous calibration while collecting the new calibration points. By repeating this process, a set of 4 good measurements can eventually be collected, since each improvement to the calibration should make your measurements easier.

To compose calibrations, simply have the data collection button held down when you start the calibration. This will prevent the previous calibration from being erased.

So in our wiimote light gun example, this would involve holding down `wm_b`, and then pressing `wm_home` while `wm_b` is held. Once the process begins, let go of `wm_b` and continue to collect the 4 data points.

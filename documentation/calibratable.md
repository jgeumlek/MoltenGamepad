
Now we have a group that allow "hot calibration or online calibration". Yes, you can calibrate your gamepad while you are playing.

I design this group thinking in using the wiimote like light gun in MAME or nestopia. The syntax is the following:

```
<gamepad>.(<key,<axes>,<key>) = calibratable(<axes_code>)
```

We are going to put an example to understand this:

```
wiimote.(wm_home,wm_ir_x,wm_b) = calibratable(abs_x)
```

We want to calibrate wiimote infrared x axes. And this will be mapped to abs_x of our gamepad.
 - The first key event triggers the calibration. After we press _wm_home_ we start to calibrate.
 - The second is the input we want to calibrate.
 - The third is key event that we use to recolect data to calibrate. In other words, if we press _wm_b_ we save the data value of wiimote to use it to calibration process.

Now we are going to explain the process to calibrate with this configuration. The idea is to calibrate while we are playing a game. The game should have a crosshair or some feedback to know where we are shooting. 
I suppose we want to use the wiimote like light gun so we have something like this configuration in profile:

```
wiimote.(wm_home,wm_ir_x,wm_b) = calibratable(abs_x)
wiimote.(wm_home,wm_ir_y,wm_b) = calibratable(abs_y)
```

Yes, we use the same keys for events, so we can calibrate axes *x* and *y* at once. The process is:

 1. We press *wm_home*.
 2. We aim with the wiimote to the left up corner of the game screen and then we press *wm_b* (shot), not the physical screen. Not always are the same, we can have black bars on the sides because of aspect ratio.
 3. Now we point with the crosshair of the game to the left up corner of the game screen and press *wm_b*(shot). Remember you put at the corner the feedback information for shot positions.
 4. We aim with the wiimote to the right down corner of the game screen and then we press *wm_b* (shot).
 5. After we point with the crosshair of the game to the right down corner of the game screen adn press *wm_b*(shot). 
 6. Now you can play aiming with the gun without need the crosshair of the game.

Usually with one calibration all is working perfect, but sometimes we need to calibrate again to have a more accurate calibration. At this situation we can use other calibration mode: the *compose calibration mode*.

We have two modes of calibration:

  1. Before start the calibration, we **reset** calibration data.
  2. We **compose** the new calibration data with the old.

If you only press *wm_home* you are using the mode *1*. When you start the calibration, the current calibration data are reset and you restart the calibration.

If you press *wm_b* and after *wm_home* (keeping pressed *wm_b*). Now you start calibration process, keeping the old data calibration. The result is a composition of the two calibration. This mode is good for fine tune calibration.

I have explained this for lightgun, but could be use with whatever analog controls you want to use. And you can configure whatever keys or axes you want.

Have a good time with your wiimote light gun!!!

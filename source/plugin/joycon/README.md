# Joy-Con Plugin

This plugin supports Nintendo Switch JoyCon via the hidraw interface.

The protocol is still not fully understood, and the implementation here is based off the work of [riking](https://github.com/riking/joycon) [dekuNukem](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering).

Basic button and analog stick inputs are supported.

Devices appear to sometimes disconnect for unknown reasons.

LEDs will be constantly cycling while the device is connected.

# Joy-Con Activation

Joy-Con are special, as each device represents an independent half of a gamepad. When first connected, MoltenGamepad is unaware of how the user wishes to combine these separate devices.

A new Joy-Con starts in an "unactivated" state, where it will not output any events. There are two ways to activate a Joy-Con:

* As a Solo Joy-Con: Press SL + SR. This treats the Joy-Con as its own entire gamepad, held sideways.
* As part of a partnership: Press ZL + ZR. This requires two Joy-Con, and appropriately combines the two half controllers.

A solo Joy-Con does not have enough input events to directly match a standard gamepad, which requires some mapping decisions to be made. (e.g Should SL/SR be mapped to the upper or lower triggers?) For this reason, all the solo Joy-Con events are exposed separately and can be mapped independently.

# Pairing

Joy-Con can be paired via standard bluetooth methods after the sync button is held on the controller.

Similar to wiimotes, it appears that the first time pairing to a Linux system can lead to the controller remembering the system. Afterwards, a button press can cause the Joy-Con to connect automatically to the Linux system.

When you wish to pair the Joy-Con back to a Switch, simply connect the controller directly while the Switch is awake.

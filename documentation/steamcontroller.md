#Steam Controller Driver

Moltengamepad can use the scraw library to process Steam Controllers. This library must be installed separately. Currently a separate make target is used to specify the inclusion of this support.

This requires MoltenGamepad to have permissions to access the Steam Controller, which would be true in a set up already configured to let Steam have access to it.

Steam and MoltenGamepad cannot both be in control of the device. Whichever one claims it first maintains control.

Hot plug detection and wired vs. wireless support are not fully understood by this author, likely as a result of not fully understanding the scraw library.

As of now, the built in keyboard / mouse emulation (the so-called "lizard" mode) is not supported. The gyroscope motion controls are also not yet supported.

The default mappings are subject to change.

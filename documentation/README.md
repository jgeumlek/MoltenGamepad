# gendev.md

This file describes the `.cfg` files used to create generic drivers. If your kernel is already sending events from your device, a quick file naming the events should be all it takes to get the device working in MoltenGamepad

# profiles.md

This file describes the syntax for setting control mappings in profiles. Look here if you are interested in changing event mappings or device-specific options.

# config_files.md

This file describes the syntax and behavior of the  `moltengamepad.cfg` configuration file, along with describing the files in the `/options/` subdirectory. Look here if you are interested in affecting how MoltenGamepad starts or if you are interested in how MoltenGamepad reads its global settings.

# wiimote.md

This file offers additional documentation on the wiimote driver.

# steamcontroller.md

This file offers additional documentation on the Steam controller driver.

# rumble.md

This file explains why rumble events are not processed by default.

# permissions_(udev).md

This files offers an explanation of the file permissions required by MoltenGamepad and some example udev rules to achieve them.

# plugin.md

This file describes the MG plugin system and offers a quick overview of what is needed to implement another driver for this project in C++. Please check the `gendev` documentation first to see if it already offers the needed functionality through its generic drivers.



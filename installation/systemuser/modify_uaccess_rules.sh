#!/usr/bin/bash

#This script is only needed for systems running udev version 216 or below.
#The uaccess tag cannot be removed, so instead we comment out the rule that set it in the first place.

sed -e 's/^SUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="?\*", TAG+="uaccess"/#commented out for MoltenGamepad\n#&/' </lib/udev/rules.d/70-uaccess.rules >/etc/udev/rules.d/70-uaccess.rules

echo "Created file /etc/udev/rules.d/70-uaccess.rules. This will cause the file /lib/udev/rules.d/70-uaccess.rules to be ignored."

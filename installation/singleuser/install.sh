#!/bin/bash
set -e


if [ ! -f ./udev.rules ]; then
  echo "udev.rules not found. Did you run \"./configure.sh <YOUR USERNAME>\"?"
fi

# Only root can do anything useful here, so might as well...
[[ $EUID -ne 0 ]] && exec sudo "$0"

# Install the files contained in the repo.
cd "$(dirname "$0")"
install -Dm0644 udev.rules /etc/udev/rules.d/90-moltengamepad.rules
install -Dm0644 profile-sdl2.sh /etc/profile.d/sdl2-gamecontroller-moltengamepad.sh


#If you are wondering, single-user doesn't need to worry about removing the uaccess tags.
#uaccess would, at worst, give the user permissions they already have.
#The permission-based device hiding will also block uaccess permissions.


# Reload the various services we have affected.
udevadm control --reload

# Tell the user we're done and what to do next.
echo "Installation of system files for a single user complete."
echo "Run the MoltenGamepad binary when desired."
echo ""
echo "Restart to ensure uinput permissions have been set."

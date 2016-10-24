#!/bin/bash
set -e

# Only root can do anything useful here, so might as well...
[[ $EUID -ne 0 ]] && exec sudo "$0"

# Install the files contained in the repo.
cd "$(dirname "$0")"
install -Dm0644 udev.rules /etc/udev/rules.d/90-moltengamepad.rules
install -Dm0644 profile-sdl2.sh /etc/profile.d/sdl2-gamecontroller-moltengamepad.sh

# Reload the various services we have affected.
udevadm control --reload

# Tell the user we're done and what to do next.
echo "Installation of system files for a single user complete."
echo "Run the MoltenGamepad binary when desired."
echo ""
echo "Restart to ensure uinput permissions have been set."

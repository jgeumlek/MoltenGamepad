#!/bin/bash
set -e

# Only root can do anything useful here, so might as well...
[[ $EUID -ne 0 ]] && exec sudo "$0"

# Create system user and group for MG.
useradd -r -U gamepad

# Install the files contained in the repo.
cd "$(dirname "$0")"
install -Dm0644 profile-sdl2.sh /etc/profile.d/sdl2-gamecontroller-moltengamepad.sh
install -Dm0644 systemd.service /etc/systemd/system/moltengamepad.service 
install -Dm0644 tmpfiles.conf /etc/tmpfiles.d/moltengamepad.conf
install -Dm0644 udev.rules /etc/udev/rules.d/90-moltengamepad.rules

# Reload the various services we have affected.
udevadm control --reload
systemd-tmpfiles --create
systemctl daemon-reload
systemctl enable moltengamepad.service

# Tell the user we're done and what to do next.
echo "Installation of system files complete. Install the MoltenGamepad binary"
echo "as /usr/local/bin/moltengamepad and run systemctl start moltengamepad"
echo "to start system-mode MoltenGamepad."

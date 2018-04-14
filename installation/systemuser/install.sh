#!/bin/bash
set -e

# Only root can do anything useful here, so might as well...
[[ $EUID -ne 0 ]] && exec sudo "$0"

# Create system user and group for MG.
useradd -r -U gamepad || echo "gamepad user already existed, continuing..."

# Install the files contained in the repo.
cd "$(dirname "$0")"
install -Dm0644 profile-sdl2.sh /etc/profile.d/sdl2-gamecontroller-moltengamepad.sh
install -Dm0644 systemd.service /etc/systemd/system/moltengamepad.service
install -Dm0644 tmpfiles.conf /etc/tmpfiles.d/moltengamepad.conf
install -Dm0644 udev.rules /etc/udev/rules.d/72-moltengamepad.rules

# Reload the various services we have affected.
udevadm control --reload
systemd-tmpfiles --create
systemctl daemon-reload

echo "Please ensure the gamepad account has access to uinput before running MG. This access will be automatic after the next reboot."
systemctl enable moltengamepad.service

# Try to detect a case where "TAG-=uaccess" is not supported.
# This workaround shadows a system version of a udev rules file, so let the user know.
# The user might need to take action if the system file gets updated.
# (However, if things are getting updated, lets hope they get to a higher udev version)
UDEV_VERSION=$(udevadm --version)
if [[ "$?" -ne "0" ]]; then
  UDEV_VERSION=""
  echo "Could not detect udev version. If the version is less than 217, the uaccess tag cannot be removed."
fi

if [[ "$UDEV_VERSION" != "" && "$UDEV_VERSION" -ge "217" ]]; then
  echo "Detected version of udev that supports removing tags."
  echo ""
fi

if [[ "$UDEV_VERSION" != "" && "$UDEV_VERSION" -lt "217" ]]; then
  echo "An old version of udev was detected! This version does not support removing the uaccess tag."
  echo ""
fi

if [[ "$UDEV_VERSION" == "" || "$UDEV_VERSION" -lt "217" ]]; then
  echo "If the uaccess tag is not removed, then hiding the original device nodes may fail in certain situations."
  echo "To remedy this, please comment out the following line in /lib/udev/rules.d/70-uaccess.rules"
  echo -e '\tSUBSYSTEM=="input", ENV{ID_INPUT_JOYSTICK}=="?*", TAG+="uaccess"'
  echo ""
  echo "A modified version can be placed at /etc/udev/rules.d/70-uaccess.rules to shadow the original file in /lib/."
  echo "The provided script modify_uaccess_rules.sh will perform this for you."
  echo ""
  echo "Furthermore, if you use Steam, Steam also adds uaccess rules that would interfere with device hiding."
  echo "You'll need to comment out the appropriate lines from /usr/lib/udev/rules.d/70-steam-controller.rules"
  echo "Placing a blank file at /etc/udev/rules.d/70-steam-controller.rules will block all the Steam udev rules."
fi

# Tell the user we're done and what to do next.
echo "Installation of system files complete. Install the MoltenGamepad binary"
echo "as /usr/local/bin/moltengamepad and run systemctl start moltengamepad"
echo "to start system-mode MoltenGamepad."

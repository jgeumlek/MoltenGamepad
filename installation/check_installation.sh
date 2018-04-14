#!/bin/bash


UDEV_VERSION=$(udevadm --version)
if [[ "$?" -ne "0" ]]; then
  UDEV_VERSION=""
  echo "Could not detect udev version. If the version is less than 217, the uaccess tag cannot be removed."
fi


HAS_SINGLE_USER_FILES=false
HAS_SYSTEM_WIDE_FILES=false

for file in /etc/udev/rules.d/*moltengamepad*; do
  if [[ "$file" == '/etc/udev/rules.d/*moltengamepad*' ]]; then
    echo "NOT INSTALLED: No Moltengamepad udev rules detected."
    break;
  fi
  if grep -q "system-wide" "$file" ; then
    echo "Found system-wide udev rules: " $file
    HAS_SYSTEM_WIDE_FILES=true
    continue;
  fi
  if grep -q "single-user" "$file" ; then
    echo "Found single-user udev rules: " $file
    HAS_SINGLE_USER_FILES=true
    continue;
  fi
  echo "Found udev rules, but could not detect version: " $file
done

if [ -f /etc/udev/rules.d/70-uaccess.rules ]; then
  if [[ "$UDEV_VERSION" != "" && "$UDEV_VERSION" -ge "217" ]]; then
    echo "WARNING: found uaccess shadowing rules, but udev supports removing tags: /etc/udev/rules.d/70-uaccess.rules"
    echo "         Shadowing not needed, removing this file suggested."
  else
    echo "Found uaccess shadowing rules, needed for udev below v217: /etc/udev/rules.d/70-uaccess.rules"
  fi
else
  if [[ HAS_SYSTEM_WIDE_FILES == "true" && ( "$UDEV_VERSION" == "" || "$UDEV_VERSION" -lt "217" ) ]]; then
    echo "WARNING: Did not detect ability to remove uaccess tag, and did not detect file to prevent the uaccess tags."
    echo "         Device hiding may fail for a system-wide installation."
  fi
fi

if [ -f /etc/udev/rules.d/70-steam-controller.rules ]; then
  if [[ "$UDEV_VERSION" != "" && "$UDEV_VERSION" -ge "217" ]]; then
    echo "WARNING: found Steam shadowing rules, but udev supports removing tags: /etc/udev/rules.d/70-steam-controller.rules"
    echo "         Shadowing not needed. Perhaps remove this file depending on your uses."
  else
    echo "Found Steam uaccess shadowing rules, needed for udev below v217: /etc/udev/rules.d/70-steam-controller.rules"
  fi
else
  if [[ -f /usr/lib/udev/rules.d/70-steam-controller.rules && HAS_SYSTEM_USER_FILES == "true" && ( "$UDEV_VERSION" == "" || "$UDEV_VERSION" -lt "217" ) ]]; then
    echo "WARNING: Did not detect ability to remove uaccess tag, and did not detect file to prevent the Steam uaccess tags."
    echo "         Device hiding may fail for a system-wide installation."
  fi
fi

if [ -f /etc/profile.d/sdl2-gamecontroller-moltengamepad.sh ]; then
  echo "Found SDL2 var config for default MoltenGamepad virtual game pad: /etc/profile.d/sdl2-gamecontroller-moltengamepad.sh"
fi

if [ -f /etc/systemd/system/moltengamepad.service ]; then
  echo "Found systemd service for moltengamepad: " /etc/systemd/system/moltengamepad.service
  if  grep -q "ExecStart=/usr/local/bin/moltengamepad" /etc/systemd/system/moltengamepad.service ; then
   if [ ! -f /usr/local/bin/moltengamepad ];
    then
      echo "WARNING: did not find executable where service file expected, missing: /usr/local/bin/moltengamepad"
   fi
  fi
fi

if [ -f /usr/local/bin/moltengamepad ]; then
    echo "Found moltengamepad executable: " /usr/local/bin/moltengamepad
fi

if [ -f /etc/tmpfiles.d/moltengamepad.conf ]; then
  echo "Found tmpfiles conf used for systemd service: " /etc/tmpfiles.d/moltengamepad.conf
fi

if [[ HAS_SINGLE_USER_FILES == "true" && HAS_SYSTEM_WIDE_FILES == "true" ]]; then
  echo "WARNING: Found files for both single-user and system-wide installation!"
  echo "        Recommend deleting these detected files and re-running the appropriate"
  echo "         install script. One or the other, not both!"
fi

#!/bin/bash
set -e

if [ -z "$1" ];
then
  echo "No user name supplied."
  echo "USAGE: ./configure <USER NAME>"
  exit 1
fi

cd "$(dirname "$0")"
echo "Making udev.rules for user $1..."

sed "s/SINGLEUSER/$1/g" udev.rules.template > udev.rules

# Tell the user we're done and what to do next.
echo "Files for a single user created."
echo "Run ./install.sh to install them."


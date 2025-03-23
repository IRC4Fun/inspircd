#!/bin/sh

# This is the updates script for IRC4Fun servers, allowing for quick
# and easy git updates and even installation of InspIRCd contrib
# modules.
#
# This bash script should be placed in the user's home directory,
# outside of the inspircd installation folder.

# The location of the InspIRCd config directory.
INSPIRCD_DIR="/home/user/inspircd4/"

  # Enter the InspIRCd directory
  cd ${INSPIRCD_DIR}

  # Use 'git pull' to grab any updates to InspIRCd from IRC4Fun's
  # Github. 
  git pull

  # Update any contrib modules that have updates...
 ./modulemanager update

  # We have to now run ./configure to properly update the 
  # version and version-HASH.
 ./configure --disable-interactive --development

  # Now for the finale -- build and install the updates.
  make ; make install


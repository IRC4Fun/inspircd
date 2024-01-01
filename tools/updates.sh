#!/bin/sh

# This is the updates script for IRC4Fun servers, allowing for quick
# and easy git updates and even installation of InspIRCd contrib
# modules.
#
# This bash script should be placed in the user's home directory,
# outside of the inspircd installation folder.

# The location of the InspIRCd config directory.
INSPIRCD_DIR="/home/user/inspircd/"


  cd ${INSPIRCD_DIR}
  echo "*** Entering ${INSPIRCD_DIR}"
  echo "*** Pulling any available updates... "
  git pull

  echo "*** Updates pulled from git, preparing to build..."

  if [ "$1" != "" ]; then
        echo "--- Pulling contrib module: $1 "
    ./modulemanager install $1
  fi

  if [ "$2" != "" ]; then
        echo "--- Pulling contrib module: $2 "
    ./modulemanager install $2
  fi

  if [ "$3" != "" ]; then
        echo "--- Pulling contrib module: $3 "
    ./modulemanager install $3
  fi

 ./configure --disable-interactive --development

  make ; make install

  echo "*** Building: Issuing Make and Make install commands.."

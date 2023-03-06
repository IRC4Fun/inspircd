#!/bin/sh

# This is the updates script for IRC4Fun servers, allowing for quick
# and easy git updates and even installation of InspIRCd contrib
# modules.
#
# This bash script should be placed in the user's home directory,
# outside of the inspircd installation folder.

# The location of the InspIRCd config directory.
INSPIRCD_DIR="/home/user/inspircd/"

YELLOW='\033[0;33m'
GRAY='\033[0;37m'
GREEN='\033[1;32m'
RED='\033[1;31m'

  cd ${INSPIRCD_DIR}
  echo "${GRAY}*** Entering ${INSPIRCD_DIR} ${GRAY}"
  echo "${YELLOW}--- Pulling any available updates... "
  git pull

  echo "${GREEN}*** Updates pulled from git, preparing to build...${GRAY}"

  if [ "$1" != "" ]; then
        echo "${YELLOW}--- Pulling contrib module: $1 "
    ./modulemanager install $1
  fi

  if [ "$2" != "" ]; then
        echo "${YELLOW}--- Pulling contrib module: $2 "
    ./modulemanager install $2
  fi

  if [ "$3" != "" ]; then
        echo "${YELLOW}--- Pulling contrib module: $3 "
    ./modulemanager install $3
  fi

  make ; make install

  echo "${GREEN}*** Building: Issuing Make and Make install commands..${GRAY}"

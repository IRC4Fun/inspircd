#!/bin/sh
set -e

# Updates Helper: RESTART
#
# This script is to be called when servers are needed to be RESTARTed
# after upgrades or maintenance.  This script should be called by the
# Server Administrator OR the server bot.
#
# Make sure to properly specify the INSPIRCD_PID_FILE or else This
# script will not work (properly).

# SETUP:
# 1) `mv /home/ircd/inspircd/tools/restart.sh /home/ircd/restart.sh`
# 2) `nano restart.sh` and set correct INSPIRCD_PID_FILE  &
#    INSPIRCD_RUN_DIR.
# 3) `chmod +x restart.sh`
# 4) Rejoice in success!

# The location of the InspIRCd pid file.
INSPIRCD_PID_FILE="/home/ircd/inspircd/run/data/inspircd.pid"

# The location of the InspIRCd runtime directory.
INSPIRCD_RUN_DIR="/home/ircd/inspircd/run/bin/inspircd"


# DO NOT EDIT BELOW THIS LINE!
#

if [ -e ${INSPIRCD_PID_FILE} ]
then
    kill `cat ${INSPIRCD_PID_FILE}`
fi
${INSPIRCD_RUN_DIR}

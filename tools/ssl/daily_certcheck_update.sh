#!/bin/sh
set -e

# The location your renewal tool places your certificates.
CERT_DIR="/home/ircd/ssl/irc4fun.net/staffdocs"

# The location of the InspIRCd config directory.
INSPIRCD_CONFIG_DIR="/home/ircd/inspircd/run/conf"

# The location of the InspIRCd pid file.
INSPIRCD_PID_FILE="/home/ircd/inspircd/run/data/inspircd.pid"

# The user:group that InspIRCd runs as.
INSPIRCD_OWNER="ircd:ircd"

# Script to download the new SSL certs -- we'll do this DAILY, along with this script! -siniStar
#

wget -r --user USERNAME --password PASSWORD https://irc4fun.net/staffdocs/fullchain.pem
wget -r --user USERNAME --password PASSWORD https://irc4fun.net/staffdocs/privkey.pem

# Script to extract new SSL certs pulled in from secure site
#

if [ -e ${CERT_DIR} -a -e ${INSPIRCD_CONFIG_DIR} ]
then
    cp "${CERT_DIR}/fullchain.pem" "${INSPIRCD_CONFIG_DIR}/cert.pem"
    cp "${CERT_DIR}/privkey.pem" "${INSPIRCD_CONFIG_DIR}/key.pem"
    chown ${INSPIRCD_OWNER} "${INSPIRCD_CONFIG_DIR}/cert.pem" "${INSPIRCD_CONFIG_DIR}/key.pem"
    if [ -e ${INSPIRCD_PID_FILE} ]
    then
        kill -USR1 `cat ${INSPIRCD_PID_FILE}`
    fi
fi

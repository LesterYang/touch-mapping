#!/bin/bash

export TSLIB_CALIBFILE="/etc/pointercal"
export TSLIB_CONFFILE="/usr/etc/ts.conf"
export TSLIB_CONSOLEDEVICE="/dev/tty"
export TSLIB_FBDEVICE=$1
export TSLIB_PLUGINDIR="/usr/lib/ts"
export TSLIB_TSDEVICE=$2

echo "set fb    : $1"
echo "set event : $2"

ts_test

#!/bin/bash
while read value; do printf "$value\n" >> $(ls /dev/cu.usbserial-* | head -1); done <<< '\
speedmin=0
speedmax=1
deadzone=0
callow=0
calhigh=1
sensitivity=0
'

# Usage:
# Make sure the device can connect to a wifi network and a server
# Run this script to configure speed for calibration
# Turn potentiometer all the way in one direction so it shows the smallest value
# If the value jumps back and forth between two values, pick the higher value
# That value is what callow should be
# Turn the potentiometer all the way in the other direction
# If the value jumps back and forth between two values, pick the lower value
# That value is what calhigh should be

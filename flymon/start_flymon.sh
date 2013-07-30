#!/bin/bash
##
## Add the following rule to /etc/udev/rules.d/60-usb-devices.rules (change serial code to match the one shown with lsusb -v )
## SUBSYSTEMS=="usb", KERNEL=="ttyUSB*", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", ATTRS{serial}=="A600dVHF", SYMLINK+="ttyFTDI0" #flymon
## SUBSYSTEMS=="usb", KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ATTRS{serial}=="0309", SYMLINK+="ttyFTDI1 #Trikinetics

FLYMON_PATH='/home/gg/git/ardufly/flymon/flymon.py'
FTDI_DEV='/dev/ttyFTDI0'
OUTPUT_FOLDER='/home/gg/sleepData/flymon/'

python2 $FLYMON_PATH -p $FTDI_DEV -d $OUTPUT_FOLDER &

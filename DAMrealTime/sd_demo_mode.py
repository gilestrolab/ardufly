#!/usr/bin/env python2

command = 'T\n'

import serial
from time import sleep

ser = serial.Serial('/dev/ttyACM0', 57600)
sleep(2)

while 1:
    ser.write(command)
    sleep(60)
    
ser.close()

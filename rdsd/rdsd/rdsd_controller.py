#!/usr/bin/env python2
import serial

port = '/dev/ttyUSB0'
arduino = serial.Serial(port, 9600)
arduino.timeout = 0.1

arduino.write('A')
print arduino.read(300)

#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
#  fake_serial.py
#  
#  Copyright 2016 Giorgio Gilestro <giorgio@gilest.ro>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  

import serial
import time, random
import optparse

"""
This is used for debugging
Start two serial ports with the command

socat -d -d pty,raw,echo=0 pty,raw,echo=0

then assign one here and one to the server
"""

filename = "/home/gg/smart_incubator.csv"

def random_values(port, delta=5):
    """
    creates random values
    """
    ser=serial.Serial(port, 115200)
    pause = 10
    counter = 0
    while True:
        
        counter +=1
        line = " ".join([str(i) for i in [random.randint(0,10), "R", counter, time.time(), random.uniform(18,26), random.uniform(58,67), random.randint(0, 1000), 23.0, 62.0, 100, 32400, 75600, 0]])
        ser.write(line+"\r\n")
        time.sleep(delta)
        
        
def transfer_file(port, filename, downsample=5, verbose=False):
    """
    transfer CSV content to mySQL
    """
    ser=serial.Serial(port, 115200)

    DD_MODE_MAP = ["DD","LD", "LL", "DL", "MM"]
    
    with open(filename, "r") as fh:
        for line in fh.readlines()[::downsample]:
            id, command, counter, device_time, server_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode = line.strip().split(",")
            dd_mode = DD_MODE_MAP.index(dd_mode)
        
            lineout = " ".join([str(a) for a in [id, command, counter, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode ]])
            ser.write(lineout+"\r\n")
            if verbose: print lineout
            
    ser.close()


if __name__ == '__main__':            

    parser = optparse.OptionParser()

    parser.add_option("-p", "--port", dest="port", default=None,help="Serial port to write to")
    parser.add_option("-f", "--file", dest="filename", default=None, help="Transfer data from filename")
    parser.add_option("-r", "--random", dest="random", default=False, help="Use random mode", action="store_true")


    (options, args) = parser.parse_args()
    option_dict = vars(options)

    if option_dict['random'] and option_dict['port']: random_values(option_dict['port'])
    if option_dict['filename'] and option_dict['port']: transfer_file( option_dict['port'], option_dict['filename'])
    


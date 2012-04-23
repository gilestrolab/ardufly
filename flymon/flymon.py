#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  flymon.py
#  
#  Copyright 2012 Giorgio Gilestro <gg@kinsey>
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

import serial, os, datetime
from platform import system
from time import strftime
import optparse

__version__ = '0.2'    
    
def saveValues(l, path):
    '''
    '''
    m, c, t1, h1, l1, t2, bat = l.split(',')
    filename = os.path.join(path, 'flymon%02d.txt' % int(m) )
    
    date = strftime("%d %b %Y")
    time = strftime("%H:%M:%S")
    
    fh = open (filename, 'a')
    string = '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' % (c, date, time, m, t1, h1, l1, t2, bat)
    fh.write(string)
    fh.close()
    
    

def readInput(port, path):
    '''
    ID,count,t1,h1,l1,t2,bat
    4,150,23.49,19.03,200,0,0
    '''
    
    ser = serial.Serial(port, 57600)

    while True:
        l = ser.readline().strip()
        if 'Ready' not in l: saveValues(l, path)
        
    return 0

if __name__ == '__main__':

    usage =  '%prog [options] [argument]\n'
    version= '%prog version ' + str(__version__)
    
    parser = optparse.OptionParser(usage=usage, version=version )
    parser.add_option('-p', '--port', dest='port', metavar="/dev/ttyXXX", help="Specifies the serial port to which the reader is connected")
    parser.add_option('-d', '--d', dest='path', metavar="/path/to/data/", help="Specifies where data are going to be saved")

    (options, args) = parser.parse_args()
    
    if options.port and options.path:
        readInput(options.port, options.path)
    else:
        parser.error("You must specify port and path")


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

if system() == 'Linux': 
    port = '/dev/ttyUSB0'
    path_save = ''

elif system() == 'Windows':
    port = 'com5'
    path_save = 'DAM/flymons/'

else:
    print 'Operative system not supported'
    os.sys.exit(1)
    
def saveValues(l):
    '''
    '''
    m, c, t1, h1, l1, t2, bat = l.split(',')
    filename = os.path.join(path_save, 'flymon%02d.txt' % int(m) )
    
    date = strftime("%d %b %Y")
    time = strftime("%H:%M:%S")
    
    fh = open (filename, 'a')
    string = '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' % (c, date, time, m, t1, h1, l1, t2, bat)
    fh.write(string)
    fh.close()
    
    

def readInput():
    '''
    ID,count,t1,h1,l1,t2,bat
    4,150,23.49,19.03,200,0,0
    '''
    
    ser = serial.Serial(port, 57600)

    while True:
        l = ser.readline().strip()
        if 'Ready' not in l: saveValues(l)
        
    return 0

if __name__ == '__main__':
    readInput()


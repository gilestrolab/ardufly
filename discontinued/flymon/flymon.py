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

#  For automatic upload to google spreadsheet, see http://www.danielhansen.net/2013/03/raspberry-pi-temperature-logging-using.html

import serial, os, datetime
from platform import system
from time import strftime, sleep
import threading

import logging


import optparse
import ConfigParser

__version__ = '0.3'

class flymon():
    def __init__(self, port, outputpath):
        self.port = port
        self.baud = 57600

        self.path = outputpath

        self.__reading = False
        self.__use_thread = False
        self.__filename_prefix = 'flymon'
        
        #LOGGING
        handler = logging.FileHandler('flymon.log')
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        handler.setLevel(logging.INFO)
        handler.setFormatter(formatter)

        self.logger = logging.getLogger(__name__)
        self.logger.addHandler(handler)
        
        #open serial
        self.serial = serial.Serial(self.port, self.baud)
        sleep(1)

    def recordingLoop(self):
        '''
        start loop reading from serial
        '''
        self.logger.info('Started flymon monitor')
        
        while self.__reading:
            self.readInput()
            
    def start(self):
        """
        Can be used with threads or without
        """

        self.__reading = True
        
        if self.__use_thread:
            self.thread = threading.Thread(target=self.recordingLoop)
            self.thread.start()
        else:
            self.recordingLoop()
        
    def stop(self):
        """
        In thread mode, stop collecting data and ends program
        """
        self.__reading = False            

    def readInput(self):
        '''
        ID,count,t1,h1,l1,t2,bat
        4,150,23.49,19.03,200,0,0
        '''
        
        values = self.serial.readline().strip()
        self.logger.debug('Read: %s' % values)

        if values and 'Ready' not in values: 
            self.saveValues(values)
            
    def saveValues(self, values):
        '''
        '''
        m, c, t1, h1, l1, t2, bat = values.split(',')
        filename = os.path.join(self.path, '%s%02d.txt' % (self.__filename_prefix, int(m)) )
        
        date = strftime("%d %b %Y")
        time = strftime("%H:%M:%S")
        
        fh = open (filename, 'a')
        string = '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' % (c, date, time, m, t1, h1, l1, t2, bat)
        fh.write(string)
        fh.close()

    def readFromConfig(self, filename):
        """
        """
       
        if os.path.exists(filename):
            config = ConfigParser.RawConfigParser()
            config.read(filename)

            self.port = config.get('options', 'input')
            self.path = config.get('options', 'output')
            
        else:
            msg = "Configfile does not exist. Please create one first"
            self.logger.error(msg)
            raise msg
         

    


if __name__ == '__main__':

    usage =  '%prog [options] [argument]\n'
    version= '%prog version ' + str(__version__)
    
    parser = optparse.OptionParser(usage=usage, version=version )
    parser.add_option('-c', '--config', dest='config', metavar="/path/to/config", help="Specifies path to configuration file")
    parser.add_option('-p', '--port', dest='port', metavar="/dev/ttyXXX", help="Specifies the serial port to which the reader is connected")
    parser.add_option('-d', '--d', dest='path', metavar="/path/to/data/", help="Specifies where data are going to be saved")
    parser.add_option('-v', '--debug', action="store_true", default=False, dest='debug', help="Activates logging DEBUG mode (default is INFO)")

    (options, args) = parser.parse_args()
    
    if options.debug:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)
    
    if options.port and options.path:
        fm = flymon(options.port, options.path)
        fm.start()
    else:
        parser.error("You must specify port and path")


#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
#  plot_flymon.py
#  
#  Copyright 2013 Giorgio Gilestro <giorgio@gilest.ro>
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

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import time, datetime
import os
import optparse

HEADER_LENGTH = 3
TAB = '\t'

def open_read(input_file):
    """
    """
    data = []
    try:
        fh = open(input_file, 'r')
        rawfile = fh.read().split('\n')
        fh.close()

        for line in rawfile:
            if line:
                dt = TAB.join(line.split(TAB)[1:3])
                timestamp = time.mktime(datetime.datetime.strptime(dt, "%d %b %Y\t%H:%M:%S").timetuple())
                data.append( [timestamp]+[v for v in line.split(TAB)[HEADER_LENGTH:] ] )
        
        a = np.array(data, dtype=float)
        
        return a
    
    except IOError:
        print "Error opening the file"
        return False


def draw(input_file, save=False, show=False):
    """
    """
    values = open_read(input_file)
    
    
    #dates = values[:,0]
    temperature = values[:,2]
    humidity = values[:,3]
    light = values[:,4]
    
    dates = [ datetime.datetime.fromtimestamp(v) for v in values[:,0] ]
   
    fig=plt.figure(figsize=(12, 9))
    #fig.suptitle(input_file)
    ax1 = fig.add_subplot(311)
    ax2 = fig.add_subplot(312, sharex=ax1)
    ax3 = fig.add_subplot(313, sharex=ax1)

    ax1.plot(dates,humidity,'g+-')
    ax1.xaxis.set_major_formatter(mdates.DateFormatter('%H hrs \n%d-%m'))
    ax1.set_title('Humidity (%)')

    ax2.plot(dates,light,'ro-')
    ax2.xaxis.set_major_formatter(mdates.DateFormatter('%H hrs \n%d-%m'))
    ax2.set_title('Light (AU)')

    ax3.plot(dates,temperature,'b*-')
    ax3.xaxis.set_major_formatter(mdates.DateFormatter('%H hrs \n%d-%m'))
    ax3.set_title('Temperature (Cel)')

    plt.tight_layout()
    #plt.subplots_adjust(top=0.85)
    
    if show: 
        plt.show()
        
    if save:
        fn, _ = os.path.splitext(input_file)
        #fo = os.path.join (fn, ".png")
        plt.savefig(fn)
    
    return values

if __name__ == '__main__':
    parser = optparse.OptionParser(usage='%prog [options] [argument]', version='%prog version 0.1')
    parser.add_option('-i', '--input', dest='source', metavar="SOURCE", help="Input file to be processed")

    parser.add_option('--draw', action="store_true", default=False, dest='draw', help="Draw Figure and show")
    parser.add_option('--save', action="store_true", default=False, dest='save', help="Save Figure to file")
    
    (options, args) = parser.parse_args()

    if not options.source:
        parser.print_help()
    else:
        draw(options.source, save=options.save, show=options.draw)
        


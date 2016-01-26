#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
#  damAlert.py
#  
#  Copyright 2012 Giorgio Gilestro <giorgio@gilest.ro>
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


from DAMrealtime import DAMrealtime, ENVrealtime
import optparse

__version__ = '0.2'    


if __name__ == '__main__':

    usage =  '%prog [options] [argument]\n'
    version= '%prog version ' + str(__version__)
    
    parser = optparse.OptionParser(usage=usage, version=version )
    parser.add_option('-d', '--dampath', dest='dam_path', metavar="/path/to/DAM/", help="Specifies where the RAW TriKinetics files are located")
    parser.add_option('-e', '--envpath', dest='env_path', metavar="/path/to/flymon/", help="Specifies where the RAW flymon files are located")

    (options, args) = parser.parse_args()

    email = dict (
                  sender = 'g.gilestro@imperial.ac.uk',
                  recipient  = 'giorgio.gilestro@gmail.com',
                  server = 'automail.cc.ic.ac.uk'
                  )

    if options.dam_path:
       d = DAMrealtime(path=options.dam_path, email=email)
       DAM_problems = [( fname, d.getDAMStatus(fname) ) for fname in d.listDAMMonitors() if d.getDAMStatus(fname) == '50']
       if DAM_problems: d.alert(DAM_problems)
       
    if options.env_path:
       e = ENVrealtime(path=options.env_path, email=email)
       ENV_problems = [( fname, e.hasEnvProblem(fname) ) for fname in e.listEnvMonitors() if e.hasEnvProblem(fname)]
       if ENV_problems: e.alert(ENV_problems)


    if not options.dam_path and not options.env_path:
        parser.error("You must specify a path for TriKinetics or flymon files")
        parser.print_help()  






import serial
import time
from platform import system
from time import strftime

import collections
import csv
import logging
import optparse


__version__ = '0.2'



class SerialController(object):
    def __init__(self,port,baud):
        self._serial = serial.Serial(port,baud)
        self._serial.write('I 5')
        # # a map of device_id -> last time seen.
        # self._detected_devices = {}

        # we sync time when and only when device had drifter more than this value
        self._delta_time_threshold = 15 #s

    def _parse_serial_line(self,line):
        logging.debug("Getting line = " + line)
        fields = line.rstrip().split(" ")
        if len(fields) != 13:
            logging.warning("Wrong number of fields in serial input. Expect 13, got %i.", len(fields))
            logging.debug('fields = ' + str(fields))
            return

        id, code, counter, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode = fields
    
        out = collections.OrderedDict()
        out['id']=int(id)
        out['code']=code
        out['counter']=int(counter)
        out['device_time']=float(device_time)
        out['server_time']=time.time()
        out['temperature']=float(temperature)
        out['humidity']=float(humidity)
        out['light']=int(light)
        out['set_temp']=float(set_temp)
        out['set_hum']=float(set_hum)
        out['set_light']=int(set_light)
        out['lights_on']=float(lights_on)
        out['lights_off']=float(lights_off)
        out['dd_mode']=['DD', 'LD', 'LL'][int(dd_mode)]

        logging.debug(str(out))
        return out



    def _sync_time(self,device_id, device_time):
        now = int(round(time.time()))
        if abs(device_time - now) > self._delta_time_threshold:
            logging.warning("Current time is %i, but time on device %i is %i, syncing this device"% (now, device_id, device_time))
            command = " ".join(['T', str(device_id), str(now)])
            logging.debug("Sending " + command)
            self._serial.write(command + '\n')
            out = self._serial.readline()
            if "failed" in out:
                logging.error("Failed to sync time!")

    def __iter__(self):
        while True:
            serial_line = self._serial.readline()

            fields = self._parse_serial_line(serial_line)
            if fields is None:
                continue
            self._sync_time( fields["id"], fields["device_time"])
            yield fields




if __name__ == '__main__':

    RESULT_FILE = "/tmp/test.csv"
    BAUD =  115200

    parser = optparse.OptionParser()

    parser.add_option("-o", "--output", dest="output", default=RESULT_FILE, help="Where is the result stored")
    parser.add_option("-p", "--port", dest="port", default='/dev/ttyACM1',help="Serial port to use")
    parser.add_option("-D", "--debug", dest="debug", default=False, help="Shows all logging messages", action="store_true")


    (options, args) = parser.parse_args()
    option_dict = vars(options)
    if(option_dict["debug"]):
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Logger in DEBUG mode")



    serial_fetcher = SerialController(option_dict["port"], BAUD)
    time.sleep(10)
    writer = None
    with open(option_dict["output"],"a") as output:
        for fields in serial_fetcher:
            # for now lets just do:
	    if writer is None:
                 writer = csv.DictWriter(output, fields.keys())
                 #writer.writeheader()
            writer.writerow(fields)







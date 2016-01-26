
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
        # # a map of device_id -> last time seen.
        # self._detected_devices = {}

        # we sync time when and only when device had drifter more than this value
        self._delta_time_threshold = 15 #s

    def _parse_serial_line(self,line):
        logging.debug("Getting" + line)
        fields = line.rstrip().spit(" ")
        if len(fields) != 8:
            logging.error("Wrong number of fields in serial input")
            raise Exception("Wrong number of fields in serial input")

        id, code, counter, device_time, temperature, humidity, light, led_level = fields

        out = collections.OrderedDict(id=id, code=code, counter=counter,
                                      device_time=device_time, temperature=temperature,
                                      humidity=humidity, light=light, led_level=led_level)
        logging.debug(str(out))
        return out



    def _sync_time(self,device_id, device_time):
        now = time.time()
        if abs(device_time - now) > self._delta_time_threshold:
            logging.warning("Current time is %i, but time on device %i is %i, syncing this device", (now, device_id, device_time))
            command = " ".join(['T', str(device_id), str(now)])
            logging.debug("Sending " + command)
            self._serial.write(command)


    def __iter__(self):
        while True:
            serial_line = self._serial.readline()
            fields = self._parse_serial_line(serial_line)
            self._sync_time( fields["id"], fields["device_time"])





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

    with open(option_dict["output"],"a") as output:
        for fields in serial_fetcher:
            # for now lets just do:
            csv_line = ",".join([str(f) for f in fields])
            output.write(csv_line)





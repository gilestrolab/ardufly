
import serial
import time
import collections
import csv
import logging
import optparse
import sys
import glob



class SerialController(object):
    def __init__(self,baud,port=None):
        if port is None:
            logging.debug("No port specified. Scanning for ports...")
            port = self._find_port(baud)


        self._serial = serial.Serial(port,baud)
        # we sync time when and only when device had drifter more than this value
        self._delta_time_threshold = 15 #s

        self._dd_mode_map = {0:'DD', 1:'LD', 2:'LL'}
    @staticmethod
    def _find_port(baud):
        """
        from http://stackoverflow.com/questions/12090503/listing-available-com-ports-with-python
        Lists serial port names

            :raises EnvironmentError:
                On unsupported or unknown platforms
            :returns:
                A list of the serial ports available on the system
        """

        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                logging.debug("Scanning %s ..." % port)
                s = serial.Serial(port,baud,timeout=2)
                s.write("H\n")
                line = s.readline()

                logging.debug("Getting '%s'" % line)

                s.close()
                if not line:
                    logging.debug("Port %s does not return any data, skipping" % port)
                    continue
                result.append(port)
            except (OSError, serial.SerialException) as e:
                logging.debug(str(e))
                pass

        logging.debug("Ports found: %s" % str(result))
        if len(result) == 0:
            raise Exception("No port found! Is the device plugged? Run in debug mode (-D) to show more info.")
        elif len(result) > 1:
            logging.warning("Several candidate ports have been detected. Using %s" % result[0])
        return result[0]

    def _parse_serial_line(self,line):
        logging.debug("Getting line = " + line)
        fields = line.rstrip().split(" ")
        if len(fields) != 13:
            logging.warning("Wrong number of fields in serial input. Expect 13, got %i.", len(fields))
            logging.debug('fields = ' + str(fields))
            return

        id, code, counter, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode = fields
    
        out = collections.OrderedDict()
        out['id'] = int(id)
        out['code'] = code
        out['counter'] = int(counter)
        out['device_time']=round(float(device_time),2)
        out['server_time'] = round(time.time(),2)
        out['temperature'] =float(temperature)
        out['humidity'] = float(humidity)
        out['light'] = int(light)
        out['set_temp'] = float(set_temp)
        out['set_hum'] = float(set_hum)
        out['set_light'] = int(set_light)
        out['lights_on'] = float(lights_on)
        out['lights_off'] = float(lights_off)
        out['dd_mode'] = self._dd_mode_map[int(dd_mode)]

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
    BAUD =  115200

    parser = optparse.OptionParser()

    parser.add_option("-o", "--output", dest="output", help="Where is the result stored")
    parser.add_option("-p", "--port", dest="port", default=None,help="Serial port to use")
    parser.add_option("-D", "--debug", dest="debug", default=False, help="Shows all logging messages", action="store_true")


    (options, args) = parser.parse_args()
    option_dict = vars(options)
    if(option_dict["debug"]):
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Logger in DEBUG mode")



    serial_fetcher = SerialController(BAUD, option_dict["port"])
    time.sleep(1)
    writer = None
    with open(option_dict["output"],"a") as output:
        logging.debug('Output file is ' + option_dict['output'])
        for fields in serial_fetcher:
            # for now lets just do:
	    if writer is None:
                 writer = csv.DictWriter(output, fields.keys())
                 writer.writeheader()
            logging.debug('writing row: ' + str(fields))
            writer.writerow(fields)
            line = ','.join([str(f) for f in fields.values()])
            output.flush()

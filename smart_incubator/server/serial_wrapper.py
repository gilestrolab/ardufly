__author__ = 'diana'

import serial
import collections
import logging
import time
import sys
import glob



class SerialWrapper(object):
    _n_expected_values = 13

    _command_dict = {"set_temp": 'C',
                     "set_hum": 'H',
                     "set_light": 'L',
                     "dd_mode": 'M',
                     "lights_on": '1',
                     "light_off": '0',
                     "set_time": 'T',
                     "interval": 'F'
                    }

    _dd_mode_map = {0:'DD', 1:'LD', 2:'LL', 3:'DL', 4:'MM'}

    _delta_time_threshold = 15 # we sync time when and only when device had drifter more than this value (seconds)



    def __init__(self, port=None, baud=115200, serial_class=serial.Serial):
        """
        """
        if port is None:
            logging.debug("No port specified. Scanning for ports...")
            port = self._find_port(baud)

        self._last_fields = {}

        self._serial = serial_class(port, baud)


        # self._datatail = collections.deque(maxlen=100) #keeps last 100 lines in memory as dict
        # self._serial_queue = collections.deque(maxlen=100) #keeps last 100 lines in memory as raw lines

        time.sleep(1)

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
                time.sleep(2)
                s.write("HELP\n")
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

    def read_line(self):
        serial_line = self._serial.readline()
        fields = self._parse_serial_line(serial_line)

        if fields is None:
            return None

        inc_id = fields["inc_id"]
        self._last_fields[inc_id] = fields
        return fields


    def _parse_serial_line(self, line):
        """
        """
        logging.debug("Getting line = " + line)
        fields = line.rstrip().split(" ")

        if len(fields) != self._n_expected_values:
            logging.warning("Wrong number of fields in serial input. Expect %s, got %i." % (self._n_expected_values, len(fields)))
            logging.debug('fields = ' + str(fields))
            return

        inc_id, command, counter, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode = fields

        #if it's the room reading
        if int(inc_id) == 0 :
            device_time = time.time()
            set_temp = 21
            set_hum = 35

        out = collections.OrderedDict()
        out['inc_id'] = int(inc_id)
        out['row_type'] = command
        out['counter'] = int(counter)
        out['device_time'] = round(float(device_time),2)
        out['server_time'] = round(time.time(),2)
        #out['server_time'] = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime())
        out['temperature'] = round(float(temperature), 1)
        out['humidity'] = round(float(humidity), 1)
        out['light'] = int(light)
        out['set_temp'] = float(set_temp)
        out['set_hum'] = float(set_hum)
        out['set_light'] = int(set_light)
        out['lights_on'] = float(lights_on)
        out['lights_off'] = float(lights_off)
        out['dd_mode'] = int(dd_mode)

        logging.debug(str(out))
        return out

    def _sync_time(self, device_id, device_time):
        """
        """
        now = int(round(time.time()))
        if abs(device_time - now) > self._delta_time_threshold and device_id > 0:
            logging.warning("Current time is %i, but time on device %i is %i, syncing this device" % (now, device_id, device_time))
            return self.sendCommand(device_id, cmd='set_time', value=now)


    def sendRaw(self, line):
        """
        """
        line = line.strip().encode('ascii')
        logging.debug("Sending " + line)
        self._serial.write(line + '\n')
        return True


    def sendCommand(self, inc_id, cmd, value):
        """
        """


        if cmd not in self._command_dict.keys():
            logging.error("Command non existent: %s" % cmd) ## no such command
            raise Exception("Command not found")

        line = self._command_dict[cmd] + "%s %s"

        line = line % (inc_id, value)

        logging.debug("Sending " + line)
        self._serial.write(line + '\n')
        #we should add a way to check for success here
        #out = self._serial.readline()
        #if "failed" in out:
            #logging.error("Failed to send command: %s" % line)
            #return False
        return True




    def update(self, inc_id, new_values):
        """
        """

        last_field_inc = self._last_fields[inc_id]
        resp = []
        #todo what is first data i.e. we have not reqad field for this inc yet...?
        for field, value  in new_values:
            if field not in last_field_inc.keys():
                logging.error("...")
            if value != last_field_inc[field]:
                resp.append("Set %s to %s" % (field, str(value)))

        return resp


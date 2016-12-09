import serial
import time, datetime
import collections
import csv
import logging
import optparse
import sys
import glob
import os
 
import threading
from bottle import Bottle, template, static_file, url, request
import json
 
import MySQLdb
from MySQLdb import OperationalError
import MySQLdb.cursors
from MySQLdb.constants import FIELD_TYPE

 
class mySQLConnection():
    """
    MariaDB [(none)]> CREATE DATABASE incubators;
    MariaDB [(none)]> CREATE USER 'incubators'@'localhost' IDENTIFIED BY 'incubators';
    MariaDB [(none)]> GRANT ALL PRIVILEGES ON incubators.* to 'incubators'@'localhost';
    MariaDB [(none)]> FLUSH PRIVILEGES;
     
    CREATE TABLE incubators.incubators
    (
    id int NOT NULL AUTO_INCREMENT,
    inc_id VARCHAR(255),
    row_type CHAR,
    counter BIGINT,
    device_time TIMESTAMP,
    temperature FLOAT,
    humidity FLOAT,
    light SMALLINT,
    set_temp FLOAT,
    set_hum FLOAT,
    set_light TINYINT,
    lights_on MEDIUMINT,
    lights_off MEDIUMINT,
    dd_mode TINYINT,
    PRIMARY_KEY(id)
    );
     
    COMMIT;

    """
 
    def __init__(self, db_credentials):
        
        self._DB_CREDENTIALS = db_credentials
        self.connect()
        
    def connect(self):

        _db_name = self._DB_CREDENTIALS["db_name"]
        _db_user_name = self._DB_CREDENTIALS["username"]
        _db_user_pass = self._DB_CREDENTIALS["password"]
        
        my_conv = { FIELD_TYPE.TIMESTAMP: str, FIELD_TYPE.FLOAT: float, FIELD_TYPE.TINY: int, FIELD_TYPE.LONG: int, FIELD_TYPE.INT24: int}
        
        logging.debug("Creating a new connection with the `incubators` db")

        self.connection = MySQLdb.connect('localhost', _db_user_name, _db_user_pass, _db_name, conv=my_conv, cursorclass=MySQLdb.cursors.DictCursor)
        self.connection.autocommit(True)
 
    @staticmethod
    def _timestamp_to_datetime(timestamp):
        return time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(timestamp))
 
    def insert(self, query):
        """
        """
        try:
            with self.connection as cursor:
                cursor.execute(query)

        except (AttributeError, OperationalError):
            self.connect()
            self.insert(query)

    def query(self, query):
        """
        """
        try:
            with self.connection as cursor:
                cursor.execute(query)
                result = cursor.fetchall()

        except (AttributeError, OperationalError):
            self.connect()
            result = self.query(query)

        return result
         
    def insert_row(self, data):
        """
        """
     
        d = data.copy()
     
        del(d['server_time'])
        d['device_time'] = self._timestamp_to_datetime(d['device_time'])
         
        names = ",".join(d.keys())
        values = ",".join(["'%s'" % s for s in d.values()])
        command = "INSERT INTO incubators (%s) VALUES (%s)" % (names, values)
        return self.insert(command)
         
    def retrieve_day(self, incubator, days=0, full=False):
        """
        """
        days = int(days)
        ZT0 = (9,0,0)
         
        start = datetime.date.today() - datetime.timedelta(days=days)
        end = start + datetime.timedelta(days=1)
         
        #datetime format
        start = datetime.datetime(start.year, start.month, start.day, *ZT0).strftime('%Y-%m-%d %H:%M:%S')
        end = datetime.datetime(end.year, end.month, end.day, *ZT0).strftime('%Y-%m-%d %H:%M:%S')
         
        #timestamp format
        #start = time.mktime(datetime.datetime(start.year, start.month, start.day, *ZT0).timetuple())
        #end = time.mktime(datetime.datetime(end.year, end.month, end.day, *ZT0).timetuple())
 
        if full:
            select_query = "SELECT inc_id, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode FROM incubators WHERE inc_id = %s AND device_time BETWEEN '%s' AND '%s'" % ( int(incubator), start, end)
        else:
            select_query = "SELECT inc_id, device_time, temperature, humidity, light/10 as light FROM incubators WHERE inc_id = %s AND device_time BETWEEN '%s' AND '%s'" % ( int(incubator), start, end)
         
        data = self.query(select_query)
        return data
 
    def getAllIDs(self):
        """
        """
        select_query = "SELECT DISTINCT(inc_id) as inc_id FROM incubators ORDER BY inc_id ASC;"
        data = self.query(select_query)
        return [i['inc_id'] for i in data]
 
    def retrieve_last_line(self, incubator):
        """
        """
        if incubator == 'all' or incubator < 0:
            select_query = "SELECT * FROM incubators.incubators WHERE id IN ( SELECT MAX(id) FROM incubators.incubators GROUP BY inc_id) ORDER BY ABS(inc_id);"
            data = self.query(select_query)

        else:
            select_query = "SELECT * FROM incubators WHERE inc_id = %s ORDER BY device_time DESC LIMIT 1;" % incubator
            data = self.query(select_query)[0]
        
        return data
 
class SerialController(threading.Thread):
    
    _n_expected_values = 13

    _command_dict = {"set_temp"  :  ['C', 'Temperature'],
                     "set_hum"   :  ['H', 'Humidity'],
                     "set_light" :  ['L', 'Light'],
                     "dd_mode"   :  ['M', 'Light mode'],
                     "lights_on" :  ['1', 'Lights on time'],
                     "lights_off":  ['0', 'Lights off time'],
                     "set_time"  :  ['T', 'Clock time'],
                     "interval"  :  ['F', 'Transmit Interval']
                    }

    _dd_mode_map = {0:'DD', 1:'LD', 2:'LL', 3:'DL', 4:'MM'}

    _delta_time_threshold = 15 # we sync time when and only when device had drifter more than this value (seconds)

    
    def __init__(self, port, baud, db_credentials):
        """
        """
        
        if baud is None:
            baud = 115200
        
        if port is None:
            logging.debug("No port specified. Scanning for ports...")
            port = self._find_port(baud)
 
        super(SerialController, self).__init__()
 
        self._serial = serial.Serial(port,baud)
       
        self._datatail = collections.deque(maxlen=100) #keeps last 100 lines in memory as dict
        self._serial_queue = collections.deque(maxlen=100) #keeps last 100 lines in memory as raw lines
        self._is_stopped = False
        
        self._database = mySQLConnection(db_credentials)
 
 
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
 
    def _parse_serial_line(self,line):
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
        if int(inc_id) <= 0 :
            device_time = time.time()
            set_temp = 21
            set_hum = 35
     
        out = collections.OrderedDict()
        out['inc_id'] = int(inc_id)
        out['row_type'] = command
        out['counter'] = int(counter)
        out['device_time']=round(float(device_time),2)
        out['server_time'] = round(time.time(),2)
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
            logging.warning("Current time is %i, but time on device %i is %i, syncing this device"% (now, device_id, device_time))
            return self.sendCommand(device_id, cmd='set_time', value=now)
 
    def getSerialBuffer(self):
        """
        """
        return self._serial_queue
     
    def getlastDataFromBuffer(self, incubator, json_mode=True):
        """
        """
        if incubator == "all" or incubator < 0 :
            all_incubators = []
            all_incubators_id = set()
             
            for row in reversed(self._datatail):
                if row['inc_id'] not in all_incubators_id:
                    all_incubators.append(row)
                    all_incubators_id.add(row['id'])
            if json_mode:
                return json.dumps(sorted(all_incubators, key=lambda k: k['id']) )
            else:
                return all_incubators
             
 
        else:
            for row in reversed(self._datatail):
                if row['inc_id'] == int(incubator):
                    return row
        return None
 
    def sendRaw(self, line):
        """
        """
        line = line.strip().encode('ascii')
        logging.debug("Sending " + line)
        self._serial.write(line + '\n')
        return True
         
    def allowedCommands(self):
        """
        """
        return self._command_dict.keys()

    def sendCommand(self, inc_id, cmd, value):
        """
        """
        if cmd not in self._command_dict.keys():
            logging.error("Command does not exist: %s" % cmd) ## no such command
            raise Exception("Command not found")

        line = "%s %s %s" % ( self._command_dict[cmd][0], inc_id, value)

        if self.sendRaw(line):
            return "%s set to %s for incubator %s\n" % ( self._command_dict[cmd][1], value, inc_id)

        #we should add a way to check for success here
        return
        
    def run(self):
        """
        """
        while not self._is_stopped:
            serial_line = self._serial.readline()
            self._serial_queue.append(serial_line)
             
            fields = self._parse_serial_line(serial_line)
            if fields is None:
                continue
             
            self._sync_time( fields["inc_id"], fields["device_time"])
            self._database.insert_row(fields)
             
    def stop(self):
        self._is_stopped = True
 
 
class webServer(Bottle):
 
    def __init__(self, db_credentials, serial_port, baud):
        super(webServer, self).__init__()

        self._serial_interface = SerialController(serial_port, baud, db_credentials)
        self._serial_interface.start()
        
        self._database = mySQLConnection(db_credentials)
        self._route()
 
    def _route(self):
        self.get('/', callback=self._index)
        self.get('/serial', callback=self._serialmonitor)
        self.route('/graph/<inc_id>/<day>', callback=self._get_graph, method=["post", "get"])
 
        self.get('/json/<inc_id>', callback=self._incubator_json)
        self.get('/incubator/<inc_id>/<days>', callback=self._get_incubator)
 
        self.post('/send', callback=self._send_to_serial)
 
        self.get('/static/<filepath:path>', callback=self._get_static)        
        self.get('/listen/<status>', callback=self._listen_to_serial)
        self.get('/quit', callback=self._quit)
 
    def _get_static(self, filepath):
        return static_file(filepath, root="./static")
 
    def _index(self):
        return static_file('index.html', root="static")
 
    def _serialmonitor(self):
        return static_file('serialmonitor.html', root="static")
         
    def _send_to_serial(self):
        myDict = request.json['myDict']
        self._serial_interface.sendRaw ( myDict['line'] )
        return {"result": "OK"}
         
    def _get_graph(self, inc_id, day):
         
        rep = {}
        if request.forms.get("submitted"):
            values = {'inc_id' : inc_id}
            values['set_temp'] = float(request.forms.get("set_temp"))
            values['set_hum'] = int(request.forms.get("set_hum"))
            values['set_light'] = int(request.forms.get("set_light"))
            values['dd_mode'] = int(request.forms.get("dd_mode"))
            values['lights_on'] = int(request.forms.get("lights_on"))
            values['lights_off'] = int(request.forms.get("lights_off"))
            rep['message'] = self.update(inc_id, values)
        else:  
            rep['message'] = '' 
             
        rep['incubator_id'] = inc_id
        rep['day'] = day
        return  template('static/graph.tpl', rep)
 
    def _incubator_json(self, inc_id=0):
        """
        """
        if inc_id == 'serial':
 
            serial = list(self._serial_interface.getSerialBuffer())
            serial.reverse()
            data = {'result': ''.join(serial) }
            return data
            
        else:
            return self.getlastData(inc_id)
         
    def _get_incubator(self, inc_id, days):
         
        data = self._database.retrieve_day(inc_id, days)
        return json.dumps(data)
                 
    def getlastData(self, incubator, json_mode=True):
        """
        """
        if json_mode or (incubator != 'all' and incubator < 0 ):
            return json.dumps(self._database.retrieve_last_line(incubator))
        else:
            return self._database.retrieve_last_line(incubator)
     
    def update(self, inc_id, new_values):
        """
        """
        current = self.getlastData( inc_id, json_mode=False)
        resp = ""

        for key in self._serial_interface.allowedCommands():
            if key in new_values and ( new_values [key] != current[key] ):
                resp += self._serial_interface.sendCommand(inc_id, cmd=key, value=new_values[key])
 
        return resp

    def _listen_to_serial(self, status=True):
        """
        """
        if status:
            self._serial_interface.start()
        else:
            self._serial_interface.stop()
             
        return {"listener_status" : status}
         
    def _quit(self):
        os._exit(1)
 
def transfer_file_to_db(filename, DB_CREDENTIALS):
     
    with open(filename, "r") as fh:
        for line in fh.readlines()[1:]:
            print line
 
 
if __name__ == '__main__':
    parser = optparse.OptionParser()
 
    parser.add_option("-p", "--port", dest="port", default=None,help="Serial port to use")
    parser.add_option("-D", "--debug", dest="debug", default=False, help="Shows all logging messages", action="store_true")
    parser.add_option("-w", "--web", dest="web", default=False, help="Starts the webserver", action="store_true")
 
 
    (options, args) = parser.parse_args()
    option_dict = vars(options)
    
    if(option_dict["debug"]):
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Logger in DEBUG mode")

    #this should be stored in a .ini file
    dbc = {'username': 'incubators', 'password' : 'incubators', 'db_name' : 'incubators' }
 
    if option_dict['web']:
        app = webServer(db_credentials=dbc, serial_port=option_dict["port"], baud=115200)
        app.run(host='0.0.0.0', port=8090)
    
    else:
        
        serial_fetcher = SerialController(serial_port, db_credentials=dbc)
        serial_fetcher.start()

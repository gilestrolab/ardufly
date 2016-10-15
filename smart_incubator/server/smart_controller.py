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
from MySQLdb.constants import FIELD_TYPE
 
 
class mySQLDatabase():
    """
    MariaDB [(none)]> CREATE DATABASE incubators;
    MariaDB [(none)]> CREATE USER 'incubators'@'localhost' IDENTIFIED BY 'incubators';
    MariaDB [(none)]> GRANT ALL PRIVILEGES ON incubators.* to 'incubators'@'localhost';
    MariaDB [(none)]> FLUSH PRIVILEGES;
     
    CREATE TABLE incubators.incubators
    (
    id TINYINT,
    row_type CHAR,
    counter BIGINT,
    device_time TIMESTAMP,
    temperature FLOAT,
    humidity FLOAT,
    light SMALLINT,
    set_temp FLOAT,
    set_hum FLOAT,
    set_light SMALLINT,
    lights_on MEDIUMINT,
    lights_off MEDIUMINT,
    dd_mode TINYINT
    );
     
    COMMIT;
 
    """
 
    def __init__(self, db_credentials):
        
        self._db_credentials = db_credentials
        self.connect()
        
    def connect(self):

        _db_name = self._db_credentials["db_name"]
        _db_user_name = self._db_credentials["username"]
        _db_user_pass = self._db_credentials["password"]
        
        my_conv = { FIELD_TYPE.TIMESTAMP: str }

        self.connection = MySQLdb.connect('127.0.0.1', _db_user_name, _db_user_pass, _db_name, conv=my_conv)
        self.cursor = self.connection.cursor(MySQLdb.cursors.DictCursor)
        self.cursor.connection.autocommit(True)
 
    @staticmethod
    def _timestamp_to_datetime(timestamp):
        return time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(timestamp))
 
    def insert(self, query):
        try:
            self.cursor.execute(query)
            self.connection.commit()
            self.connection.close()
        except:
            
            self.connect()
            self.cursor.execute(query)
            self.connection.commit()            
            self.connection.close()

    def query(self, query):
        self.connect()
        cursor = self.connection.cursor( MySQLdb.cursors.DictCursor )
        cursor.execute(query)
        result = cursor.fetchall()
        
        #self.cursor.execute(query)
        #result = self.cursor.fetchall()

        self.connection.close()

        return result
 
    def __del__(self):
        self.connection.close()
         
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
        zt0 = (9,0,0)
         
        start = datetime.date.today() - datetime.timedelta(days=days)
        end = start + datetime.timedelta(days=1)
         
        #datetime format
        start = datetime.datetime(start.year, start.month, start.day, *zt0).strftime('%Y-%m-%d %H:%M:%S')
        end = datetime.datetime(end.year, end.month, end.day, *zt0).strftime('%Y-%m-%d %H:%M:%S')
         
        #timestamp format
        #start = time.mktime(datetime.datetime(start.year, start.month, start.day, *zt0).timetuple())
        #end = time.mktime(datetime.datetime(end.year, end.month, end.day, *zt0).timetuple())
 
        if full:
            select_query = "SELECT id, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode FROM incubators WHERE id = %s AND device_time BETWEEN '%s' AND '%s'" % ( int(incubator), start, end)
        else:
            select_query = "SELECT id, device_time, temperature, humidity, light FROM incubators WHERE id = %s AND device_time BETWEEN '%s' AND '%s'" % ( int(incubator), start, end)
         
        data = self.query(select_query)
        return data
 
    def getAllIDs(self):
        """
        """
        select_query = "SELECT DISTINCT(id) as id FROM incubators ORDER BY id ASC;"
        data = self.query(select_query)
        return [i['id'] for i in data]
 
    def retrieve_last_line(self, incubator):
        """
        """
        if incubator == 'all' or incubator < 0:
            select_query = "SELECT * FROM incubators.incubators WHERE device_time IN (SELECT MAX(device_time) FROM incubators.incubators GROUP BY id) ORDER BY id;"
            data = self.query(select_query)

        else:
            select_query = "SELECT * FROM incubators WHERE id = %s ORDER BY device_time DESC LIMIT 1;" % incubator
            data = self.query(select_query)[0]

        return data
 
class SerialController(threading.Thread):
    def __init__(self, port=None, baud=115200, filename=None, db_credentials=None):
        """
        """
        if port is None:
            logging.debug("No port specified. Scanning for ports...")
            port = self._find_port(baud)
 
        super(SerialController, self).__init__()
 
        self._serial = serial.Serial(port,baud)
         
        self._delta_time_threshold = 15 # we sync time when and only when device had drifter more than this value (seconds)
        self._dd_mode_map = {0:'DD', 1:'LD', 2:'LL', 3:'DL', 4:'MM'}
        
        self._datatail = collections.deque(maxlen=100) #keeps last 100 lines in memory as dict
        self._serial_queue = collections.deque(maxlen=100) #keeps last 100 lines in memory as raw lines
         
        self._is_stopped = False
 
        self._filename = filename
         
        if db_credentials:
            self._database = mySQLDatabase(db_credentials)
 
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
 
    def _parse_serial_line(self,line):
        """
        """
        n_expected_values=13
        logging.debug("Getting line = " + line)
        fields = line.rstrip().split(" ")
         
        if len(fields) != n_expected_values:
            logging.warning("Wrong number of fields in serial input. Expect %s, got %i." % (n_expected_values, len(fields)))
            logging.debug('fields = ' + str(fields))
            return
 
        id, command, counter, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode = fields
        if int(id) == 0 : device_time = time.time()
     
        out = collections.OrderedDict()
        out['id'] = int(id)
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
 
    def _write_row_to_file(self, fields):
        """
        """
         
        try:
 
            if not os.path.isfile(self._filename):
                fh = open(self._filename, "w")
                self._writer = csv.DictWriter(fh, fields.keys())
                self._writer.writeheader()
                logging.debug("The output file did not exist before. Writing headers")
 
            else:
                fh = open(self._filename, "a")
                self._writer = csv.DictWriter(fh, fields.keys())
             
        except NameError:
            logging.error("Cannot save to file. Filename or fields not specified.")
        except (OSError, IOError) as e:
            logging.error(str(e))
             
                 
        logging.debug('writing row: ' + str(fields))
        self._writer.writerow(fields)
        fh.close()
     
    def getHistory(self, incubator,days=0):
        """
        """
        if self._database:
            return self._database.retrieve_day(incubator, days)
                 
    def getlastData(self, incubator, json_mode=True):
        """
        """
        if self._database:
            if json_mode or (incubator != 'all' and incubator >= 0 ):
                return json.dumps(self._database.retrieve_last_line(incubator))
            else:
                return self._database.retrieve_last_line(incubator)
                 
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
                if row['id'] not in all_incubators_id:
                    all_incubators.append(row)
                    all_incubators_id.add(row['id'])
            if json_mode:
                return json.dumps(sorted(all_incubators, key=lambda k: k['id']) )
            else:
                return all_incubators
             
 
        else:
            for row in reversed(self._datatail):
                if row['id'] == int(incubator):
                    return row
        return None
 
    def __iter__(self):
        """
        """
        while True:
            serial_line = self._serial.readline()
 
            fields = self._parse_serial_line(serial_line)
            if fields is None:
                continue
            self._sync_time( fields["id"], fields["device_time"])
            yield fields
     
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
        if cmd == 'set_temp':
            line = 'C %s %s' % (inc_id, value)
        elif cmd == 'set_hum':
            line = 'H %s %s' % (inc_id, value)
        elif cmd == 'set_light':
            line = 'L %s %s' % (inc_id, value)
        elif cmd == 'dd_mode':
            line = 'M %s %s' % (inc_id, value)
        elif cmd == 'lights_on':
            line = '1 %s %s' % (inc_id, value)
        elif cmd == 'lights_off':
            line = '0 %s %s' % (inc_id, value)
        elif cmd == 'set_time':
            line = 'T %s %s' % (inc_id, value)
        elif cmd == 'interval':
            line = 'F %s %s' % (inc_id, value)
             
        logging.debug("Sending " + line)
        self._serial.write(line + '\n')
        #we should add a way to check for success here
        #out = self._serial.readline()
        #if "failed" in out:
            #logging.error("Failed to send command: %s" % line)
            #return False
        return True
 
 
     
    def update(self, inc_id, values):
        """
        """
        current = self.getlastData( inc_id )
         
        resp = ""
         
        if values['set_temp'] != current['set_temp'] : 
            if self.sendCommand(inc_id, cmd='set_temp', value=values['set_temp']):
                resp += "Temperature set to %s\n" % values['set_temp']
        if values['set_hum'] != current['set_hum'] : 
            if self.sendCommand(inc_id, cmd='set_hum', value=values['set_hum']):
                resp += "Humidity set to %s\n" % values['set_hum']
        if values['set_light'] != current['set_light'] : 
            if self.sendCommand(inc_id, cmd='set_light', value=values['set_light']):
                resp += "Light set to %s\n" % values['set_light']
        if values['dd_mode'] != current['dd_mode'] : 
            if self.sendCommand(inc_id, cmd='dd_mode', value=values['dd_mode']):
                resp += "Light Regime set to %s\n" % values['dd_mode']
        if values['lights_on'] != current['lights_on'] : 
            if self.sendCommand(inc_id, cmd='lights_on', value=values['lights_on']):
                resp += "Lights on set to %s\n" % values['lights_on']
        if values['lights_off'] != current['lights_off'] : 
            if self.sendCommand(inc_id, cmd='lights_off', value=values['lights_off']):
                resp += "Lights off set to %s\n" % values['lights_off']
 
        return resp
 
    def run(self):
        """
        """
        while not self._is_stopped:
            serial_line = self._serial.readline()
            self._serial_queue.append(serial_line)
             
            fields = self._parse_serial_line(serial_line)
            if fields is None:
                continue
             
            self._sync_time( fields["id"], fields["device_time"])
 
             
            if self._filename: 
                self._write_row_to_file(fields)
                self._datatail.append(fields)
             
            elif self._database: 
                self._database.insert_row(fields)
             
 
             
    def stop(self):
        self._is_stopped = True
 
 
class webServer:
    def __init__(self, host, port, serial_fetcher):
        self._host = host
        self._port = port
        self._app = Bottle()
        self._route()
         
        self._serial_fetcher = serial_fetcher
        self._serial_fetcher.start()
 
    def _route(self):
        self._app.get('/', callback=self._index)
        self._app.get('/serial', callback=self._serialmonitor)
        self._app.route('/graph/<inc_id>', callback=self._get_graph, method=["post", "get"])
 
        self._app.get('/json/<inc_id>', callback=self._incubator_json)
        self._app.get('/incubator/<inc_id>/<days>', callback=self._get_incubator)
 
        self._app.post('/send', callback=self._send_to_serial)
 
        self._app.get('/static/<filepath:path>', callback=self._get_static)        
        self._app.get('/listen/<status>', callback=self._listen_to_serial)
        self._app.get('/quit', callback=self._quit)
 
    def start(self):
        self._app.run(host=self._host, port=self._port)
 
    def _get_static(self, filepath):
        return static_file(filepath, root="./static")
 
    def _index(self):
        return static_file('index.html', root="static")
 
    def _serialmonitor(self):
        return static_file('serialmonitor.html', root="static")
         
    def _send_to_serial(self):
        myDict = request.json['myDict']
        self._serial_fetcher.sendRaw ( myDict['line'] )
        return {"result": "OK"}
         
    def _get_graph(self, inc_id):
         
        rep = {}
        if request.forms.get("submitted"):
            values = {'id' : inc_id}
            values['set_temp'] = float(request.forms.get("set_temp"))
            values['set_hum'] = int(request.forms.get("set_hum"))
            values['set_light'] = int(request.forms.get("set_light"))
            values['dd_mode'] = int(request.forms.get("dd_mode"))
            values['lights_on'] = int(request.forms.get("lights_on"))
            values['lights_off'] = int(request.forms.get("lights_off"))
            rep['message'] = self._serial_fetcher.update(inc_id, values)
        else:  
            rep['message'] = '' 
             
        rep['incubator_id'] = inc_id 
        return  template('static/graph.tpl', rep)
 
    def _incubator_json(self, inc_id=0):
        """
        """
        if inc_id == 'serial':
 
            serial = list(self._serial_fetcher.getSerialBuffer())
            serial.reverse()
            data = {'result': ''.join(serial) }
            return data
        else:
            return self._serial_fetcher.getlastData(inc_id)
         
    def _get_incubator(self, inc_id, days):
         
        data = self._serial_fetcher.getHistory(inc_id, days)
        return json.dumps(data)
         
    def _listen_to_serial(self, status=True):
        """
        """
        if status:
            self._serial_fetcher.start()
        else:
            self._serial_fetcher.stop()
             
        return {"listener_status" : status}
         
    def _quit(self):
        os._exit(1)
 
def transfer_file_to_db(filename, db_credentials):
     
    with open(filename, "r") as fh:
        for line in fh.readlines()[1:]:
            print line
 
 
if __name__ == '__main__':
    parser = optparse.OptionParser()
 
    parser.add_option("-o", "--output", dest="output", help="Where is the result stored")
    parser.add_option("-p", "--port", dest="port", default=None,help="Serial port to use")
    parser.add_option("-D", "--debug", dest="debug", default=False, help="Shows all logging messages", action="store_true")
    parser.add_option("-w", "--web", dest="web", default=False, help="Starts the webserver", action="store_true")
 
 
    (options, args) = parser.parse_args()
    option_dict = vars(options)
    if(option_dict["debug"]):
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Logger in DEBUG mode")
 
    db_credentials = {'username': 'incubators', 'password' : 'incubators', 'db_name' : 'incubators' }
    db = mySQLDatabase(db_credentials)
 
    if option_dict['output']:
        serial_fetcher = SerialController(option_dict["port"], filename=option_dict['output'])
    else: 
        serial_fetcher = SerialController(option_dict["port"], db_credentials=db_credentials)
 
    if option_dict['web']:
        server = webServer(host='0.0.0.0', port=8090, serial_fetcher=serial_fetcher)
        server.start()
    else:
        serial_fetcher.start()

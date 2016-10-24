__author__ = 'diana'

import optparse
import logging
import threading
import time
from serial_wrapper import SerialWrapper
from mysql_utils import MySQLReader, MySQLWriter
from web_server import WebServer



_DB_CREDENTIALS = {'name': 'incubators', 'user': 'incubators', 'password': 'incubators', 'main_table': 'incubators'}


class webServer(object):
    pass

class SerialThread(threading.Thread):

    def __init__(self, serial_wrapper, mysql_writer):

        super(SerialThread, self).__init__()

        self._serial_wrapper = serial_wrapper
        self._mysql_writer = mysql_writer
        self._stop = False


    def run(self):
        while not self._stop:
            fields = self._serial_wrapper.read_line()
            self._mysql_writer.write(fields)
            time.sleep(1)

    def stop(self):
        self._stop = True
#

#
#
# while True:
# fields = serial_wrapper.read_line()
# my_sql_writer.write(fields)
#




class FakeSerial(object):
    def __init__(self, port, baud):
        self._i = 0


    def readline(self):
        out = "5 D %i 0.3 23.2 54.2 1003 25 60 100 10 12 0" % self._i
        self._i += 1

        #inc_id, command, counter, device_time, temperature, humidity, light, set_temp, set_hum, set_light, lights_on, lights_off, dd_mode
        # import collections
        # out = collections.OrderedDict()
        # out['inc_id'] = 5
        # out['row_type'] = "D"
        # out['counter'] = 1
        # out['device_time'] = 0.3
        # out['server_time'] = 0.4
        # out['temperature'] = 23.2
        # out['humidity'] = 54.2
        # out['light'] = 1003
        # out['set_temp'] = 25
        # out['set_hum'] = 60
        # out['set_light'] = 100
        # out['lights_on'] = 10
        # out['lights_off'] = 12
        # out['dd_mode'] = 0
        return out

    def write(self, line):
        print line


def db_connection(db_credentials):
    import MySQLdb
    db_name = db_credentials["name"]
    db_main_table = db_credentials["main_table"]
    db_user_name = db_credentials["user"]
    db_user_pass = db_credentials["password"]


    db = MySQLdb.connect(   host="localhost",
                            user=db_user_name,
                            passwd=db_user_pass,
                            db=db_name)
    return db


if __name__ == '__main__':
    parser = optparse.OptionParser()

    parser.add_option("-o", "--output", dest="output", help="Where is the result stored")
    #todo default port should be none and debug should be false
    parser.add_option("-p", "--port", dest="port", default="/dev/ttyUSB0",help="Serial port to use")
    parser.add_option("-D", "--debug", dest="debug", default=True, help="Shows all logging messages", action="store_true")
    parser.add_option("-w", "--web", dest="web", default=True, help="Starts the webserver", action="store_true")


    (options, args) = parser.parse_args()
    option_dict = vars(options)
    if option_dict["debug"]:
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Logger in DEBUG mode")


    serial_wrapper = SerialWrapper(port=option_dict["port"], serial_class=FakeSerial)

    db = db_connection(_DB_CREDENTIALS)
    my_sql_reader = MySQLReader(db)
    my_sql_writer = MySQLWriter(db)

    serial_thread = SerialThread(serial_wrapper, my_sql_writer)

    try:
        serial_thread.start()

        if option_dict["web"]:
            server = WebServer(host='0.0.0.0', port=8090,
                           serial_wrapper=serial_wrapper, my_sql_reader=my_sql_reader)
            server.start()
        else:
            while True:
                time.sleep(1)
    finally:
        logging.debug("Stopping thread")
        serial_thread.stop()
        logging.debug("Joining thread")
        serial_thread.join()

    #
    # db_credentials = {'username': 'incubators', 'password' : 'incubators', 'db_name' : 'incubators' }
    #
    # if option_dict['output']:
    #     serial_fetcher = SerialController(option_dict["port"], filename=option_dict['output'])
    # else:
    #     serial_fetcher = SerialController(option_dict["port"], db_credentials=db_credentials)

    # if option_dict['web']:
    #     server = webServer(host='0.0.0.0', port=8090, serial_fetcher=serial_fetcher)
    #     server.start()
    # else:
    #     serial_fetcher.start()
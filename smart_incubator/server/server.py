__author__ = 'diana'

import optparse
import logging
import threading
from serial_wrapper import SerialWrapper
import time

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


class MySQLWriter(object):

    def __init__(self):
        pass

    def write(self, fields):
        print fields
        if fields is None:
            return


class MySQLReader(object):
    def __init__(self):
        pass

    def read(self):
        print "Reading "
        return "read value"

class FakeSerial(object):
    def __init__(self, port, baud):
        pass

    def readline(self):
        return "fields"

    def write(self, line):
        print line


if __name__ == '__main__':
    parser = optparse.OptionParser()

    parser.add_option("-o", "--output", dest="output", help="Where is the result stored")
    #todo default port should be none and debug should be false
    parser.add_option("-p", "--port", dest="port", default="/dev/ttyUSB0",help="Serial port to use")
    parser.add_option("-D", "--debug", dest="debug", default=True, help="Shows all logging messages", action="store_true")
    parser.add_option("-w", "--web", dest="web", default=False, help="Starts the webserver", action="store_true")


    (options, args) = parser.parse_args()
    option_dict = vars(options)
    if option_dict["debug"]:
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Logger in DEBUG mode")


    serial_wrapper = SerialWrapper(port=option_dict["port"], serial_class=FakeSerial)
    my_sql_reader = MySQLReader()
    my_sql_writer = MySQLWriter()

    serial_thread = SerialThread(serial_wrapper, my_sql_writer)

    try:
        serial_thread.start()

        if option_dict["web"]:
            server = webServer(host='0.0.0.0', port=8090,
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
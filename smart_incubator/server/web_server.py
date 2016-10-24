__author__ = 'diana'
import bottle
import json
import datetime
import time
import logging

class WebServer(object):
    # reference hour of day (09:00:00)
    _zt0 = (9,0,0)
    def __init__(self, host, port, serial_wrapper, my_sql_reader):
        self._host = host
        self._port = port
        self._my_sql_reader = my_sql_reader
        self._serial_wrapper = serial_wrapper
        self._app = bottle.Bottle()
        self._route()


    def _route(self):
        self._app.get('/', callback=self._index)

        # self._app.get('/serial', callback=self._serialmonitor)
        # self._app.route('/graph/<inc_id>/<day>', callback=self._get_graph, method=["post", "get"])
        #
        self._app.get('/json/<inc_id>', callback=self._incubator_json)
        self._app.get('/incubator/<inc_id>/<days>', callback=self._get_incubator)
        #
        # self._app.post('/send', callback=self._send_to_serial)
        #
        self._app.get('/static/<filepath:path>', callback=self._get_static)
        # self._app.get('/listen/<status>', callback=self._listen_to_serial)
        # self._app.get('/quit', callback=self._quit)

    def _index(self):
        return bottle.static_file('index.html', root="static")

    def start(self):
        self._app.run(host=self._host, port=self._port)

    def _get_static(self, filepath):
        return bottle.static_file(filepath, root="./static")

    #
    def _incubator_json(self, inc_id=0):
        print "inc_id jsonnnnnnnnnnn", inc_id
    #     """
    #     """
        if inc_id == 'serial':
            #
            # serial = list(self._serial_fetcher.getSerialBuffer())
            # serial.reverse()
            # data = {'result': ''.join(serial)}
            #return data
            return []
        else:
            out =  self._my_sql_reader.get_inc_last_points(inc_id)

            return json.dumps(out)


    def _get_incubator(self, inc_id, days):
        logging.debug("getting data from %s for %s days" % (str(inc_id), str(days)))
        start = datetime.date.today() - datetime.timedelta(days=int(days))

        #end = start + datetime.timedelta(days=1)

        #datetime format
        start = datetime.datetime(start.year, start.month, start.day, *self._zt0)
        start = time.mktime(start.timetuple())
        # #fixme @gg why not simply "now" ?
        #end = datetime.datetime(end.year, end.month, end.day, *self._zt0).strftime('%Y-%m-%d %H:%M:%S')
        end = time.time()

        data = self._my_sql_reader.get_inc_data(inc_id, start=start, end=end)

        return json.dumps(data)
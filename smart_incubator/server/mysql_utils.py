__author__ = 'diana'

import logging

class MySQLWriter(object):


    def __init__(self,  db_connection, db_main_table="incubators"):
        # self._create_mysql_db()
        self._db_main_table = db_main_table
        self._db_connection = db_connection
    #
    # def _get_connection(self):
    #     import MySQLdb
    #     db =   MySQLdb.connect(host="localhost",
    #              user=self._db_user_name, passwd=self._db_user_pass,
    #             db=self._db_name)
    #     return db

    #
    # def _create_mysql_db(self):
    #     import MySQLdb
    #     db = MySQLdb.connect(host="localhost",
    #                          user=self._db_user_name,
    #                          passwd=self._db_user_pass)
    #     c = db.cursor()
    #     try:
    #         cmd = "CREATE DATABASE %s" % self._db_name
    #         c.execute(cmd)
    #     except MySQLdb.ProgrammingError:
    #         logging.debug("could not create database")
    #         pass

        # logging.info("Database created")
        # cmd = "SET GLOBAL innodb_file_per_table=1"
        # c.execute(cmd)
        # cmd = "SET GLOBAL innodb_file_format=Barracuda"
        # c.execute(cmd)
        # cmd = "SET GLOBAL autocommit=0"
        # c.execute(cmd)
        # db.close()

    def _create_table(self, name, fields, engine="InnoDB"):
        db = self._get_connection()
        c = db.cursor()
        cmd = "CREATE TABLE IF NOT EXISTS %s (%s) ENGINE %s KEY_BLOCK_SIZE=16" % (name, fields, engine)

        logging.info("Creating database table with: " + cmd)
        c.execute(cmd)
        db.close()


    def write(self, fields):
        if fields is None:
            return
        # column_names = fields.keys()
        # self._create_table(self._db_main_table, column_names)


        tp = (0, ) + tuple(fields.values())

        cmd = 'INSERT INTO %s VALUES %s' % (self._db_main_table, str(tp))

        logging.debug("Writing in database %s" % cmd)

        c = self._db_connection.cursor()
        c.execute(cmd)
        self._db_connection.commit()



class MySQLReader(object):
    def __init__(self, db_connection):
        self._db_connection = db_connection

    def _get_colnames(self, cursor):
        # command = "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS  WHERE table_name = 'incubators'"
        # cursor.execute(command)
        # result = cursor.fetch_all()
        # out = {}
        # return out
        pass

    def _query(self, select_query):
        c = self._db_connection.cursor()
        #colnames = self._get_colnames(c)
        c.execute(select_query)
        out = []
        for row in c:
            out.append(row)
        return out
    def get_inc_data(self, inc_id, start, end):

        select_query = "SELECT * FROM incubators WHERE inc_id = %s AND server_time BETWEEN '%s' AND '%s'"

        select_query = select_query % (int(inc_id), int(start), int(end))
        self._query(select_query)

    def get_inc_last_points(self, id=None):
        if id is not None:
            select_query = "SELECT * FROM incubators WHERE server_time IN (SELECT MAX(server_time) FROM incubators GROUP BY inc_id) ORDER BY inc_id"
            return self._query(select_query)
        else:
            return []


    def read(self):

        print "Reading "
        return "read value"
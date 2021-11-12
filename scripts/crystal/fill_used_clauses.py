#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

from __future__ import print_function
import sqlite3
import argparse
import os
import struct
import time


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()

        self.cl_used = []
        self.cl_used_num = 0
        self.cl_used_total = 0

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

    def delete_tbls(self, table):
        queries = """
DROP TABLE IF EXISTS `{table}`;
create table `{table}` ( `clauseID` bigint(20) NOT NULL, `used_at` bigint(20) NOT NULL, `weight` float NOT NULL);
""".format(table=table)

        for l in queries.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Runnin query: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Query T: %-3.2f s" % (time.time() - t2))

    def get_last_good(self, basefname):
        last_good = -1
        for i in range(100):
            tfname = "%s-%d" % (basefname, i)
            print("Checking if file %s exists" % tfname)
            if os.path.isfile(tfname):
              last_good = i

        if last_good == -1:
            print("ERROR: file does not exist at all")
            exit(-1)

        return last_good

    def add_used_clauses(self, usedClFname):
        last_good = self.get_last_good(usedClFname)
        tfname = "%s-%d" % (usedClFname, last_good)
        tfname_anc = tfname.replace("usedCls", "usedCls-anc")


        for fname,table in [(tfname, "used_clauses"), (tfname_anc, "used_clauses_anc")]:
            t = time.time()
            print("Adding data from file %s to DB %s" % (fname, table))

            self.cl_used = []
            self.cl_used_num = 0
            self.cl_used_total = 0
            with open(fname, "rb") as f:
                while True:
                    b1 = f.read(8)
                    if not b1:
                        # end of file
                        break
                    b2 = f.read(8)
                    cl_id = struct.unpack("<q", b1)[0]
                    conf = struct.unpack("<q", b2)[0]

                    weight = 1.0
                    if "used_clauses_anc" in table:
                        b3 = f.read(4)
                        weight = struct.unpack("<f", b3)[0]

                    dat = (cl_id, conf, weight)
                    if options.verbose:
                        print("Use:", dat)
                    self.cl_used.append(dat)
                    self.cl_used_num += 1
                    self.cl_used_total += 1
                    if self.cl_used_num > 10000:
                        self.dump_used_clauses(table)

            self.dump_used_clauses(table)
            print("Added use data to table %s from file %s: %d time: T: %-3.2f s" %
                  (table, fname, self.cl_used_total, time.time() - t))

    def dump_used_clauses(self, table):
        self.c.executemany("""
        INSERT INTO %s (
        `clauseID`,
        `used_at`,
        `weight`)
        VALUES (?, ?, ?);""" % table, self.cl_used)
        self.cl_used = []
        self.cl_used_num = 0


if __name__ == "__main__":
    usage = """usage: %(prog)s [options] sqlite_db usedCls

Adds used_clauses to the SQLite database"""

    parser = argparse.ArgumentParser(usage=usage)


    parser.add_argument("sqlitedb", type=str, metavar='USEDCLS')
    parser.add_argument("usedcls", type=str, metavar='USEDCLS')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    options = parser.parse_args()

    if options.sqlitedb is None:
        print("Error you must give an SQLite DB. Please follow usage.")
        print(usage)
        exit(-1)

    if options.usedcls is None:
        print("Error you must give an Used CLS list. Please follow usage.")
        print(usage)
        exit(-1)

    print("Using sqlite3db file %s" % options.sqlitedb)
    print("Base used_clauses file is %s" % options.usedcls)

    with Query(options.sqlitedb) as q:
        q.delete_tbls("used_clauses")
        q.delete_tbls("used_clauses_anc")
        q.add_used_clauses(options.usedcls)

    print("Finished adding good lemma indicators to db %s" % options.sqlitedb)

    exit(0)

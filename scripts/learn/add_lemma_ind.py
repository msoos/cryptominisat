#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2017  Mate Soos
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
import optparse
import os
import struct


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        # zero out goodClauses
        self.c.execute('delete from goodClauses;')
        self.good_ids = []
        self.good_ids_num = 0
        self.good_ids_total = 0

        self.cl_used = []
        self.cl_used_num = 0
        self.cl_used_total = 0

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

    def get_last_good(self, basefname):
        last_good = -1
        for i in range(100):
            tfname = "%s-%d" % (basefname, i)
            print("Checking if file %s exists" % tfname)
            if not os.path.isfile(tfname):
                break
            last_good = i

        if last_good == -1:
            print("ERROR: file does not exist at all")
            exit(-1)

        return last_good

    def add_goodClauses(self, goodClFname):
        last_good = self.get_last_good(goodClFname)
        tfname = "%s-%d" % (goodClFname, last_good)
        print("Using goodClauses file", tfname)
        with open(tfname, "r") as f:
            for line in f:
                line = line.strip().split(" ")
                self.parse_one_line(line)

        # final dump
        self.dump_goodClauses()

        print("Parsed %d number of good lemmas" % self.good_ids_total)

    def parse_one_line(self, line):
        self.good_ids_num += 1
        self.good_ids_total += 1

        # get ID
        myid = int(line[0])
        assert myid >= 0, "ID is always at least 0"
        assert myid != 0, "ID with 0 should not even be printed"

        num_used = int(line[1])
        first_used = int(line[2])
        last_used = int(line[3])
        sum_hist_used = int(line[4])

        # append to good_ids
        self.good_ids.append(
            (myid, num_used, first_used, last_used, sum_hist_used))

        # don't run out of memory, dump early
        if self.good_ids_num > 10000:
            self.dump_goodClauses()

    def delete_goodClauses(self):
        q = """
        delete from goodClauses;
        """
        self.c.execute(q)
        print("Deleted data from goodClauses")

    def delete_usedClauses(self):
        q = """
        delete from usedClauses;
        """
        self.c.execute(q)
        print("Deleted data from usedClauses")

    def dump_goodClauses(self):
        self.c.executemany("""
        INSERT INTO goodClauses (
        `clauseID`
        , `num_used`
        , `first_confl_used`
        , `last_confl_used`
        , `sum_hist_used`)
        VALUES (?, ?, ?, ?, ?);""", self.good_ids)
        self.good_ids = []
        self.good_ids_num = 0

    def vacuum(self):
        q = """
        vacuum;
        """

        lev = self.conn.isolation_level
        self.conn.isolation_level = None
        self.c.execute(q)
        self.conn.isolation_level = lev
        print("Vacuumed database")

    def add_usedClauses(self, usedClFname):
        last_good = self.get_last_good(usedClFname)
        tfname = "%s-%d" % (usedClFname, last_good)
        print("Using usedClauses file", tfname)

        self.cl_used = []
        self.cl_used_num = 0
        self.cl_used_total = 0
        with open(tfname, "rb") as f:
            while True:
                b1 = f.read(8)
                if not b1:
                    break
                b2 = f.read(8)
                cl_id = struct.unpack("<q", b1)[0]
                conf = struct.unpack("<q", b2)[0]
                self.cl_used.append((cl_id, conf))
                self.cl_used_num += 1
                self.cl_used_total += 1
                if self.cl_used_num > 10000:
                    self.dump_usedClauses()

        self.dump_usedClauses()
        print("Added used data:", self.cl_used_total)

    def dump_usedClauses(self):
        self.c.executemany("""
        INSERT INTO usedClauses (
        `clauseID`
        , `used_at`)
        VALUES (?, ?);""", self.cl_used)
        self.cl_used = []
        self.cl_used_num = 0


if __name__ == "__main__":
    usage = """usage: %prog [options] sqlite_db goodClFname usedClFname

It adds goodClauses and usedClauses to the SQLite database"""

    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) != 3:
        print("Error. Please follow usage")
        print(usage)
        exit(-1)

    dbfname = args[0]
    goodClFname = args[1]
    usedClFname = args[2]
    print("Using sqlite3db file %s" % dbfname)
    print("Base goodClauses file is %s" % goodClFname)
    print("Base usedClauses file is %s" % usedClFname)

    with Query(dbfname) as q:
        q.delete_goodClauses()
        q.add_goodClauses(goodClFname)

        q.delete_usedClauses()
        q.add_usedClauses(usedClFname)

        q.vacuum()

    print("Finished adding good lemma indicators to db %s" % dbfname)

    exit(0)

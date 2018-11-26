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


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        # zero out goodClauses
        self.c.execute('delete from goodClauses;')
        self.cur_good_ids_num = 0
        self.num_goods_total = 0
        self.cur_good_ids = []

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

    def parse_and_add_lemmas(self, lemmafname):
        last_good = 0
        for i in range(100):
            tfname = "%s-%d" % (lemmafname, i)
            print("Checking if lemma file %s exists" % tfname)
            if not os.path.isfile(tfname):
                break

            print("Checking if lemma file %s 'Finished'" % tfname)
            with open(tfname, "r") as f:
                for line in f:
                    if "Finished" in line:
                        last_good = i

        tfname = "%s-%d" % (lemmafname, last_good)
        print("Using lemma file", tfname)
        with open(tfname, "r") as f:
            for line in f:
                line = line.strip().split(" ")
                self.parse_one_line(line)

        # final dump
        self.dump_ids()

        print("Parsed %d number of good lemmas" % self.num_goods_total)

    def parse_one_line(self, line):
        self.cur_good_ids_num += 1
        self.num_goods_total += 1

        # get ID
        myid = int(line[0])
        assert myid >= 0, "ID is always at least 0"
        assert myid != 0, "ID with 0 should not even be printed"

        num_used = int(line[1])
        first_used = int(line[2])
        last_used = int(line[3])
        sum_hist_used = int(line[4])

        # append to cur_good_ids
        self.cur_good_ids.append(
            (myid, num_used, first_used, last_used, sum_hist_used))

        # don't run out of memory, dump early
        if self.cur_good_ids_num > 10000:
            self.dump_ids()

    def dump_ids(self):
        self.c.executemany("""
        INSERT INTO goodClauses (
        `clauseID`
        , `num_used`
        , `first_confl_used`
        , `last_confl_used`
        , `sum_hist_used`)
        VALUES (?, ?, ?, ?, ?);""", self.cur_good_ids)
        self.cur_good_ids = []
        self.cur_good_ids_num = 0

    def vacuum(self):
        q = """
        vacuum;
        """

        lev = self.conn.isolation_level
        self.conn.isolation_level = None
        self.c.execute(q)
        self.conn.isolation_level = lev
        print("Vacuumed database")


if __name__ == "__main__":
    usage = """usage: %prog [options] sqlite_db lemmas

It adds lemma indices from "lemmas" to the SQLite database, indicating whether
it was good or not."""

    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) != 2:
        print("Error. Please follow usage")
        print(usage)
        exit(-1)

    dbfname = args[0]
    lemmafname = args[1]
    print("Using sqlite3db file %s" % dbfname)
    print("Base lemma file is %s" % lemmafname)

    with Query(dbfname) as q:
        q.parse_and_add_lemmas(lemmafname)
        q.vacuum()

    print("Finished adding good lemma indicators to db %s" % dbfname)

    exit(0)

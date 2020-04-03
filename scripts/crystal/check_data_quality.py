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
import time
import os.path
import helper


class Queries (helper.QueryHelper):
    def __init__(self, dbfname):
        super(Queries, self).__init__(dbfname)

    def create_indexes(self):
        helper.drop_idxs(self.c)

        print("Creating needed indexes...")
        t = time.time()
        q = """
        create index `idxclid-del` on `cl_last_in_solver` (`clauseID`, `conflicts`);
        create index `idxclid-del2` on `used_clauses` (`clauseID`);
        create index `idxclid-del3` on `used_clauses` (`clauseID`, `used_at`);
        create index `idxclid1-2` on `clause_stats` (`clauseID`);
        """

        for l in q.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Creating index: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Index creation T: %-3.2f s" % (time.time() - t2))

        print("indexes created T: %-3.2f s" % (time.time() - t))


    def check_incorrect_data_values(self):
        incorrect = [
            {"table":"clause_stats", "cond":" orig_glue = 0 and orig_size >= 2"},
            {"table":"clause_stats", "cond":" orig_glue = 1 and orig_size >= 2"},
            {"table":"clause_stats", "cond":" orig_glue > 50"},
            {"table":"reduceDB", "cond":" glue > 50"},
            {"table":"clause_stats", "cond":" glue_before_minim = 0 and orig_size >= 2"},
            {"table":"clause_stats", "cond":" glue_before_minim = 1 and orig_size >= 2"},
            {"table":"reduceDB", "cond":" act_ranking > tot_cls_in_db"},
            {"table":"reduceDB", "cond":" act_ranking < 0"},
        ]
        for incorr in incorrect:
            q = """select * from `{table}` where {cond} """.format(**incorr)
            cursor = self.c.execute(q)
            for row in cursor:
                print("ERROR: Following data in `{table}` has {cond}: ".format(**incorr))
                print(row)
                exit(-1)
            print("Checked for %s" % q)


    def check_at_least_n(self):
        checks = [
            {"table":"clause_stats", "cond":" orig_glue >= 2", "n": 100},
            {"table":"reduceDB", "cond":" dump_no == 0", "n": 10},
            {"table":"reduceDB", "cond":" dump_no == 1", "n": 10}
        ]
        for check in checks:
            q = """select count() from `{table}` where {cond}""".format(**check)
            cursor = self.c.execute(q)
            for row in cursor:
                num = int(row[0])
                print("Number of {cond} in {table}: {num}".format(num=num, **check))
                if num < check["n"]:
                    print("ERROR: That's too few")
                    exit(-1)

            print("Checked for %s" % q)


    def check_non_negative(self):
        table = "reduceDB"
        cols = helper.get_columns(table, options.verbose, self.c)
        for col in cols:
            t = time.time()
            q = """
            select * from `%s` where `%s` < 0
            """ % (table, col)
            cursor = self.c.execute(q)
            for row in cursor:
                print("ERROR: following data has %s < 0 in table %s: " % (col , table))
                print(row)
                exit(-1)
            print("Checked for %s < 0 in table %s. All are >= 0. T: %-3.2f s" %
                  (col, table, time.time() - t))


    def check_positive(self):
        check_zero = [
            ["glue_before_minim", "clause_stats"],
            ["orig_glue", "clause_stats"],
            ["orig_size", "clause_stats"],
            ["size", "reduceDB"],
            ["glue", "reduceDB"],
            ["act_ranking", "reduceDB"], #act ranking starts at 1, not 0
            ["act_ranking_top_10", "reduceDB"]
            ]

        for col,table in check_zero:
            t = time.time()
            q = """
            select * from `%s` where `%s` <= 0
            """ % (table, col)
            cursor = self.c.execute(q)
            for row in cursor:
                print("ERROR: Following data from table %s has %s as non-positive: " % (col, table))
                print(row)
                exit(-1)
            print("Checked for %s in table %s. All are positive T: %-3.2f s" %
                  (col, table, time.time() - t))

    def drop_idxs(self):
        helper.drop_idxs(self.c)

if __name__ == "__main__":
    usage = "usage: %prog [options] sqlitedb"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the sqlite file!")
        exit(-1)

    with Queries(args[0]) as q:
        #q.create_indexes()
        q.check_non_negative()
        q.check_positive()
        q.check_incorrect_data_values()
        q.check_at_least_n()
        #q.drop_idxs()

    print("Done.")

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


cl_to_conflict = {}
new_id_to_old_id = {}
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
""".format(table=table)

        for l in queries.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Running query: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Query T: %-3.2f s" % (time.time() - t2))

    def get_conflicts(self):
        query= """select id, conflicts from set_id_confl;"""
        self.c.execute(query)
        while True:
            n = self.c.fetchone()
            if n is None:
                break

            ID = n[0]
            confl = n[1]
            if ID in cl_to_conflict:
                print("ERROR: ID %d in cl_to_conflict already. Old value: %d new value: %d" % (ID, cl_to_conflict[ID], confl))
                exit(-1)
            cl_to_conflict[ID] = confl

    def get_updates(self):
        query= """select old_id, new_id from update_id order by old_id;"""
        self.c.execute(query)
        while True:
            n = self.c.fetchone()
            if n is None:
                break

            old_id = n[0]
            new_id = n[1]
            if old_id in new_id_to_old_id:
                real_old_id = new_id_to_old_id[old_id][0]
            else:
                real_old_id = old_id

            new_id_to_old_id[new_id] = [real_old_id, 1.0]

def fix_up_frat(fratfile):
    with open(fratfile, "r") as f:
        for line in f:
            line=line.strip()
            if len(line) == 0:
                continue
            if line[0] != "a":
                continue

            print("-------------****----------------------")
            print("New line: %s" % line)
            line = line.split()
            if len(line) < 3:
                print("ERROR: Line contains 1 or 2 elements??? It needs a/o/d/l/t + at least ID")
                exit(-1)

            ID = int(line[1])
            line = line[2:]
            tracked_already = False

            if ID not in new_id_to_old_id:
                print("ID %8d is not tracked" % ID)
            else:
                print("ID %8d is tracked, it is: %s" % (ID, new_id_to_old_id[ID]))
                tracked_already = True

            if ID not in cl_to_conflict:
                print("ERROR: ID %8d not in cl_to_conflict" % ID)
                exit(-1)

            print("%d is generated at confl %d" % (ID, cl_to_conflict[ID]))

            found = False
            cl_len = 0
            for i in range(len(line)):
                if line[i].strip() == "l":
                    cl_len = i-1
                    line = line[i+1:]
                    found = True
                    break

            if not found:
                print("No explanation on line.")
                continue

            for cl in line:
                cl = cl.strip()
                if cl == "0":
                    break
                chain_ID = int(cl)
                print("Chain %8d for ID %8d, cl_len: %8d" % (chain_ID, ID, cl_len))
                if chain_ID in new_id_to_old_id:
                    print("--> tracked as %s" % new_id_to_old_id[chain_ID])

                    if tracked_already:
                        print("-----> Can't track extra, already tracked by one :S")
                    else:
                        chain_ID_upd = new_id_to_old_id[chain_ID][0]
                        val = new_id_to_old_id[chain_ID][1]
                        val *= 0.5
                        print("-----> Therefore, we will track ID %d with val %f to count as ID %d" % (ID, val, chain_ID_upd))
                        new_id_to_old_id[ID] = [chain_ID_upd, val]
                        tracked_already = True







if __name__ == "__main__":
    usage = """usage: %(prog)s [options] sqlite_db usedCls

Adds used_clauses to the SQLite database"""

    parser = argparse.ArgumentParser(usage=usage)


    parser.add_argument("fratfile", type=str, metavar='FRAT')
    parser.add_argument("sqlitedb", type=str, metavar='SQLITEDB')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    options = parser.parse_args()

    if options.sqlitedb is None:
        print("Error you must give an SQLite DB. Please follow usage.")
        print(usage)
        exit(-1)

    if options.fratfile is None:
        print("Error you must give the minimized FRAT file.")
        print(usage)
        exit(-1)

    print("Using FRAT file %s" % options.fratfile)
    print("Using sqlite3db file %s" % options.sqlitedb)

    with Query(options.sqlitedb) as q:
        q.delete_tbls("used_clauses")
        q.get_conflicts()
        q.get_updates()

    fix_up_frat(options.fratfile)

    exit(0)

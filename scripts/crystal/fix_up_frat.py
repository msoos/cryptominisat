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
#import line_profiler

cl_to_conflict = {}
new_id_to_old_id = {}
class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.cl_used = []
        self.cl_used_num = 0
        self.cl_used_total = 0
        self.children_set = 0

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

            if opts.verbose:
                print("Running query: ", l)
            self.c.execute(l)
            if opts.verbose:
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
                print("Likely, the issue is with `reloc` in FRAT, which is used in varreplacer. That is NOT understood by this system");
                exit(-1)
            cl_to_conflict[ID] = confl

        if opts.verbose:
            print("Got ID-conflict data")

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

            new_id_to_old_id[new_id] = [real_old_id, 1.0, None]

        if opts.verbose:
            print("Got ID-update data")

    def dump_used_clauses(self):
        for table in ("used_clauses_anc", "used_clauses"):
            if table == "used_clauses":
                todump = filter(lambda c: c[1] == 1.0, self.cl_used)
            else:
                todump = self.cl_used

            self.c.executemany("""
            INSERT INTO %s (
            `clauseID`,
            `weight`,
            `used_at`)
            VALUES (?, ?, ?);""" % table, todump)

        self.cl_used = []
        self.cl_used_num = 0

    #@profile
    def deal_with_chain(self, line, final_resolvent_ID, tracked_already, confl):
        for chain_str in line:
            if chain_str[0] == '0':
                break
            chain_ID = int(chain_str)
            if opts.verbose:
                print("Cl ID %8d used for Cl ID %8d" % (chain_ID, final_resolvent_ID))
            if chain_ID in new_id_to_old_id:
                if opts.verbose:
                    print("--> tracked as %s" % new_id_to_old_id[chain_ID])
                data = new_id_to_old_id[chain_ID]
                if data[2] == None:
                    # this is an original parent
                    count_as_confl = confl
                    assert data[1] == 1.0
                else:
                    # this is a child
                    count_as_confl = data[2]
                    assert data[1] < 1.0

                new_item = [data[0], data[1], count_as_confl]
                self.cl_used.append(new_item)
                self.cl_used_num+=1
                self.cl_used_total+=1
                if opts.verbose:
                    print("--> USED: %s" % new_item)
                if self.cl_used_num > 10000:
                    self.dump_used_clauses()

                # This chain_ID is tracked. Let's track Clause "final_resolvent_ID", it will be a child
                # NOTE: we only track it as ONE of the children... which is not ideal.
                if tracked_already:
                    if opts.verbose:
                        print("-----> Can't track Cl ID %d as child, already tracked either as MAIN or as a child" % final_resolvent_ID)
                    continue

                # final_resolvent_ID is not tracked already. We'll track it according to its ancestor
                data = new_id_to_old_id[chain_ID]
                chain_ID_upd = data[0]
                val = 0.5*data[1]
                if data[2] is not None:
                    # This is a child of a child, so parent's conf needs to be bumped
                    anc_confl = data[2]
                else:
                    # Children of this will need confl to be bumped
                    anc_confl = confl

                # Don't bother if it's less than 0.05
                if val <= 0.05:
                    continue

                if opts.verbose:
                    print("-----> Therefore, we will track ID %d with val %f to count as ID %d, confl %d" % (final_resolvent_ID, val, chain_ID_upd, anc_confl))
                new_id_to_old_id[final_resolvent_ID] = [chain_ID_upd, val, anc_confl]
                tracked_already = True
                self.children_set += 1

    #@profile
    def fix_up_frat(self, fratfile):
        with open(fratfile, "r") as f:
            for line in f:
                line=line.strip()
                if len(line) == 0:
                    continue
                if line[0] != "a":
                    continue

                if opts.verbose:
                    print("-------------****----------------------")
                    print("New line: %s" % line)
                line = line.split()
                if len(line) < 3:
                    print("ERROR: Line contains 1 or 2 elements??? It needs a/o/d/l/t + at least ID")
                    exit(-1)

                final_resolvent_ID = int(line[1])
                line = line[2:]
                tracked_already = False

                if final_resolvent_ID not in new_id_to_old_id:
                    if opts.verbose:
                        print("MAIN: Cl ID %8d is not tracked" % final_resolvent_ID)
                else:
                    if opts.verbose:
                        print("MAIN: Cl ID %8d is tracked, it is: %s" % (final_resolvent_ID, new_id_to_old_id[final_resolvent_ID]))
                    tracked_already = True

                if final_resolvent_ID not in cl_to_conflict:
                    print("ERROR: ID %8d not in cl_to_conflict" % final_resolvent_ID)
                    exit(-1)

                confl = cl_to_conflict[final_resolvent_ID]
                if opts.verbose:
                    print("MAIN: Cl ID %8d is generated at confl %d" % (final_resolvent_ID, confl))

                found = False
                for i in range(len(line)):
                    if line[i] == "l":
                        line = line[i+1:]
                        found = True
                        break

                if not found:
                    #if opts.verbose:
                    print("No explanation on line %s for ID %d" % (line, final_resolvent_ID))
                    continue

                self.deal_with_chain(line, final_resolvent_ID, tracked_already, confl)


if __name__ == "__main__":
    usage = """usage: %(prog)s [opts] sqlite_db usedCls

Adds used_clauses to the SQLite database"""

    parser = argparse.ArgumentParser(usage=usage)


    parser.add_argument("fratfile", type=str, metavar='FRAT')
    parser.add_argument("sqlitedb", type=str, metavar='SQLITEDB')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    opts = parser.parse_args()

    if opts.sqlitedb is None:
        print("Error you must give an SQLite DB. Please follow usage.")
        print(usage)
        exit(-1)

    if opts.fratfile is None:
        print("Error you must give the minimized FRAT file.")
        print(usage)
        exit(-1)

    print("Using FRAT file %s" % opts.fratfile)
    print("Using sqlite3db file %s" % opts.sqlitedb)

    with Query(opts.sqlitedb) as q:
        q.delete_tbls("used_clauses")
        q.delete_tbls("used_clauses_anc")
        q.get_conflicts()
        q.get_updates()
        q.fix_up_frat(opts.fratfile)
        # dump remaining ones (we dump 1000-by-1000)
        q.dump_used_clauses()

        print("Total num:    %10d" % q.cl_used_total)
        print("Children set: %10d" % q.children_set)

    exit(0)

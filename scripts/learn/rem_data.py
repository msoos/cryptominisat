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
import pickle
import re
import pandas as pd
import numpy as np
import os.path

class QueryHelper:
    def __init__(self, dbfname):
        if not os.path.isfile(dbfname):
            print("ERROR: Database file '%s' does not exist" % dbfname)
            exit(-1)

        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

class QueryDatRem(QueryHelper):
    def __init__(self, dbfname):
        super(QueryDatRem, self).__init__(dbfname)

    def create_used_ID_table(self):
        q = """
        DROP TABLE IF EXISTS `usedClauseIDs`;
        """
        self.c.execute(q)

        q = """
        CREATE TABLE `usedClauseIDs` (
          `clauseID` int(20) NOT NULL
        );
        """
        self.c.execute(q)

        print("Recreated usedClauseIDs table")

        queries = """
        drop index if exists `idxclid30`;
        drop index if exists `idxclid31`;
        drop index if exists `idxclid32`;
        drop index if exists `idxclid33`;
        drop index if exists `idxclid34`;

        create index `idxclid30` on `usedClauseIDs` (`clauseID`);
        create index `idxclid31` on `clauseStats` (`clauseID`);
        create index `idxclid32` on `reduceDB` (`clauseID`);
        create index `idxclid33` on `goodClauses` (`clauseID`);
        create index `idxclid34` on `usedClauses` (`clauseID`);
        """

        for q in queries.split("\n"):
            self.c.execute(q)

        print("Recreated indexes needed")

    def fill_used_cl_ids_table_cheat(self):
        t = time.time()
        val = int(options.limit)
        q = """
        insert into usedClauseIDs
        select
        clauseID from goodClauses order by random() limit %d;
        """ % val
        self.c.execute(q)

        ret = self.c.execute("select count() from usedClauseIDs")
        rows = self.c.fetchall()
        good_ids = rows[0][0]
        print("IDs from goodClauses: {ids} T: {time}".format(
            ids=good_ids, time=time.time()-t))

        val = int(options.limit)*0.5
        q = """
        insert into usedClauseIDs
        select
        clauseStats.clauseID
        from clauseStats left join goodClauses
        on clauseStats.clauseID = goodClauses.clauseID
        where goodClauses.clauseID is NULL
        order by random() limit %d;
        """ % val
        self.c.execute(q)

        ret = self.c.execute("select count() from usedClauseIDs")
        rows = self.c.fetchall()
        all_ids = rows[0][0]
        print("IDs from clauseStats that are not in good: %d" % (all_ids-good_ids))

    def fill_used_cl_ids_table_full(self):
        val = int(options.limit)
        q = """
        insert into usedClauseIDs
        select
        clauseStats.clauseID
        from clauseStats
        order by random() limit %d;
        """ % val
        self.c.execute(q)

    def filter_tables(self):
        tables = ["clauseStats", "reduceDB", "goodClauses", "usedClauses"]
        q = """
        DELETE FROM {table} WHERE clauseID NOT IN
        (SELECT clauseID from usedClauseIDs );"""

        for table in tables:
            t = time.time()
            self.c.execute(q.format(table=table))
            print("Filtered table {table} T: {time}".format(
                table=table, time=time.time()-t))

    def vacuum(self):
        queries = """
        drop index if exists `idxclid30`;
        drop index if exists `idxclid31`;
        drop index if exists `idxclid32`;
        drop index if exists `idxclid33`;
        drop index if exists `idxclid34`;
        """
        for q in queries.split("\n"):
            self.c.execute(q)

        print("Deleted indexes")

        q = """
        vacuum;
        """

        lev = self.conn.isolation_level
        self.conn.isolation_level = None
        self.c.execute(q)
        self.conn.isolation_level = lev
        print("Vacuumed database")


if __name__ == "__main__":
    usage = "usage: %prog [options] sqlitedb"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--limit", default=40000, type=int,
                      dest="limit", help="Number of clauses to limit ourselves to")
    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the sqlite file!")
        exit(-1)


    with QueryDatRem(args[0]) as q:
        q.create_used_ID_table()
        q.fill_used_cl_ids_table_cheat()
        q.filter_tables()
        q.vacuum()

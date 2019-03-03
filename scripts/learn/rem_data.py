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

    def dangerous(self):
        self.c.execute("PRAGMA journal_mode = MEMORY")
        self.c.execute("PRAGMA synchronous = OFF")
        pass

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

    def create_indexes(self):
        print("Recreating indexes...")
        t = time.time()
        q = """
        drop index if exists `idxclid1`;
        drop index if exists `idxclid1-2`;
        drop index if exists `idxclid1-3`;
        drop index if exists `idxclid1-4`;
        drop index if exists `idxclid1-5`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;
        drop index if exists `idxclid4`;
        drop index if exists `idxclid5`;
        drop index if exists `idxclid6`;
        drop index if exists `idxclid6-2`;
        drop index if exists `idxclid6-3`;
        drop index if exists `idxclid6-4`;
        drop index if exists `idxclid7`;
        drop index if exists `idxclid8`;
        drop index if exists `idxclidUCLS-1`;
        drop index if exists `idxclidUCLS-2`;

        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        create index `idxclidUCLS-1` on `usedClauses` ( `clauseID`, `used_at`);
        create index `idxclidUCLS-2` on `usedClauses` ( `used_at`);
        """
        for l in q.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Creating index: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Index creation T: %-3.2f s" % (time.time() - t2))

        print("indexes created T: %-3.2f s" % (time.time() - t))

    def fill_later_useful_data(self):
        t = time.time()
        q = """
        DROP TABLE IF EXISTS `used_later`;
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later dropped T: %-3.2f s" % (time.time() - t))

        q = """
        create table `used_later` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later` bigint(20)
        );"""
        self.c.execute(q)
        print("used_later recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""insert into used_later
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl.used_at) as `useful_later`
        FROM
        reduceDB as rdb0
        left join usedClauses as ucl

        -- for any point later than now
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > rdb0.conflicts)

        WHERE
        rdb0.clauseID != 0

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later filled T: %-3.2f s" % (time.time() - t))


        t = time.time()
        q = """
        drop index if exists `used_later_idx1`;
        drop index if exists `used_later_idx2`;

        create index `used_later_idx1` on `used_later` (`clauseID`, rdb0conflicts);
        create index `used_later_idx2` on `used_later` (`clauseID`, rdb0conflicts, used_later);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later indexes added T: %-3.2f s" % (time.time() - t))

    def delete_too_many_rdb_rows(self):
        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Have %d lines of RDB" % (rdb_rows))

        if rdb_rows <= options.goal_rdb:
            print("Num RDB rows: %d which is OK already, goal is %d" %
                  (rdb_rows, options.goal_rdb));
            return

        ratio_needed = float(options.goal_rdb)/float(rdb_rows)*100.0
        q = """DELETE FROM reduceDB WHERE (abs(random()) %% 100) > %d""" % int(ratio_needed)
        self.c.execute(q)
        print("Kept only %.3f %% of RDB: T: %.3f" % (ratio_needed, time.time()-t))

        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Only %d RDB rows remain" % rdb_rows)

    def delete_too_many_rdb_rows_cheat(self):
        ret = self.c.execute("drop index if exists `idxclid32`;")

        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Have %d lines of RDB" % (rdb_rows))

        q = """
        drop table if exists stuff;
        """
        self.c.execute(q)

        t = time.time()
        q = """create table stuff (
            id bigint(20) not null
        );"""
        self.c.execute(q)
        print("Created stuff T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        insert into stuff (id)
        select
        rdb0.rowid
        from reduceDB as rdb0, used_later
        where
        used_later.clauseID=rdb0.clauseID
        and used_later.rdb0conflicts=rdb0.conflicts
        and used_later.used_later > 0
        order by random()
        limit %d""" % options.goal_rdb
        self.c.execute(q)
        print("Insert good to stuff T: %-3.2f s" % (time.time() - t))

        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from stuff")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("We now have %d lines stuff" % (rdb_rows))

        t = time.time()
        q = """
        insert into stuff (id)
        select
        rdb0.rowid
        from reduceDB as rdb0
        order by random()
        limit %d""" % options.goal_rdb
        self.c.execute(q)
        print("Insert random to stuff T: %-3.2f s" % (time.time() - t))

        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from stuff")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("We now have %d lines stuff" % (rdb_rows))


        t = time.time()
        q = """
        drop index if exists `idx_bbb`;
        drop index if exists `idxclid6-4`; -- the other index on reduceDB

        create index `idx_bbb` on `stuff` (`id`);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later indexes added T: %-3.2f s" % (time.time() - t))

        q = """
        delete from reduceDB
        where reduceDB.rowid not in (select id from stuff)
        """
        self.c.execute(q)
        print("Delete from reduceDB T: %-3.2f s" % (time.time() - t))

        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Finally have %d lines of RDB" % (rdb_rows))

    # inserts less than 1-1 ratio, inserting only 0.3*N from unused ones
    def fill_used_cl_ids_table_cheat(self):

        t = time.time()
        val = int(options.limit)
        q = """
        insert into usedClauseIDs
        select
        clauseID from goodClauses
        where num_used > 20
        order by random() limit %d;
        """ % int(val/4)
        self.c.execute(q)
        print("Added >20 from goodClauses T: %-3.2f s" % (time.time() - t))

        t = time.time()
        val = int(options.limit)
        q = """
        insert into usedClauseIDs
        select
        clauseID from goodClauses
        where num_used > 5
        order by random() limit %d;
        """ % int(val/2)
        self.c.execute(q)
        print("Added >5 from goodClauses T: %-3.2f s" % (time.time() - t))

        t = time.time()
        val = int(options.limit)
        q = """
        insert into usedClauseIDs
        select
        clauseID from goodClauses
        order by random() limit %d;
        """ % val
        self.c.execute(q)
        print("Added any from goodClauses T: %-3.2f s" % (time.time() - t))

        ret = self.c.execute("select count() from usedClauseIDs")
        rows = self.c.fetchall()
        good_ids = rows[0][0]
        print("IDs from goodClauses: %d   T: %-3.2f s" % (good_ids, time.time() - t))

        t = time.time()
        val = int(options.limit)
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
        print("IDs from clauseStats that are not in good: %d  T: %-3.2f s" % ((all_ids-good_ids), time.time() - t))

    def filter_tables_of_ids(self):
        tables = ["clauseStats", "reduceDB", "goodClauses", "usedClauses"]
        q = """
        DELETE FROM {table} WHERE clauseID NOT IN
        (SELECT clauseID from usedClauseIDs );"""

        for table in tables:
            t = time.time()
            self.c.execute(q.format(table=table))
            print("Filtered table '%s' T: %-3.2f s" % (table, time.time() - t))

    def vacuum(self):
        t = time.time()
        queries = """
        drop index if exists `idx_aaa`;
        drop index if exists `idx_bbb`;
        drop index if exists `idxclid30`;
        drop index if exists `idxclid31`;
        drop index if exists `idxclid32`;
        drop index if exists `idxclid33`;
        drop index if exists `idxclid34`;
        drop index if exists `idxclid1`;
        drop index if exists `idxclid1-2`;
        drop index if exists `idxclid1-3`;
        drop index if exists `idxclid1-4`;
        drop index if exists `idxclid1-5`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;
        drop index if exists `idxclid4`;
        drop index if exists `idxclid5`;
        drop index if exists `idxclid6`;
        drop index if exists `idxclid6-2`;
        drop index if exists `idxclid6-3`;
        drop index if exists `idxclid6-4`;
        drop index if exists `idxclid7`;
        drop index if exists `idxclid8`;
        drop index if exists `idxclidUCLS-1`;
        drop index if exists `idxclidUCLS-2`;
        drop index if exists `idxclid20`;
        drop index if exists `idxclid21`;
        drop index if exists `idxclid21-2`;
        drop index if exists `idxclid22`;
        drop index if exists `used_later_idx1`;
        drop index if exists `used_later_idx2`;
        drop index if exists `idxclid6-4`;
        drop index if exists `idxclid1`;
        drop index if exists `idxclid1-2`;
        drop index if exists `idxclid1-3`;
        drop index if exists `idxclid1-4`;
        drop index if exists `idxclid1-5`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;
        drop index if exists `idxclid4`;
        drop index if exists `idxclid5`;
        drop index if exists `idxclid6`;
        drop index if exists `idxclid6-2`;
        drop index if exists `idxclid6-3`;
        drop index if exists `idxclid6-4`;
        drop index if exists `idxclid7`;
        drop index if exists `idxclid8`;
        drop index if exists `idxclidUCLS-1`;
        drop index if exists `idxclidUCLS-2`;
        drop index if exists `idxclid20`;
        drop index if exists `idxclid21`;
        drop index if exists `idxclid21-2`;
        drop index if exists `idxclid22`;

        DROP TABLE IF EXISTS `used_later`;
        DROP TABLE IF EXISTS `used_later10k`;
        DROP TABLE IF EXISTS `used_later100k`;
        DROP TABLE IF EXISTS `usedlater`;
        DROP TABLE IF EXISTS `usedlater10k`;
        DROP TABLE IF EXISTS `usedlater100k`;

        DROP TABLE IF EXISTS `used_later`;
        DROP TABLE IF EXISTS `usedlater`;
        DROP TABLE IF EXISTS `goodClausesFixed`;
        DROP TABLE IF EXISTS `stuff`;
        DROP TABLE IF EXISTS `usedClauseIDs`;
        """
        for q in queries.split("\n"):
            self.c.execute(q)
        print("Deleted indexes and misc tables T: %-3.2f s" % (time.time() - t))

        q = """
        vacuum;
        """

        t = time.time()
        lev = self.conn.isolation_level
        self.conn.isolation_level = None
        self.c.execute(q)
        self.conn.isolation_level = lev
        print("Vacuumed database T: %-3.2f s" % (time.time() - t))


if __name__ == "__main__":
    usage = "usage: %prog [options] sqlitedb"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--limit", default=20000, type=int,
                      dest="limit", help="Number of clauses to limit ourselves to")
    parser.add_option("--goalrdb", default=200000, type=int,
                      dest="goal_rdb", help="Number of RDB neeeded")
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the sqlite file!")
        exit(-1)


    with QueryDatRem(args[0]) as q:
        q.dangerous()
        q.vacuum()

        q.dangerous()
        q.create_indexes()
        q.create_used_ID_table()
        q.fill_used_cl_ids_table_cheat()
        q.filter_tables_of_ids()
        q.vacuum()
        q.dangerous()

        q.create_indexes()
        q.fill_later_useful_data()
        q.delete_too_many_rdb_rows_cheat();

        q.vacuum()

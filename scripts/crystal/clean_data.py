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


class QueryFill (QueryHelper):
    def __init__(self, dbfname):
        super(QueryFill, self).__init__(dbfname)

    def create_indexes(self):
        print("Getting indexes to drop")
        q = """
        SELECT name FROM sqlite_master WHERE type == 'index'
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        q = ""
        for row in rows:
            print("Will delete index:", row[0])
            q += "drop index if exists `%s`;\n" % row[0]

        print("Creating needed indexes...")
        t = time.time()
        q += """
        create index `idxclid3` on `goodClauses` (`clauseID`);
        create index `idxclid1-2` on `clauseStats` (`clauseID`, `conflicts`);
        create index `idxclidM` on `reduceDB` (`clauseID`, `propagations_made`);
        """

        for l in q.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Creating index: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Index creation T: %-3.2f s" % (time.time() - t2))

        print("indexes created T: %-3.2f s" % (time.time() - t))

    def fill_last_prop(self):
        print("Adding last prop...")
        t = time.time()
        q = """
        update goodClauses
        set last_prop_used =
        (select max(conflicts)
            from reduceDB
            where reduceDB.clauseID = goodClauses.clauseID
                and reduceDB.propagations_made > 0
        );
        """
        self.c.execute(q)
        print("last_prop_used filled T: %-3.2f s" % (time.time() - t))

    def fill_sum_cl_use(self):
        print("Filling sum_cl_use...")

        t = time.time()
        q = """DROP TABLE IF EXISTS `sum_cl_use`;"""
        self.c.execute(q)
        q = """
        create table `sum_cl_use` (
            `clauseID` bigint(20) NOT NULL,
            `num_used` bigint(20) NOT NULL,
            `first_confl_used` bigint(20),
            `last_confl_used` bigint(20),
            `sum_hist_used` bigint(20) DEFAULT NULL,
            `avg_hist_used` double,
            `var_hist_used` double,
            `last_prop_used` bigint(20) DEFAULT NULL
        );"""
        self.c.execute(q)
        print("sum_cl_use recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """insert into sum_cl_use
        (
        `clauseID`,
        `num_used`,
        `first_confl_used`,
        `last_confl_used`,
        `sum_hist_used`,
        `avg_hist_used`,
        `last_prop_used`
        )
        select
        clauseID
        , sum(num_used)
        , min(first_confl_used)
        , max(last_confl_used)
        , sum(sum_hist_used)
        , (1.0*sum(sum_hist_used))/(1.0*sum(num_used))
        , max(last_prop_used)
        from goodClauses as c group by clauseID;"""
        self.c.execute(q)
        print("sum_cl_use filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        create index `idxclid20` on `sum_cl_use` (`clauseID`, num_used, avg_hist_used);
        create index `idxclid21` on `sum_cl_use` (`clauseID`);
        create index `idxclid21-2` on `sum_cl_use` (`clauseID`, avg_hist_used);
        create index `idxusedClauses` on `usedClauses` (`clauseID`, `used_at`);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("sum_cl_use indexes added T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """update sum_cl_use
        set `var_hist_used` = (
        select
        sum(1.0*(u.used_at-cs.conflicts-sum_cl_use.avg_hist_used)*(u.used_at-cs.conflicts-sum_cl_use.avg_hist_used))/(sum_cl_use.num_used*1.0)
        from
        clauseStats as cs,
        usedClauses as u
        where sum_cl_use.clauseID = u.clauseID
        and cs.clauseID = u.clauseID
        group by u.clauseID );
        """
        self.c.execute(q)
        print("sum_cl_use added variance T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""
        insert into sum_cl_use
        (
        `clauseID`,
        `num_used`,
        `first_confl_used`,
        `last_confl_used`,
        `sum_hist_used`,
        `avg_hist_used`,
        `last_prop_used`
        )
        select cl.clauseID,
        0,     --  `num_used`,
        NULL,   --  `first_confl_used`,
        NULL,   --  `last_confl_used`,
        0,      --  `sum_hist_used`,
        NULL,   --  `avg_hist_used`,
        NULL   --  `last_prop_used`
        from clauseStats as cl left join sum_cl_use as goodcl
        on cl.clauseID = goodcl.clauseID
        where
        goodcl.clauseID is NULL
        and cl.clauseID != 0;
        """
        self.c.execute(q)
        print("sum_cl_use added bad claues T: %-3.2f s" % (time.time() - t))

    def drop_idxs_tables(self):
        print("Dropping indexes/tables")
        print("Getting indexes to drop...")
        q = """
        SELECT name FROM sqlite_master WHERE type == 'index'
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        q = ""
        for row in rows:
            print("Will delete index:", row[0])
            q += "drop index if exists `%s`;\n" % row[0]

        t = time.time()
        q += """
        drop table if exists `goodClauses`;
        drop table if exists `idxusedClauses`;
        """

        for l in q.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Dropping index/table: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Dopping index/table T: %-3.2f s" % (time.time() - t2))

        print("Indexes/tables dropped T: %-3.2f s" % (time.time() - t))

if __name__ == "__main__":
    usage = "usage: %prog [options] sqlitedb"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the sqlite file!")
        exit(-1)

    with QueryFill(args[0]) as q:
        q.create_indexes()
        q.fill_last_prop()
        q.fill_sum_cl_use()
        q.drop_idxs_tables()

    print("Done.")

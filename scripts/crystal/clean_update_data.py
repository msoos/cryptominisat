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


class QueryFill (helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryFill, self).__init__(dbfname)

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

    # the clause can be deleted multiple times, because of clause being
    # minimized. In these cases, a new clause with an identical ID is
    # created, that is smaller. This clause ID would then be in the DB more
    # than once. So here we clean the "cl_last_in_solver" from the bogus
    # deletes, and keep the last one only
    def only_keep_last_conf_from_cl_last_in_solver(self):
        print("Keeping only last one from cl_last_in_solver...")

        t = time.time()
        q = """
        DROP TABLE IF EXISTS `cl_last_in_solver2`;
        CREATE TABLE `cl_last_in_solver2` AS SELECT clauseID,max(conflicts) as conflicts FROM `cl_last_in_solver` group by clauseID;
        DROP TABLE IF EXISTS `cl_last_in_solver`;
        ALTER TABLE `cl_last_in_solver2` RENAME TO `cl_last_in_solver`;
        """

        for l in q.split('\n'):
            self.c.execute(l)
        print("cl_last_in_solver has been made unique T: %-3.2f s" % (time.time() - t))

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
            `last_confl_used` bigint(20)
        );"""
        self.c.execute(q)
        print("sum_cl_use recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """insert into sum_cl_use
        (
        `clauseID`
        , `num_used`
        , `first_confl_used`
        , `last_confl_used`
        )
        select
        clauseID
        , count()
        , min(used_at)
        , max(used_at)
        from used_clauses as c group by clauseID;"""
        self.c.execute(q)
        print("sum_cl_use filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        create index `idxclid21` on `sum_cl_use` (`clauseID`);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("sum_cl_use indexes added T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        insert into sum_cl_use
        (
        `clauseID`
        , `num_used`
        , `first_confl_used`
        , `last_confl_used`
        )
        select
        clstats.clauseID -- `clauseID`
        , 0     --  `num_used`,
        , NULL   --  `first_confl_used`
        , NULL   --  `last_confl_used`
        from clause_stats as clstats left join sum_cl_use
        on clstats.clauseID = sum_cl_use.clauseID
        where
        sum_cl_use.clauseID is NULL
        and clstats.clauseID != 0;
        """
        self.c.execute(q)
        print("sum_cl_use added bad claues T: %-3.2f s" % (time.time() - t))

    def drop_idxs_tables(self):
        helper.drop_idxs(self.c)

        print("Dropping tables...")
        t = time.time()
        q = """
        drop table if exists `goodClauses`;
        drop table if exists `idxused_clauses`;
        """

        for l in q.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Dropping table: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Dopped table T: %-3.2f s" % (time.time() - t2))

        print("Tables dropped T: %-3.2f s" % (time.time() - t))


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
        q.only_keep_last_conf_from_cl_last_in_solver()
        q.fill_sum_cl_use()
        q.drop_idxs_tables()

    print("Done.")

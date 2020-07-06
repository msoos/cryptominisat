#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2020  Mate Soos
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


class QueryDatRem(helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryDatRem, self).__init__(dbfname)

    def create_percentiles_table(self):
        # Drop table
        q_drop = """
        DROP TABLE IF EXISTS `used_later_percentiles`;
        """
        self.c.execute(q_drop)

        # Create and fill used_later_X tables
        q_create = """
        create table `used_later_percentiles` (
            `type_of_dat` string NOT NULL,
            `percentile_descr` string NOT NULL,
            `val` float NOT NULL
        );"""
        self.c.execute(q_create)


    def get_all_percentile_X(self, name=""):
        t = time.time()
        print("Calculating percentiles now...")

        q2 = """
        insert into used_later_percentiles (type_of_dat, percentile_descr, val)
        {q}
        """

        q = "select '{name}', 'avg', avg(used_later{name}) from used_later{name};".format(name=name)
        self.c.execute(q2.format(name=name, q=q))

        q = """
        SELECT
        '{name}', 'top_non_zero_{perc}_perc', used_later{name}
        FROM used_later{name}
        WHERE used_later{name}>0
        ORDER BY used_later{name} ASC
        LIMIT 1
        OFFSET (SELECT
         COUNT(*)
        FROM used_later{name}
        WHERE used_later{name}>0) * (100-{perc}) / 100 - 1;
        """
        for perc in range(0,100):
            myq = q.format(name=name, perc=perc)
            self.c.execute(q2.format(name=name, q=myq))

        q = """
        SELECT
        '{name}', 'top_also_zero_{perc}_perc', used_later{name}
        FROM used_later{name}
        ORDER BY used_later{name} ASC
        LIMIT 1
        OFFSET (SELECT
         COUNT(*)
        FROM used_later{name}) * (100-{perc}) / 100 - 1;
        """
        for perc in range(0,100):
            myq = q.format(name=name, perc=perc)
            self.c.execute(q2.format(name=name, q=myq))
        print("Calculated percentiles/averages, T:", time.time()-t)

    def print_percentiles(self):
        q_check = "select * from used_later_percentiles"
        cur = self.conn.cursor()
        cur.execute(q_check)
        rows = cur.fetchall()
        print("Percentiles/average for used_later_percentiles:")
        for row in rows:
            print(" -> %s -- %s : %s" %(row[0], row[1], row[2]))


    def create_indexes1(self):
        print("Recreating indexes...")
        t = time.time()
        queries = """
        create index `idxclid31` on `clause_stats` (`clauseID`);
        create index `idxclid32` on `reduceDB` (`clauseID`);
        create index `idxclid33` on `sum_cl_use` (`clauseID`);
        create index `idxclid34` on `used_clauses` (`clauseID`);
        create index `idxclid44` on `restart_dat_for_cl` (`clauseID`);
        create index `idxclid35` on `var_data_fintime` (`var`, `sumConflicts_at_picktime`);
        create index `idxclid36` on `var_data_picktime` (`var`, `sumConflicts_at_picktime`);
        create index `idxclid37` on `dec_var_clid` (`var`, `sumConflicts_at_picktime`);
        create index `idxclid40` on `restart_dat_for_var` (`conflicts`);
        """

        for q in queries.split("\n"):
            self.c.execute(q)

        print("Created indexes needed T: %-3.2f s"% (time.time() - t))

    def recreate_used_ID_table(self):
        q = """
        DROP TABLE IF EXISTS `used_cl_ids`;
        """
        self.c.execute(q)

        q = """
        CREATE TABLE `used_cl_ids` (
          `clauseID` int(20) NOT NULL
        );
        """
        self.c.execute(q)

        q = """
        create index `idxclid30` on `used_cl_ids` (`clauseID`);
        """
        self.c.execute(q)

    def remove_too_many_vardata(self):
        t = time.time()
        q = """
        select count()
        from var_data_picktime
        """
        ret = self.c.execute(q)
        rows = self.c.fetchall()
        assert len(rows) == 1
        num_vardata = rows[0][0]
        print("Current number of elements in var_data: %d" % num_vardata)

        if num_vardata < options.goal_vardata:
            print("Not too many in var_data, skipping removal.")
            return

        q = """
        DROP TABLE IF EXISTS `used_vardat`;
        """
        self.c.execute(q)

        q = """
        CREATE TABLE `used_vardat` (
          `var` bigint(20) NOT NULL
          , `sumConflicts_at_picktime` bigint(20) NOT NULL
        );
        """
        self.c.execute(q)

        q = """
        create index `idxclidxx` on `used_vardat`
        (`var`, `sumConflicts_at_picktime`);
        """
        self.c.execute(q)

        q = """
        insert into `used_vardat`
        SELECT
        var, sumConflicts_at_picktime
        FROM var_data_picktime
        order by random()
        limit {limit}
        """.format(limit=options.goal_vardata)
        self.c.execute(q)
        print("Added {limit} to `used_vardat`".format(limit=options.goal_vardata))
        print("--> T: %-3.2f s"% (time.time() - t))

        t = time.time()
        del_from = ["var_data_picktime", "var_data_fintime", "dec_var_clid"]
        for table in del_from:

            q = """
            DROP TABLE IF EXISTS `myrows`;
            """
            self.c.execute(q)

            q = """
            CREATE TABLE `myrows` (
              `myrowid` bigint(20) NOT NULL
            );
            """
            self.c.execute(q)

            q = """
            INSERT INTO `myrows`
            SELECT `rowid`
            FROM `{table}` WHERE (`var`, `sumConflicts_at_picktime`)
            in (SELECT `var`, `sumConflicts_at_picktime` from `used_vardat`);
            """
            self.c.execute(q.format(table=table))

            q = """
            create index `myidx111` on `myrows` (`myrowid`);
            """

            q = """
            DELETE FROM `{table}` WHERE (rowid) NOT IN
            (SELECT `myrowid` from `myrows` );"""
            self.c.execute(q.format(table=table))
            print("Deleted unused data from %s" % table)

        # cleanup
        q = """
        DROP TABLE IF EXISTS `myrows`;
        """
        self.c.execute(q)

        # sample restart_dat_for_var
        q = """
        DELETE FROM restart_dat_for_var WHERE `conflicts` NOT IN
        (SELECT `sumConflicts_at_picktime` from `used_vardat` group by sumConflicts_at_picktime);"""
        self.c.execute(q)
        print("Deleted unused data from restart_dat_for_var")

        # cleanup
        q = """
        DROP TABLE IF EXISTS `used_vardat`;
        """
        self.c.execute(q)
        print("Cleaned up var_data_x & restart_dat_for_var tables T: %-3.2f s"
              % (time.time() - t))

    def insert_into_used_cls_ids_from_clstats(self, min_used, limit, max_used=None):
        min_used = int(min_used)

        max_const = ""
        if max_used is not None:
            max_const = " and num_used <= %d" % max_used

        t = time.time()
        val = int()
        q = """
        insert into used_cl_ids
        select
        clauseID from sum_cl_use
        where
        num_used >= {min_used}
        {max_const}
        order by random() limit {limit}
        """.format(
            min_used=min_used,
            limit=int(limit),
            max_const=max_const)


        self.c.execute(q)
        print("Added num_used >= %d from sum_cl_use to used_cls_ids T: %-3.2f s"
              % (min_used, time.time() - t))

    def insert_into_used_cls_ids_ternary_resolvents(self, limit):
        limit = int(limit)

        t = time.time()
        val = int()
        q = """
        insert into used_cl_ids
        select
        clauseID from reduceDB
        where
        is_ternary_resolvent = 1
        group by clauseID
        order by random() limit {limit}
        """.format(limit=int(limit))


        self.c.execute(q)
        print("Added clauseIDs from reduceDB that are TERNARY from reduceDB to used_cls_ids T: %-3.2f s"
              % (time.time() - t))

    # inserts less than 1-1 ratio, inserting only 0.3*N from unused ones
    def fill_used_cl_ids_table(self, fair, limit):
        t = time.time()
        if not fair:
            self.insert_into_used_cls_ids_from_clstats(min_used=50000, limit=limit/20)
            self.insert_into_used_cls_ids_from_clstats(min_used=10000, limit=limit/20)
            self.insert_into_used_cls_ids_from_clstats(min_used=1000, limit=limit/10)
            self.insert_into_used_cls_ids_from_clstats(min_used=100, limit=limit/10)
            self.insert_into_used_cls_ids_from_clstats(min_used=30, limit=limit/5)
            self.insert_into_used_cls_ids_from_clstats(min_used=20, limit=limit/4)
            self.insert_into_used_cls_ids_from_clstats(min_used=5, limit=limit/3)
            self.insert_into_used_cls_ids_from_clstats(min_used=1, limit=limit/2)

        #self.insert_into_used_cls_ids_ternary_resolvents(limit=limit/2)
        self.insert_into_used_cls_ids_from_clstats(min_used=0, limit=limit, max_used=None)

        q = """
        select count()
        from used_cl_ids, sum_cl_use
        where
        used_cl_ids.clauseID = sum_cl_use.clauseID
        and sum_cl_use.num_used > 0
        """
        ret = self.c.execute(q)
        rows = self.c.fetchall()
        assert len(rows) == 1
        good_ids = rows[0][0]

        q = """
        select count()
        from used_cl_ids, sum_cl_use
        where
        used_cl_ids.clauseID = sum_cl_use.clauseID
        and sum_cl_use.num_used = 0
        """
        ret = self.c.execute(q)
        rows = self.c.fetchall()
        assert len(rows) == 1
        bad_ids = rows[0][0]

        print("IDs in used_cl_ids that are 'good' (sum_cl_use.num_used > 0) : %d" % good_ids)
        print("IDs in used_cl_ids that are 'bad'  (sum_cl_use.num_used = 0) : %d" % bad_ids)
        print("   T: %-3.2f s" % (time.time() - t))

    def print_idxs(self):
        print("Using indexes: ")
        q = """
        SELECT * FROM sqlite_master WHERE type == 'index'
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        queries = ""
        for row in rows:
            print("-> index:", row)

    def create_used_clauses_red(self):
        t = time.time()
        q = """
        CREATE TABLE used_clauses_red AS
        SELECT * FROM used_clauses WHERE clauseID IN (SELECT clauseID from used_cl_ids );
        """

        self.c.execute(q)
        print("Filtered into used_clauses_red T: %-3.2f s" % (time.time() - t))

    def drop_used_clauses_red(self):
        q = """
        drop TABLE if exists used_clauses_red;
        """
        self.c.execute(q)

    def filter_tables_of_ids(self):

        queries = """
        drop index if exists idxclid34;
        drop index if exists idxclid32;
        """
        for q in queries.split("\n"):
            self.c.execute(q)

        self.print_idxs()

        tables = ["clause_stats", "reduceDB", "sum_cl_use",
                  "used_clauses", "restart_dat_for_cl", "cl_last_in_solver"]
        q = """
        DELETE FROM {table} WHERE clauseID NOT IN
        (SELECT clauseID from used_cl_ids );"""

        for table in tables:
            t = time.time()
            self.c.execute(q.format(table=table))
            print("Filtered table '%s' T: %-3.2f s" % (table, time.time() - t))

    def print_sum_cl_use_distrib(self):
        q = """
        select c, num_used from (
        select count(*) as c, num_used
        from sum_cl_use
        group by num_used) order by num_used
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        print("Distribution of clause uses:")
        total = 0
        zero_use = 0
        for i in range(len(rows)):
            cnt = int(rows[i][0])
            numuse = int(rows[i][1])
            if numuse == 0:
                zero_use = cnt
            total += cnt

        i = 0
        while i < len(rows):
            cnt = int(rows[i][0])
            numuse = int(rows[i][1])

            this_cnt_tot = 0
            this_numuse_tot = 0
            for x in range(100000):
                if i+x >= len(rows):
                    i+=x
                    break

                this_cnt = int(rows[i+x][0])
                this_numuse = int(rows[i+x][1])
                this_cnt_tot += this_cnt
                this_numuse_tot += this_numuse
                if this_cnt_tot > 300:
                    i+=x
                    i+=1
                    break
            print("  ->  {cnt:-8d} of sum_cl_use: {numuse:-7d}-{this_numuse:-7d}  --  {percent:-3.5f} ratio".format(
                    cnt=this_cnt_tot, numuse=numuse, this_numuse=this_numuse, percent=(this_cnt_tot/total)))


        print("Total: %d of which zero_use: %d" % (total, zero_use))
        if zero_use == 0 or zero_use/total < 0.01:
            print("ERROR: Zero use is very low, this is almost surely a bug!")
            exit(-1)

    def check_db_sanity(self):
        print("Checking tables in DB...")
        q = """
        SELECT name FROM sqlite_master WHERE type == 'table'
        """
        found_sum_cl_use = False
        self.c.execute(q)
        rows = self.c.fetchall()
        for row in rows:
            if row[0] == "sum_cl_use":
                found_sum_cl_use = True

            print("-> We have table: ", row[0])
            if row[0] == "used_later_short" or row[0] == "used_later_long":
                print("ERROR: 'gen_pandas.py' has been already ran on this DB")
                print("       this will be a mess. We cannot run. ")
                print("       Exiting.")
                exit(-1)

        if not found_sum_cl_use:
            print("ERROR: Did not find sum_cl_use table. You probably didn't run")
            print("       the 'clean_update_data.py' on this database")
            print("       Exiting.")
            exit(-1)

        q = """
        SELECT count() FROM sum_cl_use where num_used = 0
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        assert len(rows) == 1
        num = int(rows[0][0])
        print("Unused clauses in sum_cl_use: ", num)
        if num == 0:
            print("ERROR: You most likely didn't run 'clean_data.py' on this database")
            print("       Exiting.")
            exit(-1)

        print("Tables seem OK")

    def insert_into_only_keep_rdb(self, min_used_later, limit):
        limit = int(limit)
        t = time.time()
        q = """
        insert into only_keep_rdb (id)
        select
        rdb0.rowid
        from reduceDB as rdb0, used_later
        where
        used_later.clauseID=rdb0.clauseID
        and used_later.rdb0conflicts=rdb0.conflicts
        and used_later.used_later >= {min_used_later}
        order by random()
        limit {limit}""".format(min_used_later=min_used_later, limit=limit)
        self.c.execute(q)
        print("Insert only_keep_rdb where used_later >= %d T: %-3.2f s" %
              (min_used_later, time.time() - t))

    def delete_too_many_rdb_rows(self):
        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Have %d lines of RDB" % (rdb_rows))

        q = """
        drop table if exists only_keep_rdb;
        """
        self.c.execute(q)

        t = time.time()
        q = """create table only_keep_rdb (
            id bigint(20) not null
        );"""
        self.c.execute(q)
        print("Created only_keep_rdb T: %-3.2f s" % (time.time() - t))

        if not options.fair:
            self.insert_into_only_keep_rdb(1000, options.goal_rdb/20)
            self.insert_into_only_keep_rdb(100, options.goal_rdb/10)
            self.insert_into_only_keep_rdb(20, options.goal_rdb/5)
            self.insert_into_only_keep_rdb(10, options.goal_rdb/5)
            self.insert_into_only_keep_rdb(5, options.goal_rdb/3)
            self.insert_into_only_keep_rdb(1, options.goal_rdb/2)

        self.insert_into_only_keep_rdb(0, options.goal_rdb)

        t = time.time()
        ret = self.c.execute("select count() from only_keep_rdb")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("We now have %d lines only_keep_rdb" % (rdb_rows))


        t = time.time()
        q = """
        drop index if exists `idxclid6-4`; -- the other index on reduceDB
        create index `idx_bbb` on `only_keep_rdb` (`id`);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later indexes added T: %-3.2f s" % (time.time() - t))

        q = """
        delete from reduceDB
        where reduceDB.rowid not in (select id from only_keep_rdb)
        """
        self.c.execute(q)
        print("Delete from reduceDB T: %-3.2f s" % (time.time() - t))

        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Finally have %d lines of RDB" % (rdb_rows))


    def del_table_and_vacuum(self):
        helper.drop_idxs(self.c)

        t = time.time()
        queries = """
        DROP TABLE IF EXISTS `used_later`;
        DROP TABLE IF EXISTS `only_keep_rdb`;
        DROP TABLE IF EXISTS `used_cl_ids`;
        """
        for q in queries.split("\n"):
            self.c.execute(q)
        print("Deleted tables T: %-3.2f s" % (time.time() - t))

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
    parser.add_option("--goalvardata", default=50000, type=int,
                      dest="goal_vardata", help="Number of varData points neeeded")
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--noidx", action="store_true", default=False,
                      dest="noidx", help="Don't recreate indexes")
    parser.add_option("--fair", "-f", action="store_true", default=False,
                      dest="fair", help="Fair sampling. NOT DEFAULT.")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the sqlite file!")
        exit(-1)

    if not options.fair:
        print("NOTE: Sampling will NOT be fair.")
        print("      This is because otherwise, DB will be huge")
        print("      and we need lots of positive datapoints")
        print("      most of which will be from clauses that are more used")

    with QueryDatRem(args[0]) as q:
        q.check_db_sanity()
        helper.dangerous(q.c)
        helper.drop_idxs(q.c)
        q.create_indexes1()
        q.remove_too_many_vardata()

    if False:
        print("This is good for verifying that the fast ones are close")
        # slower percentiles
        t = time.time()
        with helper.QueryFill(args[0]) as q:
            helper.dangerous(q.c)
            q.delete_and_create_all()
            q.create_indexes(verbose=options.verbose)
            q.fill_used_later()
            q.fill_used_later_X("short", duration=10000)
            q.fill_used_later_X("long", duration=50000)
            q.fill_used_later_X("forever", duration=1000*1000*1000, forever=True)
        with QueryDatRem(args[0]) as q:
            helper.dangerous(q.c)
            q.create_percentiles_table()
            q.get_all_percentile_X("")
            q.get_all_percentile_X("_short")
            q.get_all_percentile_X("_long")
            q.print_percentiles()
        with helper.QueryFill(args[0]) as q:
            q.delete_and_create_all()
        print("SLOWER percentiles:", time.time()-t)

    # Percentile generation
    t = time.time()
    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.recreate_used_ID_table()
        q.fill_used_cl_ids_table(True, limit=4*options.limit)
        q.drop_used_clauses_red()
        q.create_used_clauses_red()
    with helper.QueryFill(args[0]) as q:
        helper.dangerous(q.c)
        q.delete_and_create_all()
        q.create_indexes(verbose=options.verbose, used_clauses="used_clauses_red")
        q.fill_used_later(used_clauses="used_clauses_red")
        q.fill_used_later_X("short", duration=10000, used_clauses="used_clauses_red")
        q.fill_used_later_X("long", duration=50000, used_clauses="used_clauses_red")
        q.fill_used_later_X("forever", duration=1000*1000*1000, used_clauses="used_clauses_red", forever=True)
    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.create_percentiles_table()
        q.get_all_percentile_X("")
        q.get_all_percentile_X("_short")
        q.get_all_percentile_X("_long")
        q.get_all_percentile_X("_forever")
        q.print_percentiles()
        q.drop_used_clauses_red()
    with helper.QueryFill(args[0]) as q:
        helper.drop_idxs(q.c)
        q.delete_and_create_all()
    print("FASTER percentiles:", time.time()-t)

    # Filtering for clauseIDs in tables:
    # ["clause_stats", "reduceDB", "sum_cl_use",
    #      "used_clauses", "restart_dat_for_cl", "cl_last_in_solver"]
    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.recreate_used_ID_table()
        q.fill_used_cl_ids_table(options.fair, limit=options.limit)
        q.filter_tables_of_ids()
        q.print_sum_cl_use_distrib()
        print("-------------")

    # RDB filtering
    with helper.QueryFill(args[0]) as q:
        helper.dangerous(q.c)
        helper.drop_idxs(q.c)
        q.delete_and_create_all()
        q.create_indexes(verbose=options.verbose)
        q.fill_used_later()
    with QueryDatRem(args[0]) as q:
        print("-------------")
        q.delete_too_many_rdb_rows()

        helper.drop_idxs(q.c)
        q.del_table_and_vacuum()

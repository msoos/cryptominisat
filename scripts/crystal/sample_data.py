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
import optparse
import time
import os.path
import helper


class QueryDatRem(helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryDatRem, self).__init__(dbfname)

    def create_percentiles_table(self):
        for table in ["used_later", "used_later_anc"]:
            # drop table
            q_drop = """
            DROP TABLE IF EXISTS `{table}_percentiles`;
            """
            self.c.execute(q_drop.format(table=table))

            # Create percentiles table
            q_create = """
            create table `{table}_percentiles` (
                `type_of_dat` string NOT NULL,
                `percentile_descr` string NOT NULL,
                `percentile` float DEFAULT NULL,
                `val` float NOT NULL
            );"""
            self.c.execute(q_create.format(table=table))

            idxs = """
            create index `{table}_percentiles_idx3` on `{table}_percentiles` (`type_of_dat`, `percentile_descr`, `percentile`, `val`);
            create index `{table}_percentiles_idx2` on `{table}_percentiles` (`type_of_dat`, `percentile_descr`, `val`);"""
            for q in idxs.split("\n"):
                self.c.execute(q.format(table=table))


    # "tier" here is "short", "long", or "forever"
    def get_all_percentile_X(self, tier):
        t = time.time()
        for table in ["used_later", "used_later_anc"]:
            print("Calculating percentiles now for table {table} and tier {tier} ...".format(
                tier=tier, table=table))

            q2 = """
            insert into {table}_percentiles (type_of_dat, percentile_descr, percentile, val)
            {q}
            """

            q = "select '{tier}', 'avg', NULL, avg(used_later) from {table}_{tier};".format(
                tier=tier, table=table)
            self.c.execute(q2.format(tier=tier, table=table, q=q))

            q = """
            SELECT
            '{tier}', 'top_non_zero', {perc}, used_later
            FROM {table}_{tier}
            WHERE used_later>0
            ORDER BY used_later ASC
            LIMIT 1
            OFFSET round((SELECT
             COUNT(*)
            FROM {table}_{tier}
            WHERE used_later>0) * ((100-{perc}) / 100.0)) - 1;
            """
            for perc in list(range(0,30,1))+list(range(30,100, 5)):
                myq = q.format(tier=tier, table=table, perc=perc)
                self.c.execute(q2.format(tier=tier, table=table, q=myq))
            # the 100% perecentile is not 0 (remember, this is "non-zero"), but let's cheat and add it in
            self.c.execute(q2.format(
                tier=tier, table=table, q="select '{tier}', 'top_non_zero', 100.0, 0.0;".format(
                    tier=tier, table=table)))

            q = """
            SELECT
            '{tier}', 'top_also_zero', {perc}, used_later
            FROM {table}_{tier}
            ORDER BY used_later ASC
            LIMIT 1
            OFFSET round((SELECT
             COUNT(*)
            FROM {table}_{tier}) * ((100.0-{perc}) / 100.0)) - 1;
            """
            for perc in range(0,100, 10):
                myq = q.format(tier=tier, table=table, perc=perc)
                self.c.execute(q2.format(tier=tier, table=table, q=myq))
            self.c.execute(
                q2.format(tier=tier, table=table,
                          q="select '{tier}', 'top_also_zero', 100.0, 0.0;".format(tier=tier, table=table)))

            print("Calculated percentiles/averages, T:", time.time()-t)

    def print_percentiles(self):
        q_check = "select * from used_later_percentiles"
        cur = self.conn.cursor()
        cur.execute(q_check)
        rows = cur.fetchall()
        print("Percentiles/average for used_later_percentiles:")
        for row in rows:
            print(" -> %s %s -- %s : %s" %(row[0], row[1], row[2], row[3]))


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

    def insert_into_used_cls_ids_from_clstats(self, min_used, limit, table):
        min_used = int(min_used)

        t = time.time()
        val = int()
        q = """
        insert into used_cl_ids
        select
        clauseID from {table}
        where
        num_used >= {min_used}
        order by random() limit {limit}
        """.format(
            min_used=min_used,
            limit=int(limit),
            table=table)

        self.c.execute(q)
        print("Added num_used >= %d from sum_cl_use to used_cls_ids T: %-3.2f s"
              % (min_used, time.time() - t))

    # inserts ratio that's slanted towards >=1 use
    def fill_used_cl_ids_table(self, fair, limit):
        t = time.time()
        for table in ["sum_cl_use"]:
            if not fair:
                self.insert_into_used_cls_ids_from_clstats(min_used=100000, limit=limit/20, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=50000, limit=limit/10, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=10000, limit=limit/10, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=1000, limit=limit/10, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=100, limit=limit/10, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=30, limit=limit/5, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=20, limit=limit/4, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=5, limit=limit/3, table=table)
                self.insert_into_used_cls_ids_from_clstats(min_used=1, limit=limit/2, table=table)
            self.insert_into_used_cls_ids_from_clstats(min_used=0, limit=limit/2, table=table)

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

        q = """
        SELECT * FROM sqlite_master WHERE type == 'index'
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        queries = ""

        if options.verbose:
            print("Using indexes: ")
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

        tables = ["clause_stats", "reduceDB", "sum_cl_use", "used_clauses_anc",
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
        # grain-53-80-0s0-seed-125-4-init-35.cnf.out actually has only 0.0098 use (i.e. 0.98%)
        if zero_use == 0 or zero_use/total < 0.009:
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

    def insert_into_only_keep_rdb(self, min_used_later, limit, tier, table):
        limit = int(limit)
        t = time.time()
        q = """
        insert into only_keep_rdb (id)
        select
        rdb0.rowid

        FROM
        reduceDB as rdb0,
        {table}_{tier}

        WHERE
        {table}_{tier}.clauseID=rdb0.clauseID
        and {table}_{tier}.rdb0conflicts=rdb0.conflicts
        and {table}_{tier}.used_later >= {min_used_later}
        order by random()
        limit {limit}""".format(min_used_later=min_used_later, limit=limit,
                                tier=tier, table=table)
        self.c.execute(q)

        ret = self.c.execute("""select count() from only_keep_rdb""")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]

        print("Insert only_keep_rdb where %s_%s >= %d T: %-3.2f s. Now size: %s" %
              (table, tier, min_used_later, time.time() - t, rdb_rows))

    def delete_too_many_rdb_rows(self):
        t = time.time()
        val = int(options.limit)
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("Have %d lines of RDB, let's do non-fair selection" % (rdb_rows))

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

        mygoal = options.goal_rdb/13
        table="used_later" # we could iterate with "used_later_anc", but not doing that
        for tier in ["short", "long", "forever"]:
            mygoal*=3 # this gives 8%, 23%, 70% distribution (total 100% if not rounded)
            if not options.fair:
                self.insert_into_only_keep_rdb(100000, mygoal/20, tier=tier, table=table)
                self.insert_into_only_keep_rdb(10000, mygoal/20, tier=tier, table=table)
                self.insert_into_only_keep_rdb(1000, mygoal/20, tier=tier, table=table)
                self.insert_into_only_keep_rdb(100, mygoal/10, tier=tier, table=table)
                self.insert_into_only_keep_rdb(20, mygoal/5, tier=tier, table=table)
                self.insert_into_only_keep_rdb(10, mygoal/5, tier=tier, table=table)
                self.insert_into_only_keep_rdb(5, mygoal/3, tier=tier, table=table)
                self.insert_into_only_keep_rdb(1, mygoal/2, tier=tier, table=table)
            self.insert_into_only_keep_rdb(0, mygoal, tier=tier, table=table)

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
        print("only_keep_rdb indexes added T: %-3.2f s" % (time.time() - t))

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

    # lengths of short/long
    parser.add_option("--short", default=10*1000, type=int,
                      dest="short", help="Short duration. Default: %default")
    parser.add_option("--long", default=30*1000, type=int,
                      dest="long", help="Long duration. Default: %default")
    parser.add_option("--forever", default=120*1000, type=int,
                      dest="forever", help="Forever duration. Default: %default")

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

    # this is the SLOW way of doing it -- without pre-sampling it
    if False:
        print("This is good for verifying that the fast ones are close")
        # slower percentiles
        t = time.time()
        with helper.QueryFill(args[0]) as q:
            helper.dangerous(q.c)
            q.delete_and_create_used_laters()
            q.create_indexes(verbose=options.verbose)
            for tier in ["short", "long", "forever"]:
                q.fill_used_later_X(tier, getattr(options, tier))
        with QueryDatRem(args[0]) as q:
            helper.dangerous(q.c)
            q.create_percentiles_table()
            for tier in ["short", "long", "forever"]:
                q.get_all_percentile_X(tier)
            q.print_percentiles()
        with helper.QueryFill(args[0]) as q:
            q.delete_and_create_used_laters()
        print("SLOWER percentiles:", time.time()-t)

    # Percentile generation
    t = time.time()

    # first, we create "used_clauses_red" that contains a reduced
    # list of clauseIDs we want to do this over. Here we sample FAIRLY!!!!
    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.recreate_used_ID_table()
        q.fill_used_cl_ids_table(True, limit=4*options.limit) # notice the FAIR sampling!
        q.drop_used_clauses_red()
        q.create_used_clauses_red()

    # now we generate the used_later data
    with helper.QueryFill(args[0]) as q:
        helper.dangerous(q.c)
        q.delete_and_create_used_laters()
        q.create_indexes(verbose=options.verbose, used_clauses="used_clauses_red")
        for table in ["used_later", "used_later_anc"]:
            for tier in ["short", "long", "forever"]:
                q.fill_used_later_X(tier, duration=getattr(options, tier),
                                    used_clauses="used_clauses_red",
                                    table=table)

    # now we calculate the distributions and save them
    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.create_percentiles_table()
        for tier in ["short", "long", "forever"]:
                q.get_all_percentile_X(tier)
        q.print_percentiles()
        q.drop_used_clauses_red()
    with helper.QueryFill(args[0]) as q:
        helper.drop_idxs(q.c)
        q.delete_and_create_used_laters()
    print("FASTER percentiles:", time.time()-t)

    # Filtering for clauseIDs in tables:
    # ["clause_stats", "reduceDB", "sum_cl_use",
    #      "used_clauses", "restart_dat_for_cl", "cl_last_in_solver"]
    # here we sample NON-FAIRLY (options.fair) by default!
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
        q.delete_and_create_used_laters()
        q.create_indexes(verbose=options.verbose)

        # this is is needed for RDB row deletion below (since it's not fair)
        table = "used_later" # we could also do used_later_anc (for RDB)
        for tier in ["short", "long", "forever"]:
            q.fill_used_later_X(tier=tier, table=table, duration=getattr(options, tier))

    with QueryDatRem(args[0]) as q:
        print("-------------")
        q.delete_too_many_rdb_rows() # NOTE: NOT FAIR!!!

        helper.drop_idxs(q.c)
        q.del_table_and_vacuum()

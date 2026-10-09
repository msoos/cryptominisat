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

# Fills the used_later*_disc label tables (with the rank among ALL the
# tracked clauses of the reduce) on the full DB, then picks the rows the
# frames are made of and shrinks the DB to them:
# - frame_rows: the training sample. Cells of label stratum (top cut1 %
#   of the used rows, up to cut2 %, the rest) x age (dump_no 0, 1, 2-5,
#   more), at most limit/4 rows of each, picked in a fixed pseudo-random
#   order. weight = rows in the cell / rows picked: what a row stands for
# - eval_rows: a fair sample, for measuring. Evenly spaced reduces, up to
#   --evalperreduce rows of each, no strata
#
# usage: sample_data.py --halflife 4
#           --cut1 3.0 --cut2 25.0 --limit 3000 data-min.db
from __future__ import print_function
import sqlite3
import optparse
import time
import os.path
import helper

TABLES = ["used_later", "used_later_anc"]


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


    def get_all_percentiles(self):
        t = time.time()
        for table in ["used_later", "used_later_anc"]:
            print("Calculating percentiles now for table {table} ...".format(
                table=table))
            self.c.execute("select count(*) from {table}_disc".format(table=table))
            if self.c.fetchone()[0] == 0:
                print("   -> empty (run shorter than two half-lives), no percentiles")
                continue

            q2 = """
            insert into {table}_percentiles (type_of_dat, percentile_descr, percentile, val)
            {q}
            """

            q = "select 'disc', 'avg', NULL, avg(used_later) from {table}_disc;".format(
                table=table)
            self.c.execute(q2.format(table=table, q=q))

            q = """
            SELECT
            'disc', 'top_non_zero', {perc}, used_later
            FROM {table}_disc
            WHERE used_later>0
            ORDER BY used_later ASC
            LIMIT 1
            OFFSET round((SELECT
             COUNT(*)
            FROM {table}_disc
            WHERE used_later>0) * ((100-{perc}) / 100.0)) - 1;
            """
            for perc in list(range(0,30,1))+list(range(30,100, 5)):
                myq = q.format(table=table, perc=perc)
                self.c.execute(q2.format(table=table, q=myq))
            # the 100% perecentile is not 0 (remember, this is "non-zero"), but let's cheat and add it in
            self.c.execute(q2.format(
                table=table, q="select 'disc', 'top_non_zero', 100.0, 0.0;".format(
                    table=table)))

            q = """
            SELECT
            'disc', 'top_also_zero', {perc}, used_later
            FROM {table}_disc
            ORDER BY used_later ASC
            LIMIT 1
            OFFSET round((SELECT
             COUNT(*)
            FROM {table}_disc) * ((100.0-{perc}) / 100.0)) - 1;
            """
            for perc in range(0,100, 10):
                myq = q.format(table=table, perc=perc)
                self.c.execute(q2.format(table=table, q=myq))
            self.c.execute(
                q2.format(table=table,
                          q="select 'disc', 'top_also_zero', 100.0, 0.0;".format(table=table)))

            print("Calculated percentiles/averages, T:", time.time()-t)

    def print_percentiles(self):
        q_check = "select * from used_later_percentiles"
        cur = self.conn.cursor()
        cur.execute(q_check)
        rows = cur.fetchall()
        print("Percentiles/average for used_later_percentiles:")
        for row in rows:
            print(" -> %s %s -- %s : %s" %(row[0], row[1], row[2], row[3]))


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

    def create_row_tables(self):
        for t in ["frame_rows", "eval_rows"]:
            self.c.execute("drop table if exists %s" % t)
            self.c.execute("""create table %s (
                `tbl` string NOT NULL,
                `clauseID` bigint(20) NOT NULL,
                `conflicts` bigint(20) NOT NULL,
                `weight` float NOT NULL)""" % t)
        self.c.execute("create index `idxrdbsamp` on `reduceDB` (`clauseID`, `conflicts`)")

    def get_cut(self, table, perc):
        self.c.execute("""select val from {table}_percentiles where type_of_dat = 'disc'
            and percentile_descr = 'top_non_zero' and percentile = {perc}""".format(
                table=table, perc=perc))
        row = self.c.fetchone()
        return None if row is None else row[0]

    def pick_frame_rows(self, table):
        cuts = [self.get_cut(table, p) for p in [0.0, options.cut1, options.cut2]]
        if None in cuts:
            print("WARNING: no clause is ever used in {table}_disc, no rows".format(table=table))
            return
        # top cut1%, cut1..cut2%, the rest with the never-used
        stratas = [
            "ul.used_later > %r" % cuts[1],
            "ul.used_later <= %r and ul.used_later > %r" % (cuts[1], cuts[2]),
            "ul.used_later <= %r" % cuts[2]]
        ages = ["rdb0.dump_no = 0", "rdb0.dump_no = 1",
                "rdb0.dump_no > 1 and rdb0.dump_no <= 5", "rdb0.dump_no > 5"]
        limit = int(options.limit/4)
        q_from = """
        from {table}_disc as ul join reduceDB as rdb0
        on rdb0.clauseID = ul.clauseID and rdb0.conflicts = ul.rdb0conflicts
        where {strata} and {age}"""
        for strata in stratas:
            for age in ages:
                f = q_from.format(table=table, strata=strata, age=age)
                self.c.execute("select count(*) " + f)
                num = self.c.fetchone()[0]
                if num == 0:
                    continue
                self.c.execute("""
                insert into frame_rows
                select '{table}', ul.clauseID, ul.rdb0conflicts, {weight}
                {f}
                order by rowhash(ul.clauseID, ul.rdb0conflicts) limit {limit}""".format(
                    table=table, f=f, limit=limit, weight=repr(max(1.0, num/float(limit)))))
                print("%s: %-45s %-40s %8d rows, weight %.2f" % (
                    table, strata, age, num, max(1.0, num/float(limit))))

    def pick_eval_rows(self, table):
        # only what the solver would have at the reduce: not the clauses
        # that live on because of the lock (gone)
        q_from = """
        from {table}_disc as ul join reduceDB as rdb0
        on rdb0.clauseID = ul.clauseID and rdb0.conflicts = ul.rdb0conflicts
        where rdb0.gone = 0""".format(table=table)
        self.c.execute("select ul.rdb0conflicts, count(*) " + q_from + " group by ul.rdb0conflicts order by ul.rdb0conflicts")
        reduces = self.c.fetchall()
        if not reduces:
            return
        n = min(options.eval_reduces, len(reduces))
        picked = sorted(set(int((i + 0.5) * len(reduces) / n) for i in range(n)))
        for i in picked:
            confl, num = reduces[i]
            self.c.execute("""
            insert into eval_rows
            select '{table}', ul.clauseID, ul.rdb0conflicts, {weight}
            {q_from} and ul.rdb0conflicts = {confl}
            order by rowhash(ul.clauseID, ul.rdb0conflicts) limit {limit}""".format(
                table=table, q_from=q_from, confl=confl, limit=options.eval_per_reduce,
                weight=repr(max(1.0, num/float(options.eval_per_reduce)))))
        print("%s: eval rows from %d of %d reduces" % (table, len(picked), len(reduces)))

    def filter_tables(self):
        t = time.time()
        self.c.execute("drop table if exists keep_rows")
        self.c.execute("""create table keep_rows as
            select clauseID, conflicts from frame_rows union select clauseID, conflicts from eval_rows""")
        self.c.execute("create index `idxkeep1` on keep_rows (clauseID, conflicts)")
        self.c.execute("create index `idxfr1` on frame_rows (tbl, clauseID, conflicts)")
        self.c.execute("create index `idxev1` on eval_rows (tbl, clauseID, conflicts)")
        self.c.execute("""delete from reduceDB where not exists
            (select 1 from keep_rows k where k.clauseID = reduceDB.clauseID and k.conflicts = reduceDB.conflicts)""")
        for table in TABLES:
            self.c.execute("""delete from {table}_disc where not exists
                (select 1 from keep_rows k where k.clauseID = {table}_disc.clauseID
                 and k.conflicts = {table}_disc.rdb0conflicts)""".format(table=table))
        for table in ["clause_stats", "sum_cl_use", "cl_last_in_solver"]:
            self.c.execute("delete from %s where clauseID not in (select clauseID from keep_rows)" % table)
        # the labels are filled, the uses are not needed any more
        for table in ["used_clauses_anc", "used_clauses"]:
            self.c.execute("drop table if exists %s" % table)
        self.c.execute("select count(*) from reduceDB")
        print("Kept %d reduceDB rows T: %-3.2f s" % (self.c.fetchone()[0], time.time() - t))

    def del_table_and_vacuum(self):
        helper.drop_idxs(self.c)

        t = time.time()
        queries = """
        DROP TABLE IF EXISTS `keep_rows`;
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
                      dest="limit", help="Max number of rows from each label stratum, per table")
    parser.add_option("--cut1", default=3.0, type=float,
                      dest="cut1", help="The top stratum: this %% of the used rows. Default: %default")
    parser.add_option("--cut2", default=25.0, type=float,
                      dest="cut2", help="The middle stratum ends at this %%. Default: %default")
    parser.add_option("--evalreduces", default=10, type=int,
                      dest="eval_reduces", help="Reduces in the fair evaluation sample. Default: %default")
    parser.add_option("--evalperreduce", default=3000, type=int,
                      dest="eval_per_reduce", help="Max rows per reduce in the fair evaluation sample. Default: %default")
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    helper.add_label_options(parser)

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the sqlite file!")
        exit(-1)

    with QueryDatRem(args[0]) as q:
        q.check_db_sanity()
        helper.dangerous(q.c)
        helper.drop_idxs(q.c)
        q.print_sum_cl_use_distrib()

    # the labels, over every row of every tracked clause
    t = time.time()
    with helper.QueryFill(args[0]) as q:
        helper.dangerous(q.c)
        q.delete_and_create_used_laters()
        q.create_indexes(verbose=options.verbose)
        for table in TABLES:
            q.fill_used_later(options.halflife, table=table)

    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.create_percentiles_table()
        q.get_all_percentiles()
        q.print_percentiles()
    with helper.QueryFill(args[0]) as q:
        helper.dangerous(q.c)
        for table in TABLES:
            q.fill_used_later_perc_fit(table=table)
    print("Labels and percentiles T: %-3.2f s" % (time.time() - t))

    with QueryDatRem(args[0]) as q:
        helper.dangerous(q.c)
        q.create_row_tables()
        for table in TABLES:
            q.pick_frame_rows(table)
            q.pick_eval_rows(table)
        q.filter_tables()
        helper.drop_idxs(q.c)
        q.del_table_and_vacuum()

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
import sys


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

    def measure_size(self):
        t = time.time()
        ret = self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("We have %d lines of RDB" % (rdb_rows))

        t = time.time()
        ret = self.c.execute("select count() from clauseStats")
        rows = self.c.fetchall()
        clss_rows = rows[0][0]
        print("We have %d lines of clauseStats" % (clss_rows))

    def create_indexes(self):
        print("Deleting & recreating indexes...")
        t = time.time()
        q = """
        SELECT name FROM sqlite_master WHERE type == 'index'
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        queries = ""
        for row in rows:
            if options.verbose:
                print("Will delete index:", row[0])
            queries += "drop index if exists `%s`;\n" % row[0]

        queries += """
        create index `idxclid33` on `sum_cl_use` (`clauseID`, `last_confl_used`);
        create index `idxclid1` on `clauseStats` (`clauseID`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid1-2` on `clauseStats` (`clauseID`);
        create index `idxclid1-3` on `clauseStats` (`clauseID`, restarts);
        create index `idxclid1-4` on `clauseStats` (`clauseID`, restarts, prev_restart);
        create index `idxclid1-5` on `clauseStats` (`clauseID`, prev_restart);
        create index `idxclid2` on `clauseStats` (clauseID, `prev_restart`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid4` on `restart` ( `restarts`);
        create index `idxclid5` on `tags` ( `tagname`);
        create index `idxclid6` on `reduceDB` (`clauseID`, conflicts, latest_satzilla_feature_calc);
        create index `idxclid6-2` on `reduceDB` (`clauseID`, `dump_no`);
        create index `idxclid6-3` on `reduceDB` (`clauseID`, `conflicts`, `dump_no`);
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        create index `idxclid7` on `satzilla_features` (`latest_satzilla_feature_calc`);
        create index `idxclidUCLS-1` on `usedClauses` ( `clauseID`, `used_at`);
        create index `idxclidUCLS-2` on `usedClauses` ( `used_at`);
        create index `idxcl_last_in_solver-1` on `cl_last_in_solver` ( `clauseID`, `conflicts`);

        """
        for l in queries.split('\n'):
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
        DROP TABLE IF EXISTS `used_later10k`;
        DROP TABLE IF EXISTS `used_later100k`;
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
        q = """
        create table `used_later10k` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later10k` bigint(20)
        );"""
        self.c.execute(q)
        print("used_later10k recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        create table `used_later100k` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later100k` bigint(20)
        );"""
        self.c.execute(q)
        print("used_later100k recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """insert into used_later
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
        -- reduceDB is always present, used_later may not be, hence left join
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > rdb0.conflicts)
        , cl_last_in_solver

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.clauseID = rdb0.clauseID
        and cl_last_in_solver.conflicts > rdb0.conflicts

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        insert into used_later10k
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later10k`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl10k.used_at) as `used_later10k`

        FROM
        reduceDB as rdb0
        left join usedClauses as ucl10k

        -- reduceDB is always present, used_later may not be, hence left join
        on (ucl10k.clauseID = rdb0.clauseID
            and ucl10k.used_at > rdb0.conflicts
            and ucl10k.used_at <= (rdb0.conflicts+10000))
        , cl_last_in_solver

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.clauseID = rdb0.clauseID
        and cl_last_in_solver.conflicts > rdb0.conflicts + 10000

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later10k filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""
        insert into used_later100k
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later100k`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl100k.used_at) as `used_later100k`

        FROM
        reduceDB as rdb0
        left join usedClauses as ucl100k

        -- reduceDB is always present, used_later may not be, hence left join
        on (ucl100k.clauseID = rdb0.clauseID
            and ucl100k.used_at > rdb0.conflicts
            and ucl100k.used_at <= (rdb0.conflicts+100000))
        , cl_last_in_solver

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.clauseID = rdb0.clauseID
        and cl_last_in_solver.conflicts > rdb0.conflicts + 100000

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later100k filled T: %-3.2f s" % (time.time() - t))


        t = time.time()
        q = """
        drop index if exists `used_later_idx1`;
        drop index if exists `used_later_idx2`;
        drop index if exists `used_later10k_idx1`;
        drop index if exists `used_later10k_idx2`;
        drop index if exists `used_later100k_idx1`;
        drop index if exists `used_later100k_idx2`;

        create index `used_later_idx1` on `used_later` (`clauseID`, rdb0conflicts);
        create index `used_later_idx2` on `used_later` (`clauseID`, rdb0conflicts, used_later);
        create index `used_later10k_idx1` on `used_later10k` (`clauseID`, rdb0conflicts);
        create index `used_later10k_idx2` on `used_later10k` (`clauseID`, rdb0conflicts, used_later10k);
        create index `used_later100k_idx1` on `used_later100k` (`clauseID`, rdb0conflicts);
        create index `used_later100k_idx2` on `used_later100k` (`clauseID`, rdb0conflicts, used_later100k);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later indexes added T: %-3.2f s" % (time.time() - t))


class QueryCls (QueryHelper):
    def __init__(self, dbfname, conf):
        super(QueryCls, self).__init__(dbfname)
        self.conf = conf
        self.fill_sql_query()

    def get_columns(self, tablename):
        q="SELECT name FROM PRAGMA_TABLE_INFO('%s');" % tablename
        self.c.execute(q)
        rows = self.c.fetchall()
        columns = []
        for row in rows:
            if options.verbose:
                print("Using column in table {tablename}: {col}".format(
                    tablename=tablename
                    , col=row[0]))
            columns.append(row[0])

        return columns

    def query_fragment(self, tablename, not_cols, short_name):
        cols = self.get_columns(tablename)
        filtered_cols = list(set(cols).difference(not_cols))
        ret = ""
        for col in filtered_cols:
            ret += ", {short_name}.`{col}` as `{short_name}.{col}`\n".format(
                col=col
                , short_name=short_name)

        if options.verbose:
            print("query for short name {short_name}: {ret}".format(
                short_name=short_name
                , ret=ret))

        return ret

    def fill_sql_query(self):
        # sum_cl_use
        self.sum_cl_use = self.query_fragment("sum_cl_use", [], "sum_cl_use")

        # restart data
        not_cols =[
            "simplifications"
            , "restarts"
            , "conflicts"
            , "latest_satzilla_feature_calc"
            , "runtime"
            , "propagations"
            , "decisions"
            , "flipped"
            , "replaced"
            , "eliminated"
            , "set"
            , "clauseIDstartInclusive"
            , "clauseIDendExclusive"]
        self.restart_dat = self.query_fragment("restart", not_cols, "rst")

        # RDB data
        not_cols = [
            "simplifications"
            , "restarts"
            , "runtime"
            , "clauseID"
            , "glue"
            , "size"]
        self.rdb0_dat = self.query_fragment("reduceDB", not_cols, "rdb0")

        # clause data
        not_cols = [
            "simplifications"
            , "restarts"
            , "prev_restart"
            , "latest_satzilla_feature_calc"
            , "clauseID"]
        self.clause_dat = self.query_fragment("clauseStats", not_cols, "cl")


        not_cols = [
            "simplifications"
            , "restarts"
            , "latest_satzilla_feature_calc"
            , "irred_glue_distr_mean"
            , "irred_glue_distr_var"]
        self.satzfeat_dat = self.query_fragment("satzilla_features", not_cols, "szfeat")

        self.common_limits = """
        order by random()
        limit {limit}
        """

        # TODO magic queries
        if self.conf == 0:
            self.case_stmt_10k = """
            CASE WHEN

            sum_cl_use.last_confl_used > rdb0.conflicts and
            (
                -- useful in the next round
                   used_later10k.used_later10k > 3

                   or
                   (used_later10k.used_later10k > 2 and used_later100k.used_later100k > 40)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 1:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k > 5
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k >= max(cast({avg_used_later10k}+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 3:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k >= max(cast({avg_used_later10k}/2+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 4:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k >= max({median_used_later10k},1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        ############
        # LONG magic here
        ############
        if self.conf == 0:
            self.case_stmt_100k = """
            CASE WHEN

            sum_cl_use.last_confl_used > rdb0.conflicts+100000 and
            (
                -- used a lot over a wide range
                   (used_later100k.used_later100k > 10 and used_later.used_later > 20)

                -- used quite a bit but less dispersion
                or (used_later100k.used_later100k > 6 and used_later.used_later > 30)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 1:
            self.case_stmt_100k = """
            CASE WHEN

            sum_cl_use.last_confl_used > rdb0.conflicts+100000 and
            (
                -- used a lot over a wide range
                   (used_later100k.used_later100k > 13 and used_later.used_later > 24)

                -- used quite a bit but less dispersion
                or (used_later100k.used_later100k > 8 and used_later.used_later > 40)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_100k = """
            CASE WHEN

           -- useful in the next round
               used_later100k.used_later100k >= max(cast({avg_used_later100k}+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 3:
            self.case_stmt_100k = """
            CASE WHEN

           -- useful in the next round
               used_later100k.used_later100k >= max(cast({avg_used_later100k}/2+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 4:
            self.case_stmt_100k = """
            CASE WHEN

           -- useful in the next round
               used_later100k.used_later100k >= max({median_used_later100k}, 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """

        self.dump_no_larger_than_zero = """
        and rdb0.dump_no > 0
        """

        # GOOD clauses
        self.q_ok_select = """
        SELECT
        tags.tag as `fname`
        {clause_dat}
        {restart_dat}
        {satzfeat_dat_cur}
        {rdb0_dat}
        {rdb1_dat}
        {sum_cl_use}
        , sum_cl_use.num_used as `x.a_num_used`
        , `sum_cl_use`.`last_confl_used`-`cl`.`conflicts` as `x.a_lifetime`
        , {case_stmt}
        """

        self.q_ok = """
        FROM
        clauseStats as cl
        , sum_cl_use as sum_cl_use
        , restart as rst
        , satzilla_features as szfeat_cur
        , reduceDB as rdb0
        , reduceDB as rdb1
        , tags
        , used_later
        , used_later10k
        , used_later100k
        , cl_last_in_solver
        WHERE

        cl.clauseID = sum_cl_use.clauseID
        and cl.clauseID != 0
        and used_later.clauseID = cl.clauseID
        and used_later.rdb0conflicts = rdb0.conflicts

        and rdb0.clauseID = cl.clauseID
        and rdb1.clauseID = cl.clauseID
        and rdb0.dump_no = rdb1.dump_no+1

        and used_later10k.clauseID = cl.clauseID
        and used_later10k.rdb0conflicts = rdb0.conflicts

        and used_later100k.clauseID = cl.clauseID
        and used_later100k.rdb0conflicts = rdb0.conflicts

        -- to avoid missing clauses and their missing data to affect results
        and cl_last_in_solver.clauseID = cl.clauseID
        and rdb0.conflicts + 100000 > cl_last_in_solver.conflicts

        and cl.restarts > 1 -- to avoid history being invalid
        and szfeat_cur.latest_satzilla_feature_calc = rdb0.latest_satzilla_feature_calc
        and rst.restarts = cl.prev_restart
        and tags.tagname = "filename"
        """

        self.myformat = {
            "limit": 1000*1000*1000,
            "restart_dat": self.restart_dat,
            "clause_dat": self.clause_dat,
            "satzfeat_dat_cur": self.satzfeat_dat.replace("szfeat.", "szfeat_cur."),
            "rdb0_dat": self.rdb0_dat,
            "rdb1_dat": self.rdb0_dat.replace("rdb0", "rdb1"),
            "sum_cl_use": self.sum_cl_use
            }

    def add_dump_no_filter(self, q):
        q += self.dump_no_larger_than_zero
        return q

    def get_ok(self):
        t = time.time()

        # calc OK -> which can be both BAD and OK
        q = self.q_count + self.q_ok
        q = self.add_dump_no_filter(q)
        q = q.format(**self.myformat)
        if options.dump_sql:
            print("query:\n %s" % q)
        cur = self.conn.execute(q.format(**self.myformat))
        num_lines = int(cur.fetchone()[0])
        print("Num datapoints (K): %-3.5f T: %-3.1f" % ((num_lines/1000.0), time.time()-t))
        return num_lines

    def get_avg_used_later(self, long_or_short):
        cur = self.conn.cursor()
        q = "select avg(used_later10k) from used_later10k, used_later where used_later.clauseID = used_later10k.clauseID and used_later > 0;"
        if long_or_short == "long":
            q = q.replace("used_later10k", "used_later100k")
        cur.execute(q)
        rows = cur.fetchall()
        assert len(rows) == 1
        if rows[0][0] is None:
            return False, None

        avg = float(rows[0][0])
        print("%s avg used_later is: %.2f"  % (long_or_short, avg))
        return True, avg

    def get_median_used_later(self, long_or_short):
        cur = self.conn.cursor()
        q = """select used_later10k
            from used_later10k
            where used_later10k > 0
            order by used_later10k
            limit 1
            OFFSET (
            SELECT count(*) from used_later10k
            where used_later10k > 0
            ) / 2;"""
        if long_or_short == "long":
            q = q.replace("used_later10k", "used_later100k")
        cur.execute(q)
        rows = cur.fetchall()
        assert len(rows) <= 1
        if len(rows) == 0 or rows[0][0] is None:
            return False, None

        avg = float(rows[0][0])
        print("%s avg used_later is: %.2f"  % (long_or_short, avg))
        return True, avg

    def one_query(self, q, ok_or_bad):
        q = q.format(**self.myformat)
        q = "select * from ( %s ) where `x.class`='%s' " % (q, ok_or_bad)
        q += self.common_limits
        q = q.format(**self.myformat)

        t = time.time()
        sys.stdout.write("Running query for %s..." % ok_or_bad)
        sys.stdout.flush()
        if options.dump_sql:
            print("query:", q)
        df = pd.read_sql_query(q, self.conn)
        print("T: %-3.1f" % (time.time() - t))
        return df

    def get_one_data_all_dumpnos(self, long_or_short):
        df = None

        ok, df, this_fixed = self.get_data(long_or_short)
        if not ok:
            return False, None

        if options.verbose:
            print("Printing head:")
            print(df.head())
            print("Print head done.")

        return True, df


    def get_data(self, long_or_short, this_fixed=None):
        # TODO magic numbers: SHORT vs LONG data availability guess
        subformat = {}
        ok0, subformat["avg_used_later100k"] = self.get_avg_used_later("long");
        ok1, subformat["avg_used_later10k"] = self.get_avg_used_later("short");
        ok2, subformat["median_used_later100k"] = self.get_median_used_later("long");
        ok3, subformat["median_used_later10k"] = self.get_median_used_later("short");
        if not ok0 or not ok1 or not ok2 or not ok3:
            return False, None, None

        if long_or_short == "short":
            self.myformat["case_stmt"] = self.case_stmt_10k.format(**subformat)
            fixed_mult = 1.0
        else:
            self.myformat["case_stmt"] = self.case_stmt_100k.format(**subformat)
            fixed_mult = 0.2

        print("Fixed multiplier set to  %s " % fixed_mult)

        t = time.time()
        if this_fixed is None:
            this_fixed = options.fixed
            this_fixed *= fixed_mult
        print("this_fixed is set to:", this_fixed)

        q = self.q_ok_select + self.q_ok
        q = self.add_dump_no_filter(q)
        self.myformat["limit"] = this_fixed

        #get OK data
        df_ok = self.one_query(q, "OK")
        print("size of data:", df_ok.shape)

        #get BAD data
        df_bad = self.one_query(q, "BAD")
        print("size of data:", df_bad.shape)

        print("Queries finished. T: %-3.2f" % (time.time() - t))
        return True, pd.concat([df_ok, df_bad]), this_fixed


def transform(df):
    def check_clstat_row(self, row):
        if row[self.ntoc["cl.decision_level_hist"]] == 0 or \
                row[self.ntoc["cl.backtrack_level_hist"]] == 0 or \
                row[self.ntoc["cl.trail_depth_level_hist"]] == 0 or \
                row[self.ntoc["cl.size_hist"]] == 0 or \
                row[self.ntoc["cl.glue_hist"]] == 0 or \
                row[self.ntoc["cl.num_antecedents_hist"]] == 0:
            print("ERROR: Data is in error:", row)
            assert(False)
            exit(-1)

        return row

    # relative overlaps
    print("Relative overlaps...")
    df["cl.num_overlap_literals_rel"] = df["cl.num_overlap_literals"]/df["cl.antec_overlap_hist"]
    df["cl.antec_num_total_lits_rel"] = df["cl.num_total_lits_antecedents"]/df["cl.antec_sum_size_hist"]
    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"]/df["cl.num_antecedents_hist"]
    df["rst.varset_neg_polar_ratio"] = df["rst.varSetNeg"]/(df["rst.varSetPos"]+df["rst.varSetNeg"])

    # relative RDB
    print("Relative RDB...")
    df["rdb.rel_conflicts_made"] = (df["rdb0.conflicts_made"] > df["rdb1.conflicts_made"]).astype(int)
    df["rdb.rel_propagations_made"] = (df["rdb0.propagations_made"] > df["rdb1.propagations_made"]).astype(int)
    df["rdb.rel_clause_looked_at"] = (df["rdb0.clause_looked_at"] > df["rdb1.clause_looked_at"]).astype(int)
    df["rdb.rel_used_for_uip_creation"] = (df["rdb0.used_for_uip_creation"] > df["rdb1.used_for_uip_creation"]).astype(int)
    df["rdb.rel_last_touched_diff"] = (df["rdb0.last_touched_diff"] > df["rdb1.last_touched_diff"]).astype(int)
    df["rdb.rel_activity_rel"] = (df["rdb0.activity_rel"] > df["rdb1.activity_rel"]).astype(int)

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    print("size/glue/trail rel...")
    df["cl.size_rel"] = df["cl.size"] / df["cl.size_hist"]
    df["cl.glue_rel_queue"] = df["cl.glue"] / df["cl.glue_hist_queue"]
    df["cl.glue_rel_long"] = df["cl.glue"] / df["cl.glue_hist_long"]
    df["cl.glue_rel"] = df["cl.glue"] / df["cl.glue_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"]/df["cl.trail_depth_level_hist"]
    df["cl.branch_depth_rel_queue"] = df["cl.decision_level"]/df["cl.branch_depth_hist_queue"]

    # smaller-than larger-than for glue and size
    print("smaller-than larger-than for glue and size...")
    df["cl.size_smaller_than_hist"] = (df["cl.size"] < df["cl.size_hist"]).astype(int)
    df["cl.glue_smaller_than_hist"] = (df["cl.glue"] < df["cl.glue_hist"]).astype(int)
    df["cl.glue_smaller_than_hist_lt"] = (df["cl.glue"] < df["cl.glue_hist_long"]).astype(int)
    df["cl.glue_smaller_than_hist_queue"] = (df["cl.glue"] < df["cl.glue_hist_queue"]).astype(int)

    # relative decisions
    print("relative decisions...")
    df["cl.decision_level_rel"] = df["cl.decision_level"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre1_rel"] = df["cl.decision_level_pre1"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"]/df["cl.decision_level_hist"]

    # relative props
    print("relative props...")
    df["rst.all_props"] = df["rst.propBinRed"] + df["rst.propBinIrred"] + df["rst.propLongRed"] + df["rst.propLongIrred"]
    df["rst.propBinRed_ratio"] = df["rst.propBinRed"]/df["rst.all_props"]
    df["rst.propBinIrred_ratio"] = df["rst.propBinIrred"]/df["rst.all_props"]
    df["rst.propLongRed_ratio"] = df["rst.propLongRed"]/df["rst.all_props"]
    df["rst.propLongIrred_ratio"] = df["rst.propLongIrred"]/df["rst.all_props"]

    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"]/df["rst.free"]

    # relative resolutions
    print("relative resolutions...")
    df["rst.resolBinIrred_ratio"] = df["rst.resolBinIrred"]/df["rst.resolutions"]
    df["rst.resolBinRed_ratio"] = df["rst.resolBinRed"]/df["rst.resolutions"]
    df["rst.resolLRed_ratio"] = df["rst.resolLRed"]/df["rst.resolutions"]
    df["rst.resolLIrred_ratio"] = df["rst.resolLIrred"]/df["rst.resolutions"]

    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"] / df["cl.num_antecedents_hist"]
    df["cl.decision_level_rel"] = df["cl.decision_level"] / df["cl.decision_level_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"] / df["cl.trail_depth_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"] / df["cl.backtrack_level_hist"]

    # smaller-or-greater comparisons
    print("smaller-or-greater comparisons...")
    df["cl.decision_level_smaller_than_hist"] = (df["cl.decision_level"] < df["cl.decision_level_hist"]).astype(int)
    df["cl.backtrack_level_smaller_than_hist"] = (df["cl.backtrack_level"] < df["cl.backtrack_level_hist"]).astype(int)
    df["cl.trail_depth_level_smaller_than_hist"] = (df["cl.trail_depth_level"] < df["cl.trail_depth_level_hist"]).astype(int)
    df["cl.num_antecedents_smaller_than_hist"] = (df["cl.num_antecedents"] < df["cl.num_antecedents_hist"]).astype(int)
    df["cl.antec_sum_size_smaller_than_hist"] = (df["cl.antec_sum_size_hist"] < df["cl.num_total_lits_antecedents"]).astype(int)
    df["cl.antec_overlap_smaller_than_hist"] = (df["cl.antec_overlap_hist"] < df["cl.num_overlap_literals"]).astype(int)
    df["cl.overlap_smaller_than_hist"] = (df["cl.num_overlap_literals"]<df["cl.antec_overlap_hist"]).astype(int)
    df["cl.branch_smaller_than_hist_queue"] = (df["cl.decision_level"]<df["cl.branch_depth_hist_queue"]).astype(int)

    # avg conf/used_per confl
    print("avg conf/used_per confl 1...")
    # df["rdb0.avg_confl"] = df["rdb0.sum_uip1_used"]/df["rdb0.sum_delta_confl_uip1_used"]
    # df["rdb0.avg_confl"].fillna(0, inplace=True)

    print("avg conf/used_per confl 2...")
    # df["rdb0.used_per_confl"] = df["rdb0.sum_uip1_used"]/(df["rdb0.conflicts"] - df["cl.conflicts"])
    # df["rdb0.used_per_confl"].fillna(0, inplace=True)

    print("flatten/list...")
    old = set(df.columns.values.flatten().tolist())
    df = df.dropna(how="all")
    new = set(df.columns.values.flatten().tolist())
    if len(old - new) > 0:
        print("ERROR: a NaN number turned up")
        print("columns: ", (old - new))
        assert(False)
        exit(-1)

    # making sure "x.class" is the last one
    new_no_class = list(new)
    new_no_class.remove("x.class")
    df = df[new_no_class + ["x.class"]]

    return df


def dump_dataframe(df, name):
    if options.dump_csv:
        fname = "%s.csv" % name
        print("Dumping CSV data to:", fname)
        df.to_csv(fname, index=False, columns=sorted(list(df)))

    fname = "%s.dat" % name
    print("Dumping pandas data to:", fname)
    with open(fname, "wb") as f:
        pickle.dump(df, f)


def one_database(dbfname):
    with QueryFill(dbfname) as q:
        q.measure_size()
        if not options.no_recreate_indexes:
            q.create_indexes()
            q.fill_later_useful_data()

    match = re.match(r"^([0-9]*)-([0-9]*)$", options.confs)
    if not match:
        print("ERROR: we cannot parse your config options: '%s'" % options.confs)
        exit(-1)

    conf_from = int(match.group(1))
    conf_to = int(match.group(2))+1
    if conf_to <= conf_from:
        print("ERROR: Conf range is not increasing")
        exit(-1)

    print("Running configs:", range(conf_from, conf_to))
    print("Using sqlite3db file %s" % dbfname)
    for long_or_short in ["long", "short"]:
        for conf in range(conf_from, conf_to):
            print("------> Doing config {conf} -- {long_or_short}".format(
                conf=conf, long_or_short=long_or_short))

            with QueryCls(dbfname, conf) as q:
                ok, df = q.get_one_data_all_dumpnos(long_or_short)

            if not ok:
                print("-> Skipping file {file} config {conf} {ls}".format(
                    conf=conf, file=dbfname, ls=long_or_short))
                continue

            if options.verbose:
                print("Describing----")
                dat = df.describe()
                print(dat)
                print("Describe done.---")
                print("Features: ", df.columns.values.flatten().tolist())

            print("Transforming...")
            df = transform(df)

            if options.verbose:
                print("Describing post-transform ----")
                print(df.describe())
                print("Describe done.---")
                print("Features: ", df.columns.values.flatten().tolist())

            cleanname = re.sub(r'\.cnf.gz.sqlite$', '', dbfname)
            cleanname = re.sub(r'\.db$', '', dbfname)
            cleanname = re.sub(r'\.sqlitedb$', '', dbfname)
            cleanname = "{cleanname}-{long_or_short}-conf-{conf}".format(
                cleanname=cleanname,
                long_or_short=long_or_short,
                conf=conf)

            dump_dataframe(df, cleanname)


if __name__ == "__main__":
    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--csv", action="store_true", default=False,
                      dest="dump_csv", help="Dump CSV (for weka)")

    parser.add_option("--sql", action="store_true", default=False,
                      dest="dump_sql", help="Dump SQL queries")

    parser.add_option("--fixed", default=20000, type=int,
                      dest="fixed", help="Exact number of examples to take. -1 is to take all. Default: %default")

    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    parser.add_option("--confs", default="0-0", type=str,
                      dest="confs", help="Configs to generate. Default: %default")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)

    np.random.seed(2097483)
    for dbfname in args:
        print("----- FILE %s -------" % dbfname)
        one_database(dbfname)

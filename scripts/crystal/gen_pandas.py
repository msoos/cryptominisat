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
import helper

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
        ret = self.c.execute("select count() from clause_stats")
        rows = self.c.fetchall()
        clss_rows = rows[0][0]
        print("We have %d lines of clause_stats" % (clss_rows))

    def create_indexes(self):
        helper.drop_idxs(self.c)

        t = time.time()
        print("Recreating indexes...")
        queries = """
        create index `idxclid33` on `sum_cl_use` (`clauseID`, `last_confl_used`);
        create index `idxclid1` on `clause_stats` (`clauseID`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid1-2` on `clause_stats` (`clauseID`);
        create index `idxclid1-3` on `clause_stats` (`clauseID`, restarts);
        create index `idxclid1-4` on `clause_stats` (`clauseID`, restarts, prev_restart);
        create index `idxclid1-5` on `clause_stats` (`clauseID`, prev_restart);
        create index `idxclid2` on `clause_stats` (clauseID, `prev_restart`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid4` on `restart` ( `restarts`);
        create index `idxclid88` on `restart_dat_for_cl` ( `conflicts`);
        create index `idxclid5` on `tags` ( `name`);
        create index `idxclid6` on `reduceDB` (`clauseID`, conflicts, latest_satzilla_feature_calc);
        create index `idxclid6-2` on `reduceDB` (`clauseID`, `dump_no`);
        create index `idxclid6-3` on `reduceDB` (`clauseID`, `conflicts`, `dump_no`);
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        create index `idxclid7` on `satzilla_features` (`latest_satzilla_feature_calc`);
        create index `idxclidUCLS-1` on `used_clauses` ( `clauseID`, `used_at`);
        create index `idxclidUCLS-2` on `used_clauses` ( `used_at`);
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
        DROP TABLE IF EXISTS `used_later_short`;
        DROP TABLE IF EXISTS `used_later_long`;
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
        left join used_clauses as ucl

        -- for any point later than now
        -- reduceDB is always present, used_later may not be, hence left join
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > rdb0.conflicts)
        , cl_last_in_solver

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.clauseID = rdb0.clauseID
        and cl_last_in_solver.conflicts >= rdb0.conflicts

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later filled T: %-3.2f s" % (time.time() - t))


        q_create = """
        create table `used_later_{name}` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later_{name}` bigint(20)
        );"""

        q_fill = """
        insert into used_later_{name}
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later_{name}`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl.used_at) as `used_later_{name}`

        FROM
        reduceDB as rdb0
        left join used_clauses as ucl

        -- reduceDB is always present, used_later may not be, hence left join
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > rdb0.conflicts
            and ucl.used_at <= (rdb0.conflicts+{duration}))
        , cl_last_in_solver

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.clauseID = rdb0.clauseID
        and cl_last_in_solver.conflicts >= rdb0.conflicts + {duration}

        group by rdb0.clauseID, rdb0.conflicts;"""

        idx = """
        create index `used_later_{name}_idx1` on `used_later_{name}` (`clauseID`, rdb0conflicts);
        create index `used_later_{name}_idx2` on `used_later_{name}` (`clauseID`, rdb0conflicts, used_later_{name});"""

        t = time.time()
        idxs = ""
        for name in ["short", "long"]:
            self.c.execute(q_create.format(name=name))
            self.c.execute(q_fill.format(name=name, duration=getattr(options, name)))
            idxs += idx.format(name=name)
        print("used_later recreated T: %-3.2f s" % (time.time() - t))


        t = time.time()
        idxs += """
        create index `used_later_idx1` on `used_later` (`clauseID`, rdb0conflicts);
        create index `used_later_idx2` on `used_later` (`clauseID`, rdb0conflicts, used_later);
        """
        for l in idxs.split('\n'):
            self.c.execute(l)
        print("used_later indexes added T: %-3.2f s" % (time.time() - t))


class QueryCls (QueryHelper):
    def __init__(self, dbfname, conf):
        super(QueryCls, self).__init__(dbfname)
        self.conf = conf
        self.fill_sql_query()

    def fill_sql_query(self):
        # sum_cl_use
        self.sum_cl_use = helper.query_fragment(
            "sum_cl_use", [], "sum_cl_use", options.verbose, self.c)

        # restart data
        not_cols = [
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
            , "free"
            , "numredLits"
            , "numIrredLits"
            , "all_props"
            , "clauseIDstartInclusive"
            , "clauseIDendExclusive"]
        self.rst_cur = helper.query_fragment(
            "restart_dat_for_cl", not_cols, "rst_cur", options.verbose, self.c)

        # RDB data
        not_cols = [
            "simplifications"
            , "restarts"
            , "conflicts"
            , "latest_satzilla_feature_calc"
            , "runtime"
            , "clauseID"
            , "glue"
            , "size"
            , "in_xor"
            , "locked"
            , "activity_rel"]
        self.rdb0_dat = helper.query_fragment(
            "reduceDB", not_cols, "rdb0", options.verbose, self.c)

        # clause data
        not_cols = [
            "simplifications"
            , "restarts"
            , "prev_restart"
            , "latest_satzilla_feature_calc"
            , "antecedents_long_red_age_max"
            , "antecedents_long_red_age_min"
            , "clauseID"]
        self.clause_dat = helper.query_fragment(
            "clause_stats", not_cols, "cl", options.verbose, self.c)

        # satzilla data
        not_cols = [
            "simplifications"
            , "restarts"
            , "conflicts"
            , "latest_satzilla_feature_calc"
            , "irred_glue_distr_mean"
            , "irred_glue_distr_var"]
        self.satzfeat_dat = helper.query_fragment(
            "satzilla_features", not_cols, "szfeat", options.verbose, self.c)

        self.common_limits = """
        order by random()
        limit {limit}
        """

        ############
        # SHORT magic here
        ############
        if self.conf == 0:
            self.case_stmt_short = """
            CASE WHEN

            sum_cl_use.last_confl_used > rdb0.conflicts and
            (
                -- useful in the next round
                   used_later_short.used_later_short > 3

                   or
                   (used_later_short.used_later_short > 2 and used_later_long.used_later_long > 40)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 1:
            self.case_stmt_short = """
            CASE WHEN

            -- useful in the next round
                   used_later_short.used_later_short > 5
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_short = """
            CASE WHEN

            -- useful in the next round
                   used_later_short.used_later_short >= max(cast({avg_used_later_short}+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 3:
            self.case_stmt_short = """
            CASE WHEN

            -- useful in the next round
                   used_later_short.used_later_short >= max(cast({avg_used_later_short}/2+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 4:
            self.case_stmt_short = """
            CASE WHEN

            -- useful in the next round
                   used_later_short.used_later_short >= max({median_used_later_short},1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        ############
        # LONG magic here
        ############
        if self.conf == 0:
            self.case_stmt_long = """
            CASE WHEN

            sum_cl_use.last_confl_used > rdb0.conflicts+{long_duration} and
            (
                -- used a lot over a wide range
                   (used_later_long.used_later_long > 10 and used_later.used_later > 20)

                -- used quite a bit but less dispersion
                or (used_later_long.used_later_long > 6 and used_later.used_later > 30)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """.format(long_duration=options.long_duration)
        elif self.conf == 1:
            self.case_stmt_long = """
            CASE WHEN

            sum_cl_use.last_confl_used > rdb0.conflicts+{long_duration} and
            (
                -- used a lot over a wide range
                   (used_later_long.used_later_long > 13 and used_later.used_later > 24)

                -- used quite a bit but less dispersion
                or (used_later_long.used_later_long > 8 and used_later.used_later > 40)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """.format(long_duration=options.long_duration)
        elif self.conf == 2:
            self.case_stmt_long = """
            CASE WHEN

           -- useful in the next round
               used_later_long.used_later_long >= max(cast({avg_used_later_long}+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 3:
            self.case_stmt_long = """
            CASE WHEN

           -- useful in the next round
               used_later_long.used_later_long >= max(cast({avg_used_later_long}/2+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 4:
            self.case_stmt_long = """
            CASE WHEN

           -- useful in the next round
               used_later_long.used_later_long >= max({median_used_later_long}, 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """

        # GOOD clauses
        self.q_select = """
        SELECT
        tags.val as `fname`
        {clause_dat}
        {rst_cur}
        {satzfeat_dat_cur}
        {rdb0_dat}
        {rdb1_dat}
        {sum_cl_use}
        , (rdb0.conflicts - cl.conflicts) as `cl.time_inside_solver`
        , sum_cl_use.num_used as `x.a_num_used`
        , `sum_cl_use`.`last_confl_used`-`cl`.`conflicts` as `x.a_lifetime`
        , {case_stmt}

        FROM
        clause_stats as cl
        , sum_cl_use as sum_cl_use
        , restart_dat_for_cl as rst_cur
        , satzilla_features as szfeat_cur
        , reduceDB as rdb0
        , reduceDB as rdb1
        , tags
        , used_later
        , used_later_short
        , used_later_long
        , cl_last_in_solver

        WHERE
        cl.clauseID = sum_cl_use.clauseID
        and cl.clauseID != 0
        and used_later.clauseID = cl.clauseID
        and used_later.rdb0conflicts = rdb0.conflicts

        and rdb0.clauseID = cl.clauseID
        and rdb1.clauseID = cl.clauseID
        and rdb0.dump_no = rdb1.dump_no+1

        and rst_cur.conflicts = cl.conflicts

        and used_later_short.clauseID = cl.clauseID
        and used_later_short.rdb0conflicts = rdb0.conflicts

        and used_later_long.clauseID = cl.clauseID
        and used_later_long.rdb0conflicts = rdb0.conflicts

        -- to avoid missing clauses and their missing data to affect results
        and cl_last_in_solver.clauseID = cl.clauseID
        and rdb0.conflicts + {del_at_least} <= cl_last_in_solver.conflicts

        and cl.restarts > 1 -- to avoid history being invalid
        and szfeat_cur.latest_satzilla_feature_calc = rdb0.latest_satzilla_feature_calc
        and tags.name = "filename"
        """

        self.myformat = {
            "limit": 1000*1000*1000,
            "clause_dat": self.clause_dat,
            "satzfeat_dat_cur": self.satzfeat_dat.replace("szfeat.", "szfeat_cur."),
            "rdb0_dat": self.rdb0_dat,
            "rdb1_dat": self.rdb0_dat.replace("rdb0", "rdb1"),
            "sum_cl_use": self.sum_cl_use,
            "rst_cur": self.rst_cur
        }

    def get_avg_used_later(self, long_or_short):
        cur = self.conn.cursor()
        q = """
        select avg(used_later_short)
        from used_later_short, used_later
        where
        used_later.clauseID = used_later_short.clauseID
        and used_later > 0;
        """
        if long_or_short == "long":
            q = q.replace("short", "long")
        cur.execute(q)
        rows = cur.fetchall()
        assert len(rows) == 1
        if rows[0][0] is None:
            print("ERROR: No data for avg generation, not a single line for '%s'"
                  % long_or_short)
            return False, None

        avg = float(rows[0][0])
        print("%s avg used_later is: %.2f" % (long_or_short, avg))
        return True, avg

    def get_median_used_later(self, long_or_short):
        cur = self.conn.cursor()
        q = """select used_later_short
            from used_later_short
            where used_later_short > 0
            order by used_later_short
            limit 1
            OFFSET (
            SELECT count(*) from used_later_short
            where used_later_short > 0
            ) / 2;"""
        if long_or_short == "long":
            q = q.replace("short", "long")
        cur.execute(q)
        rows = cur.fetchall()
        assert len(rows) <= 1
        if len(rows) == 0 or rows[0][0] is None:
            print("ERROR: No data for median generation, not a single line for '%s'"
                  % long_or_short)
            return False, None

        avg = float(rows[0][0])
        print("%s median used_later is: %.2f" % (long_or_short, avg))
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

        ok, df, this_limit = self.get_data(long_or_short)
        if not ok:
            return False, None

        if options.verbose:
            print("Printing head:")
            print(df.head())
            print("Print head done.")

        return True, df

    def get_data(self, long_or_short, this_limit=None):
        # TODO magic numbers: SHORT vs LONG data availability guess
        subformat = {}
        ok0, subformat["avg_used_later_long"] = self.get_avg_used_later("long")
        ok1, subformat["avg_used_later_short"] = self.get_avg_used_later(
            "short")
        ok2, subformat["median_used_later_long"] = self.get_median_used_later(
            "long")
        ok3, subformat["median_used_later_short"] = self.get_median_used_later(
            "short")
        if not ok0 or not ok1 or not ok2 or not ok3:
            return False, None, None

        if long_or_short == "short":
            self.myformat["case_stmt"] = self.case_stmt_short.format(
                **subformat)
            self.myformat["del_at_least"] = options.short_duration
            fixed_mult = 1.0
        else:
            self.myformat["case_stmt"] = self.case_stmt_long.format(
                **subformat)
            self.myformat["del_at_least"] = options.long_duration
            fixed_mult = options.fixed_mult_long

        print("Fixed multiplier set to  %s " % fixed_mult)

        t = time.time()
        if this_limit is None:
            this_limit = options.limit
            this_limit *= fixed_mult
        print("this_limit is set to:", this_limit)

        q = str(self.q_select)

        #get OK data
        df = {}
        for type_data in ["OK", "BAD"]:
            df_parts = []

            self.myformat["limit"] = int(this_limit/2)
            extra = " and rdb0.dump_no = 1 "
            df_parts.append(self.one_query(q + extra, type_data))
            print("shape for %s: %s" % (extra, df_parts[0].shape))

            self.myformat["limit"] = int(this_limit/3)
            extra = " and rdb0.dump_no = 2 "
            df_parts.append(self.one_query(q + extra, type_data))
            print("shape for %s: %s" % (extra, df_parts[1].shape))

            self.myformat["limit"] = int(this_limit/4)
            extra = " and rdb0.dump_no > 2 "
            df_parts.append(self.one_query(q + extra, type_data))
            print("shape for %s: %s" % (extra, df_parts[2].shape))

            df[type_data] = pd.concat(df_parts)
            print("size of {type} data: {size}".format(
                type=type_data, size=df[type_data].shape))

        print("Queries finished. T: %-3.2f" % (time.time() - t))
        return True, pd.concat([df["OK"], df["BAD"]]), this_limit


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

    # verbosity level
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--sql", action="store_true", default=False,
                      dest="dump_sql", help="Dump SQL queries")
    parser.add_option("--csv", action="store_true", default=False,
                      dest="dump_csv", help="Dump CSV (for weka)")

    # limits
    parser.add_option("--limit", default=20000, type=int,
                      dest="limit", help="Exact number of examples to take. -1 is to take all. Default: %default")
    parser.add_option("--limitlongmult", default="0.2", type=float,
                      dest="fixed_mult_long", help="Use this much less samples for long")

    # debugging is faster with this
    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    # configs
    parser.add_option("--confs", default="2-2", type=str,
                      dest="confs", help="Configs to generate. Default: %default")

    # lengths of short/long
    parser.add_option("--short", default="10000", type=str,
                      dest="short", help="Short duration. Default: %default")
    parser.add_option("--long", default="50000", type=str,
                      dest="long", help="Long duration. Default: %default")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)

    np.random.seed(2097483)
    for dbfname in args:
        print("----- FILE %s -------" % dbfname)
        one_database(dbfname)

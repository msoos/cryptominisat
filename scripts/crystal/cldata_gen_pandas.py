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


class QueryAddIdxes (helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryAddIdxes, self).__init__(dbfname)

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
        t = time.time()
        print("Recreating indexes...")
        queries = """
        create index `idxclid33` on `sum_cl_use` (`clauseID`, `last_confl_used`);
        ---
        create index `idxclid1` on `clause_stats` (`clauseID`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid1-2` on `clause_stats` (`clauseID`);
        create index `idxclid1-3` on `clause_stats` (`clauseID`, restarts);
        create index `idxclid1-4` on `clause_stats` (`clauseID`, restarts, prev_restart);
        create index `idxclid1-5` on `clause_stats` (`clauseID`, prev_restart);
        create index `idxclid2` on `clause_stats` (clauseID, `prev_restart`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid4` on `restart` ( `restarts`);
        create index `idxclid88` on `restart_dat_for_cl` ( `clauseID`);
        create index `idxclid5` on `tags` ( `name`);
        ---
        create index `idxclid6` on `reduceDB` (`clauseID`, conflicts, latest_satzilla_feature_calc);
        create index `idxclid6-2` on `reduceDB` (`clauseID`, `dump_no`);
        create index `idxclid6-3` on `reduceDB` (`clauseID`, `conflicts`, `dump_no`);
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        ---
        create index `idxclid7` on `satzilla_features` (`latest_satzilla_feature_calc`);
        ---
        create index `idxclidUCLS-1` on `used_clauses` ( `clauseID`, `used_at`);
        create index `idxclidUCLS-2` on `used_clauses` ( `used_at`);
        ---
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


class QueryCls (helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryCls, self).__init__(dbfname)
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
            , "clauseID"
            , "restartID"]
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

        # final big query
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
        , `sum_cl_use`.`last_confl_used`-`cl`.`conflicts` as `x.a_lifetime`
        , used_later_short.used_later_short as `x.used_later_short`
        , used_later_long.used_later_long as `x.used_later_long`
        , used_later_forever.used_later_forever as `x.used_later_forever`
        , sum_cl_use.num_used as `x.sum_cl_use`

        FROM
        reduceDB as rdb0
        -- WARN: ternary clauses are explicity NOT enabled here
        --       since it's a FULL join
        join clause_stats as cl on
            cl.clauseID = rdb0.clauseID

        -- WARN: ternary clauses are explicity NOT enabled here
        --       since it's a FULL join
        join restart_dat_for_cl as rst_cur
            on rst_cur.clauseID = rdb0.clauseID

        join sum_cl_use on
            sum_cl_use.clauseID = rdb0.clauseID

        join reduceDB as rdb1 on
            rdb1.clauseID = rdb0.clauseID

        join satzilla_features as szfeat_cur
            on szfeat_cur.latest_satzilla_feature_calc = cl.latest_satzilla_feature_calc

        join used_later_short on
            used_later_short.clauseID = rdb0.clauseID
            and used_later_short.rdb0conflicts = rdb0.conflicts

        join used_later_long on
            used_later_long.clauseID = rdb0.clauseID
            and used_later_long.rdb0conflicts = rdb0.conflicts

        join used_later_forever on
            used_later_forever.clauseID = rdb0.clauseID
            and used_later_forever.rdb0conflicts = rdb0.conflicts

        join used_later on
            used_later.clauseID = rdb0.clauseID
            and used_later.rdb0conflicts = rdb0.conflicts


        join cl_last_in_solver on
            cl_last_in_solver.clauseID = rdb0.clauseID

        , tags

        WHERE
        cl.clauseID != 0
        and tags.name = "filename"
        and rdb0.dump_no = rdb1.dump_no+1
        and used_later_long.offset = 0
        and used_later_short.offset = 0
        and used_later_forever.offset = 0
        -- and used_later_short_offset.offset = {offset_short}


        -- to avoid missing clauses and their missing data to affect results
        and rdb0.conflicts + {del_at_least} <= cl_last_in_solver.conflicts
        """

        self.myformat = {
            "limit": 1000*1000*1000,
            "clause_dat": self.clause_dat,
            "satzfeat_dat_cur": self.satzfeat_dat.replace("szfeat.", "szfeat_cur."),
            "rdb0_dat": self.rdb0_dat,
            "rdb1_dat": self.rdb0_dat.replace("rdb0", "rdb1"),
            "sum_cl_use": self.sum_cl_use,
            "rst_cur": self.rst_cur,
            "offset_short" : options.short,
            "offset_long" : options.long
        }

    def get_used_later_percentiles(self, name):
        cur = self.conn.cursor()
        q = """
        select
            `type_of_dat`,
            `percentile_descr`,
            `val`
        from used_later_percentiles
        where `type_of_dat` = '{name}'
        """.format(name=name)
        cur.execute(q)
        rows = cur.fetchall()
        lookup = {}
        for row in rows:
            lookup[row[1]] = row[2]
        #print("perc lookup:", lookup)

        return lookup

    def get_one_data_all_dumpnos(self, tier):
        df = None

        ok, df, this_limit = self.get_data(tier)
        if not ok:
            return False, None

        if options.verbose:
            print("Printing head:")
            print(df.head())
            print("Print head done.")

        return True, df


    def get_data(self, tier):
        perc = self.get_used_later_percentiles("_"+tier)

        if tier == "short":
            self.myformat["del_at_least"] = options.short

        elif tier == "long":
            self.myformat["del_at_least"] = options.long

        elif tier == "forever":
            self.myformat["del_at_least"] = 0

        # Make sure these stratas are equally represented
        t = time.time()
        limit = options.limit
        limit, _ = self.run_stratified_queries(limit, perc, tier)
        print("Setting limit to minimum of all of above weighted sizes:", limit)
        print("** Running final queries")
        _, dfs = self.run_stratified_queries(limit, perc, tier)

        data = pd.concat(dfs)
        print("** Queries finished. Total size: %s  -- T: %-3.2f" % (
            data.shape, time.time() - t))
        return True, data , limit

    def run_stratified_queries(self, limit, perc, tier):
        dfs = []
        # NOTE: these are NON-ZERO percentages, but we replace 100 with "0", so the LAST chunk contains ALL, including 0, which is a large part of the data
        for beg_perc, end_perc in [(0, 5), (5, 40), (40, 80), (80, 100)]:
            beg = perc["top_non_zero_{perc}_perc".format(perc=beg_perc)]
            if end_perc == 100:
                end = 0
            else:
                end = perc["top_non_zero_{perc}_perc".format(perc=end_perc)]
            what_to_strata = "`x.used_later_{tier}`".format(tier=tier)

            print("Limit is currently: {limit} value strata perc: ({a}, {b}) translates to value strata ({beg}, {end})".format(
                limit=limit, a=beg_perc, b=end_perc, beg=beg, end=end))

            df, weighted_size = self.query_strata_per_dumpno(
                str(self.q_select),
                limit,
                what_to_strata=what_to_strata,
                strata=(beg,end))
            dfs.append(df)
            # HACK below!
            #limit = int(min(weighted_size))
        return limit, dfs

    def query_strata_per_dumpno(self, q, limit, what_to_strata, strata):
        print("-> Getting one set of data with limit %s" % limit)
        weighted_size = []
        df_parts = []

        def one_part(mult, extra):
            self.myformat["limit"] = int(limit*mult)
            df_parts.append(self.one_query(q + extra, what_to_strata, strata))
            print("--> Num rows for strata %s -- '%s': %s" % (strata, extra, df_parts[-1].shape[0]))

            ws = df_parts[-1].shape[0]/mult
            print("--> The weight was %f so wegthed size is: %d" % (mult, int(ws)))
            weighted_size.append(ws)

        one_part(1/2.0, " and rdb0.dump_no = 1 ")
        one_part(1/4.0, " and rdb0.dump_no = 2 ")
        one_part(1/4.0, " and rdb0.dump_no > 2 ")

        df = pd.concat(df_parts)
        print("-> size of all dump_no-s, strata {strata} data: {size}".format(
            strata=strata, size=df.shape))

        return df, weighted_size

    def one_query(self, q, what_to_strata, strata):
        q = q.format(**self.myformat)
        q = """
        select * from ( {q} )
        where
        {what_to_strata} <= {beg}
        and {what_to_strata} >= {end}""".format(
            q=q,
            what_to_strata=what_to_strata,
            beg=strata[0],
            end=strata[1])

        q += self.common_limits
        q = q.format(**self.myformat)

        t = time.time()
        sys.stdout.write("-> Running query for {} stratas {}...".format(what_to_strata, strata))
        sys.stdout.flush()
        if options.dump_sql:
            print("query:", q)
        df = pd.read_sql_query(q, self.conn)
        print("T: %-3.1f" % (time.time() - t))
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
    with QueryAddIdxes(dbfname) as q:
        q.measure_size()
        helper.drop_idxs(q.c)
        q.create_indexes()

    with helper.QueryFill(dbfname) as q:
        q.delete_and_create_all()
        q.fill_used_later()
        q.fill_used_later_X("long", offset=0, duration=options.long)
        q.fill_used_later_X("short", offset=0, duration=options.short)
        q.fill_used_later_X("forever", offset=0, duration=(1000*1000*1000), forever=True)

        q.fill_used_later_X("long", offset=options.long, duration=options.long)
        q.fill_used_later_X("short", offset=options.short, duration=options.short)

    print("Using sqlite3 DB file %s" % dbfname)
    for tier in ["short", "long", "forever"]:
        print("------> Doing tier {tier}".format(tier=tier))

        with QueryCls(dbfname) as q:
            ok, df = q.get_one_data_all_dumpnos(tier)

        if not ok:
            print("-> Skipping file {file} {tier}".format(
                file=dbfname, tier=tier))
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
        cleanname = "{cleanname}-cldata-{tier}".format(
            cleanname=cleanname,
            tier=tier)

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

    # debugging is faster with this
    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    # lengths of short/long
    parser.add_option("--short", default="10000", type=str,
                      dest="short", help="Short duration. Default: %default")
    parser.add_option("--long", default="50000", type=str,
                      dest="long", help="Long duration. Default: %default")

    (options, args) = parser.parse_args()

    if len(args) != 1:
        print("ERROR: You must give exactly one file")
        exit(-1)

    np.random.seed(2097483)
    one_database(args[0])

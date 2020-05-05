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

        ############
        # Labeling SHORT
        ############
        if self.conf == 1:
            self.case_stmt_short = """
            CASE WHEN

            -- useful in the next round
            used_later_short.used_later_short >= {name_short_top_non_zero_X_perc}

            THEN 1
            ELSE 0
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_short = """
            CASE WHEN

            -- useful in the next round
            (used_later_short.used_later_short >= {name_short_top_non_zero_X_perc}
            or used_later.used_later >= ({name_top_non_zero_15_perc}))

            THEN 1
            ELSE 0
            END AS `x.class`
            """
        else:
            assert(False)

        ############
        # Labeling LONG
        ############
        if self.conf == 1:
            self.case_stmt_long = """
            CASE WHEN

            -- useful in the next round
            used_later_long.used_later_long >= {name_long_top_non_zero_X_perc}

            THEN 1
            ELSE 0
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_long = """
            CASE WHEN

            -- useful in the next round
            (used_later_long.used_later_long >= {name_long_top_non_zero_X_perc}
            or used_later.used_later >= ({name_top_non_zero_10_perc}))

            THEN 1
            ELSE 0
            END AS `x.class`
            """
        else:
            assert(False)

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
        , sum_cl_use.num_used as `x.a_num_used`
        , `sum_cl_use`.`last_confl_used`-`cl`.`conflicts` as `x.a_lifetime`
        , {case_stmt}

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

        -- join used_later_short as `used_later_short_offset` on
            -- used_later_short_offset.clauseID = rdb0.clauseID
            -- and used_later_short_offset.rdb0conflicts = rdb0.conflicts

        join used_later_long on
            used_later_long.clauseID = rdb0.clauseID
            and used_later_long.rdb0conflicts = rdb0.conflicts

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
            #print("row:", row)
            lookup["name"+row[0]+"_"+row[1]] = row[2]
        #print("rows:", rows)

        return lookup

    def one_query(self, q, ok_or_bad):
        q = q.format(**self.myformat)
        q = "select * from ( %s ) where `x.class` = %d " % (q, ok_or_bad)
        q += self.common_limits
        q = q.format(**self.myformat)

        t = time.time()
        sys.stdout.write("Running query for %d..." % ok_or_bad)
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

    def one_set_of_data(self, q, limit):
        print("Getting one set of data with limit %s" % limit)
        dfs = {}
        weighted_size = []
        for type_data in [1, 0]:
            df_parts = []

            def one_part(mult, extra):
                self.myformat["limit"] = int(limit*mult)
                df_parts.append(self.one_query(q + extra, type_data))
                print("-> Num rows for %s -- '%s': %s" % (type_data, extra, df_parts[-1].shape[0]))

                ws = df_parts[-1].shape[0]/mult
                print("--> The weight was %f so wegthed size is: %d" % (mult, int(ws)))
                weighted_size.append(ws)

            one_part(1/2.0, " and rdb0.dump_no = 1 ")
            one_part(1/8.0, " and rdb0.dump_no = 2 ")
            one_part(1/8.0, " and rdb0.dump_no > 2 ")

            dfs[type_data] = pd.concat(df_parts)
            print("size of {t} data: {size}".format(
                t=type_data, size=dfs[type_data].shape))

        return dfs, weighted_size


    def get_data(self, long_or_short):
        # TODO magic numbers: SHORT vs LONG data availability guess
        subformat = {}
        x = self.get_used_later_percentiles("_short")
        for a,b in x.items():
            subformat[a] = b
        x = self.get_used_later_percentiles("_long")
        for a,b in x.items():
            subformat[a] = b
        x = self.get_used_later_percentiles("")
        for a,b in x.items():
            subformat[a] = b

        subformat["name_short_top_non_zero_X_perc"] = subformat[
            "name_short_top_non_zero_%d_perc" % options.top_percentile_short]
        subformat["name_long_top_non_zero_X_perc"] = subformat[
            "name_long_top_non_zero_%d_perc" % options.top_percentile_long]
        subformat["name_top_non_zero_X_perc"] = subformat[
            "name_top_non_zero_%d_perc" % options.top_percentile_long]

        if long_or_short == "short":
            self.myformat["del_at_least"] = options.short
            self.myformat["case_stmt"] = self.case_stmt_short.format(
                **subformat)
        else:
            self.myformat["del_at_least"] = options.long
            self.myformat["case_stmt"] = self.case_stmt_long.format(
                **subformat)

        t = time.time()
        limit = options.limit
        print("Limit is originally:", limit)
        _, weighted_size = self.one_set_of_data(str(self.q_select), limit)
        limit = int(min(weighted_size))
        print("Setting limit to minimum of all of above weighted sizes:", limit)
        df, weighted_size = self.one_set_of_data(str(self.q_select), limit)

        print("Queries finished. T: %-3.2f" % (time.time() - t))
        return True, pd.concat([df[1], df[0]]), limit


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
        q.fill_used_later_X("long", offset=options.long, duration=options.long)
        q.fill_used_later_X("short", offset=options.short, duration=options.short)

    conf_from, conf_to = helper.parse_configs(options.confs)
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
            cleanname = "{cleanname}-cldata-{long_or_short}-conf-{conf}-pshort{percshort}-plong{perclong}".format(
                cleanname=cleanname,
                long_or_short=long_or_short,
                conf=conf,
                percshort=options.top_percentile_short,
                perclong=options.top_percentile_long)

            # some cleanup, stats
            #df["fname"] = df["fname"].astype("category")
            #df["cl.cur_restart_type"] = df["cl.cur_restart_type"].astype("category")
            #df["rdb0.cur_restart_type"] = df["rdb0.cur_restart_type"].astype("category")
            #df["rdb1.cur_restart_type"] = df["rdb1.cur_restart_type"].astype("category")

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
    parser.add_option("--toppercentileshort", type=int, default=50,
                      dest="top_percentile_short", help="Top percentile of short predictor to mark KEEP")
    parser.add_option("--toppercentilelong", type=int, default=40,
                      dest="top_percentile_long", help="Top percentile of long predictor to mark KEEP")

    # limits
    parser.add_option("--limit", default=20000, type=int,
                      dest="limit", help="Exact number of examples to take. -1 is to take all. Default: %default")

    # debugging is faster with this
    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    # configs
    parser.add_option("--confs", default="1-1", type=str,
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

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

# Makes the training frames from the sampled DB (data-min.db): one
# pandas pickle per (label table, tier), rows = (clause, reduce) with the
# clause's state at the reduce, the DB-wide numbers of that reduce, what
# was known when the clause was learnt, and the labels (count, rank).
# The rows are the ones sample_data.py picked: frame_rows -> the training
# frame (x.weight = rows of the full data a row stands for), eval_rows ->
# the fair frame the ranking quality is measured on.
#
# usage: cldata_gen_pandas.py --tiers disc --halflife 4
#            --cut1 3.0 --cut2 25.0 --limit 6000 data-min.db
from __future__ import print_function
import optparse
import time
import pickle
import re
import pandas as pd
import numpy as np
import sys
import helper


class QueryAddIdxes (helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryAddIdxes, self).__init__(dbfname)

    def measure_size(self):
        self.c.execute("select count() from reduceDB")
        rows = self.c.fetchall()
        rdb_rows = rows[0][0]
        print("We have %d lines of RDB" % (rdb_rows))

        self.c.execute("select count() from clause_stats")
        rows = self.c.fetchall()
        clss_rows = rows[0][0]
        print("We have %d lines of clause_stats" % (clss_rows))

    def create_indexes(self):
        t = time.time()
        print("Recreating indexes...")
        queries = """
        create index `idxclid33` on `sum_cl_use` (`clauseID`, `last_confl_used`);
        ---
        create index `idxclid1` on `clause_stats` (`clauseID`, conflicts);
        create index `idxclid1-2` on `clause_stats` (`clauseID`);
        create index `idxclid5` on `tags` ( `name`);
        ---
        create index `idxclid6` on `reduceDB` (`clauseID`, conflicts);
        create index `idxclid6-9` on `reduceDB` (`conflicts`);
        create index `idxclid9-3` on `reduceDB_common` (`conflicts`);
        create index `idxclid6-2` on `reduceDB` (`clauseID`, `dump_no`);
        create index `idxclid6-3` on `reduceDB` (`clauseID`, `conflicts`, `dump_no`);
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        ---
        create index `idxclid7` on `satzilla_features` (`latest_satzilla_feature_calc`);
        ---
        create index `idxcl_last_in_solver-1` on `cl_last_in_solver` ( `clauseID`, `conflicts`);
        ---
        create index `used_later_percentiles_idx3` on `used_later_percentiles` (`type_of_dat`, `percentile_descr`, `percentile`, `val`);
        create index `used_later_percentiles_idx2` on `used_later_percentiles` (`type_of_dat`, `percentile_descr`, `val`);
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
    def __init__(self, dbfname, tier, table):
        super(QueryCls, self).__init__(dbfname)
        self.fill_sql_query(tier, table=table)

    def fill_sql_query(self, tier, table):
        # sum_cl_use
        self.sum_cl_use = helper.query_fragment(
            "sum_cl_use", [], "sum_cl_use", options.verbose, self.c)

        # RDB data
        not_cols = [
            "reduceDB_called"
            , "clauseID"
            , "locked"
            , "conflicts"
            , "activity_rel"]
        self.rdb0_dat = helper.query_fragment(
            "reduceDB", not_cols, "rdb0", options.verbose, self.c)

        # reduceDB_common data
        not_cols = [
            "reduceDB_called"
            , "simplifications"
            , "restarts"
            #, "conflicts"
            ]
        self.rdb0_common_dat = helper.query_fragment(
            "reduceDB_common", not_cols, "rdb0_common", options.verbose, self.c)

        # clause data
        not_cols = [
            "simplifications"
            , "restarts"
            , "clauseID"]
        self.clause_dat = helper.query_fragment(
            "clause_stats", not_cols, "cl", options.verbose, self.c)

        # the rows sample_data.py picked, and how many each stands for
        q_time_base="""
        join {table}_{tier} on
            {table}_{tier}.clauseID = rdb0.clauseID
            and {table}_{tier}.rdb0conflicts = rdb0.conflicts
        join {{rows}} as picked on
            picked.tier = '{tier}' and picked.tbl = '{table}'
            and picked.clauseID = rdb0.clauseID
            and picked.conflicts = rdb0.conflicts
        """

        q_columns_base="""
            , {table}_{tier}.used_later as `x.{table}_{tier}`
            , {table}_{tier}.percentile_fit as `x.{table}_{tier}_topperc`
            , {table}_{tier}.rel as `x.{table}_{tier}_rel`
            , picked.weight as `x.weight`
            """

        q_time = q_time_base.format(tier=tier, table=table)
        q_columns = q_columns_base.format(tier=tier, table=table)

        # final big query
        self.q_select = """
        SELECT
        tags.val as `fname`
        {clause_dat}
        {rdb0_dat}
        {rdb0_common_dat}
        {sum_cl_use}
        , (rdb0.conflicts - rdb0.introduced_at_conflict) as `cl.time_inside_solver`
        , (sum_cl_use.last_confl_used - rdb0.introduced_at_conflict) as `x.a_lifetime`
        {q_columns}
        , sum_cl_use.num_used as `x.sum_cl_use`


        FROM
        reduceDB as rdb0

        -- this is DELIBERATEY left-join: this way, clauses that as ternary
        -- resolvents or otherwise generated during in-processing
        -- can still be used
        left join clause_stats as cl on
            cl.clauseID = rdb0.clauseID

        join reduceDB_common as rdb0_common on
            rdb0_common.conflicts = rdb0.conflicts

        join sum_cl_use on
            sum_cl_use.clauseID = rdb0.clauseID

        {q_time}

        join cl_last_in_solver on
            cl_last_in_solver.clauseID = rdb0.clauseID

        , tags

        WHERE
        (cl.clauseID != 0 OR cl.clauseID is NULL)
        and tags.name = "filename"
        order by rdb0.conflicts, rdb0.clauseID
        """

        self.myformat = {
            "clause_dat": self.clause_dat,
            "rdb0_dat": self.rdb0_dat,
            "sum_cl_use": self.sum_cl_use,
            "rdb0_common_dat": self.rdb0_common_dat,
            "q_time": q_time,
            "q_columns": q_columns
        }

    def get_one_data(self, tier, table, rows):
        self.c.execute("select count(*) from {rows} where tier = '{tier}' and tbl = '{table}'".format(
            rows=rows, tier=tier, table=table))
        if self.c.fetchone()[0] == 0:
            print("WARNING: no {rows} for {table}_{tier} (run shorter than two half-lives, or no clause ever used)".format(
                rows=rows, tier=tier, table=table))
            return pd.DataFrame()
        t = time.time()
        q = self.q_select.format(**self.myformat).format(rows=rows)
        if options.dump_sql:
            print("query:", q)
        data = pd.read_sql_query(q, self.conn)
        print("** %s query finished. Total size: %s  -- T: %-3.2f" % (
            rows, data.shape, time.time() - t))
        return data


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

    with helper.QueryHelper(dbfname) as q:
        for t in ["frame_rows", "eval_rows"]:
            q.c.execute("create index `idx%s` on %s (tier, tbl, clauseID, conflicts)" % (t, t))
        for tier in options.tiers.split(","):
            for table in ["used_later", "used_later_anc"]:
                q.c.execute("create index `idxul_{table}_{tier}` on {table}_{tier} (clauseID, rdb0conflicts)".format(
                    tier=tier, table=table))

    print("Using sqlite3 DB file %s" % dbfname)
    for tier in options.tiers.split(","):
        for table in ["used_later", "used_later_anc"]:
            print("------> Doing tier {tier} table {table}".format(
                tier=tier,table=table))

            with QueryCls(dbfname, tier, table) as q:
                df = q.get_one_data(tier, table, "frame_rows")
                df_eval = q.get_one_data(tier, table, "eval_rows")
            if df.shape[0] < 100:
                print("WARNING: only %d rows for tier %s table %s (run shorter than the "
                      "horizon?), no frame written" % (df.shape[0], tier, table))
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
            dump_dataframe(df, "{cleanname}-cldata-{table}-{tier}-cut1-{cut1}-cut2-{cut2}-limit-{limit}".format(
                cleanname=cleanname,
                cut1=options.cut1,
                cut2=options.cut2,
                limit=options.limit,
                tier=tier,
                table=table))
            # the fair sample: what the ranking quality is measured on
            if df_eval.shape[0] > 0:
                dump_dataframe(df_eval, "{cleanname}-evaldata-{table}-{tier}".format(
                    cleanname=cleanname, tier=tier, table=table))


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
                      dest="limit", help="Max number of samples to take from each strata (for each table/tier)")
    parser.add_option("--cut1", default=5.0, type=float,
                      dest="cut1", help="Where to cut the distrib. Default: %default")
    parser.add_option("--cut2", default=30.0, type=float,
                      dest="cut2", help="Where to cut the distrib. Default: %default")

    # debugging is faster with this
    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    helper.add_tier_options(parser)

    (options, args) = parser.parse_args()

    if len(args) != 1:
        print("ERROR: You must give exactly one file")
        exit(-1)

    np.random.seed(2097483)
    one_database(args[0])

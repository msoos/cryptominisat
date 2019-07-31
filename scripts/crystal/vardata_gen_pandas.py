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
import argparse
import time
import pickle
import re
import pandas as pd
import numpy as np
import os.path
import sys
import helper


def dump_df(df):
    # rename columns if we have to:
    cleanname = re.sub(r'\.cnf.gz.sqlite$', '', options.fname)
    cleanname = re.sub(r'\.db$', '', options.fname)
    cleanname = re.sub(r'\.sqlitedb$', '', options.fname)
    cleanname = "{cleanname}-vardata".format(
        cleanname=cleanname)
    dump_dataframe(df, cleanname)


def dump_dataframe(df, name):
    if options.dump_csv:
        fname = "%s.csv" % name
        print("Dumping CSV data to:", fname)
        df.to_csv(fname, index=False, columns=sorted(list(df)))

    fname = "%s.dat" % name
    print("Dumping pandas data to:", fname)
    with open(fname, "wb") as f:
        pickle.dump(df, f)


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


class QueryVar (QueryHelper):
    def __init__(self, dbfname):
        super(QueryVar, self).__init__(dbfname)

    def create_indexes(self):
        helper.drop_idxs(self.c)

        print("Recreating indexes...")
        t = time.time()
        queries = """
        create index `idxclid8` on `varData` ( `var`, `conflicts`, `clid_start_incl`, `clid_end_notincl`);

        create index `idxclid-a1` on `varData` ( `clid_start_incl`, `clid_end_notincl`);
        create index `idxclid-s1` on `sum_cl_use` ( `clauseID`);
        create index `idxclid-s2` on `restart_dat_for_var` (`conflicts`);
        """

        for l in queries.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Creating/dropping index: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Index dropping&creation T: %-3.2f s" %
                      (time.time() - t2))

        print("indexes dropped&created T: %-3.2f s" % (time.time() - t))

    def fill_var_data_use(self):
        print("Filling var data use...")

        t = time.time()
        q = "delete from `var_data_use`;"
        self.c.execute(q)
        print("var_data_use cleared T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        insert into var_data_use

        (`var`
        , `conflicts`

        , `cls_marked`
        , `useful_clauses_used`)

        select
        v.var
        , v.conflicts

        -- measures for good
        , count(cls.num_used) as cls_marked
        , sum(cls.num_used) as useful_clauses_used

        FROM varData as v join sum_cl_use as cls
        on cls.clauseID >= v.clid_start_incl
        and cls.clauseID < v.clid_end_notincl

        -- avoid division by zero below
        where
        v.propagated > 0
        and v.sumPropagations_at_picktime > 0
        and v.decided > 0
        and v.sumDecisions_at_picktime > 0
        group by var, conflicts
        ;
        """
        if options.verbose:
            print("query:", q)
        self.c.execute(q)

        print("var_data_use filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        UPDATE var_data_use SET useful_clauses_used = 0
        WHERE useful_clauses_used IS NULL
        """
        self.c.execute(q)

        print("var_data_use updated T: %-3.2f s" % (time.time() - t))

    def create_vardata_df(self):
        not_cols = [
            "clid_start_incl"
            , "clid_end_notincl"
            , "decided_pos"
            , "propagated_pos"
            , "restarts"
            , "conflicts"
            , "var"
            , "useful_clauses_used"
            , "cls_marked"
            , "propagated_pos_perc"]
        var_data = helper.query_fragment(
            "varData", not_cols, "var_data", options.verbose, self.c)

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
        rst = helper.query_fragment(
            "restart_dat_for_var", not_cols, "rst", options.verbose, self.c)

        not_cols =[
            "useful_clauses"
            , "var"
            , "conflicts"]
        var_data_use = helper.query_fragment(
            "var_data_use", not_cols, "var_data_use", options.verbose, self.c)

        q = """
        select
        (1.0*useful_clauses_used)/(1.0*cls_marked) as `x.useful_times_per_marked`
        {rst}
        {var_data_use}
        {var_data}

        FROM
        varData as var_data
        , var_data_use as var_data_use
        , restart_dat_for_var as rst

        WHERE
        clauses_below >= {min_cls_below}
        and var_data_use.cls_marked >= {min_cls_below}
        and var_data.var = var_data_use.var
        and var_data.conflicts = var_data_use.conflicts
        and rst.conflicts = var_data_use.conflicts

        order by random()
        limit {limit}
        """.format(
            rst=rst, var_data_use=var_data_use,
            var_data=var_data,
            limit=options.limit,
            min_cls_below=options.min_cls_below)

        df = pd.read_sql_query(q, self.conn)
        print("DF dimensions:", df.shape)
        return df


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.sqlite"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='SQLITEFILE')
    parser.add_argument("--verbose", "-v", action="store_true",
                        default=False, dest="verbose", help="Print more output")
    parser.add_argument("--csv", action="store_true", default=False,
                        dest="dump_csv", help="Dump CSV (for weka)")
    parser.add_argument("--limit", type=int, default=10000,
                        dest="limit", help="How many data points")
    parser.add_argument("--minclsbelow", type=int, default=1,
                        dest="min_cls_below", help="Minimum number of clauses below to generate data point")

    options = parser.parse_args()

    if options.fname is None:
        print("ERROR: You must give exactly one file")
        exit(-1)

    np.random.seed(2097483)
    with QueryVar(options.fname) as q:
        q.create_indexes()
        q.fill_var_data_use()
        df = q.create_vardata_df()
        dump_df(df)

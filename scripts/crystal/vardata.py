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
        t = time.time()

        print("Recreating indexes...")
        print("Getting indexes to drop...")
        q = """
        SELECT name FROM sqlite_master WHERE type == 'index'
        """
        self.c.execute(q)
        rows = self.c.fetchall()
        queries = ""
        for row in rows:
            print("Will delete index:", row[0])
            queries += "drop index if exists `%s`;\n" % row[0]

        t = time.time()
        queries += """
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
                print("Index dropping&creation T: %-3.2f s" % (time.time() - t2))

        print("indexes dropped&created T: %-3.2f s" % (time.time() - t))

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

    def create_vardata_df(self,fname):

        not_cols = [
            "clid_start_incl"
            , "clid_end_notincl"
            , "decided_pos"
            , "propagated_pos"
            , "propagated_pos_perc"]
        var_data = self.query_fragment("varData", not_cols, "var_data")

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
        rst = self.query_fragment("restart_dat_for_var", not_cols, "rst")

        not_cols =[
            "useful_clauses"
            , "var"
            , "conflicts"]
        var_data_use = self.query_fragment("varDataUse", not_cols, "var_data_use")

        q = """
        select
        (1.0*useful_clauses_used)/(1.0*cls_marked) as `x.useful_times_per_marked`
        {rst}
        {var_data_use}
        {var_data}
        , CASE WHEN
         (1.0*useful_clauses_used)/(1.0*cls_marked) > 2
        THEN "OK"
        ELSE "BAD"
        END AS `x.class`

        FROM
        varData as var_data
        , varDataUse as var_data_use
        , restart_dat_for_var as rst

        WHERE
        clauses_below > 10
        and var_data.var = var_data_use.var
        and var_data.conflicts = var_data_use.conflicts
        and rst.conflicts = var_data_use.conflicts
        """.format(rst=rst, var_data_use=var_data_use, var_data=var_data)

        df = pd.read_sql_query(q, self.conn)
        print("Relative data...")
        df["var_data.inside_conflict_clause_during"] = \
            df["var_data.inside_conflict_clause_at_fintime"]-df["var_data.inside_conflict_clause_at_picktime"]
        df["var_data.inside_conflict_clause_antecedents_during"] = \
            df["var_data.inside_conflict_clause_antecedents_at_fintime"]-df["var_data.inside_conflict_clause_antecedents_at_picktime"]
        df["var_data.inside_conflict_clause_glue_during"] = \
            df["var_data.inside_conflict_clause_glue_at_fintime"]-df["var_data.inside_conflict_clause_glue_at_picktime"]

        df["var_data.sumDecisions_during"] = \
            df["var_data.sumDecisions_at_fintime"]-df["var_data.sumDecisions_at_picktime"]
        df["var_data.sumPropagations_during"] = \
            df["var_data.sumPropagations_at_fintime"]-df["var_data.sumPropagations_at_picktime"]
        df["var_data.sumConflicts_during"] = \
            df["var_data.sumConflicts_at_fintime"]-df["var_data.sumConflicts_at_picktime"]
        df["var_data.sumAntecedents_during"] = \
            df["var_data.sumAntecedents_at_fintime"]-df["var_data.sumAntecedents_at_picktime"]
        df["var_data.sumAntecedentsLits_during"] = \
            df["var_data.sumAntecedentsLits_at_fintime"]-df["var_data.sumAntecedentsLits_at_picktime"]
        df["var_data.sumConflictClauseLits_during"] = \
            df["var_data.sumConflictClauseLits_at_fintime"]-df["var_data.sumConflictClauseLits_at_picktime"]
        df["var_data.sumDecisionBasedCl_during"] = \
            df["var_data.sumDecisionBasedCl_at_fintime"]-df["var_data.sumDecisionBasedCl_at_picktime"]

        df["var_data.rel_inside_confl_cl"] = df["var_data.inside_conflict_clause_during"]/df["var_data.sumConflicts_during"]
        df["var_data.rel_inside_confl_cl_ant"] = df["var_data.inside_conflict_clause_antecedents_during"]/df["var_data.sumAntecedents_during"]
        df["var_data.rel_inside_confl_cl_glue"] = df["var_data.inside_conflict_clause_glue_during"]/df["var_data.sumConflicts_during"]

        df["cl_below_per_dec_depth"]=df["var_data.clauses_below"]/df["var_data.dec_depth"]
        df["var_data.propagated_per_sumconfl"]=df["var_data.propagated"]/df["var_data.sumConflicts_at_fintime"]
        df["var_data.propagated_per_sumprop"]=df["var_data.propagated"]/df["var_data.sumPropagations_at_fintime"]
        df["var_data.clauses_below_per_sumDecisions_during"]=df["var_data.clauses_below"]/df["var_data.sumDecisions_during"]

        if True:

            del df["var_data.propagated"]
            del df["var_data.decided"]
            del df["var_data.clauses_below"]
            del df["var_data.dec_depth"]

            cols = list(df)
            for c in cols:
                if "at_picktime" in c or "at_fintime" in c or c[0:3] == "sum":
                    del df[c]

        df2 = df[df["var_data_use.cls_marked"] >= 10]
        # df2 = df2[df["conflicts_below"] >= 20]

        # to make things easier for me
        del df2["var_data_use.useful_clauses_used"]
        del df2["var_data_use.cls_marked"]

        # rename columns if we have to:
        # df.rename(columns={'abc':'d'}, inplace=True)

        cleanname = re.sub(r'\.cnf.gz.sqlite$', '', fname)
        cleanname = re.sub(r'\.db$', '', fname)
        cleanname += "-vardata"
        dump_dataframe(df2, cleanname)

    def fill_var_data_use(self):
        print("Filling var data use...")

        t = time.time()
        q = "delete from `varDataUse`;"
        self.c.execute(q)
        print("varDataUse cleared T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        insert into varDataUse

        (`var`
        , `conflicts`

        , `decided_this_var_per_all_decisions`
        , `decided_this_var_pos_perc`
        , `prop_this_var_per_all_decisions`
        , `propagated_this_var_pos_perc`

        , `cls_marked`
        , `useful_clauses_used`)

        select
        v.var
        , v.conflicts

        -- data about var
        , (v.decided*1.0)/(v.sumDecisions_at_fintime*1.0) as `decided_this_var_per_all_decisions`
        , (v.decided_pos*1.0)/(v.decided*1.0) as `decided_this_var_pos_perc`
        , (v.propagated*1.0)/(v.sumPropagations_at_fintime*1.0) as `prop_this_var_per_all_decisions`
        , (v.propagated_pos*1.0)/(v.propagated*1.0) as `propagated_this_var_pos_perc`

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

        print("varDataUse filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        UPDATE varDataUse SET useful_clauses_used = 0
        WHERE useful_clauses_used IS NULL
        """
        self.c.execute(q)

        print("varDataUse updated T: %-3.2f s" % (time.time() - t))


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.sqlite"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='SQLITEFILE')
    parser.add_argument("--verbose", "-v", action="store_true", default=False
                        , dest="verbose", help="Print more output")
    parser.add_argument("--csv", action="store_true", default=False
                        , dest="dump_csv", help="Dump CSV (for weka)")


    options = parser.parse_args()

    if options.fname is None:
        print("ERROR: You must give exactly one file")
        exit(-1)

    np.random.seed(2097483)
    with QueryVar(options.fname) as q:
        q.create_indexes()
        q.fill_var_data_use()
        q.create_vardata_df(options.fname)

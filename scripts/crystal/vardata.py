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
        """

        for l in queries.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Creating/dropping index: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Index dropping&creation T: %-3.2f s" % (time.time() - t2))

        print("indexes dropped&created T: %-3.2f s" % (time.time() - t))

    def create_vardata_df(self,fname):
        q = """
        select
        *
        , CASE WHEN
         (1.0*useful_clauses)/(1.0*clauses_below) > 0.5
        THEN "OK"
        ELSE "BAD"
        END AS `class`

        from varData, varDataUse
        where
        clauses_below > 10
        and varData.var = varDataUse.var
        and varData.conflicts = varDataUse.conflicts
        """

        df = pd.read_sql_query(q, self.conn)

        cleanname = re.sub(r'\.cnf.gz.sqlite$', '', fname)
        cleanname = re.sub(r'\.db$', '', fname)
        cleanname += "-vardata"
        dump_dataframe(df, cleanname)

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

        , `decided_avg`
        , `decided_pos_perc`
        , `propagated_avg`
        , `propagated_pos_perc`

        , `cls_marked`
        , `useful_clauses_used`
        , `useful_clauses_sum_hist_used`)

        select
        v.var
        , v.conflicts

        -- data about var
        , (v.decided*1.0)/(v.sumDecisions_at_picktime*1.0)
        , (v.decided_pos*1.0)/(v.decided*1.0)
        , (v.propagated*1.0)/(v.sumPropagations_at_picktime*1.0)
        , (v.propagated_pos*1.0)/(v.propagated*1.0)

        -- measures for good
        , count(cls.num_used) as cls_marked
        , sum(cls.num_used) as useful_clauses_used
        , sum(cls.sum_hist_used) as useful_clauses_sum_hist_used

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
    usage = "usage: %prog [options] file.sqlite"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='SQLITEFILE')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_argument("--csv", action="store_true", default=False,
                      dest="dump_csv", help="Dump CSV (for weka)")


    options = parser.parse_args()

    if options.fname is None:
        print("ERROR: You must give exactly one file")
        exit(-1)

    np.random.seed(2097483)
    with QueryVar(options.fname) as q:
        q.create_indexes()
        q.fill_var_data_use()
        q.create_vardata_df(options.fname)

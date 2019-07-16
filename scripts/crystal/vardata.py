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
import sklearn
ver = sklearn.__version__.split(".")
if int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split
import sklearn.ensemble


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

    def add_computed_features(self, df):
        print("Relative data...")
        cols = list(df)
        for col in cols:
            if "_at_fintime" in col:
                during_name = col.replace("_at_fintime", "_during")
                at_picktime_name = col.replace("_at_fintime", "_at_picktime")
                if options.verbose:
                    print("fintime name: ", col)
                    print("picktime name: ", at_picktime_name)
                df[during_name] = df[col]-df[at_picktime_name]

        # remove stuff
        del df["var_data_use.useful_clauses_used"]
        del df["var_data_use.cls_marked"]

        # more complicated
        df["var_data.propagated_per_sumconfl"]=df["var_data.propagated"]/df["var_data.sumConflicts_at_fintime"]
        df["var_data.propagated_per_sumprop"]=df["var_data.propagated"]/df["var_data.sumPropagations_at_fintime"]

        # remove picktime & fintime
        cols = list(df)
        for c in cols:
            if "at_picktime" in c or "at_fintime" in c:
                del df[c]

        # per-conflicts, per-decisions, per-lits
        names = [
            "var_data.sumDecisions_during"
            , "var_data.sumPropagations_during"
            , "var_data.sumConflicts_during"
            , "var_data.sumAntecedents_during"
            , "var_data.sumConflictClauseLits_during"
            , "var_data.sumAntecedentsLits_during"
            , "var_data.sumClSize_during"
            , "var_data.sumClLBD_during"
            , "var_data.dec_depth"
            ]

        cols = list(df)
        for col in cols:
            if "restart_type" not in col and "x." not in col and "useful_clauses" not in col:
                for name in names:
                    if options.verbose:
                        print("dividing col '%s' with '%s' " % (col, name))
                    df["(" + col + "/" + name + ")"]=df[col]/df[name]

        # remove sum
        #cols = list(df)
        #for c in cols:
            #if c[0:3] == "sum":
                #del df[c]

        if True:
            # remove these
            torem = [
                "var_data.propagated"
                , "var_data.decided"
                , "var_data.clauses_below"
                , "var_data.dec_depth"
                , "var_data.sumDecisions_during"
                , "var_data.sumPropagations_during"
                , "var_data.sumConflicts_during"
                , "var_data.sumAntecedents_during"
                , "var_data.sumAntecedentsLits_during"
                , "var_data.sumConflictClauseLits_during"
                , "var_data.sumDecisionBasedCl_during"
                , "var_data.sumClLBD_during"
                , "var_data.sumClSize_during"
                ]
            cols = list(df)

            # also remove rst
            for col in cols:
                if "rst." in col:
                    torem.append(col)
                    pass

            # actually remove
            for x in torem:
                del df[x]

    def create_vardata_df(self):
        not_cols = [
            "clid_start_incl"
            , "clid_end_notincl"
            , "decided_pos"
            , "propagated_pos"
            , "restarts"
            , "conflicts"
            , "var"
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
--        , CASE WHEN
--         (1.0*useful_clauses_used)/(1.0*cls_marked) > 2
--        THEN "OK"
--        ELSE "BAD"
--        END AS `x.class`

        FROM
        varData as var_data
        , varDataUse as var_data_use
        , restart_dat_for_var as rst

        WHERE
        clauses_below > 10
        and var_data_use.cls_marked > 10
        and var_data.var = var_data_use.var
        and var_data.conflicts = var_data_use.conflicts
        and rst.conflicts = var_data_use.conflicts

        order by random()
        limit {limit}
        """.format(
            rst=rst, var_data_use=var_data_use,
            var_data=var_data,
            limit=options.limit)

        df = pd.read_sql_query(q, self.conn)
        print("DF dimensions:", df.shape)
        return df

    def dump_df(self, df):
        # rename columns if we have to:
        # df.rename(columns={'abc':'d'}, inplace=True)

        cleanname = re.sub(r'\.db$', '', options.fname)
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

class Predict:
    def __init__(self):
        pass

    def calc_min_split_point(self, df):
        split_point = int(float(df.shape[0])*options.min_samples_split)
        if split_point < 10:
            split_point = 10
        print("Minimum split point: ", split_point)
        return split_point

    def print_confusion_matrix(self, cm, classes,
                               normalize=False,
                               title='Confusion matrix'):
        """
        This function prints and plots the confusion matrix.
        Normalization can be applied by setting `normalize=True`.
        """
        if normalize:
            cm = cm.astype('float') / cm.sum(axis=1)[:, np.newaxis]
        print(title)

        np.set_printoptions(precision=2)
        print(cm)

    def conf_matrixes(self, test2, features, to_predict, clf):
        # get test data
        X_test = test2[features]
        y_test = test2[to_predict]
        print("Number of elements:", X_test.shape)
        if test2.shape[0] == 0:
            print("Cannot calculate confusion matrix, too few elements")
            return 0, 0, 0

        # Preform prediction
        y_pred = clf.predict(X_test)

        # calc acc, precision, recall
        accuracy = sklearn.metrics.accuracy_score(
            y_test, y_pred)
        precision = sklearn.metrics.precision_score(
            y_test, y_pred, pos_label="OK", average="micro")
        recall = sklearn.metrics.recall_score(
            y_test, y_pred, pos_label="OK", average="micro")
        print("test prec : %-3.4f  recall: %-3.4f accuracy: %-3.4f" % (
            precision, recall, accuracy))

        # Plot "test" confusion matrix
        cnf_matrix = sklearn.metrics.confusion_matrix(
            y_true=y_test, y_pred=y_pred)
        self.print_confusion_matrix(
            cnf_matrix, classes=clf.classes_,
            title='Confusion matrix, without normalization -- test')
        self.print_confusion_matrix(
            cnf_matrix, classes=clf.classes_, normalize=True,
            title='Normalized confusion matrix -- test')

        return precision, recall, accuracy

    def get_top_features(self, df):
        split_point = self.calc_min_split_point(df)
        df["x.class"]=pd.qcut(df["x.useful_times_per_marked"],
                             q=4,
                             labels=False)
        features = list(df)
        features.remove("x.class")
        features.remove("x.useful_times_per_marked")
        to_predict="x.class"

        print("-> Number of features  :", len(features))
        print("-> Number of datapoints:", df.shape)
        print("-> Predicting          :", to_predict)

        train, test = train_test_split(df, test_size=0.33)
        X_train = train[features]
        y_train = train[to_predict]
        split_point = self.calc_min_split_point(df)
        clf = sklearn.ensemble.RandomForestClassifier(
                    n_estimators=400,
                    max_features="sqrt",
                    min_samples_leaf=split_point)

        t = time.time()
        clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time() - t))

        best_features = []
        importances = clf.feature_importances_
        std = np.std([tree.feature_importances_ for tree in clf.estimators_], axis=0)
        indices = np.argsort(importances)[::-1]
        indices = indices[:options.top_num_features]
        myrange = min(X_train.shape[1], options.top_num_features)

        # Print the feature ranking
        print("Feature ranking:")
        for f in range(myrange):
            print("%-3d  %-55s -- %8.4f" %
                  (f + 1, features[indices[f]], importances[indices[f]]))

        self.conf_matrixes(test, features, to_predict, clf)

if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.sqlite"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='SQLITEFILE')
    parser.add_argument("--verbose", "-v", action="store_true", default=False
                        , dest="verbose", help="Print more output")
    parser.add_argument("--csv", action="store_true", default=False
                        , dest="dump_csv", help="Dump CSV (for weka)")
    parser.add_argument("--limit", type=int, default=10000
                        , dest="limit", help="How many data points")

    # dataframe
    parser.add_argument("--split", default=0.002, type=float, metavar="RATIO",
                      dest="min_samples_split", help="Split in tree if this many samples or above. Used as a percentage of datapoints")
    parser.add_argument("--prefok", default=2.0, type=float,
                      dest="prefer_ok", help="Prefer OK if >1.0, equal weight if = 1.0, prefer BAD if < 1.0")
    parser.add_argument("--top", default=40, type=int, metavar="TOPN",
                      dest="top_num_features", help="Candidates are top N features for greedy selector")


    options = parser.parse_args()

    if options.fname is None:
        print("ERROR: You must give exactly one file")
        exit(-1)

    np.random.seed(2097483)
    with QueryVar(options.fname) as q:
        q.create_indexes()
        q.fill_var_data_use()
        df = q.create_vardata_df()
        q.add_computed_features(df)
        q.dump_df(df)

    p = Predict()
    p.get_top_features(df)

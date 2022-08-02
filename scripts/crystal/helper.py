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

# pylint: disable=invalid-name,line-too-long,too-many-locals,consider-using-sys-exit

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import sklearn
import sklearn.metrics
import re
import ast
import math
import time
import os.path
import sqlite3
import functools
from ccg import *

try:
    from termcolor import cprint
except ImportError:
    termcolor_avail = False
else:
    termcolor_avail = True

from pprint import pprint
try:
    import mlflow
except ImportError:
    mlflow_avail = False
else:
    mlflow_avail = True


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
    def create_indexes(self, verbose=False, used_clauses="used_clauses"):
        t = time.time()
        print("Recreating indexes...")
        queries = """
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        create index `idxclidUCLS-2` on `{used_clauses}` ( `clauseID`, `used_at`);
        create index `idxcl_last_in_solver-1` on `cl_last_in_solver` ( `clauseID`, `conflicts`);
        """.format(used_clauses=used_clauses)
        for l in queries.split('\n'):
            t2 = time.time()

            if verbose:
                print("Creating index: ", l)
            self.c.execute(l)
            if verbose:
                print("Index creation T: %-3.2f s" % (time.time() - t2))

        print("indexes created T: %-3.2f s" % (time.time() - t))

    def delete_and_create_used_laters(self):
        tiers = ["short", "long", "forever"]
        tables = ["used_later", "used_later_anc"]
        for tier in tiers:
            for table in tables:
                q = """
                    DROP TABLE IF EXISTS `{table}_{tier}`;
                """
                self.c.execute(q.format(tier=tier, table=table))

        # Create and fill used_later_X tables
        q_create = """
        create table `{table}_{tier}` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later` float,
            `percentile_fit` float DEFAULT NULL
        );"""
        # NOTE: "percentile_fit" is the top percentile this use belongs to. Filled in later.

        for tier in tiers:
            for table in tables:
                self.c.execute(q_create.format(tier=tier, table=table))

        idxs = """
        create index `{table}_{tier}_idx3` on `{table}_{tier}` (`used_later`);
        create index `{table}_{tier}_idx1` on `{table}_{tier}` (`clauseID`, `rdb0conflicts`);
        create index `{table}_{tier}_idx2` on `{table}_{tier}` (`clauseID`, `rdb0conflicts`, `used_later`);"""

        t = time.time()
        for tier in tiers:
            for table in tables:
                for l in idxs.format(tier=tier, table=table).split('\n'):
                    self.c.execute(l)

        print("used_later* dropped and recreated T: %-3.2f s" % (time.time() - t))

    # The most expesive operation of all, when called with "forever"
    def fill_used_later_X(self, tier, duration, used_clauses="used_clauses",
                          table="used_later"):

        min_del_distance = duration
        if min_del_distance > 2*1000*1000:
            min_del_distance = 100*1000

        mult = 1.2

        q_fill = """
        insert into {table}_{tier}
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , sum(ucl.weight* (({duration}*{mult}-(ucl.used_at-rdb0.conflicts)+0.001)/({duration}*{mult}+0.001)) ) as `used_later`

        FROM
        reduceDB as rdb0
        left join {used_clauses} as ucl

        -- reduceDB is always present, {used_clauses} may not be, hence left join
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > (rdb0.conflicts)
            and ucl.used_at <= (rdb0.conflicts+{duration}))

        join cl_last_in_solver
        on cl_last_in_solver.clauseID = rdb0.clauseID

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.conflicts >= (rdb0.conflicts + {min_del_distance})

        group by rdb0.clauseID, rdb0.conflicts;"""

        t = time.time()
        q = q_fill.format(
            tier=tier, used_clauses=used_clauses,
            duration=duration,
            table=table,
            mult=mult,
            min_del_distance=min_del_distance)
        self.c.execute(q)

        q_fix_null = "update {table}_{tier} set used_later = 0 where used_later is NULL".format(
            tier=tier, table=table)
        self.c.execute(q_fix_null)


        q_num = "select count(*) from {table}_{tier}".format(tier=tier, table=table)
        self.c.execute(q_num)
        rows = self.c.fetchall()
        for row in rows:
            num = row[0]

        if table == "used_later" and num == 0:
            print("ERROR: number of rows in {table}_{tier} is 0!".format(tier=tier, table=table))
            print("Query was: %s" % q)
            exit(-1)


        print("%s_%s filled T: %-3.2f s -- num rows: %d" %
              (table, tier, time.time() - t, num))

    def fill_used_later_X_perc_fit(self, tier, table):
        print("Filling percentile_fit for {table}_{tier}".format(tier=tier, table=table))

        q = """
        update {table}_{tier}
        set percentile_fit = (
            select max({table}_percentiles.percentile)
            from {table}_percentiles
            where
            {table}_percentiles.type_of_dat="{tier}"
            and {table}_percentiles.percentile_descr="top_non_zero"
            and {table}_percentiles.val >= {table}_{tier}.used_later);
        """
        t = time.time()
        self.c.execute(q.format(tier=tier, table=table))
        print("used_later_%s percentile filled T: %-3.2f s" % (tier, time.time() - t))



def write_mit_header(f):
    f.write("""/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/\n\n""")

def parse_configs(confs):
    match = re.match(r"^([0-9]*)-([0-9]*)$", confs)
    if not match:
        print("ERROR: we cannot parse your config options: '%s'" % confs)
        exit(-1)

    conf_from = int(match.group(1))
    conf_to = int(match.group(2))+1
    if conf_to <= conf_from:
        print("ERROR: Conf range is not increasing")
        exit(-1)

    print("Running configs:", range(conf_from, conf_to))
    return conf_from, conf_to


def get_features(fname):
    best_features = []
    check_file_exists(fname)
    with open(fname, "r") as f:
        for l in f:
            l = l.strip()
            if len(l) == 0:
                continue

            if l[0] == "#":
                continue

            best_features.append(l)

    return best_features


def helper_divide(dividend, divisor, df, features, verb, name=None):
    """
    to be used like:
    import functools
    divide = functools.partial(helper.divide, df=df, features=features, verb=options.verbose)
    """

    # dividend feature not present
    #if dividend not in features:
        #return None

    # divisorfeature not present
    #if divisor not in features:
        #return None

    # divide
    if verb:
        print("Dividing. dividend: '%s' divisor: '%s' " % (dividend, divisor))

    if name is None:
        name = "(%s/%s)" % (dividend, divisor)

    df[name] = df[dividend].div(df[divisor])
    return name

def helper_larger_than(lhs, rhs, df, features, verb):
    """
    to be used like:
    import functools
    larger_than = functools.partial(helper.larger_than, df=df, features=features, verb=options.verbose)
    """

    # divide
    if verb:
        print("Calulating '%s' >: '%s' " % (lhs, rhs))

    name = "(" + lhs + ">" + rhs + ")"
    df[name] = (df[lhs] > df[rhs]).astype(int)
    return name

def helper_add(toadd, df, features, verb):
    """
    to be used like:
    import functools
    larger_than = functools.partial(helper.larger_than, df=df, features=features, verb=options.verbose)
    """

    # add
    if verb:
        print("Calulating: the feature addition of: %s", toadd)

    name = "("
    for i in range(1, len(toadd)):
        name = toadd[i]
        if i < len(toadd)-1:
            name+="+"
    name += ")"

    df[name] = df[toadd[0]]
    for i in range(1, len(toadd)):
        df[name] += df[toadd[i]]
    return name


def dangerous(conn):
    conn.execute("PRAGMA journal_mode = MEMORY")
    conn.execute("PRAGMA synchronous = OFF")


def drop_idxs(conn):
    q = """
    SELECT name FROM sqlite_master WHERE type == 'index'
    """
    conn.execute(q)
    rows = conn.fetchall()
    queries = ""
    for row in rows:
        #print("Will delete index:", row[0])
        queries += "drop index if exists `%s`;\n" % row[0]

    t = time.time()
    for q in queries.split("\n"):
        conn.execute(q)

    print("Removed indexes: T: %-3.2f s"% (time.time() - t))


def get_columns(tablename, verbose, conn):
    q = "pragma table_info(%s);" % tablename
    conn.execute(q)
    rows = conn.fetchall()
    columns = []
    for row in rows:
        if verbose:
            print("Using column in table {tablename}: {col}".format(
                tablename=tablename, col=row[1]))
        columns.append(row[1])

    return columns

def query_fragment(tablename, not_cols, short_name, verbose, conn):
    cols = get_columns(tablename, verbose, conn)
    filtered_cols = list(set(cols).difference(not_cols))
    ret = ""
    for col in filtered_cols:
        ret += ", {short_name}.`{col}` as `{short_name}.{col}`\n".format(
            col=col, short_name=short_name)

    if verbose:
        print("query for short name {short_name}: {ret}".format(
            short_name=short_name, ret=ret))

    return ret


def not_inside(not_these, inside_here):
    for not_this in not_these:
        if not_this in inside_here:
            return False

    return True


# to check for too large or NaN values:
def check_too_large_or_nan_values(df, features=None):
    print("Checking for too large or NaN values...")
    if features is None:
        features = df.columns.values.flatten().tolist()

    index = 0
    for index, row in df[features].iterrows():
        print("-------------")
        print("At row index: ", index)
        for val, name in zip(row, features):
            print("Name: '%s', val: %s" % (name, val))
            if type(val) == str:
                continue

            if math.isnan(val) or not np.isfinite(val) or val > np.finfo(np.float32).max:
                print("issue with feature '%s' Value: '%s'  Type: '%s" % (
                    name, val, type(val)))
        index += 1

    print("Checking finished.")


def print_confusion_matrix(cm,
                           normalize=False,
                           title='Confusion matrix',
                           cmap=plt.cm.Blues):
    """
    This function prints and plots the confusion matrix.
    Normalization can be applied by setting `normalize=True`.
    """
    if normalize:
        cm = cm.astype('float') / cm.sum(axis=1)[:, np.newaxis]

    print(title)
    if mlflow_avail:
        mlflow.log_metric(title, cm[0][0])
    np.set_printoptions(precision=2)
    print(cm)


def calc_min_split_point(df, min_samples_split):
    split_point = int(float(df.shape[0])*min_samples_split)
    if split_point < 10:
        split_point = 10
    print("Minimum split point: ", split_point)
    return split_point


def error_format(error):
    if error is None:
        return "XXX"
    else:
        return "{0:<2.2E}".format(error)


def calc_regression_error(data, features, to_predict, clf, toprint,
                  average="binary", highlight=False):
    X_data = data[features]
    y_data = data[to_predict]
    print("Number of elements:", X_data.shape)
    if data.shape[0] <= 1:
        print("Cannot calculate regression error, too few elements")
        return None
    y_pred = clf.predict(X_data)
    main_error = sklearn.metrics.mean_squared_error(y_data, y_pred)
    print("Mean squared error is: %9s" % error_format(main_error))
    median_absolute_error = sklearn.metrics.median_absolute_error(y_data, y_pred)
    print("Median abs error is  : %9s" % error_format(median_absolute_error))

    # use distrib
    for start,end in [(0,10), (1,10), (10, 100), (100, 1000), (1000,10000), (10000, 1000000)]:
        x = "--> Strata  %6d <= %21s < %8d " % (start, to_predict, end)
        myfilt = data[(data[to_predict] >= start) & (data[to_predict] < end)]
        X_data = myfilt[features]
        y_data = myfilt[to_predict]
        y = " -- elems: {:12}".format(str(X_data.shape))
        if myfilt.shape[0] <= 1:
            msqe = None
            med_abs_err = None
            mean_err = None
        else:
            y_pred = clf.predict(X_data)
            msqe = sklearn.metrics.mean_squared_error(y_data, y_pred)
            med_abs_err = sklearn.metrics.median_absolute_error(y_data, y_pred)
            mean_err = (y_data - y_pred).sum()/len(y_data)
        print("{} {}   msqe: {:9s}   mabse: {:9s} abs: {:9s}".format(
            x, y,  error_format(msqe), error_format(med_abs_err), error_format(mean_err)))

    # glue distrib
    for start,end in [(0,3), (3,8), (8, 15), (15, 25), (25,50), (50, 100), (100, 1000000)]:
        x = "--> Strata  %6d <= %21s < %8d " % (start, "rdb0.glue", end)
        myfilt = data[(data["rdb0.glue"] >= start) & (data["rdb0.glue"] < end)]
        X_data = myfilt[features]
        y_data = myfilt[to_predict]
        y = " -- elems: {:12}".format(str(X_data.shape))
        if myfilt.shape[0] <= 1:
            msqe = None
            med_abs_err = None
            mean_err = None
        else:
            y_pred = clf.predict(X_data)
            msqe = sklearn.metrics.mean_squared_error(y_data, y_pred)
            med_abs_err = sklearn.metrics.median_absolute_error(y_data, y_pred)
            mean_err = (y_data - y_pred).sum()/len(y_data)
        print("{} {}   msqe: {:9s}   mabse: {:9s} abs: {:9s}".format(
            x, y,  error_format(msqe), error_format(med_abs_err), error_format(mean_err)))

    return main_error


def conf_matrixes(data, features, to_predict, clf, toprint,
                  average="binary", highlight=False):
    # get data
    X_data = data[features]
    y_data = data[to_predict]
    print("Number of elements:", X_data.shape)
    if data.shape[0] <= 1:
        print("Cannot calculate confusion matrix, too few elements")
        return None, None, None, None

    # Preform prediction
    def f(x):
        if x > 0.5:
            return 1
        else:
            return 0

    y_pred = clf.predict(X_data)
    #print("type(y_pred[0]): ",  type(y_pred[0]))
    if type(y_pred[0]) == np.float32:
        y_pred = np.array([f(x) for x in y_pred])

    # calc acc, precision, recall
    accuracy = sklearn.metrics.accuracy_score(
        y_data, y_pred)

    precision = sklearn.metrics.precision_score(
        y_data, y_pred, pos_label=1, average=average)


    recall = sklearn.metrics.recall_score(
        y_data, y_pred, pos_label=1, average=average)

    # ROC AUC
    predsi = np.array(y_pred)
    y_testi = pd.DataFrame(y_data)["x.class"].squeeze()
    try:
        roc_auc = sklearn.metrics.roc_auc_score(y_testi, predsi)
    except:
        print("NOTE: ROC AUC is set to 0 because of completely one-sided OK/BAD")
        roc_auc = 0

    # record to mlflow
    if mlflow_avail:
        mlflow.log_metric(toprint + " -- accuracy", accuracy)
        mlflow.log_metric(toprint + " -- precision", precision)
        mlflow.log_metric(toprint + " -- recall", recall)
        mlflow.log_metric(toprint + " -- roc_auc", roc_auc)

    color = "white"
    bckgrnd = "on_grey"
    if highlight:
        color="green"
        bckgrnd = "on_grey"

    txt = "%s prec : %-3.4f  recall: %-3.4f accuracy: %-3.4f roc_auc: %-3.4f"
    vals = (toprint, precision, recall, accuracy, roc_auc)
    if termcolor_avail:
        cprint(txt % vals , color, bckgrnd)
    else:
        cprint(txt % vals)

    # Plot confusion matrix
    cnf_matrix = sklearn.metrics.confusion_matrix(
        y_true=y_data, y_pred=y_pred)
    print_confusion_matrix(
        cnf_matrix,
        title='Confusion matrix without normalization -- %s' % toprint)
    print_confusion_matrix(
        cnf_matrix, normalize=True,
        title='Normalized confusion matrix -- %s' % toprint)

    return roc_auc


def check_file_exists(fname):
    try:
        f = open(fname)
    except IOError:
        print("File '%s' not accessible" % fname)
        exit(-1)
    finally:
        f.close()


def output_to_classical_dot(clf, features, fname):

    feat_tmp = []
    for f in features:
        x = str(f)
        x = x.replace("rdb0.", "")
        x = x.replace("cl.", "")
        x = x.replace("HistLT.", "History_Long_Term")
        x = x.replace("rdb0_common.", "all_learnts")
        feat_tmp.append(x)

    sklearn.tree.export_graphviz(clf, out_file=fname,
                                 feature_names=feat_tmp,
                                 #class_names=clf.classes_,
                                 filled=True, rounded=True,
                                 special_characters=True,
                                 proportion=True)
    print("Run dot:")
    print("dot -Tpng {fname} -o {fname}.png".format(fname=fname))
    print("gwenview {fname}.png".format(fname=fname))


def print_feature_ranking(clf, X_train, top_num_features, features, plot=False):
    best_features = []
    importances = clf.feature_importances_
    std = np.std(
        [tree.feature_importances_ for tree in clf.estimators_], axis=0)
    indices = np.argsort(importances)[::-1]
    indices = indices[:top_num_features]
    myrange = min(X_train.shape[1], top_num_features)

    # Print the feature ranking
    print("Feature ranking:")

    for f in range(myrange):
        print("%-3d  %-55s -- %8.4f" %
              (f + 1, features[indices[f]], importances[indices[f]]))
        best_features.append(features[indices[f]])

    # Plot the feature importances of the clf
    if plot:
        plot_feature_importances(importances, indices, myrange, std, features)

    return best_features


def plot_feature_importances(importances, indices, myrange, std, features):
        plt.figure()
        plt.title("Feature importances")
        plt.bar(range(myrange), importances[indices],
                color="r", align="center",
                yerr=std[indices])
        plt.xticks(range(myrange), [features[x]
                                    for x in indices], rotation=45)
        plt.xlim([-1, myrange])


def add_features_from_fname(df, features_fname, verbose=False):
    print("Adding features...")
    if not os.path.exists(features_fname):
        print("ERROR: Feature file '%s' does not exist" % features_fname)
        exit(-1)

    cldata_add_minimum_computed_features(df, verbose)
    best_features = get_features(features_fname)
    for feat in best_features:
        toeval = ccg.to_source(ast.parse(feat))
        print("Adding feature %s as eval %s" % (feat, toeval))
        df[feat] = eval(toeval)


def add_features_from_list(df, best_features, verbose=False):
    print("Adding features...")
    cldata_add_minimum_computed_features(df, verbose)
    for feat in best_features:
        toeval = ccg.to_source(ast.parse(feat))
        print("Adding feature %s as eval %s" % (feat, toeval))
        df[feat] = eval(toeval)


def make_missing_into_nan(df):
    print("Making None into NaN...")
    def make_none_into_nan(x):
        if x is None:
            return np.nan
        else:
            return x

    for col in list(df):
        if type(None) in df[col].apply(type).unique():
            df[col] = df[col].apply(make_none_into_nan)
    print("Done.")

def cldata_add_minimum_computed_features(df, verbose):
    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    divide("rdb0.act_ranking", "rdb0_common.tot_cls_in_db", name="rdb0.act_ranking_rel")
    divide("rdb0.prop_ranking", "rdb0_common.tot_cls_in_db", name="rdb0.prop_ranking_rel")
    divide("rdb0.uip1_ranking", "rdb0_common.tot_cls_in_db", name="rdb0.uip1_ranking_rel")
    divide("rdb0.sum_uip1_per_time_ranking", "rdb0_common.tot_cls_in_db",
           name="rdb0.sum_uip1_per_time_ranking_rel")
    divide("rdb0.sum_props_per_time_ranking", "rdb0_common.tot_cls_in_db",
           name="rdb0.sum_props_per_time_ranking_rel")

    df["rdb0_common.tot_irred_cls"] = df["rdb0_common.num_bin_irred_cls"] + df["rdb0_common.num_long_irred_cls"]
    divide("rdb0_common.tot_irred_cls", "rdb0_common.num_vars")
    divide("rdb0_common.num_long_irred_cls", "rdb0_common.num_long_irred_cls_lits")
    divide("rdb0_common.num_long_irred_cls_lits", "rdb0_common.num_vars")
    divide("rdb0_common.num_long_irred_cls", "rdb0_common.num_vars")


def cldata_add_computed_features(df, verbose):
    print("Adding computed features...")
    cldata_add_minimum_computed_features(df, verbose)

    del df["cl.conflicts"]
    del df["cl.restartID"]
    del df["rdb0.introduced_at_conflict"]

    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    larger_than = functools.partial(helper_larger_than, df=df, features=list(df), verb=verbose)
    add = functools.partial(helper_add, df=df, features=list(df), verb=verbose)

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    print("size/glue/trail rel...")
    divide("cl.trail_depth_level", "cl.trailDepthHistLT_avg")
    divide("cl.trail_depth_level", "cl.trailDepthHist_avg")

    divide("cl.num_total_lits_antecedents", "cl.num_antecedents")

    del df["rdb0.uip1_ranking"]
    del df["rdb0.prop_ranking"]
    del df["rdb0.act_ranking"]
    del df["rdb0.sum_uip1_per_time_ranking"]
    del df["rdb0.sum_props_per_time_ranking"]
    del df["rdb0_common.tot_cls_in_db"]

    # divide by avg and median
    divide("rdb0.uip1_used", "rdb0_common.avg_uip1_used")
    divide("rdb0.props_made", "rdb0_common.avg_props")
    divide("rdb0.glue", "rdb0_common.avg_glue")

    divide("rdb0.uip1_used", "rdb0_common.median_uip1_used")
    divide("rdb0.props_made", "rdb0_common.median_props")

    time_in_solver = "cl.time_inside_solver"
    sum_props_per_time = divide("rdb0.sum_props_made", time_in_solver)
    sum_uip1_per_time = divide("rdb0.sum_uip1_used", time_in_solver)
    divide(sum_props_per_time, "rdb0_common.median_sum_uip1_per_time")
    divide(sum_uip1_per_time, "rdb0_common.median_sum_props_per_time")
    divide(sum_props_per_time, "rdb0_common.avg_sum_uip1_per_time")
    divide(sum_uip1_per_time, "rdb0_common.avg_sum_props_per_time")
    #del df[time_in_solver]

    divisors = [
        "cl.conflSizeHistlt_avg"
        , "cl.glueHistLT_avg"
        , "rdb0.glue"
        , "rdb0.size"
        # , "cl.orig_connects_num_communities"
        # , "rdb0.connects_num_communities"
        , "cl.orig_glue"
        , "cl.glue_before_minim"
        , "cl.glueHist_avg"
        , "cl.glueHist_longterm_avg"
        # , "cl.decision_level_hist"
        , "cl.numResolutionsHistLT_avg"
        , "cl.trailDepthHistLT_avg"
        , "cl.trailDepthHist_avg"
        , "cl.branchDepthHistQueue_avg"
        , "cl.overlapHistLT_avg"
        , "(cl.num_total_lits_antecedents/cl.num_antecedents)"
        , "cl.num_antecedents"
        , "rdb0.act_ranking_rel"
        , "rdb0.prop_ranking_rel"
        , "rdb0.uip1_ranking_rel"
        , "rdb0.sum_uip1_per_time_ranking_rel"
        , "rdb0.sum_props_per_time_ranking_rel"
        , "cl.time_inside_solver"
        # , "cl.num_overlap_literals"
        ]

    # discounted stuff
    divide("rdb0.discounted_uip1_used", "rdb0_common.avg_uip1_used")
    divide("rdb0.discounted_props_made", "rdb0_common.avg_props")
    divide("rdb0.discounted_uip1_used", "rdb0_common.median_uip1_used")
    divide("rdb0.discounted_props_made", "rdb0_common.median_props")
    #==
    divide("rdb0.discounted_uip1_used2", "rdb0_common.avg_uip1_used")
    divide("rdb0.discounted_props_made2", "rdb0_common.avg_props")
    divide("rdb0.discounted_uip1_used2", "rdb0_common.median_uip1_used")
    divide("rdb0.discounted_props_made2", "rdb0_common.median_props")

    sum_uip1_per_time = divide("rdb0.sum_uip1_used", "cl.time_inside_solver")
    sum_props_per_time = divide("rdb0.sum_props_made", "cl.time_inside_solver")
    antec_rel = divide("cl.num_total_lits_antecedents", "cl.antec_data_sum_sizeHistLT_avg")
    divisors.append(sum_uip1_per_time)
    divisors.append(sum_props_per_time)
    divisors.append("rdb0.discounted_uip1_used")
    divisors.append("rdb0.discounted_props_made")
    divisors.append(antec_rel)

    orig_cols = list(df)

    # Thanks to Chai Kian Ming Adam for the idea of using LOG instead of SQRT
    # add LOG
    if False:
        toadd = []
        for divisor in divisors:
            x = "log2("+divisor+")"
            df[x] = df[divisor].apply(np.log2)
            toadd.append(x)
        divisors.extend(toadd)

    # relative data
    cols = list(df)
    for col in cols:
        if ("rdb" in col or "cl." in col) and "restart_type" not in col and "tot_cls_in" not in col:
            for divisor in divisors:
                divide(divisor, col)
                divide(col, divisor)

    # smaller/larger than
    print("smaller-or-greater comparisons...")
    if False:
        for col in cols:
            if "avg" in col or "median" in col:
                for divisor in divisors:
                    larger_than(col, divisor)

    # smaller-or-greater comparisons
    #if not short:
        #larger_than("cl.antec_data_sum_sizeHistLT_avg", "cl.num_total_lits_antecedents")
        #larger_than("cl.overlapHistLT_avg", "cl.num_overlap_literals")

    # print("flatten/list...")
    #old = set(df.columns.values.flatten().tolist())
    #df = df.dropna(how="all")
    #new = set(df.columns.values.flatten().tolist())
    #if len(old - new) > 0:
        #print("ERROR: a NaN number turned up")
        #print("columns: ", (old - new))
        #assert(False)
        #exit(-1)

def print_datatypes(df):
    pd.set_option('display.max_rows', len(df.dtypes))
    print(df.dtypes)
    pd.reset_option('display.max_rows')

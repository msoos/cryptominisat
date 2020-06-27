#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2018  Mate Soos
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

    def delete_and_create_all(self):
        tables = ["short", "long", "forever"]

        t = time.time()
        q = """DROP TABLE IF EXISTS `used_later`;"""
        self.c.execute(q)

        for table in tables:
            q = """
                DROP TABLE IF EXISTS `used_later_{name}`;
            """
            self.c.execute(q.format(name=table))

        # Create and fill used_later_X tables
        q_create = """
        create table `used_later_{name}` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later_{name}` bigint(20),
            `offset` bigint(20) NOT NULL
        );"""
        self.c.execute(q_create.format(name="short"))
        self.c.execute(q_create.format(name="long"))
        self.c.execute(q_create.format(name="forever"))

        # Create used_later table
        t = time.time()
        q = """
        create table `used_later` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later` bigint(20)
        );"""
        self.c.execute(q)

        idxs = """
        create index `used_later_{name}_idx3` on `used_later_{name}` (used_later_{name});
        create index `used_later_{name}_idx1` on `used_later_{name}` (`clauseID`, rdb0conflicts, offset);
        create index `used_later_{name}_idx2` on `used_later_{name}` (`clauseID`, rdb0conflicts, used_later_{name}, offset);"""

        for table in tables:
            for l in idxs.format(name=table).split('\n'):
                self.c.execute(l)

        idxs = """
        create index `used_later_idx3` on `used_later` (`used_later`);
        create index `used_later_idx1` on `used_later` (`clauseID`, rdb0conflicts);
        create index `used_later_idx2` on `used_later` (`clauseID`, rdb0conflicts, used_later);
        """
        t = time.time()
        for l in idxs.split('\n'):
            self.c.execute(l)

        print("used_later* dropped and recreated T: %-3.2f s" % (time.time() - t))

    def fill_used_later(self, used_clauses="used_clauses"):
        # Fill used_later table
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
        left join {used_clauses} as ucl

        -- for any point later than now
        -- reduceDB is always present, used_later may not be, hence left join
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > rdb0.conflicts)

        join cl_last_in_solver
        on cl_last_in_solver.clauseID = rdb0.clauseID

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.conflicts >= rdb0.conflicts

        group by rdb0.conflicts, rdb0.clauseID;"""

        self.c.execute(q.format(used_clauses=used_clauses))
        print("used_later filled T: %-3.2f s" % (time.time() - t))

    def fill_used_later_X(self, name, duration, offset=0,
                          used_clauses="used_clauses",
                          forever=False):
        q_fill = """
        insert into used_later_{name}
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later_{name}`,
        `offset`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl.used_at) as `used_later_{name}`
        , {offset}

        FROM
        reduceDB as rdb0
        left join {used_clauses} as ucl

        -- reduceDB is always present, {used_clauses} may not be, hence left join
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > (rdb0.conflicts+{offset})
            and ucl.used_at <= (rdb0.conflicts+{duration}+{offset}))

        join cl_last_in_solver
        on cl_last_in_solver.clauseID = rdb0.clauseID

        WHERE
        rdb0.clauseID != 0
        and (cl_last_in_solver.conflicts >= (rdb0.conflicts + {duration} + {offset})
        or 1=={forever})

        group by rdb0.clauseID, rdb0.conflicts;"""

        t = time.time()
        self.c.execute(q_fill.format(
            name=name, used_clauses=used_clauses,
            duration=duration, offset=offset, forever=int(forever)))
        print("used_later_%s filled T: %-3.2f s" %
              (name, time.time() - t))


def write_mit_header(f):
    f.write("""/******************************************
Copyright (c) 2018, Mate Soos

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
            if l != "":
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
        name = "(" + dividend + "/" + divisor + ")"

    df[name] = df[dividend].div(df[divisor]+0.000000001)
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
        print("Will delete index:", row[0])
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

def add_computed_szfeat_for_clustering(df):
    print("Adding computed clustering features...")

    todiv = []
    for x in list(df):
        not_these = ["std", "min", "mean", "_per_", "binary", "vcg", "pnr", "horn", "max"]
        if "szfeat_cur" in x and x[-3:] != "var" and not_inside(not_these, x):
            todiv.append(x)

    # relative data
    cols = list(df)
    for col in cols:
        if "szfeat_cur" in col:
            for divper in todiv:
                df["("+col+"/"+divper+")"] = df[col]/df[divper]
                df["("+col+"<"+divper+")"] = (df[col] < df[divper]).astype(int)

    print("Added computed features.")


# to check for too large or NaN values:
def check_too_large_or_nan_values(df, features):
    print("Checking for too large or NaN values...")
    # features = df.columns.values.flatten().tolist()
    index = 0
    for index, row in df[features].iterrows():
        for x, name in zip(row, features):
            try:
                np.isfinite(x)
            except:
                print("Name:", name)
                print("Prolbem with value:", x)
                print(row)

            if not np.isfinite(x) or x > np.finfo(np.float32).max:
                print("issue with data for features: ", name, x)
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
    print("Mean squared error is: ", main_error)

    for start,end in [(0,10), (1,10), (10, 100), (100, 1000), (1000,10000), (10000, 1000000)]:
        x = "--> Strata  %10d <= %20s < %10d " % (start, to_predict, end)
        myfilt = data[(data[to_predict] >= start) & (data[to_predict] < end)]
        X_data = myfilt[features]
        y_data = myfilt[to_predict]
        y = " -- elements: {:20}".format(str(X_data.shape))
        if myfilt.shape[0] <= 1:
            #print("Cannot calculate regression error, too few elements")
            error = -1
        else:
            y_pred = clf.predict(X_data)
            error = sklearn.metrics.mean_squared_error(y_data, y_pred)
        print("%s %s msqe: %13.1lf" % (x, y, error))

    for start,end in [(0,3), (3,8), (8, 15), (15, 25), (25,50), (50, 100), (100, 1000000)]:
        x = "--> Strata  %10d <= %20s < %10d " % (start, "rdb0.glue", end)
        myfilt = data[(data["rdb0.glue"] >= start) & (data["rdb0.glue"] < end)]
        X_data = myfilt[features]
        y_data = myfilt[to_predict]
        y = " -- elements: {:20}".format(str(X_data.shape))
        if myfilt.shape[0] <= 1:
            error = -1
        else:
            y_pred = clf.predict(X_data)
            error = sklearn.metrics.mean_squared_error(y_data, y_pred)
        print("%s %s msqe: %13.1lf" % (x, y, error))

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


def clear_data_from_str(df):
    values2nums = {'luby': 0, 'glue': 1, 'geom': 2}
    df.loc[:, ('cl.cur_restart_type')] = \
        df.loc[:, ('cl.cur_restart_type')].map(values2nums)

    df.loc[:, ('rdb0.cur_restart_type')] = \
        df.loc[:, ('rdb0.cur_restart_type')].map(values2nums)

    df.loc[:, ('rst_cur.restart_type')] = \
        df.loc[:, ('rst_cur.restart_type')].map(values2nums)

    if "rdb1.cur_restart_type" in df:
        df.loc[:, ('rdb1.cur_restart_type')] = \
            df.loc[:, ('rdb1.cur_restart_type')].map(values2nums)


def output_to_classical_dot(clf, features, fname):
    sklearn.tree.export_graphviz(clf, out_file=fname,
                                 feature_names=features,
                                 class_names=clf.classes_,
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


def cldata_add_computed_features(df, verbose):
    print("Adding computed features...")
    del df["cl.conflicts"]
    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    larger_than = functools.partial(helper_larger_than, df=df, features=list(df), verb=verbose)
    add = functools.partial(helper_add, df=df, features=list(df), verb=verbose)

    # relative overlaps
    print("Relative overlaps...")
    divide("cl.num_total_lits_antecedents", "cl.antec_sum_size_hist")

    # deleting this feature which is NONE
    del df["cl.antecedents_glue_long_reds_avg"]
    del df["cl.antecedents_glue_long_reds_max"]
    del df["cl.antecedents_glue_long_reds_min"]
    del df["cl.antecedents_glue_long_reds_var"]
    del df["cl.antecedents_long_red_age_avg"]
    del df["cl.antecedents_long_red_age_var"]
    del df["cl.decision_level_hist"]
    del df["sum_cl_use.first_confl_used"]
    del df["sum_cl_use.last_confl_used"]

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    print("size/glue/trail rel...")
    divide("cl.trail_depth_level", "cl.trail_depth_level_hist")

    rst_cur_all_props = add(["rst_cur.propBinRed",
                            "rst_cur.propBinIrred",
                            "rst_cur.propLongRed",
                            "rst_cur.propLongIrred"])

    divide("cl.num_total_lits_antecedents", "cl.num_antecedents")

    # sum RDB
    orig_cols = list(df)
    for col in orig_cols:
        if ("rdb0" in col) and "restart_type" not in col:
            col2 = col.replace("rdb0", "rdb1")
            cboth = "("+col+"+"+col2+")"
            df[cboth] = df[col]+df[col2]

    rdb0_act_ranking_rel = divide("rdb0.act_ranking", "rdb0.tot_cls_in_db", name="rdb0_act_ranking_rel")
    rdb1_act_ranking_rel = divide("rdb1.act_ranking", "rdb1.tot_cls_in_db", name="rdb1_act_ranking_rel")
    rdb0_plus_rdb1_ranking_rel = add([rdb0_act_ranking_rel, rdb1_act_ranking_rel])

    divide("rdb0.sum_uip1_used", "cl.time_inside_solver")
    divide("rdb1.sum_uip1_used", "cl.time_inside_solver")
    divide("rdb0.sum_propagations_made", "cl.time_inside_solver")
    divide("rdb1.sum_propagations_made", "cl.time_inside_solver")

    divisors = [
        "cl.size_hist"
        , "cl.glue_hist"
        , "rdb0.glue"
        , "cl.orig_glue"
        , "cl.glue_before_minim"
        , "cl.glue_hist_queue"
        , "cl.glue_hist_long"
        # , "cl.decision_level_hist"
        , "cl.num_resolutions_hist_lt"
        # , "cl.trail_depth_level_hist"
        # , "cl.backtrack_level_hist"
        , "cl.branch_depth_hist_queue"
        , "cl.antec_overlap_hist"
        , "(cl.num_total_lits_antecedents/cl.num_antecedents)"
        , "cl.num_antecedents"
        , rdb0_act_ranking_rel
        , rdb1_act_ranking_rel
        #, "szfeat_cur.var_cl_ratio"
        , "cl.time_inside_solver"
        #, "((double)(rdb0.act_ranking_rel+rdb1.act_ranking_rel)/2.0)"
        #, "sqrt(rdb0.act_ranking_rel)"
        #, "sqrt(rdb1.act_ranking_rel)"
        #, "sqrt(rdb0_and_rdb1.act_ranking_rel_avg)"
        # , "cl.num_overlap_literals"
        # , "rst_cur.resolutions"
        #, "rdb0.act_ranking_top_10"
        ]

    # Thanks to Chai Kian Ming Adam for the idea of using LOG instead of SQRT
    # add LOG
    if True:
        toadd = []
        for divisor in divisors:
            x = "log2("+divisor+")"
            df[x] = df[divisor].apply(np.log2)
            toadd.append(x)
        divisors.extend(toadd)

    # relative data
    cols = list(df)
    for col in cols:
        if ("rdb" in col or "cl." in col or "rst" in col) and "restart_type" not in col and "tot_cls_in" not in col and "rst_cur" not in col:
            for divisor in divisors:
                divide(divisor, col)
                divide(col, divisor)

    divisors.extend([
        rst_cur_all_props
        , "rdb0.last_touched_diff"
        , "rdb0.used_for_uip_creation"
        , "rdb1.used_for_uip_creation"
        , "rdb0.propagations_made"
        , "rdb1.propagations_made"
        , "rdb0.sum_propagations_made"
        , "(rdb0.sum_propagations_made/cl.time_inside_solver)"
        , "(rdb1.sum_propagations_made/cl.time_inside_solver)"
        , "(rdb0.sum_uip1_used/cl.time_inside_solver)"
        , "(rdb1.sum_uip1_used/cl.time_inside_solver)"])

    # smaller/larger than
    if False:
        for col in cols:
            if ("rdb" in col or "cl." in col or "rst" in col) and "restart_type" not in col:
                for divisor in divisors:
                    larger_than(col, divisor)

    # satzilla stuff
    if False:
        divisors = [
            "szfeat_cur.numVars",
            "szfeat_cur.numClauses",
            "szfeat_cur.var_cl_ratio",
            "szfeat_cur.avg_confl_size",
            "szfeat_cur.avg_branch_depth",
            "szfeat_cur.red_glue_distr_mean"
        ]
        for col in orig_cols:
            if "szfeat" in col:
                for divisor in divisors:
                    if "min" not in divisor:
                        divide(col, divisor)
                        larger_than(col, divisor)

    # relative RDB
    if True:
        print("Relative RDB...")
        for col in orig_cols:
            if "rdb0" in col and "restart_type" not in col:
                rdb0 = col
                rdb1 = col.replace("rdb0", "rdb1")
                larger_than(rdb0, rdb1)

                raw_col = col.replace("rdb0.", "")
                if raw_col not in ["sum_propagations_made", "propagations_made", "dump_no", "conflicts_made", "used_for_uip_creation", "sum_uip1_used", "activity_rel", "last_touched_diff", "ttl"]:
                    print(rdb0)
                    divide(rdb0, rdb1)

    # smaller-or-greater comparisons
    print("smaller-or-greater comparisons...")
    larger_than("cl.antec_sum_size_hist", "cl.num_total_lits_antecedents")
    larger_than("cl.antec_overlap_hist", "cl.num_overlap_literals")

    # print("flatten/list...")
    #old = set(df.columns.values.flatten().tolist())
    #df = df.dropna(how="all")
    #new = set(df.columns.values.flatten().tolist())
    #if len(old - new) > 0:
        #print("ERROR: a NaN number turned up")
        #print("columns: ", (old - new))
        #assert(False)
        #exit(-1)

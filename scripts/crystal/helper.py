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
import crystalcodegen as ccg
import ast
import math
import time
import mlflow
import os.path
import sqlite3
import functools
from pprint import pprint


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


def print_confusion_matrix(cm, classes,
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
    mlflow.log_metric(title, cm[0][0])
    np.set_printoptions(precision=2)
    print(cm)


def calc_min_split_point(df, min_samples_split):
    split_point = int(float(df.shape[0])*min_samples_split)
    if split_point < 10:
        split_point = 10
    print("Minimum split point: ", split_point)
    return split_point


def conf_matrixes(data, features, to_predict, clf, toprint, average="binary"):
    # get data
    X_data = data[features]
    y_data = data[to_predict]
    print("Number of elements:", X_data.shape)
    if data.shape[0] <= 1:
        print("Cannot calculate confusion matrix, too few elements")
        return None, None, None, None

    # Preform prediction
    y_pred = clf.predict(X_data)

    # calc acc, precision, recall
    accuracy = sklearn.metrics.accuracy_score(
        y_data, y_pred)
    mlflow.log_metric(toprint + " -- accuracy", accuracy)

    precision = sklearn.metrics.precision_score(
        y_data, y_pred, pos_label="OK", average=average)
    mlflow.log_metric(toprint + " -- precision", precision)

    recall = sklearn.metrics.recall_score(
        y_data, y_pred, pos_label="OK", average=average)
    mlflow.log_metric(toprint + " -- recall", recall)

    # ROC AUC
    values2nums = {'OK': 1, 'BAD': 0}
    predsi = np.array([values2nums[xi] for xi in y_pred])
    y_testi = pd.DataFrame(y_data)["x.class"].map(values2nums).squeeze()
    try:
        roc_auc = sklearn.metrics.roc_auc_score(y_testi, predsi)
    except:
        print("NOTE: ROC AUC is set to 0 because of completely one-sided OK/BAD")
        roc_auc = 0
    mlflow.log_metric(toprint + " -- roc_auc", roc_auc)

    print("%s prec : %-3.4f  recall: %-3.4f accuracy: %-3.4f roc_auc: %-3.4f"
          % (toprint, precision, recall, accuracy, roc_auc))

    # Plot confusion matrix
    cnf_matrix = sklearn.metrics.confusion_matrix(
        y_true=y_data, y_pred=y_pred)
    print_confusion_matrix(
        cnf_matrix, classes=clf.classes_,
        title='Confusion matrix without normalization -- %s' % toprint)
    print_confusion_matrix(
        cnf_matrix, classes=clf.classes_, normalize=True,
        title='Normalized confusion matrix -- %s' % toprint)

    return precision, recall, accuracy, roc_auc


def check_file_exists(fname):
    try:
        f = open(fname)
    except IOError:
        print("File '%s' not accessible" % fname)
        exit(-1)
    finally:
        f.close()


def calc_greedy_best_features(top_feats, get_best_topn_feats, myobj):
    best_features = [top_feats[0]]
    for i in range(get_best_topn_feats-1):
        print("*** Round %d Best feature set until now: %s"
              % (i, best_features))

        best_sum = 0.0
        best_feat = None
        feats_to_try = [i for i in top_feats if i not in best_features]
        print("Will try to get next best from ", feats_to_try)
        for feat in feats_to_try:
            this_feats = list(best_features)
            this_feats.append(feat)
            print("Trying feature set: ", this_feats)
            mysum = myobj.one_classifier(this_feats, "x.class", final=True)
            print("Reported mysum: ", mysum)
            if mysum > best_sum:
                best_sum = mysum
                best_feat = feat
                print("-> Making this best accuracy")

        print("*** Best feature for round %d was: %s with mysum: %lf"
              % (i, best_feat, mysum))
        best_features.append(best_feat)

        print("\n\n")
        print("Final best feature selection is: ", best_features)

    return best_features


def clear_data_from_str_na(df):
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

    df[df.select_dtypes(include='float').columns] = df[df.select_dtypes(include='float').columns].fillna(0)


def filter_min_avg_dump_no(df, min_avg_dumpno):
    print("Filtering to minimum average dump_no of {min_avg_dumpno}...".format(
        min_avg_dumpno=min_avg_dumpno))
    print("Pre-filter number of datapoints:", df.shape)
    df_nofilter = df.copy()

    df['rdb0.dump_no'].replace(['None'], 0, inplace=True)
    df.fillna(0, inplace=True)
    # print(df[["fname", "sum_cl_use.num_used"]])
    files = df[["fname", "rdb0.dump_no"]].groupby("fname").mean()
    fs = files[files["rdb0.dump_no"] >= min_avg_dumpno].index.values
    filenames = list(fs)
    print("Left with {num} files".format(num=len(filenames)))
    df = df[df["fname"].isin(fs)].copy()

    print("Post-filter number of datapoints:", df.shape)
    return df_nofilter, df

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


class CodeWriter:
    def __init__(self, clf, features, func_name, code_file, verbose):
        self.f = open(code_file, 'w')
        self.code_file = code_file
        write_mit_header(self.f)
        self.clf = clf
        self.feat = features
        self.func_name = func_name
        self.verbose = verbose
        self.max_pred_val = 1

    def clean_up(self):
        x = [["file_header", 0], ["per_func_defines", 4], ["func_call", 8], ["func_signature", 4]]
        for toclean, num_spaces in x:
            cleaned = ""
            for line in getattr(self, toclean).split("\n"):
                if line.strip() == "":
                    continue
                cleaned += " "*num_spaces + line.strip() + "\n"

            setattr(self, toclean, cleaned)

    def print_full_code(self):
        self.f.write("""
#ifndef {mydef}
#define {mydef}\n""".format(mydef=self.func_name.upper()+"_H"))
        self.f.write(self.file_header)

        num_trees = 1
        if type(self.clf) is sklearn.tree.DecisionTreeClassifier:
            self.f.write("""
static int estimator_{funcname}_0(\n{func_signature}) {{\n
{per_func_defines}""".format(
    funcname=self.func_name,
    func_signature=self.func_signature,
    per_func_defines=self.per_func_defines
    ))
            if self.verbose:
                print(self.clf)
                print(self.clf.get_params())
            self.get_code(self.clf, 1)
            self.f.write("}\n")
        else:
            num_trees = len(self.clf.estimators_)
            for tree, i in zip(self.clf.estimators_, range(1000)):
                self.f.write("""
static int estimator_{funcname}_{est_num}(\n{func_signature}) {{\n
{per_func_defines}""".format(
    est_num=i,
    funcname=self.func_name,
    func_signature=self.func_signature,
    per_func_defines=self.per_func_defines
    ))
                self.get_code(tree, 1)
                self.f.write("}\n")

        #######################
        # Final tally
        self.f.write("""
static int {funcname}(\n{func_signature}) {{
    int votes[{max_pred_val}] = {{ {init0} }};
    int est;

""".format(
    funcname=self.func_name,
    func_signature=self.func_signature,
    max_pred_val=self.max_pred_val,
    init0=("0,"*self.max_pred_val).rstrip(",")
    ))

        for i in range(num_trees):
            self.f.write("""
    est = estimator_{funcname}_{est_num}(\n{func_call}\n    );
    votes[est]++;
""".format(est_num=i, funcname=self.func_name, func_call=self.func_call))
        self.f.write("""

    int best = 0;
    int best_val = 0;
    for(int i = 0; i < {max_pred_val}; i++) {{
        if (votes[i] > best_val) {{
            best = i;
            best_val = votes[i];
        }}
    }}
    return best;
}}

//namespace end
}}

//ifdef end
#endif
""".format(max_pred_val=self.max_pred_val))

        print("Wrote code to: ", self.code_file)

    def recurse(self, left, right, threshold, features, node, tabs):
        tabsize = tabs*"    "
        if (threshold[node] != -2):
            # decision node
            feat_name = features[node]
            feat_name = feat_name.replace(".", "_")
            feat_name = ccg.to_source(ast.parse(feat_name), self.fix_feat_name)

            self.f.write("{tabs}if ( {feat} <= {threshold} ) {{\n".format(
                tabs=tabsize,
                feat=feat_name,threshold=str(threshold[node])))

            # recruse left
            if left[node] != -1:
                self.recurse(left, right, threshold,
                             features, left[node], tabs+1)

            self.f.write("{tabs}}} else {{\n".format(tabs=tabsize))

            # recurse right
            if right[node] != -1:
                self.recurse(left, right, threshold,
                             features, right[node], tabs+1)

            self.f.write("{tabs}}}\n".format(tabs=tabsize))
        else:
            # leaf node
            scores = self.value[node][0]
            self.max_pred_val = max(self.max_pred_val, len(scores))
            scores_sum = scores.sum()

            largest = 0
            largest_score = 0
            for score, i in zip(scores, range(10000)):
                if score > largest_score:
                    largest = i
                    largest_score = score
                    assert largest_score == scores[largest]

            self.f.write("{tabs}return {winner};\n".format(
                tabs=tabsize, winner=largest))

    def get_code(self, clf, starttab=0):
        left = clf.tree_.children_left
        right = clf.tree_.children_right
        threshold = clf.tree_.threshold
        if self.verbose:
            print("Node count:", clf.tree_.node_count)
            print("Left: %s Right: %s Threshold: %s" %
                  (left, right, threshold))
            print("clf.tree_.feature:", clf.tree_.feature)
        features = [self.feat[i % len(self.feat)] for i in clf.tree_.feature]
        self.value = clf.tree_.value

        self.recurse(left, right, threshold, features, 0, starttab)

def cldata_add_computed_features(df, verbose):
    print("Adding computed features...")
    del df["cl.conflicts"]
    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    larger_than = functools.partial(helper_larger_than, df=df, features=list(df), verb=verbose)
    add = functools.partial(helper_add, df=df, features=list(df), verb=verbose)

    # relative overlaps
    print("Relative overlaps...")
    divide("cl.num_total_lits_antecedents", "cl.antec_sum_size_hist")

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

    divisors = [
        "cl.size_hist"
        , "cl.glue_hist"
        , "cl.glue"
        , "cl.old_glue"
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
                divide(col, divisor)

    divisors.extend([
        rst_cur_all_props
        , "rdb0.last_touched_diff"
        , "rdb0.sum_delta_confl_uip1_used"
        , "rdb0.used_for_uip_creation"
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
                if raw_col not in ["propagations_made", "dump_no", "conflicts_made", "used_for_uip_creation", "sum_uip1_used", "clause_looked_at", "sum_delta_confl_uip1_used", "activity_rel", "last_touched_diff", "ttl"]:
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

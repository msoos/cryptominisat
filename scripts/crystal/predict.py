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

import operator
import re
import pandas as pd
import pickle
import sklearn
import sklearn.svm
import sklearn.tree
import sklearn.cluster
from sklearn.preprocessing import StandardScaler
import argparse
import sys
import numpy as np
import sklearn.metrics
import time
import itertools
import matplotlib.pyplot as plt
import sklearn.ensemble
import os
import helper
ver = sklearn.__version__.split(".")
if int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split


def add_computed_features(df):
    print("Adding computed features...")

    # relative overlaps
    print("Relative overlaps...")
    df["cl.antec_num_total_lits_rel"] = df["cl.num_total_lits_antecedents"] / \
        df["cl.antec_sum_size_hist"]

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    print("size/glue/trail rel...")
    df["(cl.trail_depth_level/cl.trail_depth_level_hist)"] = df["cl.trail_depth_level"] / \
        df["cl.trail_depth_level_hist"]

    df["rst_cur.all_props"] = df["rst_cur.propBinRed"] + df["rst_cur.propBinIrred"] + \
        df["rst_cur.propLongRed"] + df["rst_cur.propLongIrred"]
    df["(cl.num_total_lits_antecedents/cl.num_antecedents)"] = df["cl.num_total_lits_antecedents"] / \
        df["cl.num_antecedents"]

    # sum RDB
    orig_cols = list(df)
    for col in orig_cols:
        if ("rdb0" in col) and "restart_type" not in col:
            col2 = col.replace("rdb0", "rdb1")
            cboth = "("+col+"+"+col2+")"
            df[cboth] = df[col]+df[col2]

    df["rdb0.act_ranking_rel"] = df["rdb0.act_ranking"]/df["rdb0.tot_cls_in_db"]
    df["rdb1.act_ranking_rel"] = df["rdb1.act_ranking"]/df["rdb1.tot_cls_in_db"]
    df["(rdb0.act_ranking_rel+rdb1.act_ranking_rel)/2.0)"] = \
        (df["rdb0.act_ranking_rel"]+df["rdb1.act_ranking_rel"])/2

    df["(rdb0.sum_uip1_used/cl.time_inside_solver)"] = df["rdb0.sum_uip1_used"] / \
        df["cl.time_inside_solver"]
    df["(rdb1.sum_uip1_used/cl.time_inside_solver)"] = df["rdb1.sum_uip1_used"] / \
        df["cl.time_inside_solver"]

    todiv = [
        "cl.size_hist"
        , "cl.glue_hist"
        , "cl.glue"
        , "cl.old_glue"
        , "cl.glue_hist_queue"
        , "cl.glue_hist_long"
        # , "cl.decision_level_hist"
        , "cl.num_antecedents_hist"
        # , "cl.trail_depth_level_hist"
        # , "cl.backtrack_level_hist"
        , "cl.branch_depth_hist_queue"
        , "cl.antec_overlap_hist"
        , "(cl.num_total_lits_antecedents/cl.num_antecedents)"
        , "cl.num_antecedents"
        , "rdb0.act_ranking_rel"
        , "rdb1.act_ranking_rel"
        , "szfeat_cur.var_cl_ratio"
        , "cl.time_inside_solver"
        #, "rdb1.act_ranking_rel"
        #, "((double)(rdb0.act_ranking_rel+rdb1.act_ranking_rel)/2.0)"
        #, "sqrt(rdb0.act_ranking_rel)"
        #, "sqrt(rdb1.act_ranking_rel)"
        #, "sqrt(rdb0_and_rdb1.act_ranking_rel_avg)"
        # , "cl.num_overlap_literals"
        # , "rst_cur.resolutions"
        #, "rdb0.act_ranking_top_10"
        ]

    # add SQRT
    if True:
        toadd = []
        for a in todiv:
            sqrt_name = "sqrt("+a+")"
            df[sqrt_name] = df[a].apply(np.sqrt)
            toadd.append(sqrt_name)
        todiv.extend(toadd)

    # relative data
    cols = list(df)
    for col in cols:
        if ("rdb" in col or "cl." in col or "rst" in col) and "restart_type" not in col:
            for divper in todiv:
                df["("+col+"/"+divper+")"] = df[col]/df[divper]

    todiv.extend([
        "rst_cur.all_props"
        , "rdb0.last_touched_diff"
        , "rdb0.sum_delta_confl_uip1_used"
        , "rdb0.used_for_uip_creation"
        , "(rdb0.sum_uip1_used/cl.time_inside_solver)"
        , "(rdb1.sum_uip1_used/cl.time_inside_solver)"])

    # smaller/larger than
    if False:
        for col in cols:
            if ("rdb" in col or "cl." in col or "rst" in col) and "restart_type" not in col:
                for divper in todiv:
                    df["("+col+"_<_"+divper+")"] = (df[col] < df[divper]).astype(int)

    # satzilla stuff
    if False:
        todiv = [
            "szfeat_cur.numVars",
            "szfeat_cur.numClauses",
            "szfeat_cur.var_cl_ratio",
            "szfeat_cur.avg_confl_size",
            "szfeat_cur.avg_branch_depth",
            "szfeat_cur.red_glue_distr_mean"
        ]
        for col in orig_cols:
            if "szfeat" in col:
                for divper in todiv:
                    if "min" not in divper:
                        df["("+col+"/"+divper+")"] = df[col]/df[divper]
                        df["("+col+"<"+divper+")"] = (df[col]
                                                      < df[divper]).astype(int)
                        pass

    # relative RDB
    if True:
        print("Relative RDB...")
        for col in orig_cols:
            if "rdb0" in col and "restart_type" not in col:
                rdb0 = col
                rdb1 = col.replace("rdb0", "rdb1")
                name_per = col.replace("rdb0", "rdb0_per_rdb1")
                name_larger = col.replace("rdb0", "rdb0_larger_rdb1")
                df[name_larger] = (df[rdb0] > df[rdb1]).astype(int)

                raw_col = col.replace("rdb0.", "")
                if raw_col not in ["propagations_made", "dump_no", "conflicts_made", "used_for_uip_creation", "sum_uip1_used", "clause_looked_at", "sum_delta_confl_uip1_used", "activity_rel", "last_touched_diff", "ttl"]:
                    print(rdb0)
                    df[name_per] = df[rdb0]/df[rdb1]

    # smaller-or-greater comparisons
    print("smaller-or-greater comparisons...")
    df["(cl.antec_sum_size_hist<cl.num_total_lits_antecedents)"] = \
        (df["cl.antec_sum_size_hist"] < df["cl.num_total_lits_antecedents"]).astype(int)

    df["(cl.antec_overlap_hist<cl.num_overlap_literals)"] = \
        (df["cl.antec_overlap_hist"] < df["cl.num_overlap_literals"]).astype(int)

    # print("flatten/list...")
    #old = set(df.columns.values.flatten().tolist())
    #df = df.dropna(how="all")
    #new = set(df.columns.values.flatten().tolist())
    #if len(old - new) > 0:
        #print("ERROR: a NaN number turned up")
        #print("columns: ", (old - new))
        #assert(False)
        #exit(-1)

def check_long_short():
    if options.longsh is None:
        print("ERROR: You must give option '--name' as 'short' or 'long'")
        assert False
        exit(-1)


class Learner:
    def __init__(self, df, funcname, fname, cluster_no):
        self.df = df
        self.func_name = funcname
        self.fname = fname
        self.cluster_no = cluster_no

    def filter_percentile(self, df, features, perc):
        low = df.quantile(perc, axis=0)
        high = df.quantile(1.0-perc, axis=0)
        df2 = df.copy()
        for i in features:
            df2 = df2[(df2[i] >= low[i]) & (df2[i] <= high[i])]
            print("Filtered to %f on %-30s, shape now: %s" %
                  (perc, i, df2.shape))

        print("Original size:", df.shape)
        print("New size:", df2.shape)
        return df2

    def filtered_conf_matrixes(self, dump_no, data, features, to_predict, clf, toprint="test"):
        # filter test data
        if dump_no is not None:
            print("\nCalculating confusion matrix -- dump_no == %s" % dump_no)
            data2 = data[data["rdb0.dump_no"] == dump_no]
        else:
            print("\nCalculating confusion matrix -- ALL dump_no")
            data2 = data

        return helper.conf_matrixes(data2, features, to_predict, clf, toprint)

    @staticmethod
    def fix_feat_name(x):
        x = re.sub(r"dump_no", r"dump_number", x)
        x = re.sub(r"^cl_", r"", x)
        x = re.sub(r"^rdb0_", r"", x)

        if x == "dump_number":
            pass
        elif x == "last_touched_diff":
            pass
        elif x == "act_ranking_top_10":
            pass
        elif x == "act_ranking_rel":
            pass
        elif x == "time_inside_solver":
            pass
        elif x == "size":
            x = "cl->" + x + "()"
        else:
            x = "cl->stats." + x

        return x

    def one_classifier(self, features, to_predict, final, write_code=False):
        print("-> Number of features  :", len(features))
        print("-> Number of datapoints:", self.df.shape)
        print("-> Predicting          :", to_predict)

        # get smaller part to work on
        # also, copy it so we don't get warning about setting a slice of a DF
        if options.only_pecr >= 0.98:
            df = self.df.copy()
        else:
            _, df_tmp = train_test_split(self.df, test_size=options.only_pecr)
            df = df_tmp.copy()
            print("-> Number of datapoints after applying '--only':", df.shape)

        if options.check_row_data:
            helper.check_too_large_or_nan_values(df, features)

        # count good and bad
        files = df[["x.class", "rdb0.dump_no"]].groupby("x.class").count()
        if files["rdb0.dump_no"].index[0] == "BAD":
            bad = files["rdb0.dump_no"][0]
            good = files["rdb0.dump_no"][1]
        else:
            bad = files["rdb0.dump_no"][1]
            good = files["rdb0.dump_no"][0]

        assert bad > 0, "No need to train, data only contains BAD"
        assert good > 0, "No need to train, data only contains GOOD"

        print("Number of BAD  elements        : %-6d" % bad)
        print("Number of GOOD elements        : %-6d" % good)

        # balance it out
        prefer_ok = float(bad)/float(good)
        print("Balanced OK preference would be: %-6.3f" % prefer_ok)

        # apply inbalance from option given
        prefer_ok *= options.prefer_ok
        print("Option to prefer OK is set to  : %-6.3f" % options.prefer_ok)
        print("Final OK preference is         : %-6.3f" % prefer_ok)

        train, test = train_test_split(df, test_size=0.33)
        X_train = train[features]
        y_train = train[to_predict]

        t = time.time()
        clf = None
        # clf = sklearn.linear_model.LogisticRegression()

        if final:
            split_point = helper.calc_min_split_point(
                df, options.min_samples_split)

            clf_tree = sklearn.tree.DecisionTreeClassifier(
                max_depth=options.tree_depth,
                class_weight={"OK": prefer_ok, "BAD": 1},
                min_samples_split=split_point)
            clf_svm_pre = sklearn.svm.SVC(C=500, gamma=10**-5)
            clf_svm = sklearn.ensemble.BaggingClassifier(
                clf_svm_pre,
                n_estimators=3,
                max_samples=0.5, max_features=0.5)
            clf_logreg = sklearn.linear_model.LogisticRegression(
                C=0.3,
                penalty="l1")
            clf_forest = sklearn.ensemble.RandomForestClassifier(
                n_estimators=options.num_trees,
                max_features="sqrt",
                class_weight={"OK": prefer_ok, "BAD": 1},
                min_samples_leaf=split_point)

            if options.final_is_tree:
                clf = clf_tree
            elif options.final_is_svm:
                clf = clf_svm
            elif options.final_is_logreg:
                clf = clf_logreg
            elif options.final_is_forest:
                clf = clf_forest
            elif options.final_is_voting:
                mylist = [["forest", clf_forest], [
                    "svm", clf_svm], ["logreg", clf_logreg]]
                clf = sklearn.ensemble.VotingClassifier(mylist)
            else:
                print(
                    "ERROR: You MUST give one of: tree/forest/svm/logreg/voting classifier")
                exit(-1)
        else:
            clf = sklearn.ensemble.RandomForestClassifier(
                n_estimators=1000,
                max_features="sqrt",
                class_weight={"OK": prefer_ok, "BAD": 1})

        del df
        clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time() - t))

        if not final:
            best_features = helper.print_feature_ranking(
                clf, X_train,
                top_num_features=options.top_num_features,
                features=features,
                plot=options.show)
        else:
            if options.dot is not None:
                if not options.final_is_tree:
                    print("ERROR: You cannot use the DOT function on non-trees")
                    exit(-1)

                #for filt in [1, 3, 10, 10000]:
                    #x = "Keep these clauses -- filtered to %d and smaller dump_no" % filt
                    #print(x)
                    #xdf = df[df["rdb0.dump_no"] <= filt]
                    #helper.output_to_dot(
                        #self.filter_percentile(df, features, options.filter_dot),
                        #clf, features, to_predict, x, xdf)
                    #del xdf

                helper.output_to_classical_dot(
                    clf, features,
                    fname=options.dot + "-" + self.func_name)

            if options.basedir and write_code:
                c = helper.CodeWriter(clf, features, self.func_name, self.fname, options.verbose)
                c.func_signature = """
                const CMSat::Clause* cl
                , const uint64_t sumConflicts
                , const uint32_t last_touched_diff
                , const double   act_ranking_rel
                , const uint32_t act_ranking_top_10
                """
                c.func_call = """
                cl
                , sumConflicts
                , last_touched_diff
                , act_ranking_rel
                , act_ranking_top_10
                """
                c.per_func_defines = """
                uint32_t time_inside_solver = sumConflicts - cl->stats.introduced_at_conflict;
                """
                c.file_header = """
                #include "clause.h"
                #include "reducedb.h"
                #include <cmath>

                namespace CMSat {
                """
                c.fix_feat_name = self.fix_feat_name
                c.print_full_code()

        print("--------------------------")
        print("-   Filtered test data   -")
        print("-   Cluster: %04d        -" % self.cluster_no)
        print("--------------------------")
        for dump_no in [1, 3, 10, 20, 40, None, 1]:
            prec, recall, acc = self.filtered_conf_matrixes(
                dump_no, test, features, to_predict, clf)

        print("--------------------------------")
        print("--      train+test data        -")
        print("-      no cluster applied      -")
        print("--------------------------------")
        for dump_no in [1, None]:
            self.filtered_conf_matrixes(
                dump_no, self.df, features, to_predict, clf)

        # Plot "train" confusion matrix
        print("--------------------------")
        print("-  Filtered train data  - ")
        print("-   Cluster: %04d       -" % self.cluster_no)
        print("-------------------------")
        self.filtered_conf_matrixes(
            dump_no, train, features, to_predict, clf, "train")

        # TODO do L1 regularization
        # TODO do principal component analysis

        if not final:
            return best_features
        else:
            return prec+recall+acc

    def rem_features(self, feat, to_remove):
        feat_less = list(feat)
        for feature in feat:
            for rem in to_remove:
                if rem in feature:
                    feat_less.remove(feature)
                    if options.verbose:
                        print("Removing feature from feat_less:", feature)

        return feat_less

    def learn(self):
        features = self.df.columns.values.flatten().tolist()
        features = self.rem_features(
            features, ["x.a_num_used", "x.class", "x.a_lifetime", "fname", "clust", "sum_cl_use"])
        if options.raw_data_plots:
            pd.options.display.mpl_style = "default"
            self.df.hist()
            self.df.boxplot()

        if not options.only_final:
            top_n_feats = self.one_classifier(features, "x.class", final=False)
            if options.show:
                plt.show()

            if options.get_best_topn_feats is not None:
                greedy_features = helper.calc_greedy_best_features(
                    top_n_feats, options.get_best_topn_feats,
                    self)

            return

        best_features = [
            '((rdb0.sum_uip1_used/cl.time_inside_solver)/sqrt(rdb0.act_ranking_rel))',
            '(rdb0.sum_uip1_used/sqrt(cl.branch_depth_hist_queue))',
            '((rdb0.sum_uip1_used/cl.time_inside_solver)/sqrt((cl.num_total_lits_antecedents/cl.num_antecedents)))',
            #'((rdb0.used_for_uip_creation+rdb1.used_for_uip_creation)/cl.glue)',
            '(rdb0.sum_uip1_used/sqrt(cl.old_glue))'
            ]
        # TODO fill best_features here
        self.one_classifier(best_features, "x.class",
                            final=True,
                            write_code=True)

        if options.show:
            plt.show()


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.pandas"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='PANDASFILE')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        dest="verbose", help="Print more output")
    parser.add_argument("--printfeat", action="store_true", default=False,
                        dest="print_features", help="Print features")
    parser.add_argument("--nocomputed", default=False, action="store_true",
                        dest="no_computed", help="Don't add computed features")

    # tree/forest options
    parser.add_argument("--depth", default=None, type=int,
                        dest="tree_depth", help="Depth of the tree to create")
    parser.add_argument("--split", default=0.01, type=float, metavar="RATIO",
                        dest="min_samples_split", help="Split in tree if this many samples or above. Used as a percentage of datapoints")
    parser.add_argument("--numtrees", default=5, type=int,
                        dest="num_trees", help="How many trees to generate for the forest")

    # generation of predictor
    parser.add_argument("--dot", type=str, default=None,
                        dest="dot", help="Create DOT file")
    parser.add_argument("--filterdot", default=0.05, type=float,
                        dest="filter_dot", help="Filter the DOT output from outliers so the graph looks nicer")
    parser.add_argument("--show", action="store_true", default=False,
                        dest="show", help="Show visual graphs")
    parser.add_argument("--check", action="store_true", default=False,
                        dest="check_row_data", help="Check row data for NaN or float overflow")
    parser.add_argument("--rawplots", action="store_true", default=False,
                        dest="raw_data_plots", help="Display raw data plots")
    parser.add_argument("--basedir", type=str,
                        dest="basedir", help="The base directory of where the CryptoMiniSat source code is")
    parser.add_argument("--conf", default=0, type=int,
                        dest="conf_num", help="Which predict configuration this is")

    # data filtering
    parser.add_argument("--only", default=0.99, type=float,
                        dest="only_pecr", help="Only use this percentage of data")

    # final generator or greedy
    parser.add_argument("--final", default=False, action="store_true",
                        dest="only_final", help="Only generate final predictor")
    parser.add_argument("--greedy", default=None, type=int, metavar="TOPN",
                        dest="get_best_topn_feats", help="Greedy Best K top features from the top N features given by '--top N'")
    parser.add_argument("--top", default=None, type=int, metavar="TOPN",
                        dest="top_num_features", help="Candidates are top N features for greedy selector")

    # type of classifier
    parser.add_argument("--tree", default=False, action="store_true",
                        dest="final_is_tree", help="Final classifier should be a tree")
    parser.add_argument("--svm", default=False, action="store_true",
                        dest="final_is_svm", help="Final classifier should be an svm")
    parser.add_argument("--lreg", default=False, action="store_true",
                        dest="final_is_logreg", help="Final classifier should be a logistic regression")
    parser.add_argument("--forest", default=False, action="store_true",
                        dest="final_is_forest", help="Final classifier should be a forest")
    parser.add_argument("--voting", default=False, action="store_true",
                        dest="final_is_voting", help="Final classifier should be a voting of all of: forest, svm, logreg")

    # classifier options
    parser.add_argument("--prefok", default=2.0, type=float,
                        dest="prefer_ok", help="Prefer OK if >1.0, equal weight if = 1.0, prefer BAD if < 1.0")

    # which one to generate
    parser.add_argument("--name", default=None, type=str,
                        dest="longsh", help="Raw C-like code will be written to this function and file name")
    parser.add_argument("--clust", default=False, action="store_true",
                        dest="use_clusters", help="Use clusters")

    options = parser.parse_args()

    if options.fname is None:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    if options.get_best_topn_feats and options.only_final:
        print("Can't do both greedy best and only final")
        exit(-1)

    if options.top_num_features and options.only_final:
        print("Can't do both top N features and only final")
        exit(-1)

    assert options.min_samples_split <= 1.0, "You must give min_samples_split that's smaller than 1.0"
    if not os.path.isfile(options.fname):
        print("ERROR: '%s' is not a file" % options.fname)
        exit(-1)

    df = pd.read_pickle(options.fname)
    if options.print_features:
        for f in sorted(list(df)):
            print(f)

    # feature manipulation
    if not options.no_computed:
        add_computed_features(df)
    helper.clear_data_from_str(df)

    # cluster setup
    if options.use_clusters:
        used_clusters = df.groupby("clust").nunique()
        clusters = []
        for clust, _ in used_clusters["clust"].iteritems():
            clusters.append(clust)
    else:
        clusters = [0]
        df["clust"] = 0

    # generation
    for clno in clusters:
        funcname = "should_keep_{name}_conf{conf_num}_cluster{clno}".format(
            clno=clno, name=options.longsh, conf_num=options.conf_num)

        fname = "final_predictor_{name}_conf{conf_num}_cluster{clno}.h".format(
            clno=clno, name=options.longsh, conf_num=options.conf_num)

        if options.basedir is not None:
            f = options.basedir+"/"+fname
        else:
            f = None

        learner = Learner(
            df[(df["clust"] == clno)],
            funcname=funcname,
            fname=f,
            cluster_no=clno)

        print("================ Cluster %3d ================" % clno)
        learner.learn()

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

import operator
import re
import time
import argparse
import sys
import os
import itertools
import pandas as pd
import pickle
import sklearn
import sklearn.svm
import sklearn.tree
from sklearn.preprocessing import StandardScaler
import numpy as np
import sklearn.metrics
import matplotlib.pyplot as plt
import sklearn.ensemble
import sklearn.linear_model
import helper
import xgboost as xgb
try:
    import mlflow
except ImportError:
    mlflow_avail = False
else:
    mlflow_avail = True

ver = sklearn.__version__.split(".")
if int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split


def check_long_short():
    if options.longsh is None:
        print("ERROR: You must give option '--name' as 'short' or 'long'")
        assert False
        exit(-1)


class Learner:
    def __init__(self, df, longsh):
        self.df = df
        self.longsh = longsh

    def count_bad_ok(self, df):
        files = df[["x.class", "rdb0.dump_no"]].groupby("x.class").count()
        if files["rdb0.dump_no"].index[0] == 0:
            bad = files["rdb0.dump_no"][0]
            ok = files["rdb0.dump_no"][1]
        else:
            bad = files["rdb0.dump_no"][1]
            ok = files["rdb0.dump_no"][0]

        assert bad > 0, "No need to train, data only contains BAD(0)"
        assert ok > 0, "No need to train, data only contains OK(1)"

        return bad, ok

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

    def filtered_conf_matrixes(self, dump_no, data, features, to_predict, clf,
                               toprint, highlight=False):
        # filter test data
        if dump_no is not None:
            print("\nCalculating confusion matrix -- dump_no == %s" % dump_no)
            toprint += " dump no %d" % dump_no
            data2 = data[data["rdb0.dump_no"] == dump_no]
        else:
            print("\nCalculating confusion matrix -- ALL dump_no")
            data2 = data

        return helper.conf_matrixes(data2, features, to_predict, clf, toprint,
                                    highlight=highlight)

    @staticmethod
    def fix_feat_name(x):
        x = re.sub(r"dump_no", r"dump_no", x)
        x = re.sub(r"^cl_", r"", x)
        x = re.sub(r"^rdb0_", r"", x)

        if x == "dump_no":
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

    def one_classifier(self, features, to_predict, final):
        print("-> Number of features  :", len(features))
        print("-> Number of datapoints:", self.df.shape)
        print("-> Predicting          :", to_predict)

        # get smaller part to work on
        # also, copy it so we don't get warning about setting a slice of a DF
        if options.only_perc >= 0.98:
            df = self.df.copy()
        else:
            df_tmp = self.df.sample(frac=options.only_perc, random_state=prng)
            df = df_tmp.copy()
            print("-> Number of datapoints after applying '--only':", df.shape)

        if options.check_row_data:
            helper.check_too_large_or_nan_values(df, features)

        # count bad and ok
        bad,ok = self.count_bad_ok(df)
        print("Number of BAD  elements        : %-6d" % bad)
        print("Number of OK   elements        : %-6d" % ok)

        # balance it out
        prefer_ok = float(bad)/float(ok)
        print("Balanced OK preference would be: %-6.3f" % prefer_ok)

        # apply inbalance from option given
        prefer_ok *= options.prefer_ok
        print("Option to prefer OK is set to  : %-6.3f" % options.prefer_ok)
        print("Final OK preference is         : %-6.3f" % prefer_ok)
        print("Value distribution of 'dump_no':\n%s" % df["rdb0.dump_no"].value_counts())

        #train, test = train_test_split(df[df["rdb0.dump_no"] == 1], test_size=0.33, random_state=prng)
        train, test = train_test_split(df, test_size=0.33, random_state=prng)

        X_train = train[features]
        y_train = train[to_predict]

        t = time.time()
        clf = None

        if final:
            split_point = helper.calc_min_split_point(
                df, options.min_samples_split)

            clf_tree = sklearn.tree.DecisionTreeClassifier(
                max_depth=options.tree_depth,
                min_samples_split=split_point,
                random_state=prng)
            clf_svm_pre = sklearn.svm.SVC(
                C=500,
                gamma=10**-5,
                random_state=prng)
            clf_svm = sklearn.ensemble.BaggingClassifier(
                clf_svm_pre,
                n_estimators=3,
                max_samples=0.5, max_features=0.5,
                random_state=prng)
            clf_logreg = sklearn.linear_model.LogisticRegression(
                C=0.3,
                random_state=prng)
            clf_forest = sklearn.ensemble.RandomForestClassifier(
                n_estimators=options.num_trees,
                #min_samples_leaf=split_point,
                random_state=prng)

            if options.final_is_tree:
                clf = clf_tree
            elif options.final_is_svm:
                clf = clf_svm
            elif options.final_is_logreg:
                clf = clf_logreg
            elif options.final_is_forest:
                clf = clf_forest
            elif options.final_is_xgboost:
                clf = xgb.XGBRegressor(objective ='binary:logistic')
            elif options.final_is_voting:
                mylist = [["forest", clf_forest], [
                    "svm", clf_svm], ["logreg", clf_logreg]]
                clf = sklearn.ensemble.VotingClassifier(mylist, random_state=prng)
            else:
                print(
                    "ERROR: You MUST give one of: tree/forest/svm/logreg/voting classifier")
                exit(-1)
        else:
            assert options.final_is_forest, "For TOP calculation, we must have --forest"
            clf_forest = sklearn.ensemble.RandomForestClassifier(
                n_estimators=options.num_trees*5,
                max_features="sqrt",
                random_state=prng)

            clf = clf_forest

        del df
        my_sample_w = np.array(y_train.values)
        my_sample_w = my_sample_w*prefer_ok
        def zero_to_one(t):
            if t == 0:
                return 1
            else:
                return t
        vfunc = np.vectorize(zero_to_one)
        my_sample_w = vfunc(my_sample_w)
        clf.fit(X_train, y_train, sample_weight=my_sample_w)

        print("Training finished. T: %-3.2f" % (time.time() - t))

        if not final:
            if mlflow_avail:
                mlflow.log_param("features used", features)
                # mlflow.log_metric("all features: ", train.columns.values.flatten().tolist())
                mlflow.log_metric("train num rows", train.shape[0])
                mlflow.log_metric("test num rows", test.shape[0])

            if options.final_is_forest:
                best_features = helper.print_feature_ranking(
                    clf, X_train,
                    top_num_features=options.top_num_features,
                    features=features,
                    plot=options.show)
            else:
                best_features = None

            #mlflow.log_artifact("best_features", best_features)
        else:
            if options.dot is not None:
                if not options.final_is_tree:
                    print("ERROR: You cannot use the DOT function on non-trees")
                    exit(-1)

                helper.output_to_classical_dot(
                    clf, features,
                    fname=options.dot + "-" + self.func_name)

            if options.basedir and options.final_is_xgboost:
                booster = clf.get_booster()
                fname = options.basedir + "/predictor_{longsh}.boost".format(
                    longsh=self.longsh)
                booster.save_model(fname)
            else:
                print("NOT writing code")

        print("--------------------------")
        print("-       test data        -")
        print("--------------------------")
        for dump_no in [1, 2, 3, 10, 20, 40, None]:
            prec, recall, acc, roc_auc = self.filtered_conf_matrixes(
                dump_no, test, features, to_predict, clf, "test data", highlight=True)

        print("--------------------------------")
        print("--      train+test data        -")
        print("--------------------------------")
        for dump_no in [1, None]:
            self.filtered_conf_matrixes(
                dump_no, self.df, features, to_predict, clf, "test and train data")

        print("--------------------------")
        print("-       train data       -")
        print("--------------------------")
        self.filtered_conf_matrixes(
            dump_no, train, features, to_predict, clf, "train data")

        # Calculate predicted value for the original dataframe
        y_pred = clf.predict(self.df[features])

        # TODO do principal component analysis
        if final:
            return roc_auc, y_pred
        else:
            return best_features, y_pred

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
        features = list(self.df)
        features = self.rem_features(
            features, ["x.a_num_used", "x.class", "x.a_lifetime", "fname", "sum_cl_use"])
        if options.raw_data_plots:
            pd.options.display.mpl_style = "default"
            self.df.hist()
            self.df.boxplot()

        if not options.only_final:
            # calculate best features
            top_n_feats, y_pred = self.one_classifier(
                features, "x.class", final=False)

            if options.get_best_topn_feats is not None:
                if not options.final_is_forest:
                    print("ERROR: we can only calculate best features for Forest!")
                    exit(-1)

                greedy_features = helper.calc_greedy_best_features(
                    top_n_feats, options.get_best_topn_feats,
                    self)

                # dump to file
                with open(options.best_features_fname, "w") as f:
                    for feat in greedy_features:
                        f.write("%s\n" % feat)
                print("Dumped final best selection to file '%s'" %
                      options.best_features_fname)

        else:
            best_features = helper.get_features(options.best_features_fname)
            roc_auc, y_pred = self.one_classifier(
                best_features, "x.class",
                final=True)

            if options.show:
                plt.show()

        return y_pred


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.pandas"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='PANDASFILE')
    parser.add_argument("--seed", default=None, type=int,
                        dest="seed", help="Seed of PRNG")
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        dest="verbose", help="Print more output")
    parser.add_argument("--printfeat", action="store_true", default=False,
                        dest="print_features", help="Print features")
    parser.add_argument("--nocomputed", default=False, action="store_true",
                        dest="no_computed", help="Don't add computed features")
    parser.add_argument("--bestfeatfile", default="../../scripts/crystal/best_features.txt", type=str,
                        dest="best_features_fname", help="Name and position of best features file that lists the best features in order")

    # tree/forest options
    parser.add_argument("--depth", default=None, type=int,
                        dest="tree_depth", help="Depth of the tree to create")
    parser.add_argument("--split", default=0.01, type=float, metavar="RATIO",
                        dest="min_samples_split", help="Split in tree if this many samples or above. Used as a percentage of datapoints")
    parser.add_argument("--numtrees", default=100, type=int,
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
    parser.add_argument("--only", default=1.00, type=float,
                        dest="only_perc", help="Only use this percentage of data")

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
    parser.add_argument("--logreg", default=False, action="store_true",
                        dest="final_is_logreg", help="Final classifier should be a logistic regression")
    parser.add_argument("--forest", default=False, action="store_true",
                        dest="final_is_forest", help="Final classifier should be a forest")
    parser.add_argument("--xgboost", default=False, action="store_true",
                        dest="final_is_xgboost", help="Final classifier should be a XGBoost")
    parser.add_argument("--voting", default=False, action="store_true",
                        dest="final_is_voting", help="Final classifier should be a voting of all of: forest, svm, logreg")

    # classifier options
    parser.add_argument("--prefok", default=2.0, type=float,
                        dest="prefer_ok", help="Prefer OK if >1.0, equal weight if = 1.0, prefer BAD if < 1.0")

    # which one to generate
    parser.add_argument("--name", default=None, type=str,
                        dest="longsh", help="Raw C-like code will be written to this function and file name")

    options = parser.parse_args()
    prng = np.random.RandomState(options.seed)

    if options.fname is None:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    if options.get_best_topn_feats and options.only_final:
        print("Can't do both --greedy best and --final")
        exit(-1)

    if options.top_num_features is None and not options.only_final:
        print("You must have either --top features OR --final")
        exit(-1)

    assert options.min_samples_split <= 1.0, "You must give min_samples_split that's smaller than 1.0"
    if not os.path.isfile(options.fname):
        print("ERROR: '%s' is not a file" % options.fname)
        exit(-1)

    # ------------
    #  Log all parameters
    # ------------
    if mlflow_avail:
        mlflow.log_param("final", options.only_final)
        if options.only_final:
            mlflow.log_param("tree", options.final_is_tree)
            mlflow.log_param("svm", options.final_is_svm)
            mlflow.log_param("logreg", options.final_is_logreg)
            mlflow.log_param("forest", options.final_is_forest)
            mlflow.log_param("voting", options.final_is_voting)
            mlflow.log_param("basedir", options.basedir)
        else:
            mlflow.log_param("top_num_features", options.top_num_features)

        mlflow.log_param("conf_num", options.conf_num)
        mlflow.log_param("prefer_ok", options.prefer_ok)
        mlflow.log_param("only_percentage", options.only_perc)
        mlflow.log_param("min_samples_split", options.min_samples_split)
        mlflow.log_param("tree_depth", options.tree_depth)
        mlflow.log_param("num_trees", options.num_trees)
        mlflow.log_param("no_computed", options.no_computed)
        mlflow.log_artifact(options.fname)

    # Read in Pandas Dataframe
    df = pd.read_pickle(options.fname)
    df_orig = df.copy()
    if options.print_features:
        for f in sorted(list(df)):
            print(f)

    # feature manipulation
    if not options.no_computed:
        helper.cldata_add_computed_features(df, options.verbose)
    helper.clear_data_from_str_na(df)

    # generation
    learner = Learner(df, longsh=options.longsh)

    y_pred = learner.learn()


    # Dump the new prediction into the dataframe
    name = None
    if options.final_is_logreg:
        name = "logreg"
    elif options.final_is_tree:
        name = "tree"
    elif options.final_is_forest:
        name = "forest"
    else:
        exit(0)

    cleanname = re.sub(r'\.dat$', '', options.fname)
    fname = cleanname + "-" + name + ".dat"
    df_orig[name] = y_pred
    df_orig[name] = df_orig[name].astype("category").cat.codes
    with open(fname, "wb") as f:
        pickle.dump(df_orig, f)
    print("Enriched data dumped to file %s" % fname)

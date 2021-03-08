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
import ast
import math
import functools
import crystalcodegen as ccg
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

MISSING=float("-1.0")

def check_long_short():
    if options.tier is None:
        print("ERROR: You must give option '--tier' as short/long/forever")
        assert False
        exit(-1)


class Learner:
    def __init__(self, df, tier):
        self.df = df
        self.tier = tier

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

        if False:
            return helper.conf_matrixes(data2, features, to_predict, clf, toprint,
                                    highlight=highlight)
        else:
            return helper.calc_regression_error(data2, features, to_predict, clf, toprint,
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
        elif x == "act_ranking_rel":
            pass
        elif x == "time_inside_solver":
            pass
        elif x == "size":
            x = "cl->" + x + "()"
        else:
            x = "cl->stats." + x

        return x

    def importance_XGB(self, clf, features):
        impdf = []
        #print("clf:", clf)
        #print("clf-booster:", clf.feature_importances_)

        for i in range(len(clf.feature_importances_)):
            score = clf.feature_importances_[i]
            ft = features[i]
            impdf.append({'feature': ft, 'importance': score})
        impdf = pd.DataFrame(impdf)
        impdf = impdf.sort_values(by='importance', ascending=False).reset_index(drop=True)
        impdf['importance'] /= impdf['importance'].sum()
        pd.set_option('display.max_rows', None)
        pd.set_option('display.max_columns', None)
        pd.set_option('display.width', None)
        pd.set_option('display.max_colwidth', None)
        print("impdf:", impdf)

        return impdf

    def dump_ml_test_data(self, test, fname, to_predict) :
        f = open(fname, "w")
        test.reset_index(inplace=True)
        for i in range(test.shape[0]):
            towrite = ""
            towrite += "%s " % test["rdb0.props_made"].iloc[i]
            towrite += "%s " % test["cl.orig_glue"].iloc[i]
            towrite += "%s " % test["rdb0.glue"].iloc[i]
            towrite += "%s " % test["cl.glue_before_minim"].iloc[i]
            #towrite += "%s " % test["rdb0.sum_uip1_used"].iloc[i]
            towrite += "%s " % test["cl.num_antecedents"].iloc[i]
            towrite += "%s " % test["cl.num_total_lits_antecedents"].iloc[i]
            towrite += "%s " % test["rdb0.uip1_used"].iloc[i]
            towrite += "%s " % test["cl.numResolutionsHistLT_avg"].iloc[i]
            towrite += "%s " % test["cl.glueHist_longterm_avg"].iloc[i]
            towrite += "%s " % test["cl.conflSizeHistlt_avg"].iloc[i]
            towrite += "%s " % test["cl.branchDepthHistQueue_avg"].iloc[i]
            towrite += "%s " % test["rdb0.act_ranking_rel"].iloc[i]
            towrite += "%s " % test["rdb0.size"].iloc[i]
            towrite += "%s " % test["cl.time_inside_solver"].iloc[i]
            towrite += "%s " % test[to_predict].iloc[i]
            towrite += "\n"
            f.write(towrite)

    def get_sample_weights(self, X_train, y_train):
        if options.sample_weight_sqrt == 0:
            print("SQRT weight is set to NONE")
            return None
        weights = np.array(y_train.values)
        #print("init weights:", weights)
        def myfunc(x):
            if x == 0:
                return 1.0
            else:
                return math.pow(x, 1.0/options.sample_weight_sqrt)
        myfunc_v = np.vectorize(myfunc)
        weights = myfunc_v(weights)
        #print("final weights:", weights)
        print("SQRT weight is set to ", options.sample_weight_sqrt)
        return weights

    def one_classifier(self, features, to_predict, final):
        print("-> Number of features  :", len(features))
        print("-> Number of datapoints:", self.df.shape)
        print("-> Predicting          :", to_predict)
        df = self.df.copy()

        if options.check_row_data:
            helper.check_too_large_or_nan_values(df, features)

        # count bad and ok
        #bad,ok = self.count_bad_ok(df)
        #print("Number of BAD  elements        : %-6d" % bad)
        #print("Number of OK   elements        : %-6d" % ok)
        # balance it out
        #prefer_ok = float(bad)/float(ok)
        #print("Balanced OK preference would be: %-6.3f" % prefer_ok)
        # apply inbalance from option given
        #prefer_ok *= options.prefer_ok
        #print("Option to prefer OK is set to  : %-6.3f" % options.prefer_ok)
        #print("Final OK preference is         : %-6.3f" % prefer_ok)

        print("Value distribution of 'dump_no':\n%s" % df["rdb0.dump_no"].value_counts())
        print("Value distribution of to_predict:\n%s" % df[to_predict].value_counts())
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
            clf_forest = sklearn.ensemble.RandomForestRegressor(
                n_estimators=options.num_trees,
                max_features="sqrt",
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
                # , ntree_limit=3
                print("Using xgboost no. estimators:", options.n_estimators_xgboost)
                clf = xgb.XGBRegressor(
                    objective='reg:squarederror',
                    missing=MISSING,
                    min_child_weight=options.min_child_weight_xgboost,
                    max_depth=options.xboost_max_depth,
                    n_estimators=options.n_estimators_xgboost)
            elif options.final_is_voting:
                mylist = [["forest", clf_forest], [
                    "svm", clf_svm], ["logreg", clf_logreg]]
                clf = sklearn.ensemble.VotingClassifier(mylist, random_state=prng)
            else:
                print(
                    "ERROR: You MUST give one of: tree/forest/svm/logreg/voting classifier")
                exit(-1)
        else:
            assert options.final_is_forest or options.final_is_xgboost, "For TOP calculation, we must have --forest or --xgboost"
            if options.final_is_forest:
                clf = sklearn.ensemble.RandomForestRegressor(
                    n_estimators=options.num_trees*5,
                    max_features="sqrt",
                    random_state=prng)
            elif options.final_is_xgboost:
                clf = xgb.XGBRegressor(
                    objective='reg:squarederror',
                    min_child_weight=options.min_child_weight_xgboost,
                    max_depth=options.xboost_max_depth,
                    missing=MISSING)
            else:
                assert False

        del df
        sample_weights = self.get_sample_weights(X_train, y_train)
        if sample_weights is None:
            clf.fit(X_train, y_train)
        else:
            clf.fit(X_train, y_train, sample_weight=sample_weights)
        del sample_weights

        print("Training finished. T: %-3.2f" % (time.time() - t))

        if not final:
            if mlflow_avail:
                mlflow.log_param("features used", features)
                # mlflow.log_metric("all features: ", train.columns.values.flatten().tolist())
                mlflow.log_metric("train num rows", train.shape[0])
                mlflow.log_metric("test num rows", test.shape[0])

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
                fname = options.basedir + "/predictor_{name}.json".format(
                    name=options.name)
                booster.save_model(fname)
                print("==> Saved model to: ", fname)
            else:
                print("WARNING: NOT writing code -- you must use xgboost and give dir for that")


        # print feature rankings
        if options.final_is_forest:
            helper.print_feature_ranking(
                clf, X_train,
                top_num_features=200,
                features=features,
                plot=options.show)
        elif options.final_is_xgboost:
            self.importance_XGB(clf, features=features)

        # print distribution of error
        print("--------------------------")
        print("-       test data        -")
        print("--------------------------")
        for dump_no in [1, 2, 3, 10, 20, 40, None]:
            self.filtered_conf_matrixes(
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

        #print(test[features+to_predict])
        #print(test[to_predict])
        if not options.no_computed and options.dump_example_data:
            with open("example-data.dat", "wb") as f:
                pickle.dump(test[features+[to_predict]], f)
            self.dump_ml_test_data(test, "../ml_perf_test.txt-%s" % options.tier, to_predict)
            print("Example data dumped")

        # Calculate predicted value for the original dataframe
        y_pred = clf.predict(self.df[features])
        return y_pred

    def rem_features(self, feat, to_remove):
        print("To remove: " , to_remove)
        feat_less = list(feat)
        for rem in to_remove:
            for feat in list(feat_less):
                if rem in feat:
                    if options.verbose:
                        print("Removing due to ", rem, " feature from feat_less:", feat)
                    feat_less.remove(feat)

        print("Done.")
        return feat_less

    def learn(self):
        if options.raw_data_plots:
            pd.options.display.mpl_style = "default"
            self.df.hist()
            self.df.boxplot()

        # get to_predict
        if options.topperc:
            to_predict = "x.used_later_{name}_topperc".format(name=options.tier)
        else:
            to_predict = "x.used_later_{name}".format(name=options.tier)

        if not options.only_final:
            features = list(self.df)
            features = self.rem_features(
            features, ["x.class",
                       "x.a_lifetime",
                       "fname",
                       "sum_cl_use.num_used",
                       "x.sum_cl_use",
                       "x.used_later_short",
                       "x.used_later_long",
                       "x.used_later_forever"])
        else:
            features = helper.get_features(options.best_features_fname)

        self.one_classifier(features, to_predict, final=options.only_final)


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
    parser.add_argument("--allcomputed", default=False, action="store_true",
                        dest="all_computed", help="Add ALL computed features")
    parser.add_argument("--bestfeatfile", type=str,
                        dest="best_features_fname", help="Name and position of best features file that lists the best features in order")
    parser.add_argument("--csv", type=str, default=None,
                        dest="csv", help="Output CSV of dataframe here")
    parser.add_argument("--dumpexample", default=False, action="store_true",
                        dest="dump_example_data", help="Dump example data")

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
    parser.add_argument("--checkverbose", action="store_true", default=False,
                        dest="check_row_data_verbose", help="Check row data for NaN or float overflow in a verbose way, printing them all")
    parser.add_argument("--rawplots", action="store_true", default=False,
                        dest="raw_data_plots", help="Display raw data plots")
    parser.add_argument("--basedir", type=str,
                        dest="basedir", help="The base directory of where the CryptoMiniSat source code is")

    # data filtering
    parser.add_argument("--only", default=1.00, type=float,
                        dest="only_perc", help="Only use this percentage of data")

    # final generator top/final
    parser.add_argument("--final", default=False, action="store_true",
                        dest="only_final", help="Only generate final predictor")
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
    parser.add_argument("--xgboostestimators", default=20, type=int,
                        dest="n_estimators_xgboost", help="Number of estimators for xgboost")
    parser.add_argument("--xgboostminchild", default=1, type=int,
                        dest="min_child_weight_xgboost", help="Number of elements in the leaf to split it in xgboost")
    parser.add_argument("--xboostmaxdepth", default=6, type=int,
                        dest="xboost_max_depth", help="Max depth of xboost trees")
    parser.add_argument("--weight", default=0.0, type=float,
                        dest="sample_weight_sqrt", help="The SQRT factor for weights. 0 = disable sample weights. Larger number (e.g. 100) will be basically like 0, while e.g. 1 will skew things a _alot_ towards higher values having more weights")

    # which one to generate
    parser.add_argument("--tier", default=None, type=str,
                        dest="tier", help="what to predict")
    parser.add_argument("--name", default=None, type=str,
                        dest="name", help="what file to generate")
    parser.add_argument("--topperc", default=False, action="store_true",
                        dest="topperc", help="Predict toppercent instead of use value")

    options = parser.parse_args()
    prng = np.random.RandomState(options.seed)

    if options.fname is None:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    if options.top_num_features and options.only_final:
        print("Can't do both --top and --final")
        exit(-1)

    if options.top_num_features is None and not options.only_final:
        print("You must have either --top features OR --final")
        exit(-1)

    assert options.min_samples_split <= 1.0, "You must give min_samples_split that's smaller than 1.0"
    if not os.path.isfile(options.fname):
        print("ERROR: '%s' is not a file" % options.fname)
        exit(-1)

    if options.name is None:
        print("Name was not set with --name, setting it to --tier, i.e. ", options.tier)
        options.name = options.tier

    if options.best_features_fname is None and not options.top_num_features:
        print("You must give best features filename")
        exit(-1)

    # ------------
    #  Log all parameters
    # ------------
    if mlflow_avail:
        mlflow.log_param("final", options.only_final)
        if options.only_final:
            mlflow.log_param("tier", options.tier)
            mlflow.log_param("name", options.name)
            mlflow.log_param("tree", options.final_is_tree)
            mlflow.log_param("svm", options.final_is_svm)
            mlflow.log_param("logreg", options.final_is_logreg)
            mlflow.log_param("forest", options.final_is_forest)
            mlflow.log_param("voting", options.final_is_voting)
            mlflow.log_param("basedir", options.basedir)
        else:
            mlflow.log_param("top_num_features", options.top_num_features)

        mlflow.log_param("only_percentage", options.only_perc)
        mlflow.log_param("min_samples_split", options.min_samples_split)
        mlflow.log_param("tree_depth", options.tree_depth)
        mlflow.log_param("num_trees", options.num_trees)
        mlflow.log_param("no_computed", options.no_computed)
        mlflow.log_artifact(options.fname)

    # Read in Pandas Dataframe
    print("Reading dataframe...")
    if False:
        df_nofloat = pd.read_pickle(options.fname)
        df = df_nofloat.convert_dtypes()
        del df_nofloat
    else:
        df = pd.read_pickle(options.fname)
        print("Making None into NaN...")
        def make_none_into_nan(x):
            if x is None:
                return np.nan
            else:
                return x

        for col in list(df):
            if type(None) in df[col].apply(type).unique():
                df[col] = df[col].apply(make_none_into_nan)
        print("Done filtering")


    df_orig = df.copy()
    if options.print_features:
        for f in sorted(list(df)):
            print(f)

    # get smaller part to work on
    # also, copy it so we don't get warning about setting a slice of a DF
    print("Applying only...")
    df_tmp = df.sample(frac=options.only_perc, random_state=prng)
    df = pd.DataFrame(df_tmp)
    del df_tmp
    print("-> Number of datapoints after applying '--only':", df.shape)

    # feature manipulation
    if not options.no_computed:
        if options.all_computed:
            short = options.tier == "short"
            helper.cldata_add_computed_features(df, options.verbose, short=short)
        else:
            print("Adding features...")
            helper.cldata_add_minimum_computed_features(df, options.verbose)
            divide = functools.partial(helper.helper_divide, df=df, features=list(df), verb=options.verbose)
            best_features = helper.get_features(options.best_features_fname)
            for feat in best_features:
                toeval = ccg.to_source(ast.parse(feat))
                print("Adding feature %s as eval %s" % (feat, toeval))
                df[feat] = eval(toeval)
    else:
        helper.cldata_add_minimum_computed_features(df, options.verbose)
        helper.delete_none_features(df)

    for name, mytype in df.dtypes.items():
        #print(mytype)
        #print(type(mytype))
        if str(mytype) == str("Int64"):
            df[name] = df[name].astype(float)

    if options.csv is not None:
        cols = list(df)
        if not options.no_computed and not options.all_computed:
            cols = best_features
            for feat in list(df):
                if "x." in feat:
                    cols.append(feat)
        df.to_csv(options.csv, index=False, columns=sorted(cols))
        print("Dumped DF to file: ", options.csv)

    print("Filling NA with MISSING..")
    df.replace(np.NINF, MISSING, inplace=True)
    df.replace(np.NaN, MISSING, inplace=True)
    df.replace(np.Infinity, MISSING, inplace=True)
    df.replace(np.inf, MISSING, inplace=True)
    df.fillna(MISSING, inplace=True)

    if options.check_row_data_verbose:
        helper.check_too_large_or_nan_values(df, list(df))

    # do the heavy lifting
    learner = Learner(df, tier=options.tier)
    learner.learn()

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
import sklearn.preprocessing
import numpy as np
import sklearn.metrics
import sklearn.impute
import matplotlib.pyplot as plt
import sklearn.ensemble
import sklearn.linear_model
import helper
import xgboost as xgb
import lightgbm as lgbm
import ast
import math
import functools
try:
    import mlflow
except ImportError:
    mlflow_avail = False
else:
    mlflow_avail = True

ver = sklearn.__version__.split(".")
print("version: ", ver)
if int(ver[0]) == 0 and int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split

MISSING=np.NaN

class MyEnsemble:
    def __init__(self, models):
        self.models = models

    def fit(self, X_train, y_train, weights=None):
        assert weights is None
        for model in self.models:
            model.fit(X_train, y_train)

    def predict(self, X_data):
        #df = pd.DataFrame(index=range(numRows),columns=range(numCols))
        vals = np.ndarray(shape=(len(self.models), len(X_data)), dtype=float)
        for i in range(len(self.models)):
            pred = self.models[i].predict(X_data)
            vals[i] = pred

        ret = np.median(vals, axis=0)
        return ret



class Learner:
    def __init__(self, df):
        self.df = df

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

        return helper.calc_regression_error(
            data2, features, to_predict, clf, toprint,
            highlight=highlight)

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
            towrite += "%s " % test[TODO_go_through_all_features_here].iloc[i]
            assert False, "not implemented, must put all features here in a loop"
            towrite += "%s " % test[to_predict].iloc[i]
            towrite += "\n"
            f.write(towrite)

    def scale_and_impute(self, train, test, features, extra_feats):
        trans_train = train[features].values
        trans_test = test[features].values

        # Scale data if linear
        my_scaler = sklearn.preprocessing.Normalizer(
            norm='l1',
            copy=False)
        my_scaler.fit_transform(trans_train)
        my_scaler.transform(trans_test)

        # Impute data
        imp_mean = sklearn.impute.SimpleImputer(
            #missing_values=MISSING,
            copy=False,
            strategy='mean')
        imp_mean.fit_transform(trans_train)
        imp_mean.transform(trans_test)

        # recreate dataframe
        trans_train = np.append(trans_train, train[extra_feats].values, axis=1)
        df_trans_train = pd.DataFrame(trans_train, columns=features+extra_feats)
        train = df_trans_train

        trans_test = np.append(trans_test, test[extra_feats].values, axis=1)
        df_trans_test = pd.DataFrame(trans_test, columns=features+extra_feats)
        test = df_trans_test

        print("Value distribution of 'rdb0.glue' in test:\n%s" % test["rdb0.glue"].value_counts())
        print("Value distribution of 'rdb0.glue' in train:\n%s" % train["rdb0.glue"].value_counts())

        return train, test

    def one_regressor(self, features, to_predict):
        print("-> Number of features  :", len(features))
        print("-> Number of datapoints:", self.df.shape)
        print("-> Predicting          :", to_predict)

        # these are needed for prediction/later checks, so let's add them in
        #       if they are not already in the features
        extra_feats = [to_predict]
        for missing_needed in ["rdb0.glue", "rdb0.dump_no"]:
            if missing_needed not in features:
                extra_feats.append(missing_needed)
        df = self.df[features+extra_feats].copy()
        if options.verbose:
            pd.set_option('display.max_rows', len(df.dtypes))
            print(df.dtypes)
            pd.reset_option('display.max_rows')


        if options.check_row_data:
            helper.check_too_large_or_nan_values(df, features)

        print("Value distribution of 'rdb0.dump_no':\n%s" % df["rdb0.dump_no"].value_counts())
        print("Value distribution of 'rdb0.glue':\n%s" % df["rdb0.glue"].value_counts())
        print("Value distribution of to_predict:\n%s" % df[to_predict].value_counts())

        train, test = train_test_split(df, test_size=0.33, random_state=prng)
        if options.regressor in ["linear", "tree"]:
            train, test = self.scale_and_impute(train, test, features, extra_feats)


        if options.poly_features:
            transformed = df[features].values

            # poly transform
            poly = sklearn.preprocessing.PolynomialFeatures(2)
            transformed = poly.fit_transform(transformed)
            features = poly.get_feature_names(input_features=features)

            # recreate dataframe
            transformed = np.append(transformed, df[extra_feats].values, axis=1)
            df_trans = pd.DataFrame(transformed, columns=features+extra_feats)
            df = df_trans

        X_train = train[features]
        y_train = train[to_predict]

        t = time.time()
        clf = None

        split_point = helper.calc_min_split_point(
            df, options.min_samples_split)

        clf_tree = sklearn.tree.DecisionTreeRegressor(
            max_depth=options.xboost_max_depth,
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
        clf_linear = sklearn.linear_model.LinearRegression()
        clf_forest = sklearn.ensemble.RandomForestRegressor(
            n_estimators=options.num_trees,
            max_depth=options.xboost_max_depth,
            max_features="sqrt",
            #min_samples_leaf=split_point,
            random_state=prng)
        clf_ridge = sklearn.linear_model.Ridge(alpha=.5)
        clf_lasso = sklearn.linear_model.Lasso()
        clf_elasticnet = sklearn.linear_model.ElasticNet()
        clf_xgboost = xgb.XGBRegressor(
                objective='reg:squarederror',
                #missing=MISSING,
                min_child_weight=options.min_child_weight_xgboost, # from doc: "In linear regression task, this simply corresponds to minimum number of instances needed to be in each node."
                max_depth=options.xboost_max_depth,
                subsample=options.xgboost_subsample,
                n_estimators=options.n_estimators_xgboost)
        clf_lgbm = model = lgbm.LGBMRegressor(
            subsample=options.xgboost_subsample,
            min_child_samples=options.min_child_weight_xgboost, # from doc: "Minimum number of data needed in a child"
            max_depth=options.xboost_max_depth,
            n_estimators=options.n_estimators_xgboost)

        if options.regressor == "tree":
            clf = clf_tree
        elif options.regressor == "svm":
            clf = clf_svm
        elif options.regressor == "linear":
            clf = clf_linear
        elif options.regressor == "ridge":
            clf = clf_ridge
        elif options.regressor == "lasso":
            clf = clf_lasso
        elif options.regressor == "elasticnet":
            clf = clf_elasticnet
        elif options.regressor == "forest":
            clf = clf_forest
        elif options.regressor == "lgbm":
            clf = clf_lgbm
        elif options.regressor == "xgb":
            print("Using xgboost no. estimators:", options.n_estimators_xgboost)
            clf = clf_xgboost
        elif options.regressor == "median":
            mylist = [("xgb", clf_xgboost), ("linear", clf_linear),
                      ("elasticnet", clf_elasticnet), ("lasso", clf_lasso)]
            clf = sklearn.ensemble.VotingRegressor(estimators=mylist, weights=[1.0, 0.5, 0.5, 0.5])
            #clf = MyEnsemble(mylist)
        else:
            print(
                "ERROR: You MUST give one of: tree/forest/svm/linear/bagging classifier")
            exit(-1)
        if options.gen_topfeats:
            print("WARNING: Replacing normal forest/xgboost with top feature computation!!!")
            if options.regressor == "forest":
                print("for TOP calculation, we are replacing the forest predictor!!!")
                clf = sklearn.ensemble.RandomForestRegressor(
                    n_estimators=options.num_trees*5,
                    max_features="sqrt",
                    random_state=prng)
            elif options.regressor == "xgb":
                print("for TOP calculation, we are replacing the XGBOOST predictor!!!")
                clf = xgb.XGBRegressor(
                    objective='reg:squarederror',
                    min_child_weight=options.min_child_weight_xgboost,
                    max_depth=options.xboost_max_depth,
                    #missing=MISSING
                    )
            else:
                print("Error: --topfeats only works with xgboost/forest")
                exit(-1)


        clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time() - t))

        if mlflow_avail:
            mlflow.log_param("features used", features)
            # mlflow.log_metric("all features: ", train.columns.values.flatten().tolist())
            mlflow.log_metric("train num rows", train.shape[0])
            mlflow.log_metric("test num rows", test.shape[0])


        if options.dot is not None:
            if options.regressor == "tree":
                helper.output_to_classical_dot(
                    clf, features,
                    fname="{name}-{table}-{tier}.dot".format(
                        name=options.dot, tier=options.tier, table=options.table))

            elif options.regressor == "xgb":
                dot_data = xgb.to_graphviz(booster=clf, num_trees=9)
                with open("x.dot", "w") as f:
                    f.write("%s" % dot_data)

            else:
                print("ERROR: You cannot use the DOT function on non-trees")
                exit(-1)

        if options.basedir:
            fname_pred_out = options.basedir + "/predictor-{table}-{tier}-{regr}.json".format(
                tier=options.tier, table=options.table, regr=options.regressor)
            if options.regressor == "xgb":
                booster = clf.get_booster()
                booster.save_model(fname_pred_out)
                print("==> Saved XGB model to: ", fname_pred_out)
            elif options.basedir and options.regressor == "lgbm":
                clf.booster_.save_model(fname_pred_out)
                print("==> Saved LGBM model to: ", fname_pred_out)
        else:
            print("WARNING: NOT writing code -- you must use xgboost/lgbm and give dir for that")


        # print feature rankings
        if options.regressor == "forest":
            helper.print_feature_ranking(
                clf, X_train,
                top_num_features=200,
                features=features,
                plot=options.show)
        elif options.regressor == "xgb":
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
                dump_no, pd.concat([test, train]), features, to_predict, clf, "test and train data")
        print("--------------------------")
        print("-       train data       -")
        print("--------------------------")
        self.filtered_conf_matrixes(
            dump_no, train, features, to_predict, clf, "train data")

        #print(test[features+to_predict])
        #print(test[to_predict])
        if options.dump_example_data:
            with open("example-data.dat", "wb") as f:
                pickle.dump(test[features+[to_predict]], f)
            self.dump_ml_test_data(test, "../ml_perf_test.txt-{table}-{tier}".format(
                table=options.table, tier=options.tier))
            print("Example data dumped")

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
            df.hist()
            df.boxplot()

        if options.features != "best_only":
            features = list(self.df)

            # remove features that would be "cheating" or useless
            torem = []
            for table in ["used_later", "used_later_anc"]:
                for tier in ["short", "long", "forever"]:
                    torem. append("x.{table}_{tier}".format(tier=tier, table=table))

            torem.extend([
                "x.class",
                "x.a_lifetime",
                "fname",
                "sum_cl_use.num_used",
                "x.sum_cl_use",
                "rdb0.dump_no",
                "fname"])
            features = self.rem_features(features, torem)
        else:
            del df["fname"]
            features = helper.get_features(options.best_features_fname)

        to_predict = "x.{table}_{tier}".format(tier=options.tier, table=options.table)
        self.one_regressor(features, to_predict)


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
    parser.add_argument("--features", default="best_only", type=str, dest="features",
                        help="What features to use: all_computed, best_only, best_also, no_computed ")
    parser.add_argument("--bestfeatfile", type=str, default="../../scripts/crystal/best_features-rdb0-only.txt",
                        dest="best_features_fname", help="Name and position of best features file that lists the best features in order")
    parser.add_argument("--polyfeats", action="store_true", default=False,
                        dest="poly_features", help="Add polynomial features")
    parser.add_argument("--dat", type=str, default=None,
                        dest="dat_file", help="Output Pickle of dataframe here")
    parser.add_argument("--dumpexample", default=False, action="store_true",
                        dest="dump_example_data", help="Dump example data")

    # tree/forest options
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
    parser.add_argument("--topfeats", default=False, action="store_true",
                        dest="gen_topfeats", help="Only generate final predictor")

    # type of regressor
    parser.add_argument("--regressor", type=str, default="xgb",
                        dest="regressor", help="Final classifier should be a: tree, svm, linear, forest, xgboost, bagging")
    parser.add_argument("--xgboostestimators", default=10, type=int,
                        dest="n_estimators_xgboost", help="Number of estimators for xgboost")
    parser.add_argument("--xgboostminchild", default=10, type=int,
                        dest="min_child_weight_xgboost", help="Number of elements in the leaf to split it in xgboost")
    parser.add_argument("--xboostmaxdepth", default=10, type=int,
                        dest="xboost_max_depth", help="Max depth of xboost trees")
    parser.add_argument("--xgboostsubsample", default=1.0, type=float,
                        dest="xgboost_subsample", help="Subsample xgboost on each iteration")

    # which one to generate
    parser.add_argument("--tier", default=None, type=str,
                        dest="tier", help="Tier to do")
    parser.add_argument("--table", default="used_later", type=str,
                        dest="table", help="Table to do")

    options = parser.parse_args()
    prng = np.random.RandomState(options.seed)

    if options.fname is None:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    assert options.min_samples_split <= 1.0, "You must give min_samples_split that's smaller than 1.0"
    if not os.path.isfile(options.fname):
        print("ERROR: '%s' is not a file" % options.fname)
        exit(-1)

    if options.tier is None:
        print("ERROR: you must set --tier, exiting")
        exit(-1)

    if options.table is None:
        print("ERROR: you must set --table, exiting")
        exit(-1)

    if options.best_features_fname is None and "best" in options.features:
        print("You must give best features filename or we cannot add best features")
        exit(-1)

    # ------------
    #  Log all parameters
    # ------------
    if mlflow_avail:
        mlflow.log_param("gen_topfeats", options.gen_topfeats)
        mlflow.log_param("tier", options.tier)
        mlflow.log_param("regressor", options.regressor)
        mlflow.log_param("basedir", options.basedir)
        mlflow.log_param("only_percentage", options.only_perc)
        mlflow.log_param("min_samples_split", options.min_samples_split)
        mlflow.log_param("xboost_max_depth", options.xboost_max_depth)
        mlflow.log_param("num_trees", options.num_trees)
        mlflow.log_param("features", options.features)
        mlflow.log_artifact(options.fname)

    # Read in Pandas Dataframe
    print("Reading dataframe....")
    df = pd.read_pickle(options.fname)

    print("Applying only...")
    df_tmp = df.sample(frac=options.only_perc, random_state=prng)
    df_before_dtype_conv = pd.DataFrame(df_tmp)
    del df_tmp
    del df
    print("-> Number of datapoints after applying '--only':", df_before_dtype_conv.shape)

    # We must convert these or we'll have trouble with inf, -inf, NaN for NULLs
    print("Converting datatypes to those supporting np.NA ...")
    if options.verbose:
        print("Datatypes before:")
        helper.print_datatypes(df_before_dtype_conv)
    df = df_before_dtype_conv.convert_dtypes(
        convert_integer=False, convert_string=False,
        convert_floating=False)
    del df_before_dtype_conv
    if options.verbose:
        print("Datatypes after:")
        helper.print_datatypes(df)

    # Check feature type sanity
    # only "fname" is allowed to be an object (a string)
    for name,ty in zip(list(df), df.dtypes):
        if name == "fname":
            assert ty == object
        else:
            if ty == object:
                print("name: " , name, " is object!")
            assert ty != object

    if options.print_features:
        for f in sorted(list(df)):
            print(f)

    # make missing (None) into NaN
    helper.make_missing_into_nan(df)

    # feature manipulation
    if options.features =="all_computed":
        helper.cldata_add_computed_features(df, options.verbose)
    elif options.features == "best_only" or options.features == "best_also":
        helper.add_features_from_fname(df, options.best_features_fname)
    elif options.features == "no_computed":
        helper.cldata_add_minimum_computed_features(df, options.verbose)
    else:
        print("ERROR: Unrecognized --features option!")
        exit(-1)

    # Check feature type sanity
    for name, mytype in df.dtypes.items():
        if str(mytype) == str("Int64") or str(mytype) == str("Float64"):
            assert False

    print("Filling NA with MISSING..")
    df.replace([np.inf, np.NaN, np.inf, np.NINF, np.Infinity], MISSING, inplace=True)

    if options.dat_file is not None:
        cols = list(df)
        df.to_pickle(options.dat_file, protocol=3)
        print("Dumped DF to pickle: ", options.dat_file)

    if options.check_row_data_verbose:
        helper.check_too_large_or_nan_values(df, list(df))

    # do the heavy lifting
    learner = Learner(df)
    learner.learn()

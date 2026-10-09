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

# Trains one xgboost model on one frame and saves it as
# predictor-<table>-disc-xgb.json; prints the use kept at reduce against
# the normal build and the oracle, and (--topfeats) the feature
# importance ranking that gen_best_feats.sh/pick_features.py use.
#
# usage: cldata_predict.py frame.dat --table used_later --features best_only
#            --bestfeatfile best_features.txt --objective rank
#            --xgboostestimators 40 --xboostmaxdepth 5 --basedir <where the model goes>
# pylint: disable=invalid-name,line-too-long,too-many-locals,consider-using-sys-exit

import time
import argparse
import os
import pandas as pd
import pickle
import sklearn
import sklearn.tree
import sklearn.base
import numpy as np
import sklearn.metrics
import helper
import xgboost as xgb
from sklearn.model_selection import GroupShuffleSplit

MISSING=np.nan

class Learner:
    def __init__(self, df, df_eval=None):
        self.df = df
        self.df_eval = df_eval

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

    def ranking_quality(self, data, features, to_predict, clf, name):
        """The solver's reduce replayed on the fair frame: the share of
        the future use the normal build keeps, and what the model would
        (helper.policy_per_reduce). Reduce ranks, so this says more than
        the squared error"""
        if data is None or data.shape[0] < 10:
            return
        X = data[features].astype(np.float32).replace([np.inf, -np.inf], MISSING)
        res = helper.policy_per_reduce(data, clf.predict(X), to_predict)
        if res is None:
            return
        m = res.drop(columns=["instance", "reduces"]).mean()
        print(res.round(1).to_string(index=False))
        print("use kept at reduce, %s, %d instances: %s" % (
            name, len(res), "  ".join("%s %.1f%%" % (k, v) for k, v in m.items())))

    def one_regressor(self, features, to_predict):
        print("-> Number of features  :", len(features))
        print("-> Number of datapoints:", self.df.shape)
        print("-> Predicting          :", to_predict)

        # these are needed for prediction/later checks, so let's add them in
        #       if they are not already in the features
        # the ranking quality is always on the use counts, whatever is learnt
        count_label = "x.{table}_disc".format(table=options.table)
        extra_feats = [to_predict]
        for missing_needed in ["rdb0.glue", "rdb0.dump_no", "rdb0.size", "rdb0.used", count_label]:
            if missing_needed not in features and missing_needed not in extra_feats:
                extra_feats.append(missing_needed)
        weights = helper.sample_weights(self.df, options.weights)
        df = self.df[features+extra_feats].copy()
        # xgboost and the solver work in float32: ratios beyond its range
        # are inf there, so make them missing here too
        df[features] = df[features].astype(np.float32)
        df[features] = df[features].replace([np.inf, -np.inf], MISSING)
        if options.verbose:
            pd.set_option('display.max_rows', len(df.dtypes))
            print(df.dtypes)
            pd.reset_option('display.max_rows')


        if options.check_row_data:
            helper.check_too_large_or_nan_values(df, features)

        print("Value distribution of 'rdb0.dump_no':\n%s" % df["rdb0.dump_no"].value_counts())
        print("Value distribution of 'rdb0.glue':\n%s" % df["rdb0.glue"].value_counts())
        print("Value distribution of to_predict:\n%s" % df[to_predict].value_counts())

        # split by instance: the test set says how the model does on
        # instances it has not seen. One or two instances: by clause (a
        # clause's rows from different dumps are near-duplicates)
        fnames = self.df["fname"].astype(str)
        by_instance = fnames.nunique() >= 3
        groups = fnames if by_instance else self.df["sum_cl_use.clauseID"]
        splitter = GroupShuffleSplit(n_splits=1, test_size=0.33, random_state=prng)
        train_idx, test_idx = next(splitter.split(df, groups=groups))
        train = df.iloc[train_idx]
        test = df.iloc[test_idx]
        print("Train/test split by %s: %d/%d rows, %d/%d %s" % (
            "instance" if by_instance else "clause",
            train.shape[0], test.shape[0],
            groups.iloc[train_idx].nunique(), groups.iloc[test_idx].nunique(),
            "instances" if by_instance else "clauses"))
        test_fnames = set(fnames.iloc[test_idx]) if by_instance else set(fnames)
        X_train = train[features]
        y_train = train[to_predict]
        y_all = df[to_predict]
        objective = "reg:squarederror"
        if options.objective == "log":
            # reduce only ranks: the few heavily used clauses need not
            # dominate the loss
            y_train = np.log1p(y_train)
            y_all = np.log1p(y_all)
        elif options.objective == "poisson":
            objective = "count:poisson"
        elif options.objective == "rank":
            # the order within a reduce is all the solver takes from the
            # model: learn that, from whether a clause is used at all
            objective = "rank:ndcg"
            y_train = train[count_label]
            y_all = df[count_label]

        t = time.time()
        clf = None

        if options.regressor == "tree":
            # a single tree, only good for looking at with --dot
            clf = sklearn.tree.DecisionTreeRegressor(
                max_depth=options.xboost_max_depth,
                min_samples_split=helper.calc_min_split_point(df, options.min_samples_split),
                random_state=prng)
        elif options.regressor == "xgb":
            print("Using xgboost no. estimators:", options.n_estimators_xgboost)
            n_estimators = options.n_estimators_xgboost
            if options.gen_topfeats:
                # more trees, so the importance ranking is less noisy
                print("--topfeats: 100 estimators, only for the feature ranking")
                n_estimators = 100
            model = xgb.XGBRanker if options.objective == "rank" else xgb.XGBRegressor
            clf = model(
                objective=objective,
                min_child_weight=options.min_child_weight_xgboost, # from doc: "In linear regression task, this simply corresponds to minimum number of instances needed to be in each node."
                max_depth=options.xboost_max_depth,
                subsample=options.xgboost_subsample,
                random_state=options.seed,
                n_estimators=n_estimators)
        else:
            print("ERROR: --regressor must be xgb or tree")
            exit(-1)

        helper.fit(clf, self.df.iloc[train_idx], X_train, y_train, weights.iloc[train_idx])
        print("Training finished. T: %-3.2f" % (time.time() - t))
        # the model that is saved learns from every row
        clf_all = sklearn.base.clone(clf)
        helper.fit(clf_all, self.df, df[features], y_all, weights)

        if options.dot is not None:
            if options.regressor == "tree":
                helper.output_to_classical_dot(
                    clf, features,
                    fname="{name}-{table}-disc.dot".format(
                        name=options.dot, table=options.table))

            elif options.regressor == "xgb":
                dot_data = xgb.to_graphviz(booster=clf, num_trees=9)
                with open("x.dot", "w") as f:
                    f.write("%s" % dot_data)

            else:
                print("ERROR: You cannot use the DOT function on non-trees")
                exit(-1)

        if options.basedir:
            fname_pred_out = options.basedir + "/predictor-{table}-disc-{regr}.json".format(
                table=options.table, regr=options.regressor)
            if options.regressor == "xgb":
                booster = clf_all.get_booster()
                self.set_provenance(booster, df, features)
                booster.save_model(fname_pred_out)
                print("==> Saved XGB model to: ", fname_pred_out)
        else:
            print("WARNING: NOT writing predictor -- you must use xgb and give --basedir for that")


        if options.regressor == "xgb":
            self.importance_XGB(clf, features=features)

        if self.df_eval is not None:
            ev = self.df_eval
            is_test = ev["fname"].astype(str).isin(test_fnames)
            self.ranking_quality(ev[is_test], features, count_label, clf, "test")
            if by_instance:
                self.ranking_quality(ev[~is_test], features, count_label, clf, "train")

        if options.objective == "rank":
            # a ranker's score has no scale: no error to print
            return

        # print distribution of error
        print("--------------------------")
        print("-       test data        -")
        print("--------------------------")
        for dump_no in [0, 1, 2, 5, 10, None]:
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

    # The training range of every feature (1st and 99th percentile) and
    # where the data came from, stored in the model: the solver counts the
    # values it computes outside the range and says so at the end, since
    # a run the model never saw the like of is one it knows nothing about
    def set_provenance(self, booster, df, features):
        lo = []
        hi = []
        for feat in features:
            v = df[feat].astype(float).replace([np.inf, -np.inf], np.nan).dropna()
            if len(v) == 0:
                lo.append("nan")
                hi.append("nan")
            else:
                lo.append(repr(float(np.percentile(v, 1))))
                hi.append(repr(float(np.percentile(v, 99))))
        booster.set_attr(feature_lo=" ".join(lo), feature_hi=" ".join(hi),
                         train_rows=str(len(df)),
                         train_frame=os.path.basename(options.fname),
                         train_date=time.strftime("%Y-%m-%d"),
                         gathered_by=options.gathered_by)
        print("Stored the training ranges of %d features over %d rows in the model" % (
            len(features), len(df)))

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
        if options.features != "best_only":
            features = list(self.df)

            # remove features that would be "cheating" or useless
            torem = []
            for table in ["used_later", "used_later_anc"]:
                torem. append("x.{table}_disc".format(table=table))

            torem.extend([
                "x.class",
                "x.weight",
                "x.a_lifetime",
                "fname",
                "sum_cl_use.",  # the future
                "x.sum_cl_use",
                "rdb0.dump_no",
                "rdb0.gone",  # not known to the solver
                # IDs and absolute times only tell instances apart
                "clauseID",
                "restartID",
                "rdb0_common.conflicts",
                "cl.conflicts",
                "rdb0.introduced_at_conflict",
                "fname"])
            features = self.rem_features(features, torem)
        else:
            features = helper.get_features(options.best_features_fname)

        to_predict = "x.{table}_disc".format(table=options.table)
        if options.target == "rel":
            to_predict += "_rel"
        self.one_regressor(features, to_predict)


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.pandas"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fname", type=str, metavar='PANDASFILE')
    parser.add_argument("--seed", default=0, type=int,
                        dest="seed", help="Seed of the train/test split. Default: %(default)s")
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        dest="verbose", help="Print more output")
    parser.add_argument("--printfeat", action="store_true", default=False,
                        dest="print_features", help="Print features")
    parser.add_argument("--features", default="best_only", type=str, dest="features",
                        help="What features to use: all_computed (every pair), ratio_computed (the clause against its reduce), best_only, best_also, no_computed")
    parser.add_argument("--bestfeatfile", type=str, default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "best_features.txt"),
                        dest="best_features_fname", help="Name and position of best features file that lists the best features in order")
    parser.add_argument("--dat", type=str, default=None,
                        dest="dat_file", help="Output Pickle of dataframe here")

    # tree/forest options
    parser.add_argument("--split", default=0.01, type=float, metavar="RATIO",
                        dest="min_samples_split", help="Split in tree if this many samples or above. Used as a percentage of datapoints")

    # generation of predictor
    parser.add_argument("--dot", type=str, default=None,
                        dest="dot", help="Create DOT file")
    parser.add_argument("--check", action="store_true", default=False,
                        dest="check_row_data", help="Check row data for NaN or float overflow")
    parser.add_argument("--checkverbose", action="store_true", default=False,
                        dest="check_row_data_verbose", help="Check row data for NaN or float overflow in a verbose way, printing them all")
    parser.add_argument("--basedir", type=str,
                        dest="basedir", help="The base directory of where the CryptoMiniSat source code is")

    # data filtering
    parser.add_argument("--only", default=1.00, type=float,
                        dest="only_perc", help="Only use this percentage of data")

    # final generator top/final
    parser.add_argument("--topfeats", default=False, action="store_true",
                        dest="gen_topfeats", help="Train with 100 estimators for a feature importance ranking")

    # type of regressor
    parser.add_argument("--target", type=str, default="count",
                        help="count: the discounted use itself, rel: the share of the clauses at the same reduce used less (0..1)")
    parser.add_argument("--objective", type=str, default="rank",
                        help="rank (the order of the clauses of a reduce; --target and --weights do not matter), squarederror, log (squared error of log(1+use)) or poisson")
    parser.add_argument("--regressor", type=str, default="xgb",
                        dest="regressor", help="xgb (default) or tree (a single tree, for --dot)")
    parser.add_argument("--xgboostestimators", default=10, type=int,
                        dest="n_estimators_xgboost", help="Number of estimators for xgboost")
    parser.add_argument("--xgboostminchild", default=10, type=int,
                        dest="min_child_weight_xgboost", help="Number of elements in the leaf to split it in xgboost")
    parser.add_argument("--xboostmaxdepth", default=10, type=int,
                        dest="xboost_max_depth", help="Max depth of xboost trees")
    parser.add_argument("--xgboostsubsample", default=1.0, type=float,
                        dest="xgboost_subsample", help="Subsample xgboost on each iteration")

    # which one to generate
    parser.add_argument("--table", default="used_later", type=str,
                        dest="table", help="Table to do")
    parser.add_argument("--weights", type=str, default="family", choices=["none", "strata", "instance", "family"],
                        help="row weights. strata: undo the strata sampling; instance: also every instance counts the same; family: also every family")
    parser.add_argument("--evalframe", type=str, default=None,
                        help="the fair frame (evaldata) the ranking quality is measured on")
    parser.add_argument("--gatheredby", default="unknown", type=str,
                        dest="gathered_by", help="solver commit(s) that gathered the data, stored in the model")

    options = parser.parse_args()
    prng = np.random.RandomState(options.seed)

    if options.fname is None:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    assert options.min_samples_split <= 1.0, "You must give min_samples_split that's smaller than 1.0"
    if not os.path.isfile(options.fname):
        print("ERROR: '%s' is not a file" % options.fname)
        exit(-1)

    if options.table is None:
        print("ERROR: you must set --table, exiting")
        exit(-1)

    if options.best_features_fname is None and "best" in options.features:
        print("You must give best features filename or we cannot add best features")
        exit(-1)

    def load_frame(fname, only):
        print("Reading dataframe %s ..." % fname)
        df = pd.read_pickle(fname)
        df = pd.DataFrame(df.sample(frac=only, random_state=prng))
        print("-> Number of datapoints after applying '--only':", df.shape)

        # We must convert these or we'll have trouble with inf, -inf, NaN for NULLs
        df = df.convert_dtypes(
            convert_integer=False, convert_string=False,
            convert_floating=False)
        if options.verbose:
            helper.print_datatypes(df)

        # only "fname" is allowed to be an object (a string)
        for name,ty in zip(list(df), df.dtypes):
            if name == "fname":
                assert ty == object or pd.api.types.is_string_dtype(ty)
            else:
                if ty == object:
                    print("name: " , name, " is object!")
                assert ty != object

        # make missing (None) into NaN
        helper.make_missing_into_nan(df)

        # feature manipulation
        if options.features =="all_computed":
            df = helper.cldata_add_computed_features(df, options.verbose)
        elif options.features == "ratio_computed":
            df = helper.cldata_add_ratio_features(df, options.verbose)
        elif options.features == "best_only" or options.features == "best_also":
            helper.add_features_from_fname(df, options.best_features_fname)
        elif options.features == "no_computed":
            helper.cldata_add_minimum_computed_features(df, options.verbose)
        else:
            print("ERROR: Unrecognized --features option!")
            exit(-1)

        for name, mytype in df.dtypes.items():
            if str(mytype) == str("Int64") or str(mytype) == str("Float64"):
                assert False

        num = df.columns.drop("fname")
        df[num] = df[num].replace([np.inf, -np.inf], MISSING)
        return df.reset_index(drop=True)

    df = load_frame(options.fname, options.only_perc)
    if options.print_features:
        for f in sorted(list(df)):
            print(f)
    if options.weights != "none" and "x.weight" not in df.columns:
        print("ERROR: no x.weight in the frame: regather, or --weights none")
        exit(-1)
    df_eval = None
    if options.evalframe is not None and not os.path.isfile(options.evalframe):
        print("WARNING: no fair frame %s, no ranking quality" % options.evalframe)
    elif options.evalframe is not None:
        df_eval = load_frame(options.evalframe, 1.0)

    if options.dat_file is not None:
        cols = list(df)
        df.to_pickle(options.dat_file, protocol=3)
        print("Dumped DF to pickle: ", options.dat_file)

    if options.check_row_data_verbose:
        helper.check_too_large_or_nan_values(df, list(df))

    # do the heavy lifting
    learner = Learner(df, df_eval)
    learner.learn()

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
import sklearn.ensemble
import sklearn.tree
import sqlite3
import argparse
import time
import pickle
import re
import pandas as pd
import numpy as np
import os.path
import sys
import helper
import sklearn
ver = sklearn.__version__.split(".")
if int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split


def add_computed_features(df):
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
    df["(var_data.propagated/var_data.sumConflicts_at_fintime)"] = df["var_data.propagated"] / \
        df["var_data.sumConflicts_at_fintime"]
    df["(var_data.propagated/var_data.sumPropagations_at_fintime)"] = df["var_data.propagated"] / \
        df["var_data.sumPropagations_at_fintime"]

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

                df["(" + col + "/" + name + ")"] = df[col]/df[name]
                pass

    # remove sum
    #cols = list(df)
    # for c in cols:
        # if c[0:3] == "sum":
            #del df[c]


def rem_useless_features(df):
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
                # torem.append(col)
                pass

        torem.append("rst.restart_type")

        # actually remove
        for x in torem:
            if x in df:
                del df[x]
    else:
        del df["rst.restart_type"]


class Predict:
    def __init__(self, df):
        self.df = df
        pass

    def cut_into_chunks(self):
        df["x.class"] = pd.qcut(df["x.useful_times_per_marked"],
                        q=options.quantiles,
                        labels=False)
        df["x.class"] = pd.cut(df["x.useful_times_per_marked"],
                               bins=[-1000, 1, 5, 10, 20,
                                     40, 100, 200, 10**20],
                               labels=False)

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
            helper.check_too_large_or_nan_values(df, features+["x.class"])
            print("Checked, all good!")

        train, test = train_test_split(df, test_size=0.33)
        X_train = train[features]
        y_train = train[to_predict]

        t = time.time()
        clf = None
        if final:
            split_point = helper.calc_min_split_point(
                df, options.min_samples_split)
            clf = sklearn.tree.DecisionTreeClassifier(
                max_depth=options.tree_depth,
                min_samples_split=split_point)
        else:
            clf = sklearn.ensemble.RandomForestClassifier(
                n_estimators=1000,
                max_features="sqrt")

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

                helper.output_to_classical_dot(
                    clf, features,
                    fname=options.dot + "-" + self.funcname)

            if write_code and options.basedir:
                c = self.CodeWriter(clf, features, self.funcname, self.fname)
                c.print_full_code()

        prec, recall, acc = helper.conf_matrixes(test, features, to_predict, clf, average="micro")
        helper.conf_matrixes(train, features, to_predict, clf, "train", average="micro")

        # TODO do L1 regularization
        # TODO do principal component analysis

        if not final:
            return best_features
        else:
            return prec+recall+acc

    def learn(self):
        self.cut_into_chunks()
        features = list(self.df)
        features.remove("x.class")
        features.remove("x.useful_times_per_marked")

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

        best_features = []
        # TODO fill best_features here
        self.one_classifier(best_features, "x.class",
                            final=True,
                            write_code=True)

        if options.show:
            plt.show()


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.sqlite"
    parser = argparse.ArgumentParser(usage=usage)

    # dataframe
    parser.add_argument("fname", type=str, metavar='PANDASFILE')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        dest="verbose", help="Print more output")
    parser.add_argument("--top", default=40, type=int, metavar="TOPN",
                        dest="top_num_features", help="Candidates are top N features for greedy selector")
    parser.add_argument("-q", default=4, type=int, metavar="QUANTS",
                        dest="quantiles", help="Number of quantiles we want")
    parser.add_argument("--nocomputed", default=False, action="store_true",
                        dest="no_computed", help="Don't add computed features")
    parser.add_argument("--check", action="store_true", default=False,
                        dest="check_row_data", help="Check row data for NaN or float overflow")
    parser.add_argument("--greedy", default=None, type=int, metavar="TOPN",
                        dest="get_best_topn_feats", help="Greedy Best K top features from the top N features given by '--top N'")
    parser.add_argument("--show", default=False, action="store_true",
                        dest="show", help="Show graphs")
    parser.add_argument("--final", default=False, action="store_true",
                        dest="only_final", help="Only generate final predictor")
    parser.add_argument("--rawplots", action="store_true", default=False,
                        dest="raw_data_plots", help="Display raw data plots")
    parser.add_argument("--dot", type=str, default=None,
                        dest="dot", help="Create DOT file")

    # tree/forest options
    parser.add_argument("--depth", default=None, type=int,
                        dest="tree_depth", help="Depth of the tree to create")
    parser.add_argument("--split", default=0.01, type=float, metavar="RATIO",
                        dest="min_samples_split", help="Split in tree if this many samples or above. Used as a percentage of datapoints")
    parser.add_argument("--numtrees", default=5, type=int,
                        dest="num_trees", help="How many trees to generate for the forest")

    # data filtering
    parser.add_argument("--only", default=0.99, type=float,
                        dest="only_pecr", help="Only use this percentage of data")

    options = parser.parse_args()

    if options.fname is None:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    df = pd.read_pickle(options.fname)

    if not options.no_computed:
        add_computed_features(df)

    rem_useless_features(df)

    p = Predict(df)
    features = p.cut_into_chunks()
    p.learn()

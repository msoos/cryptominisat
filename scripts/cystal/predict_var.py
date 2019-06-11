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

import pandas as pd
import pickle
import sklearn
import sklearn.svm
import sklearn.tree
import optparse
import numpy as np
import sklearn.metrics
import time
import itertools
from math import ceil
import matplotlib.pyplot as plt
from sklearn.ensemble import BaggingClassifier
from sklearn.model_selection import train_test_split


def output_to_dot(clf, features):
    fname = options.dot
    sklearn.tree.export_graphviz(clf, out_file=fname,
                                 feature_names=features,
                                 class_names=clf.classes_,
                                 filled=True, rounded=True,
                                 special_characters=True,
                                 proportion=True)
    print("Run dot:")
    print("dot -Tpng {fname} -o {fname}.png".format(fname=fname))
    print("gwenview {fname}.png".format(fname=fname))


def plot_confusion_matrix(cm, classes,
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
    else:
        print('%s without normalization' % title)

    print(cm)

    if options.show:
        plt.figure()
        plt.imshow(cm, interpolation='nearest', cmap=cmap)
        plt.title(title)
        plt.colorbar()
        tick_marks = np.arange(len(classes))
        plt.xticks(tick_marks, classes, rotation=45)
        plt.yticks(tick_marks, classes)

        fmt = '.2f' if normalize else 'd'
        thresh = cm.max() / 2.
        for i, j in itertools.product(range(cm.shape[0]), range(cm.shape[1])):
            plt.text(j, i, format(cm[i, j], fmt),
                     horizontalalignment="center",
                     color="white" if cm[i, j] > thresh else "black")

        plt.tight_layout()
        plt.ylabel('True label')
        plt.xlabel('Predicted label')


def rem_features(feat, to_remove):
    feat_less = list(feat)
    todel = []
    for feature in feat:
        for rem in to_remove:
            if rem in feature:
                feat_less.remove(feature)
                if options.verbose:
                    print("Removing feature from feat_less:", feature)

    return feat_less


def check_too_large_or_nan_values(df):
    features = df.columns.values.flatten().tolist()
    features = rem_features(features, ["class"])
    index = 0
    for index, row in df.iterrows():
        for x, name in zip(row, features):
            if not np.isfinite(x) or x > np.finfo(np.float32).max:
                print("issue with data for features: ", name, x)
            index += 1


###############
# generic over
###############

def learn(fname):
    with open(fname, "rb") as f:
        df = pickle.load(f)

    check_too_large_or_nan_values(df)

    features = df.columns.values.flatten().tolist()

    torem = ["class", "useful_ratio", "useful_clauses"
             , "sum_decisions_at_picktime", "conflicts"
             , "sum_propagations_at_picktime"
             , "restarts", "decided", "decisions_below"
             , "var", "runID", "propagated"
             , "clauses_below"
             ]

    features = rem_features(features, torem)
    one_classifier(df,features, "class", options.only_final)

def one_classifier(df_orig, features, to_predict, final):
    _, df = train_test_split(df_orig, test_size=options.only_pecr)
    print("================ predicting %s ================" % to_predict)
    print("-> Number of features  :", len(features))
    print("-> Number of datapoints:", df.shape)
    print("-> Predicting          :", to_predict)

    train, test = train_test_split(df, test_size=0.33)
    X_train = train[features]
    y_train = train[to_predict]

    X_test = test[features]
    y_test = test[to_predict]

    t = time.time()
    clf = None
    # clf = sklearn.linear_model.LogisticRegression()
    if final:

        clf_tree = sklearn.tree.DecisionTreeClassifier(
                max_depth=options.tree_depth,
                min_samples_split=options.min_samples_split)

        clf_svm_pre = sklearn.svm.SVC(C=500, gamma=10**-5)
        clf_svm = BaggingClassifier(
            clf_svm_pre
            , n_estimators=3
            , max_samples=0.5, max_features=0.5)

        clf_logreg = sklearn.linear_model.LogisticRegression(
            C=0.3
            , penalty="l1")

        clf_forest = sklearn.ensemble.RandomForestClassifier(
                n_estimators=5
                , min_samples_leaf=options.min_samples_split)

        if options.final_is_tree:
            clf = clf_tree
        elif options.final_is_svm:
            clf = clf_svm
        elif options.final_is_logreg:
            clf = clf_logreg
        elif options.final_is_forest:
            clf = clf_forest
        else:
            mylist = [["tree", clf_tree], ["svm", clf_svm], ["logreg", clf_logreg]]
            clf = sklearn.ensemble.VotingClassifier(mylist)
    else:
        clf = sklearn.ensemble.RandomForestClassifier(
            n_estimators=80
            , min_samples_leaf=options.min_samples_split)

    clf.fit(X_train, y_train)

    print("Training finished. T: %-3.2f" % (time.time() - t))

    best_features = []
    if not final:
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
            best_features.append(features[indices[f]])

        # Plot the feature importances of the clf
        plt.figure()
        plt.title("Feature importances")
        plt.bar(range(myrange), importances[indices],
                color="r", align="center"
                , yerr=std[indices])
        plt.xticks(range(myrange), [features[x] for x in indices], rotation=45)
        plt.xlim([-1, myrange])

    if options.dot is not None and final:
        output_to_dot(clf, features)

    print("Calculating scores for test....")
    y_pred = clf.predict(X_test)
    accuracy = sklearn.metrics.accuracy_score(
        y_test, y_pred)
    precision = sklearn.metrics.precision_score(
        y_test, y_pred, average="macro")
    recall = sklearn.metrics.recall_score(
        y_test, y_pred, average="macro")
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f T: %-3.2f" % (
        precision, recall, accuracy, (time.time() - t)))

    print("Calculating scores for train....")
    y_pred_train = clf.predict(X_train)
    train_accuracy = sklearn.metrics.accuracy_score(
        y_train, y_pred_train)
    train_precision = sklearn.metrics.precision_score(
        y_train, y_pred_train, average="macro")
    train_recall = sklearn.metrics.recall_score(
        y_train, y_pred_train, average="macro")
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f" % (
        train_precision, train_recall, train_accuracy))

    if options.confusion:
        cnf_matrix = sklearn.metrics.confusion_matrix(
            y_true=y_test, y_pred=y_pred)

        np.set_printoptions(precision=2)

        # Plot non-normalized confusion matrix
        plot_confusion_matrix(
            cnf_matrix, classes=clf.classes_,
            title='Confusion matrix, without normalization -- test')

        # Plot normalized confusion matrix
        plot_confusion_matrix(
            cnf_matrix, classes=clf.classes_, normalize=True,
            title='Normalized confusion matrix -- test')

        cnf_matrix_train = sklearn.metrics.confusion_matrix(
            y_true=y_train, y_pred=y_pred_train)
        # Plot normalized confusion matrix
        plot_confusion_matrix(
            cnf_matrix_train, classes=clf.classes_, normalize=True,
            title='Normalized confusion matrix -- train')

    # TODO do L1 regularization

    if not final:
        return best_features
    else:
        return precision+recall+accuracy


if __name__ == "__main__":
    usage = "usage: %prog [options] file.pandas"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--cross", action="store_true", default=False,
                      dest="cross_validate", help="Cross-validate prec/recall/acc against training data")
    parser.add_option("--depth", default=None, type=int,
                      dest="tree_depth", help="Depth of the tree to create")
    parser.add_option("--dot", type=str, default=None,
                      dest="dot", help="Create DOT file")
    parser.add_option("--conf", action="store_true", default=False,
                      dest="confusion", help="Create confusion matrix")
    parser.add_option("--show", action="store_true", default=False,
                      dest="show", help="Show visual graphs")
    parser.add_option("--check", action="store_true", default=False,
                      dest="check_row_data", help="Check row data for NaN or float overflow")
    parser.add_option("--rawplots", action="store_true", default=False,
                      dest="raw_data_plots", help="Display raw data plots")
    parser.add_option("--code", default=None, type=str,
                      dest="code_file", help="Get raw C-like code")
    parser.add_option("--only", default=0.999, type=float,
                      dest="only_pecr", help="Only use this percentage of data")
    parser.add_option("--nordb1", default=False, action="store_true",
                      dest="no_rdb1", help="Delete RDB1 data")
    parser.add_option("--final", default=False, action="store_true",
                      dest="only_final", help="Only generate final predictor")
    parser.add_option("--split", default=80, type=int,
                      dest="min_samples_split", help="Split in tree if this many samples or above. Haved for forests, as there it's the leaf limit")
    parser.add_option("--greedybest", default=40, type=int,
                      dest="get_best_topn_feats", help="Greedy Best K top features from the top N features given by '--top N'")
    parser.add_option("--top", default=40, type=int,
                      dest="top_num_features", help="Top N features to take to generate the final predictor")

    parser.add_option("--tree", default=False, action="store_true",
                      dest="final_is_tree", help="Final predictor should be a tree")
    parser.add_option("--svm", default=False, action="store_true",
                      dest="final_is_svm", help="Final predictor should be an svm")
    parser.add_option("--lreg", default=False, action="store_true",
                      dest="final_is_logreg", help="Final predictor should be a logistic regression")
    parser.add_option("--forest", default=False, action="store_true",
                      dest="final_is_forest", help="Final predictor should be a forest")

    parser.add_option("--funcname", default="should_keep", type=str,
                      dest="funcname", help="The finali function name")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    learn(args[0])

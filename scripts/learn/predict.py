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
import sklearn.ensemble
import optparse
import numpy as np
import sklearn.metrics
import time
import itertools
from math import ceil
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split


def output_to_dot(clf, features, class_names):
    fname = options.dot+class_names[0]
    sklearn.tree.export_graphviz(clf, out_file=fname,
                                 feature_names=features,
                                 class_names=class_names,
                                 filled=True, rounded=True,
                                 special_characters=True,
                                 proportion=True)
    print("Run dot:")
    print("dot -Tpng {fname} -o {fname}.png".format(fname=fname))
    print("gwenview {fname}.png".format(fname=fname))


def calc_cross_val():
    # calculate accuracy/prec/recall for cross-validation
    accuracy = sklearn.model_selection.cross_val_score(self.clf, X_train, y_train, cv=10)
    precision = sklearn.model_selection.cross_val_score(self.clf, X_train, y_train, cv=10, scoring='precision')
    recall = sklearn.model_selection.cross_val_score(self.clf, X_train, y_train, cv=10, scoring='recall')
    print("cv-accuracy:", accuracy)
    print("cv-precision:", precision)
    print("cv-recall:", recall)
    accuracy = np.mean(accuracy)
    precision = np.mean(precision)
    recall = np.mean(recall)
    print("cv-prec: %-3.4f  cv-recall: %-3.4f cv-accuracy: %-3.4f T: %-3.2f" %
          (precision, recall, accuracy, (time.time() - t)))


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


# to check for too large or NaN values:
def check_too_large_or_nan_values(df):
    features = df.columns.values.flatten().tolist()
    index = 0
    for index, row in df.iterrows():
        for x, name in zip(row, features):
            if not np.isfinite(x) or x > np.finfo(np.float32).max:
                print("issue with data for features: ", name, x)
            index += 1

class CodeWriter:
    def __init__(self, clf, features):
        self.f = open(options.code_file, 'w')
        self.clf = clf
        self.feat = features

    def print_full_code(self):
        self.f.write("""#include "clause.h"
#include "reducedb.h"
using namespace CMSat;

namespace CMSat {""")

        num_trees = 1
        if type(self.clf) is sklearn.tree.tree.DecisionTreeClassifier:
            self.f.write("double estimator_0(const Clause* cl, uint32_t last_touched_diff) {\n")
            self.get_code(self.clf, 1)
            self.f.write("}\n")
        else:
            num_trees = len(self.clf.estimators_)
            for tree, i in zip(self.clf.estimators_, range(100)):
                self.f.write("double estimator_%d(const Clause* cl, uint32_t last_touched_diff) {\n" % i)
                self.get_code(tree, 1)
                self.f.write("}\n")

        # Final tally
        self.f.write("bool ReduceDB::should_keep(const Clause* cl, uint32_t last_touched_diff) {\n")
        self.f.write("    int votes = 0;\n")
        for i in range(num_trees):
            self.f.write("    votes += estimator_%d(cl, last_touched_diff) < 1.0;\n" % i)
        self.f.write("    return votes >= %d;\n" % ceil(float(num_trees)/2.0))
        self.f.write("}\n")
        self.f.write("}\n")

    def recurse(self, left, right, threshold, features, node, tabs):
        tabsize = tabs*"    "
        if (threshold[node] != -2):
            feat_name = features[node]
            if feat_name[:3] == "cl.":
                feat_name = feat_name[3:]
            feat_name = feat_name.replace(".", "_")
            feat_name = feat_name.replace("rdb0_", "")
            if feat_name == "dump_no":
                feat_name = "dump_number"

            if feat_name == "size":
                feat_name = "cl->" + feat_name + "()"
            elif feat_name == "last_touched_diff":
                pass
            else:
                feat_name = "cl->stats." + feat_name
            self.f.write("{tabs}if ( {feat} <= {threshold}f ) {{\n".format(
                tabs=tabsize,
                feat=feat_name, threshold=str(threshold[node])))

            # recruse left
            if left[node] != -1:
                self.recurse(left, right, threshold, features, left[node], tabs+1)

            self.f.write("{tabs}}} else {{\n".format(tabs=tabsize))

            # recurse right
            if right[node] != -1:
                self.recurse(left, right, threshold, features, right[node], tabs+1)

            self.f.write("{tabs}}}\n".format(tabs=tabsize))
        else:
            x = self.value[node][0][0]
            y = self.value[node][0][1]
            if y == 0:
                ratio = "1"
            else:
                ratio = "%0.1f/%0.1f" % (x, y)

            self.f.write("{tabs}return {ratio};\n".format(
                tabs=tabsize, ratio=ratio))

    def get_code(self, tree, starttab=0):
        left = tree.tree_.children_left
        right = tree.tree_.children_right
        threshold = tree.tree_.threshold
        print("tree.tree_.feature:", tree.tree_.feature)
        features = [self.feat[i] for i in tree.tree_.feature]
        self.value = tree.tree_.value

        self.recurse(left, right, threshold, features, 0, starttab)


def one_classifier(df, features, to_predict, names, w_name, w_number, final):
    _, df = train_test_split(df, test_size=options.only_pecr)
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
    # clf = sklearn.svm.SVC()
    if final:
        if options.final_is_tree:
            clf = sklearn.tree.DecisionTreeClassifier(max_depth=options.tree_depth, min_samples_split=50)
        else:
            clf = sklearn.ensemble.RandomForestClassifier(n_estimators=5, min_samples_leaf=50)
    else:
        clf = sklearn.ensemble.RandomForestClassifier(n_estimators=80, min_samples_leaf=50)
        #clf = sklearn.ensemble.ExtraTreesClassifier(n_estimators=80)

    sample_weight = [w_number if i == w_name else 1 for i in y_train]
    clf.fit(X_train, y_train, sample_weight=sample_weight)

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

    if options.code_file is not None:
        c = CodeWriter(clf, features)
        c.print_full_code()

    print("Calculating scores for test....")
    y_pred = clf.predict(X_test)
    sample_weight = [w_number if i == w_name else 1 for i in y_pred]
    accuracy = sklearn.metrics.accuracy_score(
        y_test, y_pred, sample_weight=sample_weight)
    precision = sklearn.metrics.precision_score(
        y_test, y_pred, average="macro",sample_weight=sample_weight)
    recall = sklearn.metrics.recall_score(
        y_test, y_pred, average="macro", sample_weight=sample_weight)
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f T: %-3.2f" % (
        precision, recall, accuracy, (time.time() - t)))

    print("Calculating scores for train....")
    y_pred_train = clf.predict(X_train)
    sample_weight_train = [w_number if i == w_name else 1 for i in y_pred_train]
    train_accuracy = sklearn.metrics.accuracy_score(
        y_train, y_pred_train, sample_weight=sample_weight_train)
    train_precision = sklearn.metrics.precision_score(
        y_train, y_pred_train, average="macro",sample_weight=sample_weight_train)
    train_recall = sklearn.metrics.recall_score(
        y_train, y_pred_train, average="macro", sample_weight=sample_weight_train)
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f" % (
        train_precision, train_recall, train_accuracy))

    if options.confusion:
        cnf_matrix = sklearn.metrics.confusion_matrix(
            y_true=y_test, y_pred=y_pred, sample_weight=sample_weight)

        np.set_printoptions(precision=2)

        # Plot non-normalized confusion matrix
        plot_confusion_matrix(
            cnf_matrix, classes=names,
            title='Confusion matrix, without normalization -- test')

        # Plot normalized confusion matrix
        plot_confusion_matrix(
            cnf_matrix, classes=names, normalize=True,
            title='Normalized confusion matrix -- test')

        cnf_matrix_train = sklearn.metrics.confusion_matrix(
            y_true=y_train, y_pred=y_pred_train, sample_weight=sample_weight_train)
        # Plot normalized confusion matrix
        plot_confusion_matrix(
            cnf_matrix_train, classes=names, normalize=True,
            title='Normalized confusion matrix -- train')

    # TODO do L1 regularization

    if False:
        calc_cross_val()

    if options.dot is not None and final:
        output_to_dot(clf, features, names)

    if not final:
        return best_features
    else:
        return precision+recall


def remove_old_clause_features(features):
    todel = []
    for name in features:
        if "cl2" in name or "cl3" in name or "cl4" in name:
            todel.append(name)

    for x in todel:
        features.remove(x)
        if options.verbose:
            print("Removing old clause feature:", x)


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


def calc_greedy_best_features(df, features):
    top_feats = one_classifier(df, features, "x.class", ["OK", "BAD"], "OK", 4, False)
    if options.show:
        plt.show()

    best_features = [top_feats[0]]
    for i in range(options.get_best_topn_feats-1):
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
            mysum = one_classifier(df, this_feats, "x.class", ["OK", "BAD"], "OK", 4, True)
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


def learn(fname):
    with open(fname, "rb") as f:
        df = pickle.load(f)

    if options.check_row_data:
        check_too_large_or_nan_values(df)

    print("total samples: %5d" % df.shape[0])

    features = df.columns.values.flatten().tolist()
    features = rem_features(features,
                            ["x.num_used", "x.class", "x.lifetime", "fname"])
    if options.no_rdb1:
        features = rem_features(features, ["rdb1", "rdb.rel"])
        features = rem_features(features, ["rdb.rel"])

    # this needs binarization
    features = rem_features(features, ["cl.cur_restart_type"])
    # x = (df["cl.cur_restart_type"].values[:, np.newaxis] == df["cl.cur_restart_type"].unique()).astype(int)
    # print(x)

    if True:
        remove_old_clause_features(features)

    if options.raw_data_plots:
        pd.options.display.mpl_style = "default"
        df.hist()
        df.boxplot()

    if options.only_final:
        best_features = ['cl.glue_rel_long', 'rdb0.used_for_uip_creation', 'cl.glue_smaller_than_hist_lt', 'cl.glue_smaller_than_hist_queue', 'cl.overlap', 'cl.glue_rel_queue', 'cl.glue', 'rdb0.dump_no', 'cl.glue_rel', 'cl.num_total_lits_antecedents', 'cl.size', 'cl.overlap_rel', 'cl.num_overlap_literals', 'rdb1.used_for_uip_creation', 'cl.size_rel', 'rdb0.last_touched_diff']

        best_features = ['cl.glue_rel_long', 'rdb0.used_for_uip_creation', 'cl.glue_smaller_than_hist_lt', 'cl.glue_smaller_than_hist_queue', 'cl.glue_rel_queue', 'cl.glue', 'cl.glue_rel', 'cl.size', 'cl.size_rel', 'rdb1.used_for_uip_creation', 'rdb0.dump_no']

        best_features = ['cl.glue_rel', 'cl.backtrack_level_hist_lt', 'rdb0.used_for_uip_creation', 'cl.size_rel', 'cl.overlap_rel', 'cl.glue_rel_long']

        #best_features = ['rdb0.used_for_uip_creation', 'cl.size'] #'cl.glue_rel_long'

        if options.no_rdb1:
            best_features = rem_features(best_features, ["rdb.rel", "rdb1."])

        #best_features = best_features[:2]
        #best_features = features
    else:
        best_features = calc_greedy_best_features(df, features)

    #one_classifier(df, best_features, "x.class", ["OK", "BAD"], "OK", 40, False)
    #one_classifier(df, best_features, "x.class", ["OK", "BAD"], "OK", 29, True)

    if False:
        print("Using unbalanced classifier so as not to loose clauses too much")
        one_classifier(df, best_features, "x.class", ["OK", "OK"], "OK", 8, True)
    else:
        one_classifier(df, best_features, "x.class", ["OK", "BAD"], "OK", 4, True)

    if options.show:
        plt.show()


if __name__ == "__main__":
    usage = "usage: %prog [options] file.pandas"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--cross", action="store_true", default=False,
                      dest="cross_validate", help="Cross-validate prec/recall/acc against training data")
    parser.add_option("--depth", default=6, type=int,
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
    parser.add_option("--greedybest", default=40, type=int,
                      dest="get_best_topn_feats", help="Greedy Best K top features from the top N features given by '--top N'")
    parser.add_option("--top", default=40, type=int,
                      dest="top_num_features", help="Top N features to take to generate the final predictor")
    parser.add_option("--tree", default=False, action="store_true",
                      dest="final_is_tree", help="Final predictor should be a tree")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    learn(args[0])


# todo
# ./predict.py final-pandasdata.dat-2000-newdata-twoRDB --final --conf --only 0.50 --code ../src/\.cpp
#
# ./cryptominisat5 --presimp 1 -n 1 --distill 0 --everylev1 10000 --restart luby mizh-md5-47-3.cnf.gz

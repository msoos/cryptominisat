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
import numpy as np

# to check for too large or NaN values:
def check_too_large_or_nan_values(df, features):
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

    np.set_printoptions(precision=2)
    print(cm)


def calc_min_split_point(df, min_samples_split):
    split_point = int(float(df.shape[0])*min_samples_split)
    if split_point < 10:
        split_point = 10
    print("Minimum split point: ", split_point)
    return split_point


def conf_matrixes(self, data, features, to_predict, clf, toprint="test"):
    # get data
    X_data = data[features]
    y_data = data[to_predict]
    print("Number of elements:", X_data.shape)
    if data.shape[0] <= 1:
        print("Cannot calculate confusion matrix, too few elements")
        return 0, 0, 0

    # Preform prediction
    y_pred = clf.predict(X_data)

    # calc acc, precision, recall
    accuracy = sklearn.metrics.accuracy_score(
        y_data, y_pred)
    precision = sklearn.metrics.precision_score(
        y_data, y_pred, pos_label="OK", average="binary")
    recall = sklearn.metrics.recall_score(
        y_data, y_pred, pos_label="OK", average="binary")
    print("%s prec : %-3.4f  recall: %-3.4f accuracy: %-3.4f" % (
        toprint, precision, recall, accuracy))

    # Plot confusion matrix
    cnf_matrix = sklearn.metrics.confusion_matrix(
        y_true=y_data, y_pred=y_pred)
    helper.print_confusion_matrix(
        cnf_matrix, classes=clf.classes_,
        title='Confusion matrix, without normalization (%s)' % toprint)
    helper.print_confusion_matrix(
        cnf_matrix, classes=clf.classes_, normalize=True,
        title='Normalized confusion matrix (%s)' % toprint)

    return precision, recall, accuracy


def calc_greedy_best_features(top_feats):
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
            mysum = self.one_classifier(this_feats, "x.class", True)
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

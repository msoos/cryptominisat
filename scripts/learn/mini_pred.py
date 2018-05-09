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

import pandas
import pickle
import sklearn
import sklearn.svm
import sklearn.tree
import optparse
import numpy as np
import sklearn.metrics
import time
from sklearn.model_selection import train_test_split


def output_to_dot(clf, features):
    sklearn.tree.export_graphviz(clf, out_file=options.dot,
                                 feature_names=features,
                                 # class_names=["throw", "medium", "OK"],
                                 filled=True, rounded=True,
                                 special_characters=True,
                                 proportion=True)
    print("Run dot:")
    print("dot -Tpng {fname} -o {fname}.png".format(fname=options.dot))


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

def learn(fname):
    with open(fname, "rb") as f:
        df = pickle.load(f)

    print("total samples: %5d" % df.shape[0])

    features = df.columns.values.flatten().tolist()
    features.remove("x.num_used")
    features.remove("x.class")
    features.remove("x.lifetime")
    features.remove("fname")
    features.remove("x.lifetime_cut")
    features.remove("cl.cur_restart_type")
    features.remove("cl2.cur_restart_type")
    print("Number of features:", len(features))

    # pd.options.display.mpl_style = "default"
    # df.hist()
    # df.boxplot()

    train, test = train_test_split(df, test_size=0.33)
    X_train = train[features]
    y_train = train["x.lifetime_cut"]
    X_test = test[features]
    y_test = test["x.lifetime_cut"]

    t = time.time()
    # clf = sklearn.KNeighborsClassifier(5) # EXPENSIVE at prediction, NOT suitable
    # clf = sklearn.linear_model.LogisticRegression() # NOT good.
    # clf = sklearn.ensemble.RandomForestClassifier(min_samples_split=len(X)/20, n_estimators=6)
    clf = sklearn.tree.DecisionTreeClassifier(max_depth=options.tree_depth, class_weight="balanced")
    # clf = sklearn.svm.SVC()
    clf.fit(X_train, y_train)
    print("Training finished. T: %-3.2f" % (time.time() - t))

    print("Calculating scores....")
    y_pred = clf.predict(X_test)
    accuracy = sklearn.metrics.accuracy_score(y_test, y_pred)
    precision = sklearn.metrics.precision_score(y_test, y_pred, average="micro")
    recall = sklearn.metrics.recall_score(y_test, y_pred, average="micro")
    conf_matrix = sklearn.metrics.confusion_matrix(y_test, y_pred)
    print(conf_matrix)
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f T: %-3.2f" % (
        precision, recall, accuracy, (time.time() - t)))

    # TODO do L1 regularization

    if False:
        calc_cross_val()

    if options.dot is not None:
        output_to_dot(clf, features)

    # dump the classifier
    with open("classifier", "wb") as f:
        pickle.dump(clf, f)


if __name__ == "__main__":
    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--cross", action="store_true", default=False,
                      dest="cross_validate", help="Cross-validate prec/recall/acc against training data")
    parser.add_option("--depth", default=5, type=int,
                      dest="tree_depth", help="Depth of the tree to create")
    parser.add_option("--dot", type=str, default=None,
                      dest="dot", help="Create DOT file")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    np.random.seed(2097483)
    learn(args[0])

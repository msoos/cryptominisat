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
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split

class_names = ["throw", "middle", "forever"]


def output_to_dot(clf, features):
    sklearn.tree.export_graphviz(clf, out_file=options.dot,
                                 feature_names=features,
                                 class_names=class_names,
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
        print("Normalized confusion matrix")
    else:
        print('Confusion matrix, without normalization')

    print(cm)

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


def learn(fname):
    with open(fname, "rb") as f:
        df = pickle.load(f)

    print("total samples: %5d" % df.shape[0])

    # lifetime to predict
    df["x.lifetime_cut"] = pd.cut(
        df["x.lifetime"],
        [-1, 10000, 100000, 1000000000000],
        labels=class_names)

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
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f T: %-3.2f" % (
        precision, recall, accuracy, (time.time() - t)))

    if options.confusion:
        cnf_matrix = sklearn.metrics.confusion_matrix(y_test, y_pred,
                                                      labels=class_names)
        np.set_printoptions(precision=2)

        # Plot non-normalized confusion matrix
        plt.figure()
        plot_confusion_matrix(cnf_matrix, classes=class_names,
                              title='Confusion matrix, without normalization')

        # Plot normalized confusion matrix
        plt.figure()
        plot_confusion_matrix(cnf_matrix, classes=class_names, normalize=True,
                              title='Normalized confusion matrix')

        plt.show()

    # TODO do L1 regularization

    if False:
        calc_cross_val()

    if options.dot is not None:
        output_to_dot(clf, features)

    # dump the classifier
    with open("classifier", "wb") as f:
        pickle.dump(clf, f)


if __name__ == "__main__":
    usage = "usage: %prog [options] file.pandas"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--cross", action="store_true", default=False,
                      dest="cross_validate", help="Cross-validate prec/recall/acc against training data")
    parser.add_option("--depth", default=5, type=int,
                      dest="tree_depth", help="Depth of the tree to create")
    parser.add_option("--dot", type=str, default=None,
                      dest="dot", help="Create DOT file")
    parser.add_option("--confusion", action="store_true", default=False,
                      dest="confusion", help="Create confusion matrix")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    np.random.seed(2097483)
    learn(args[0])

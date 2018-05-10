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
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split

class_names = ["throw", "longer"]
cuts = [-1, 20000, 1000000000000]
class_names2 = ["middle", "forever"]
cuts2 = [-1, 100000, 1000000000000]


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


def one_classifier(df, features, to_predict, class_weight, names):
    train, test = train_test_split(df, test_size=0.33)
    X_train = train[features]
    y_train = train[to_predict]
    X_test = test[features]
    y_test = test[to_predict]

    t = time.time()
    # clf = sklearn.linear_model.LogisticRegression(class_weight={"throw": 0.1, "forever":1})
    # clf = sklearn.ensemble.RandomForestClassifier(class_weight={"throw": 0.1, "forever": 1})
    clf = sklearn.tree.DecisionTreeClassifier(max_depth=options.tree_depth,
                                              class_weight=class_weight)

    #clf = sklearn.ensemble.ExtraTreesClassifier(class_weight="balanced",
                                                #n_estimators=80,
                                                #random_state=0)

    # clf = sklearn.svm.SVC()

    # to check for too large or NaN values:
    if False:
        index = 0
        for index, row in X_train.iterrows():
            for x, name in zip(row, features):
                if not np.isfinite(x) or x > np.finfo(np.float32).max:
                    print("row:", name, x)
                index += 1

    clf.fit(X_train, y_train)
    print("Training finished. T: %-3.2f" % (time.time() - t))
    if True:
        importances = clf.feature_importances_
        # std = np.std([tree.feature_importances_ for tree in clf.estimators_], axis=0)
        indices = np.argsort(importances)[::-1]
        indices = indices[:20]
        myrange = min(X_train.shape[1], 20)

        # Print the feature ranking
        print("Feature ranking:")

        for f in range(myrange):
            print("%d. feature %s (%f)" %
                  (f + 1, features[indices[f]], importances[indices[f]]))

        # Plot the feature importances of the clf
        plt.figure()
        plt.title("Feature importances")
        plt.bar(range(myrange), importances[indices],
                color="r", align="center"
                #, yerr=std[indices]
                )
        plt.xticks(range(myrange), [features[x] for x in indices], rotation=45)
        plt.xlim([-1, myrange])

    print("Calculating scores....")
    y_pred = clf.predict(X_test)
    accuracy = sklearn.metrics.accuracy_score(y_test, y_pred)
    precision = sklearn.metrics.precision_score(y_test, y_pred, average="macro")
    recall = sklearn.metrics.recall_score(y_test, y_pred, average="macro")
    print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f T: %-3.2f" % (
        precision, recall, accuracy, (time.time() - t)))

    if options.confusion:
        cnf_matrix = sklearn.metrics.confusion_matrix(y_test, y_pred, labels=names)
        np.set_printoptions(precision=2)

        # Plot non-normalized confusion matrix
        if False:
            plt.figure()
            plot_confusion_matrix(
                cnf_matrix, classes=names,
                title='Confusion matrix, without normalization')

        # Plot normalized confusion matrix
        plt.figure()
        plot_confusion_matrix(
            cnf_matrix, classes=names, normalize=True,
            title='Normalized confusion matrix')

    # TODO do L1 regularization

    if False:
        calc_cross_val()

    if options.dot is not None:
        output_to_dot(clf, features, names[0])


def remove_old_clause_features(features):
    todel = []
    for name in features:
        if "cl2" in name or "cl3" in name or "cl4" in name:
            todel.append(name)

    for x in todel:
        features.remove(x)
        if options.verbose:
            print("Removing old clause feature:", x)

def learn(fname):
    with open(fname, "rb") as f:
        df = pickle.load(f)

    print("total samples: %5d" % df.shape[0])

    # lifetime to predict
    df["x.lifetime_cut"] = pd.cut(
        df["x.lifetime"],
        cuts,
        labels=class_names)

    df["x.lifetime_cut2"] = pd.cut(
        df["x.lifetime"],
        cuts2,
        labels=class_names2)

    features = df.columns.values.flatten().tolist()
    features.remove("x.num_used")
    features.remove("x.class")
    features.remove("x.lifetime")
    features.remove("fname")
    features.remove("x.lifetime_cut")
    features.remove("x.lifetime_cut2")

    # this needs binarization
    features.remove("cl.cur_restart_type")
    features.remove("cl2.cur_restart_type")
    features.remove("cl3.cur_restart_type")
    # x = (df["cl.cur_restart_type"].values[:, np.newaxis] == df["cl.cur_restart_type"].unique()).astype(int)
    # print(x)

    if True:
        remove_old_clause_features(features)

    print("Number of features:", len(features))
    features_less = list(features)
    # print(sorted(features_less))

    todel = []
    for name in features_less:
        if "rdb2" in name or "rdb3" in name or "rdb4" in name:
            todel.append(name)

    for x in todel:
        features_less.remove(x)
        if options.verbose:
            print("Removing feature from features_less:", x)

    # pd.options.display.mpl_style = "default"
    # df.hist()
    # df.boxplot()

    one_classifier(df, features_less, "x.lifetime_cut",
                   {"throw": 0.1, "longer": 1},
                   class_names)

    if options.show:
        plt.show()

    df2 = df[df["x.lifetime"] > 20000]
    one_classifier(df2, features, "x.lifetime_cut2",
                   {"middle": 1, "forever": 1},
                   class_names2)

    if options.show:
        plt.show()


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
    parser.add_option("--conf", action="store_true", default=False,
                      dest="confusion", help="Create confusion matrix")
    parser.add_option("--show", action="store_true", default=False,
                      dest="show", help="Show visual graphs")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    np.random.seed(2097483)
    learn(args[0])

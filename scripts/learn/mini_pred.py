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
import optparse
import numpy as np
import sklearn.metrics
from sklearn.model_selection import train_test_split


def learn(fname):
    with open(fname, "rb") as f:
        df = pickle.load(f)

    features = df.columns.values.flatten().tolist()
    features.remove("x.num_used")
    features.remove("x.class")
    features.remove("x.lifetime")
    features.remove("fname")
    features.remove("x.lifetime_cut")
    features.remove("cl.cur_restart_type")
    features.remove("cl2.cur_restart_type")

    train, test = train_test_split(df, test_size=0.33)
    X_train = train[features]
    y_train = train["x.lifetime_cut"]
    X_test = test[features]
    y_test = test["x.lifetime_cut"]

    clf = sklearn.svm.SVC()
    clf.fit(X_train, y_train)

    y_pred = clf.predict(X_test)
    accuracy = sklearn.metrics.accuracy_score(y_test, y_pred)
    precision = sklearn.metrics.precision_score(y_test, y_pred, average="micro")
    recall = sklearn.metrics.recall_score(y_test, y_pred, average="micro")
    print("accuracy:", accuracy)
    print("precision:", precision)
    print("recall:", recall)
    sklearn.metrics.confusion_matrix(y_test, y_pred)


if __name__ == "__main__":
    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    np.random.seed(2097483)
    learn(args[0])

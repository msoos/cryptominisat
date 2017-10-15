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
import sqlite3
import optparse
import numpy
import time
import pickle
import re
import pandas as pd

from sklearn.model_selection import train_test_split
import sklearn.tree
import sklearn.ensemble
import sklearn.metrics


class QueryHelper:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.runID = self.find_runID()

    def get_rststats_names(self):
        names = None
        for row in self.c.execute("select * from restart limit 1"):
            names = list(map(lambda x: x[0], self.c.description))
        return names

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

    def find_runID(self):
        q = """
        SELECT runID
        FROM startUp
        order by startTime desc
        limit 1
        """

        runID = None
        for row in self.c.execute(q):
            if runID is not None:
                print("ERROR: More than one RUN IDs in file!")
                exit(-1)
            runID = int(row[0])

        print("runID: %d" % runID)
        return runID


class Query2 (QueryHelper):
    def create_indexes(self):
        print("Recreating indexes...")
        t = time.time()
        q = """
        drop index if exists `idxclid`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;

        create index `idxclid` on `clauseStats` (`runID`,`clauseID`);
        create index `idxclid2` on `goodClauses` (`runID`,`clauseID`);
        create index `idxclid3` on `restart` (`runID`,`clauseIDstartInclusive`, `clauseIDendExclusive`);
        """
        for l in q.split('\n'):
            self.c.execute(l)

        print("indexes created %-3.2f" % (time.time() - t))

    def get_max_clauseID(self):
        q = """
        SELECT max(clauseID)
        FROM clauseStats
        WHERE runID = %d
        """ % self.runID

        max_clID = None
        for row in self.c.execute(q):
            max_clID = int(row[0])

        return max_clID

    def get_rststats(self):
        q = """
        select
            numgood.cnt,
            restart.clauseIDendExclusive-restart.clauseIDstartInclusive as total,
            restart.*
        from
            restart,
            (SELECT clauseStats.restarts as restarts, count(clauseStats.clauseID) as cnt
            FROM ClauseStats, goodClauses
            WHERE clauseStats.clauseID = goodClauses.clauseID
            and clauseStats.runID = goodClauses.runID
            and clauseStats.runID = {0}
            group by clauseStats.restarts) as numgood
        where
            restart.runID = {0}
            and restart.restarts = numgood.restarts
        """.format(self.runID)

        X = []
        y = []
        for row in self.c.execute(q):
            r = list(row)
            good = r[0]
            total = r[1]
            perc = float(good) / float(total)
            r = self.transform_rst_row(r[2:])
            X.append(r)
            y.append(perc)

        return X, y

    def get_clstats(self):
        comment = ""
        if not options.restart_used:
            comment = "--"
        q = """
        SELECT clauseStats.*, 1 as good
        {comment} , restart.*
        FROM clauseStats, goodClauses
        {comment} , restart
        WHERE

        clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        and clauseStats.restarts > 1 -- to avoid history being invalid

        {comment} and restart.clauseIDstartInclusive <= clauseStats.clauseID
        {comment} and restart.clauseIDendExclusive > clauseStats.clauseID

        and clauseStats.runID = {0}
        limit {1}
        """.format(self.runID, options.limit, comment=comment)

        df = pd.read_sql_query(q, self.conn)

        # BAD caluses
        q = """
        SELECT clauseStats.*, 0 as good
        {comment} , restart.*
        FROM clauseStats left join goodClauses
        on clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        {comment} , restart
        where

        goodClauses.clauseID is NULL
        and goodClauses.runID is NULL
        and clauseStats.restarts > 1 -- to avoid history being invalid

        {comment} and restart.clauseIDstartInclusive <= clauseStats.clauseID
        {comment} and restart.clauseIDendExclusive > clauseStats.clauseID

        and clauseStats.runID = {0}
        limit {1}
        """.format(self.runID, options.limit, comment=comment)
        df2 = pd.read_sql_query(q, self.conn)

        return pd.concat([df, df2])


class Data:
    def __init__(self, X, y, colnames):
        self.X = numpy.array(X)
        self.y = numpy.array(y)
        self.colnames = colnames

    def add(self, other):
        self.X = numpy.append(self.X, other.X, 0)
        self.y = numpy.append(self.y, other.y, 0)
        if len(self.colnames) == 0:
            self.colnames = other.colnames
        else:
            assert self.colnames == other.colnames


def get_one_file(dbfname):
    print("Using sqlite3db file %s" % dbfname)

    df = None
    with Query2(dbfname) as q:
        if not options.no_recreate_indexes:
            q.create_indexes()
        df = q.get_clstats()
        print(df.head())

    return df


class Classify:
    def __init__(self, df):
        self.features = df.columns.values[:-1].flatten().tolist()

        toremove = ["decision_level_hist",
                    "backtrack_level_hist",
                    "trail_depth_level_hist",
                    "vsids_vars_hist",
                    "runID",
                    "simplifications",
                    "restarts",
                    "conflicts",
                    "clauseID",
                    "size_hist",
                    "glue_hist",
                    "num_antecedents_hist",
                    "decision_level",
                    "backtrack_level",
                    "good"]

        if options.restart_used:
            toremove.extend([
                "restart.runID",
                "restart.simplifications",
                "restart.restarts",
                "restart.conflicts",
                "restart.runtime",
                "restart.clauseIDstartInclusive",
                "restart.clauseIDendExclusive"])

        for t in toremove:
            print("removing feature:", t)
            self.features.remove(t)
        print("features:", self.features)

    def learn(self, df, cleanname, classifiername="classifier"):

        print("total samples: %5d   percentage of good ones %-3.4f" %
              (df.shape[0],
               sum(df["good"]) / float(df.shape[0]) * 100.0))

        print("Training....")
        t = time.time()
        # clf = KNeighborsClassifier(5) # EXPENSIVE at prediction, NOT suitable
        # self.clf = sklearn.linear_model.LogisticRegression() # NOT good.
        self.clf = sklearn.tree.DecisionTreeClassifier(
            random_state=90, max_depth=5)
        # self.clf = sklearn.ensemble.RandomForestClassifier(min_samples_split=len(X)/20, n_estimators=6)
        # self.clf = sklearn.svm.SVC(max_iter=1000) # can't make it work too well..
        train, test = train_test_split(df, test_size=0.2, random_state=90)
        self.clf.fit(train[self.features], train["good"])

        print("Training finished. T: %-3.2f" % (time.time() - t))

        print("Calculating scores....")
        t = time.time()
        y_pred = self.clf.predict(test[self.features])
        recall = sklearn.metrics.recall_score(test["good"], y_pred)
        prec = sklearn.metrics.precision_score(test["good"], y_pred)
        # avg_prec = self.clf.score(X, y)
        print("prec: %-3.4f  recall: %-3.4f T: %-3.2f" %
              (prec, recall, (time.time() - t)))

        with open(classifiername, "wb") as f:
            pickle.dump(self.clf, f)

    def output_to_dot(self, fname):
        sklearn.tree.export_graphviz(self.clf, out_file=fname,
                                     feature_names=self.features,
                                     class_names=["BAD", "GOOD"],
                                     filled=True, rounded=True,
                                     special_characters=True,
                                     proportion=True
                                     )
        print("Run dot:")
        print("dot -Tpng %s -o tree.png" % fname)


class Check:
    def __init__(self, classf_fname):
        with open(classf_fname, "rb") as f:
            self.clf = pickle.load(f)

    def check(self, X, y):
        print("total samples: %5d   percentage of good ones %-3.2f" %
              (len(X), sum(y) / float(len(X)) * 100.0))

        t = time.time()
        y_pred = self.clf.predict(X)
        recall = sklearn.metrics.recall_score(y, y_pred)
        prec = sklearn.metrics.precision_score(y, y_pred)
        # avg_prec = self.clf.score(X, y)
        print("prec: %-3.3f  recall: %-3.3f T: %-3.2f" %
              (prec, recall, (time.time() - t)))


def transform(df):
    def check_clstat_row(self, row):
        if row[self.ntoc["decision_level_hist"]] == 0 or \
                row[self.ntoc["backtrack_level_hist"]] == 0 or \
                row[self.ntoc["trail_depth_level_hist"]] == 0 or \
                row[self.ntoc["vsids_vars_hist"]] == 0 or \
                row[self.ntoc["size_hist"]] == 0 or \
                row[self.ntoc["glue_hist"]] == 0 or \
                row[self.ntoc["num_antecedents_hist"]] == 0:
            print("ERROR: Data is in error:", row)
            exit(-1)

        return row

    df["cl.size_rel"] = df["size"] / df["size_hist"]
    df["cl.glue_rel"] = df["glue"] / df["glue_hist"]
    df["cl.num_antecedents_rel"] = df["num_antecedents"] / \
        df["num_antecedents_hist"]
    df["cl.decision_level_rel"] = df["decision_level"] / df["decision_level_hist"]
    df["cl.backtrack_level_rel"] = df["backtrack_level"] / \
        df["backtrack_level_hist"]
    df["cl.trail_depth_level_rel"] = df["trail_depth_level"] / \
        df["trail_depth_level_hist"]
    df["cl.vsids_vars_rel"] = df["vsids_vars_avg"] / df["vsids_vars_hist"]

    return df


def one_predictor(dbfname, final_df):
    t = time.time()
    df = get_one_file(dbfname)
    cleanname = re.sub('\.cnf.gz.sqlite$', '', dbfname)
    print("Read in data in %-5.2f secs" % (time.time() - t))

    print(df.describe())
    df = transform(df)
    print(df.describe())
    if final_df is None:
        final_df = df
    else:
        final_df = final_df.concat([df, final_df])

    if options.check:
        check = Check(options.check)
        check.check(cl.X, cl.y)
    else:
        clf = Classify(df)
        clf.learn(df, "%s.classifier" % cleanname)
        clf.output_to_dot("%s.tree.dot" % cleanname)


if __name__ == "__main__":

    usage = "usage: %prog [options] dir1 [dir2...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--pow2", "-p", action="store_true", default=False,
                      dest="add_pow2", help="Add power of 2 of all data")

    parser.add_option("--limit", "-l", default=10**9, type=int,
                      dest="limit", help="Max number of good/bad clauses")

    parser.add_option("--check", "-c", type=str,
                      dest="check", help="Check classifier")

    parser.add_option("--data", "-d", action="store_true", default=False,
                      dest="data", help="Just get the dumped data")

    parser.add_option("--restart", "-r", action="store_true", default=False,
                      dest="restart_used", help="Help use restart stat about clause")

    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes", help="Don't recreate indexes")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one directory")
        exit(-1)

    final_df = None
    for dbfname in args:
        print("----- INTERMEDIATE predictor -------\n")
        one_predictor(dbfname, final_df)

    # intermediate predictor is final
    if len(args) == 1:
        exit(0)

    # no need, we checked them all individually
    if options.check:
        exit(0)

    print("----- FINAL predictor -------\n")
    if options.check:
        check = Check()
        check.check(cl.X, cl.y)
    else:
        clf = Classify(final_df)
        clf.learn(final_df, "final.classifier")
        clf.output_to_dot("final.dot")

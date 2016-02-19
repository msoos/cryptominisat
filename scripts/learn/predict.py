#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sqlite3
import optparse
import operator
import numpy
import time
import functools
import glob
import os
import copy

from sklearn.preprocessing import StandardScaler
from sklearn.cross_validation import train_test_split
import sklearn.linear_model
import sklearn.tree

#for presentation (PDF)

from sklearn.externals.six import StringIO
import pydot
from IPython.display import Image


def mypow(to, base):
    return base**to


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.runID = self.find_runID()
        self.clstats_names = self.get_clausestats_names()[5:]
        self.rststats_names = self.get_rststats_names()[5:]

    def get_clausestats_names(self):
        names = None
        for row in self.c.execute("select * from clauseStats limit 1"):
            names = list(map(lambda x: x[0], self.c.description))
        return names

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
            runID = int(row[0])

        print("runID: %d" % runID)
        return runID


class Query2 (Query):
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
            perc = float(good)/float(total)
            r = self.transform_rst_row(r[2:])
            X.append(r)
            y.append(perc)
            #print("---<<<")
            #print(r)
            #print(perc)
            #print("---")

        return X, y

    def get_clstats(self):
        ret = []

        q = """
        SELECT clauseStats.*
        FROM clauseStats, goodClauses
        WHERE clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        and clauseStats.runID = {0}
        order by RANDOM()
        """.format(self.runID)
        for row, _ in zip(self.c.execute(q), xrange(options.limit)):
            #first 5 are not useful, such as restarts and clauseID
            r = self.transform_clstat_row(row)
            ret.append([r, 1])

        bads = []
        q = """
        SELECT clauseStats.*
        FROM clauseStats left join goodClauses
        on clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        where goodClauses.clauseID is NULL
        and goodClauses.runID is NULL
        and clauseStats.runID = {0}
        order by RANDOM()
        """.format(self.runID)
        for row, _ in zip(self.c.execute(q), xrange(options.limit)):
            #first 5 are not useful, such as restarts and clauseID
            r = self.transform_clstat_row(row)
            ret.append([r, 0])

        numpy.random.shuffle(ret)
        X = [x[0] for x in ret]
        y = [x[1] for x in ret]
        return X, y

    def transform_clstat_row(self, row):
        row = list(row[5:])
        row[1] = row[1]/row[4]
        row[4] = 0
        row[5] = 0

        ret = []
        for x in row:
            ret.extend([x, x*x])
            #ret.extend([x])

        return ret

    def transform_rst_row(self, row):
        return row[5:]


class Data:
    def __init__(self, X=[], y=[], colnames=[]):
        self.X = copy.deepcopy(X)
        self.y = copy.deepcopy(y)
        self.colnames = colnames

    def add(self, other):
        self.X.extend(other.X)
        self.y.extend(other.y)
        if len(self.colnames) == 0:
            self.colnames = other.colnames
        else:
            assert self.colnames == other.colnames


def get_one_file(dbfname):
    print("Using sqlite3db file %s" % dbfname)
    clstats_names = None

    with Query2(dbfname) as q:
        clstats_names = q.clstats_names
        X, y = q.get_clstats()
        assert len(X) == len(y)

    cl_data = Data(X, y, clstats_names)

    rst_data = Data()
    if False:
        with Query2(dbfname) as q:
            rststats_names = q.rststats_names
            X, y = q.get_rststats()
            assert len(X) == len(y)

        rst_data = Data(X, y, clstats_names)

    return cl_data, rst_data


class Classify:
    def learn(self, X, y):
        print("number of features:", len(X[0]))
        print("total samples: %5d   percentage of good ones %-3.2f" %
              (len(X), sum(y)/float(len(X))*100.0))
        X = StandardScaler().fit_transform(X)

        print("Training....")
        t = time.time()
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2)

        #clf = KNeighborsClassifier(5)
        #self.clf = sklearn.linear_model.LogisticRegression()
        self.clf = sklearn.linear_model.LogisticRegression()
        #self.clf = sklearn.tree.DecisionTreeClassifier()
        self.clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time()-t))

        print("Calculating score....")
        t = time.time()
        score = self.clf.score(X_test, y_test)
        print("score: %s T: %-3.2f" % (score, (time.time()-t)))

    def output_to_pdf(self, clstats_names, fname):
        return

        dot_data = StringIO()
        sklearn.tree.export_graphviz(self.clf, out_file=dot_data,
                                     feature_names=clstats_names,
                                     class_names=["BAD", "GOOD"],
                                     filled=True, rounded=True,
                                     special_characters=True,
                                     proportion=True
                                     )
        graph = pydot.graph_from_dot_data(dot_data.getvalue())
        #Image(graph.create_png())

        graph.write_pdf(fname)
        print("Wrote final tree to %s" % fname)


if __name__ == "__main__":

    usage = "usage: %prog [options] dir1 [dir2...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--limit", "-l", default=10**9, type=int,
                      dest="limit", help="Max number of good/bad clauses")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one directory")
        exit(-1)

    cl_data = Data()
    rst_data = Data()
    for dbfname in args:
        print("----- INTERMEDIATE predictor -------\n")
        cl, rst = get_one_file(dbfname)
        cl_data.add(cl)
        #rst_data.add(rst)

        clf = Classify()
        clf.learn(cl.X, cl.y)
        clf.output_to_pdf(rst_data.colnames, "tree_cl.pdf")

        if False:
            clf = sklearn.linear_model.LinearRegression()
            rst.X = StandardScaler().fit_transform(rst.X)
            X_train, X_test, y_train, y_test = train_test_split(rst.X, rst.y)
            clf.fit(X_train, y_train)
            score = clf.score(X_test, y_test)
            print("score: %s" % score)

    if len(args) == 1:
        exit(0)

    print("----- FINAL predictor -------\n")

    #clauses
    clf = Classify()
    clf.learn(cl_data.X, cl_data.y)
    print("Columns used were:")
    for i, name in zip(xrange(100), cl_data.colnames):
        print("%-3d  %s" % (i, name))
    clf.output_to_pdf(cl_data.colnames, "tree_cl.pdf")








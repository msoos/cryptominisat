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
import pickle

from sklearn.preprocessing import StandardScaler
from sklearn.cross_validation import train_test_split
import sklearn.linear_model
import sklearn.tree
import sklearn.ensemble
import sklearn.metrics

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
        self.get_clausestats_names()
        self.rststats_names = self.get_rststats_names()[5:]

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
        X = []
        y = []

        q = """
        SELECT clauseStats.*
        FROM clauseStats, goodClauses
        WHERE clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        and clauseStats.runID = {0}
        order by RANDOM()
        limit {1}
        """.format(self.runID, options.limit)
        for row in self.c.execute(q):
            #first 5 are not useful, such as restarts and clauseID
            r = self.transform_clstat_row(row)
            X.append(r)
            y.append(1)

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
        limit {1}
        """.format(self.runID, options.limit)
        for row in self.c.execute(q):
            #first 5 are not useful, such as restarts and clauseID
            r = self.transform_clstat_row(row)
            X.append(r)
            y.append(0)

        return X, y

    def transform_clstat_row(self, row):
        row = list(row[5:])
        row[self.ntoc["backtrack_level"]] /= float(row[self.ntoc["decision_level"]])
        row[self.ntoc["decision_level"]] = 0
        row[self.ntoc["propagation_level"]] = 0

        row.append(0)
        row[self.ntoc["size_minus_glue"]] = row[self.ntoc["size"]] - row[self.ntoc["glue"]]

        ret = []
        if options.add_pow2:
            for x in row:
                ret.extend([x, x*x])

            return ret
        else:
            return row

    def get_clausestats_names(self):
        names = None
        for row in self.c.execute("select * from clauseStats limit 1"):
            names = list(map(lambda x: x[0], self.c.description))

        names = names[5:]
        names.append("size_minus_glue")

        if options.add_pow2:
            orignames = copy.deepcopy(names)
            for name in orignames:
                names.append("%s**2" % name)

        self.clstats_names = names
        self.ntoc = {}
        for n, c in zip(names, xrange(100)):
            #print(n, ":", c)
            self.ntoc[n] = c

    def transform_rst_row(self, row):
        return row[5:]


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
    clstats_names = None

    with Query2(dbfname) as q:
        clstats_names = q.clstats_names
        X, y = q.get_clstats()
        assert len(X) == len(y)

    cl_data = Data(X, y, clstats_names)

    return cl_data


class Classify:
    def learn(self, X, y, classifiername="classifier"):
        print("number of features:", len(X[0]))

        print("total samples: %5d   percentage of good ones %-3.2f" %
              (len(X), sum(y)/float(len(X))*100.0))
        #X = StandardScaler().fit_transform(X)

        print("Training....")
        t = time.time()
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2)

        #clf = KNeighborsClassifier(5)
        #self.clf = sklearn.linear_model.LogisticRegression()
        #self.clf = sklearn.tree.DecisionTreeClassifier(max_depth=5)
        self.clf = sklearn.ensemble.RandomForestClassifier(min_samples_split=len(X)/10, n_estimators=8)
        self.clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time()-t))

        print("Calculating scores....")

        t = time.time()
        y_pred = self.clf.predict(X_test)
        recall = sklearn.metrics.recall_score(y_test, y_pred)
        prec = sklearn.metrics.precision_score(y_test, y_pred)
        # avg_prec = self.clf.score(X, y)
        print("prec: %-3.3f  recall: %-3.3f T: %-3.2f" %
              (prec, recall, (time.time()-t)))

        with open(classifiername, "w") as f:
            pickle.dump(self.clf, f)

    def output_to_pdf(self, clstats_names, fname):
        print("clstats_names len:", len(clstats_names))
        print("clstats_names: %s" % clstats_names)

        #dot_data = StringIO()
        sklearn.tree.export_graphviz(self.clf, out_file=fname,
                                     feature_names=clstats_names,
                                     class_names=["BAD", "GOOD"],
                                     filled=True, rounded=True,
                                     special_characters=True,
                                     proportion=True
                                     )
        #graph = pydot.graph_from_dot_data(dot_data.getvalue())
        print("Done with DOT: %s" % fname)
        #Image(graph.create_png())

        #graph.write_pdf(fname)
        #print("Wrote final tree to %s" % fname)


class Check:
    def __init__(self, classf_fname):
        with open(classf_fname, "r") as f:
            self.clf = pickle.load(f)

    def check(self, X, y):
        print("total samples: %5d   percentage of good ones %-3.2f" %
              (len(X), sum(y)/float(len(X))*100.0))

        t = time.time()
        y_pred = self.clf.predict(X)
        recall = sklearn.metrics.recall_score(y, y_pred)
        prec = sklearn.metrics.precision_score(y, y_pred)
        # avg_prec = self.clf.score(X, y)
        print("prec: %-3.3f  recall: %-3.3f T: %-3.2f" %
              (prec, recall, (time.time()-t)))

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

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one directory")
        exit(-1)

    cl_data = None
    for dbfname in args:
        print("----- INTERMEDIATE predictor -------\n")
        cl = get_one_file(dbfname)

        #print("cl x:")
        #print(cl.X)
        #print("cl data x:")
        #print(cl_data.X)

        if options.check:
            check = Check(options.check)
            check.check(cl.X, cl.y)
        else:
            clf = Classify()
            clf.learn(cl.X, cl.y, "classifier-%s" % dbfname.rstrip(".cnf.gz.sqlite"))
            #clf.output_to_pdf(cl.colnames, "tree_cl-%s.dot" % dbfname.rstrip(".sqlite"))
            if cl_data is None:
                cl_data = cl
            else:
                cl_data.add(cl)

    if len(args) == 1 or options.check:
        exit(0)

    print("----- FINAL predictor -------\n")
    if options.check:
        check = Check()
        check.check(cl.X, cl.y)
    else:
        clf = Classify()
        clf.learn(cl_data.X, cl_data.y)
        print("Columns used were:")
        for i, name in zip(xrange(100), cl_data.colnames):
            print("%-3d  %s" % (i, name))
        #clf.output_to_pdf(cl_data.colnames, "tree_cl.dot")








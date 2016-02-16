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

from sklearn.preprocessing import StandardScaler
from sklearn.cross_validation import train_test_split
import sklearn.tree
from sklearn.externals.six import StringIO
import pydot
from IPython.display import Image


def mypow(to, base):
    return base**to


def parse_lemmas(lemmafname):
    ret = []
    with open(lemmafname, "r") as f:
        for line in f:
            if line[0] == "d":
                continue

            line = line.strip()
            l = line.split(" ")
            good_id = l[len(l)-1]
            if options.verbose:
                print(good_id)

            good_id2 = 0
            try:
                good_id2 = int(good_id)
            except:
                print("ERROR: ID %s is not an integer!" % good_id)
                exit(-1)

            if good_id2 > 1:
                ret.append(good_id2)

    print("Parsed %d number of good lemmas" % len(ret))
    ret = sorted(ret)
    return ret


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.runID = self.find_runID()
        self.get_print_col_names()

    def get_print_col_names(self):
        self.col_names = self.get_clausestats_names()
        self.col_names = self.col_names[5:]

        if options.verbose:
            print("column names: ")
            for name, num in zip(self.col_names, xrange(1000)):
                print("%-3d: %s" % (num, name))

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
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

    def add_goods(self, ids):
        self.c.execute('delete from goodClauses;')

        id_b = [(self.runID, x) for x in ids]
        self.c.executemany("""
            INSERT INTO goodClauses (`runID`, `clauseID`)
            VALUES (?, ?)""", id_b)

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

    def get_clausestats_names(self):
        names = None
        for row in self.c.execute("select * from clauseStats limit 1"):
            names = list(map(lambda x: x[0], self.c.description))
        return names

    def get_restarts(self):
        q = """
        select
            restart.restarts,
            numgood.cnt,
            restart.clauseIDendExclusive-restart.clauseIDstartInclusive as total
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

        for row in self.c.execute(q):
            r = list(row)
            rest = r[0]
            good = r[1]
            total = r[2]
            print("rest num %-6d  conflicts %-6d good %-3.2f%%" %
                  (rest, total, float(good)/total*100.0))

    def get_all(self):
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
            r = self.transform_row(row)
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
            r = self.transform_row(row)
            ret.append([r, 0])

        numpy.random.shuffle(ret)
        X = [x[0] for x in ret]
        y = [x[1] for x in ret]
        return X, y

    def transform_row(self, row):
        row = list(row[5:])
        row[1] = row[1]/row[4]
        row[4] = 0
        row[5] = 0

        return row


def get_one_file(lemmafname, dbfname):
    print("Using sqlite3db file %s" % dbfname)
    print("Using lemma file %s" % lemmafname)
    col_names = None

    with Query(dbfname) as q:
        col_names = q.col_names
        useful_lemma_ids = parse_lemmas(lemmafname)
        q.add_goods(useful_lemma_ids)
        #q.get_restarts()

        X, y = q.get_all()
        assert len(X) == len(y)

    return X, y, col_names


class Classify:
    def predict(self, X, y):
        print("number of features:", len(X[0]))
        print("total samples: %5d   percentage of good ones %-3.2f" %
              (len(X), sum(y)/float(len(X))*100.0))
        X = StandardScaler().fit_transform(X)

        print("Training....")
        t = time.time()
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2)

        #clf = KNeighborsClassifier(5)
        self.clf = sklearn.tree.DecisionTreeClassifier(
            max_depth=options.max_depth)
        self.clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time()-t))

        print("Calculating score....")
        t = time.time()
        score = self.clf.score(X_test, y_test)
        print("score: %s T: %-3.2f" % (score, (time.time()-t)))

    def output_to_pdf(self, col_names):
        dot_data = StringIO()
        sklearn.tree.export_graphviz(self.clf, out_file=dot_data,
                                     feature_names=col_names,
                                     class_names=["BAD", "GOOD"],
                                     filled=True, rounded=True,
                                     special_characters=True,
                                     proportion=True
                                     )
        graph = pydot.graph_from_dot_data(dot_data.getvalue())
        #Image(graph.create_png())

        outf = "tree.pdf"
        graph.write_pdf(outf)
        print("Wrote final tree to %s" % outf)


def get_lemma_and_db(d):
    assert os.path.isdir(d)
    for fname in glob.glob(d + "/*"):
            fname = fname.strip()
            dbfname = None
            #print(fname)
            if fname.endswith(".sqlite"):
                dbfname = fname
                break

    assert dbfname is not None
    lemmafname = d + "/lemmas"

    return lemmafname, dbfname


if __name__ == "__main__":

    usage = "usage: %prog [options] dir1 [dir2...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--limit", "-l", default=10**9, type=int,
                      dest="limit", help="Max number of good/bad clauses")

    parser.add_option("--depth", default=6, type=int,
                      dest="max_depth", help="Max depth")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one directory")
        exit(-1)

    X = []
    y = []
    col_names = None
    for d in args:
        lemmafname, dbfname = get_lemma_and_db(d)
        a, b, col_names = get_one_file(lemmafname, dbfname)
        X.extend(a)
        y.extend(b)
        clf = Classify()
        clf.predict(a, b)

    print("FINAL!!")
    clf = Classify()
    clf.predict(X, y)
    clf.output_to_pdf(col_names)







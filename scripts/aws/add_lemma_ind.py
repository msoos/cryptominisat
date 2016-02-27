#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sqlite3
import optparse


class Data:
    def __init__(self, untilID=-1, useful=0):
        self.untilID = untilID
        self.useful = useful


def parse_lemmas(lemmafname):
    ret = {}
    last_good_id = 2
    with open(lemmafname, "r") as f:
        for line in f:
            if line[0] == "d":
                l = line.strip().split(" ")
                del_id = l[len(l)-1]
                ret[del_id] = Data(last_good_id, 0)

            l = line.strip().split(" ")
            good_id = l[len(l)-1]

            good_id2 = 0
            try:
                good_id2 = int(good_id)
            except:
                print("ERROR: ID %s is not an integer!" % good_id)
                exit(-1)

            if good_id2 > 1:
                ret[good_id2] = Data()
            last_good_id = good_id2

    print("Parsed %d number of good lemmas" % len(ret))
    return ret


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.runID = self.find_runID()

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

    def add_goods(self, ids):
        self.c.execute('delete from goodClauses;')

        id_b = [(self.runID, ID, x.useful, x.untilID) for ID, x in ids.iteritems()]
        self.c.executemany("""
            INSERT INTO goodClauses (`runID`, `clauseID`, `numUsed`, `usedUntilID`)
            VALUES (?, ?, ?, ?);""", id_b)


if __name__ == "__main__":
    usage = "usage: %prog [options] sqlite_db lemmas"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) != 2:
        print("Error. Please follow usage")
        print(usage)
        exit(-1)

    dbfname = args[0]
    lemmafname = args[1]
    print("Using sqlite3db file %s" % dbfname)
    print("Using lemma file %s" % lemmafname)

    with Query(dbfname) as q:
        useful_lemma_ids = parse_lemmas(lemmafname)
        print("Num good IDs: %d" % len(useful_lemma_ids))
        q.add_goods(useful_lemma_ids)

    print("Finished adding good lemma indicators to db %s" % dbfname)
    exit(0)

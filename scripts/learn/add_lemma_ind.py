#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
import sqlite3
import optparse


def parse_lemmas(lemmafname, runID, verbose=False):
    """Takes the lemma file and returns map with clauses' IDs and data"""

    # clause in "lemmas" file layout:
    # CLAUSE 0 ID last_used num_used

    ret = []
    with open(lemmafname, "r") as f:
        for line, lineno in zip(f, range(1000*1000*1000)):
            l = line.strip().split(" ")

            # checking that delete line is ok
            # TODO: maybe calculate used_for_time
            if line[0] == "d":
                continue

            if len(l) == 1:
                # empty clause, finished
                continue

            myid = int(l[len(l)-3])
            num_used = int(l[len(l)-1])
            ret.append((runID, myid, num_used))

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
        """

        runID = None
        for row in self.c.execute(q):
            if runID is not None:
                print("ERROR: More than one RUN in the SQL, can't add lemmas!")
                exit(-1)
            runID = int(row[0])

        print("runID: %d" % runID)
        return runID

    def add_goods(self, ids):
        self.c.execute('delete from goodClauses;')
        self.c.executemany("""
            INSERT INTO goodClauses (`runID`, `clauseID`, `numUsed`)
            VALUES (?, ?, ?);""", ids)


if __name__ == "__main__":
    usage = """usage: %prog [options] sqlite_db lemmas

It adds lemma indices from "lemmas" to the SQLite database, indicating whether
it was good or not."""

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
        useful_lemma_ids = parse_lemmas(lemmafname, q.runID, options.verbose)
        print("Num good IDs: %d" % len(useful_lemma_ids))
        q.add_goods(useful_lemma_ids)

    print("Finished adding good lemma indicators to db %s" % dbfname)
    exit(0)

#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sqlite3
import optparse


class Data:
    def __init__(self, used_for_time=-1, num_used=0):
        self.used_for_time = used_for_time
        self.num_used = num_used


def parse_lemmas(lemmafname):
    ret = {}
    with open(lemmafname, "r") as f:
        for line, lineno in zip(f, xrange(1000*1000*1000)):
            if line[0] == "d":
                l = line.strip().split(" ")
                myid = int(l[len(l)-3])
                if myid <= 1:
                    continue

                last_used = int(l[len(l)-2])
                num_used = int(l[len(l)-1])
                ret[myid].used_for_time = last_used - myid
                #print(myid)
                #print(num_used)
                #print(ret[myid].num_used)
                if ret[myid].num_used != num_used:
                    print("Line no %d wrong usage value" % lineno)
                    print("line: '%s'" % line.strip())
                    assert ret[myid].num_used == num_used
                continue

            l = line.strip().split(" ")
            if len(l) == 1:
                #empty clause, finished
                continue

            myid = int(l[len(l)-3])
            num_used = int(l[len(l)-1])

            #used until the end.
            used_for_time = 1000000
            ret[myid] = Data(used_for_time, num_used)

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

        id_b = [(self.runID, ID, x.num_used, x.used_for_time) for ID, x in ids.iteritems()]
        self.c.executemany("""
            INSERT INTO goodClauses (`runID`, `clauseID`, `numUsed`, `usedForTime`)
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

#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sqlite3
import optparse
import operator

usage = "usage: %prog [options] sqlitedb"

parser = optparse.OptionParser(usage=usage)

# parser.add_option("--extraopts", "-e", metavar="OPTS",
                  #dest="extra_options", default="",
                  #help="Extra options to give to SAT solver")

# parser.add_option("--verbose", "-v", action="store_true", default=False,
                  #dest="verbose", help="Print more output")

(options, args) = parser.parse_args()

if len(args) != 1:
    print("ERROR: You must give exactly one argument, the sqlite3 database file")
    exit(-1)

dbfname = args[0]
print("Using sqlite3db file %s" % dbfname)


class Query:
    def __init__(self):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.close()

    def find_outliers(self):
        query = """
        select name,elapsed,tag from timepassed,tags
        where name != 'search' and elapsed > 20 and
        tags.tagname="filename" and tags.runID = timepassed.runID
        order by elapsed desc;
        """

        for row in self.c.execute(query):
            operation = row[0]
            t = row[1]
            name = row[2].split("/")
            name = name[len(name)-1]
            if len(name) > 30:
                name = name[:30]
            print("%-32s    %-20s   %.1fs" % (name, operation, t))

    def calc_time_spent(self):
        query = """
        select sum(elapsed)
        from timepassed
        where name='search';
        """
        for row in self.c.execute(query):
            search = float(row[0])

        query = """
        select sum(elapsed)
        from timepassed
        where name!='search';
        """
        for row in self.c.execute(query):
            other = float(row[0])

        total = search + other
        print("Total: %10.1fh Search: %3.1f%%, Other: %3.1f%%" %
              (total/3600, search/total*100, other/total*100))

        query = """
        select name, sum(elapsed)
        from timepassed
        group by name;
        """
        times = {}
        for row in self.c.execute(query):
            times[row[0]] = float(row[1])

        # print("Names: %s" % times)
        sorted_t = sorted(times.items(), key=operator.itemgetter(1), reverse=True)
        for (name, t) in sorted_t:
            print("%-40s   %3.1f%%" % (name, t/total*100))


with Query() as q:
    q.find_outliers()
    q.calc_time_spent()



#select timepassed.runID, tag,elapsed from timepassed,tags where name like 'shorten and str red%' and elapsed > 2 and tags.runID = timepassed.runID;

#select * from timepassed where elapsed > 20 and name not like 'search';

#select * from startup, solverRun, finishup where finishup.runID = solverRun.runID and (finishup.endTime - startup.startTime) < 30 and solverRun.version = "618b5e79fdd8a15918e19fb76ca08aa069f14b54" and solverRun.runID = startup.runID;

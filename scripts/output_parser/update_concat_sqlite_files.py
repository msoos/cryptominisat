#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sqlite3
import optparse
import random

tables = ["tags", "timepassed", "memused"
          , "solverRun", "startup", "finishup"]

class Query:
    def __init__(self, dbfname):
        self.dbfname = dbfname
        self.conn = sqlite3.connect(self.dbfname)
        self.c = self.conn.cursor()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.close()

    def add_ids(self):
        print("----------- adding IDs to db %s --------------" % self.dbfname)
        runid = random.randint(0, 2**32-1)

        query = """
        alter table `{tablename}` add column runid bigint(2) default null;
        update `{tablename}` set runid={runid};
        """

        for table in tables:
            print("adding runid {runid} to table {tablename}"
                      .format(runid=runid, tablename=table))
            for q in query.split("\n"):
                self.c.execute(q.format(runid=runid, tablename=table))

        print("Finished adding IDs to tables")
        self.c.execute("commit;")


    def create_empty_tables(self):
        query="""
        DROP TABLE IF EXISTS `tags`;
        CREATE TABLE `tags` (
          `name` varchar(500) NOT NULL,
          `val` varchar(500) NOT NULL
          , runid bigint(2) default null
        );

        DROP TABLE IF EXISTS `timepassed`;
        CREATE TABLE `timepassed` (
          `simplifications` bigint(20) NOT NULL,
          `conflicts` bigint(20) NOT NULL,
          `runtime` float NOT NULL,
          `name` varchar(200) NOT NULL,
          `elapsed` float NOT NULL,
          `timeout` int(20) DEFAULT NULL,
          `percenttimeremain` float DEFAULT NULL
          , runid bigint(2) default null
        );

        DROP TABLE IF EXISTS `memused`;
        CREATE TABLE `memused` (
          `simplifications` bigint(20) NOT NULL,
          `conflicts` bigint(20) NOT NULL,
          `runtime` float NOT NULL,
          `name` varchar(200) NOT NULL,
          `MB` int(20) NOT NULL
          , runid bigint(2) default null
        );

        DROP TABLE IF EXISTS `solverRun`;
        CREATE TABLE `solverRun` (
          `runtime` float NOT NULL,
          `gitrev` varchar(100) NOT NULL
          , runid bigint(2) default null
        );

        DROP TABLE IF EXISTS `startup`;
        CREATE TABLE `startup` (
          `startTime` datetime NOT NULL
          , runid bigint(2) default null
        );

        DROP TABLE IF EXISTS `finishup`;
        CREATE TABLE `finishup` (
          `endTime` datetime NOT NULL,
          `status` varchar(255) NOT NULL
          , runid bigint(2) default null
        );"""
        self.c.executescript(query)

    def merge_data(self, files):
        query = """
        attach '{fname}' as toMerge;
        BEGIN;
        insert into {table} select * from toMerge.{table};
        COMMIT;
        detach toMerge;
        """

        for f in files:
            print("Merging file %s" % f)
            for table in tables:
                print("-> Merging table %s" % table)
                self.c.executescript(query.format(fname=f, table=table))

if __name__ == "__main__":
    usage = "usage: %prog [options] sqlitedb"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--onlyconcat", action="store_true", default=False,
                      dest="onlyconcat", help="Only concatenate")

    (options, args) = parser.parse_args()

    if len(args) < 2:
        print("ERROR: You must give at least two arguments, the sqlite3 database files")
        exit(-1)

    if not options.onlyconcat:
        for fname in args:
            print("Using sqlite3db file %s" % fname)

            #peform queries
            with Query(fname) as q:
                q.add_ids()

        print("Finished adding IDs to all tables in all files")

    print("Merging tables...")
    with Query("merged.sqlitedb") as q:
        q.create_empty_tables()
        q.merge_data(args)

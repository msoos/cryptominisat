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


import helper
import optparse
import time

class QueryFixPerc (helper.QueryHelper):
    def __init__(self, dbfname):
        super(QueryFixPerc, self).__init__(dbfname)

    def check_table_exists(self):
        q = "SELECT name FROM sqlite_master WHERE type='table' AND name='{table_name}';".format(
            table_name="used_later_percentiles_backup")
        self.c.execute(q)
        rows = self.c.fetchall()
        if len(rows) == 1:
            print("Already fixed")
            return True

        return False

    def copy_db(self):
        # Drop table
        q_drop = """
        DROP TABLE IF EXISTS `used_later_percentiles_backup`;
        """
        self.c.execute(q_drop)

        # Create and fill used_later_X tables
        q_create = """
        CREATE TABLE used_later_percentiles_backup AS
          SELECT *
          FROM used_later_percentiles;"""
        self.c.execute(q_create)

    def create_percentiles_table(self):
        # Drop table
        q_drop = """
        DROP TABLE IF EXISTS `used_later_percentiles`;
        """
        self.c.execute(q_drop)

        # Create and fill used_later_X tables
        q_create = """
        create table `used_later_percentiles` (
            `type_of_dat` string NOT NULL,
            `percentile_descr` string NOT NULL,
            `percentile` float DEFAULT NULL,
            `val` float NOT NULL
        );"""
        self.c.execute(q_create)

        idxs = """
        create index `used_later_percentiles_idx3` on `used_later_percentiles` (`type_of_dat`, `percentile_descr`, `percentile`, `val`);
        create index `used_later_percentiles_idx2` on `used_later_percentiles` (`type_of_dat`, `percentile_descr`, `val`);"""
        for q in idxs.split("\n"):
            self.c.execute(q)

    def fix(self):
        q2 = """
        insert into used_later_percentiles (type_of_dat, percentile_descr, percentile, val)
        {q}
        """

        t = time.time()
        q = "select * from used_later_percentiles_backup"
        self.c.execute(q)
        rows = self.c.fetchall()
        fixed = 0
        for i in range(len(rows)):
            if options.verbose:
                print("row: ", rows[i])
            row = rows[i]
            a = row[0][1:]
            c = None
            if row[1][:13] == "top_also_zero":
                b = "top_also_zero"
                c = row[1][13:]
            elif row[1][:12] == "top_non_zero":
                b = "top_non_zero"
                c = row[1][12:]
            elif row[1] == "avg":
                b = "avg"
                c = None
            else:
                print("c ? ", row[1])
                assert False, "What?"

            if c is not None:
                c = float(c.rstrip("_perc").strip("_"))
            else:
                c = "NULL"
            d = row[2]

            if a == "":
                # this was removed
                continue

            if options.verbose:
                print("a: ", a)
                print("b: ", b)
                print("c: ", c)
                print("d: ", d)
            self.c.execute(q2.format(q="select '{a}', '{b}', {c}, {d};".format(
                a=a, b=b, c=c, d=d)))
            fixed+=1

        print("Fixed:", fixed)

        # fix 100
        for name in ["short", "long", "forever"]:
            self.c.execute(q2.format(name=name, q="select '{name}', 'top_non_zero', 100.0, 0.0;".format(name=name)))
            self.c.execute(q2.format(name=name, q="select '{name}', 'top_also_zero', 100.0, 0.0;".format(name=name)))

        print("Fixed top 100 too")

if __name__ == "__main__":
    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    # verbosity level
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--sql", action="store_true", default=False,
                      dest="dump_sql", help="Dump SQL queries")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)


    for f in args:
        print("Doing file:" , f)
        with QueryFixPerc(f) as q:
            if q.check_table_exists():
                continue
            q.copy_db()
            q.create_percentiles_table()
            q.fix()

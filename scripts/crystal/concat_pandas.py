#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
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
import optparse
import time
import pickle
import pandas as pd
import numpy as np
import sys


if __name__ == "__main__":
    usage = """usage: %prog [options] file1.dat [file2.dat ...]
Concatenates Pandas dataframes into a single file. The dataframes must have
the same columns."""
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")
    parser.add_option("--out", "-o", default=None, type=str,
                      dest="out", help="Put combined output here")
    parser.add_option("--csv", default=None, type=str,
                      dest="csv", help="Dump CSV here")
    parser.add_option("--csvratio", default=1.0, type=float,
                      dest="csvratio", help="Ratio of data to dump to CSV")

    (options, args) = parser.parse_args()

    if options.out is None and options.csv is None:
        print("ERROR: you must either dump CSV or Pandas. Give either '--out' or '--csv' options")
        exit(-1)

    if options.csvratio < 0.0:
        print("ERROR: CSV ratio cannot be less than 0.0")
        exit(-1)

    if options.csvratio > 1.0:
        print("ERROR: CSV ratio cannot be more than 1.0")
        exit(-1)

    dfs = []
    for fname in args:
        print("----- Reading file %s -------" % fname)
        df = pd.read_pickle(fname)
        print("Read {num} datapoints from {file}".format(num=df.shape[0], file=fname))
        df["fname"] = df["fname"].astype("str")
        #df["cl.cur_restart_type"] = df["cl.cur_restart_type"].astype("str")
        #df["rdb0.cur_restart_type"] = df["rdb0.cur_restart_type"].astype("str")
        dfs.append(df)

    print("Concatenating dataframes...")
    df_full = pd.concat(dfs)
    print("Concated frame size: %d" % df_full.shape[0])

    if options.out is not None:
        fname = options.out
        print("Dumping {num} datapoint pandas data to: {fname}".format(num=df_full.shape[0], fname=fname))
        with open(fname, "wb") as f:
            pickle.dump(df_full, f)

    if options.csv is not None:
        import sklearn.model_selection

        fname = options.csv
        print("Dumpiing CSV ratio: %-3.2f" % options.csvratio)
        _, df_tmp = sklearn.model_selection.train_test_split(df_full, test_size=options.csvratio)
        print("Dumping {num} datapoint CSV data to: {fname}".format(num=df_tmp.shape[0], fname=fname))
        df_tmp.to_csv(fname, index=False, columns=sorted(list(df_tmp)))

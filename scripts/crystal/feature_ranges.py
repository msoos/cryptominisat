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

# The range each feature had in the training data (1st and 99th
# percentile), written next to the feature list as <list>.ranges. The
# predictor build compiles them in and counts, at every prediction, the
# values that fall outside: a run whose inputs the model never saw is a
# run the model knows nothing about, and the solver says so at the end.
#
# usage: feature_ranges.py -f best_features.txt -o best_features.txt.ranges frame.dat

import argparse
import os
import sys

import numpy as np
import pandas as pd

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import helper

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("frame", help="a training frame (the comb-*.dat of learn.sh)")
    parser.add_argument("-f", "--features", required=True)
    parser.add_argument("-o", "--out", required=True)
    parser.add_argument("--lo", type=float, default=1.0, help="lower percentile, default 1")
    parser.add_argument("--hi", type=float, default=99.0, help="upper percentile, default 99")
    opts = parser.parse_args()
    feats = helper.get_features(opts.features)
    df = pd.read_pickle(opts.frame)
    df = df.convert_dtypes(convert_integer=False, convert_string=False, convert_floating=False)
    helper.make_missing_into_nan(df)
    helper.add_features_from_list(df, feats)
    with open(opts.out, "w") as f:
        f.write("# feature, %gth and %gth percentile over %d training rows of %s\n" % (
            opts.lo, opts.hi, len(df), os.path.basename(opts.frame)))
        for feat in feats:
            v = df[feat].astype(float).replace([np.inf, -np.inf], np.nan).dropna()
            if len(v) == 0:
                f.write("%s\tnan\tnan\n" % feat)
                continue
            f.write("%s\t%r\t%r\n" % (feat, float(np.percentile(v, opts.lo)), float(np.percentile(v, opts.hi))))
    print("%d ranges -> %s" % (len(feats), opts.out))

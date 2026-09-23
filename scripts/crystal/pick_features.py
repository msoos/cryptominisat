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

# Picks a best_features file from the importance rankings that
# gen_best_feats.sh printed: sums each feature's importance over all
# (table, tier) runs, keeps only features the solver can compute (see
# gen_pred_features.py --list-raw), and writes the top N.
#
# usage: pick_features.py -n 30 -o best_features.txt <gen_best_feats outdir>

import argparse
import ast
import glob
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_pred_features


def computable(feat):
    try:
        gen_pred_features.to_cpp(ast.parse(feat, mode="eval"), set())
        return True
    except (ValueError, SyntaxError):
        return False


def read_importances(fname):
    """The 'impdf:' table of a cldata_predict.py output: feature, importance"""
    imps = {}
    with open(fname) as f:
        lines = f.readlines()
    for i, l in enumerate(lines):
        if l.startswith("impdf:"):
            for l2 in lines[i+1:]:
                m = re.match(r"^\s*\d+\s+(\S+)\s+([0-9.eE+-]+)\s*$", l2)
                if not m:
                    break
                imps[m.group(1)] = float(m.group(2))
            break
    return imps


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("outdir", help="output dir of gen_best_feats.sh")
    parser.add_argument("-n", "--num", type=int, default=30, help="how many features")
    parser.add_argument("-o", "--out", default=None, help="write the best_features file here")
    parser.add_argument("--computed", choices=["all", "no", "both"], default="both",
                        help="use the runs with all_computed, no_computed, or both")
    opts = parser.parse_args()

    total = defaultdict(float)
    runs = 0
    for fname in sorted(glob.glob(os.path.join(opts.outdir, "output_*computed"))):
        which = "all" if fname.endswith("_allcomputed") else "no"
        if opts.computed != "both" and which != opts.computed:
            continue
        imps = read_importances(fname)
        if not imps:
            print("WARNING: no importances in %s" % fname)
            continue
        runs += 1
        # each run's importances sum to 1, so runs weigh equally
        for feat, imp in imps.items():
            total[feat] += imp
    if runs == 0:
        print("ERROR: no importance rankings found in %s" % opts.outdir)
        exit(-1)

    ranked = sorted(total.items(), key=lambda x: -x[1])
    picked = []
    skipped = []
    for feat, imp in ranked:
        if not computable(feat):
            skipped.append((feat, imp))
            continue
        picked.append((feat, imp))
        if len(picked) >= opts.num:
            break

    print("Runs: %d. Top %d computable features (summed importance):" % (runs, len(picked)))
    for feat, imp in picked:
        print("  %-70s %.4f" % (feat, imp))
    if skipped:
        print("Skipped %d features the solver cannot compute, the best were:" % len(skipped))
        for feat, imp in skipped[:10]:
            print("  %-70s %.4f" % (feat, imp))

    if opts.out:
        with open(opts.out, "w") as f:
            f.write("# picked by pick_features.py from %s, %d runs\n" % (opts.outdir, runs))
            for feat, imp in picked:
                f.write("%s\n" % feat)
        print("Wrote %s" % opts.out)

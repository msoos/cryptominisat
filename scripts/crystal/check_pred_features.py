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

# Checks the solver's generated feature code against pandas: recomputes
# the features of a --preddump file from its raw columns, the way training
# does, and the predictions with the Python xgboost.
#
# usage: check_pred_features.py dump [-f best_features.txt] [model.json]
#        (no model = features only)

import argparse
import ast
import os
import sys
import numpy as np
import pandas as pd
from ccg import *
import gen_pred_features

FLT_MAX = float(np.finfo(np.float32).max)


def read_exact(f, dtype, count):
    a = np.fromfile(f, dtype=dtype, count=count)
    if len(a) != count:
        sys.exit("ERROR: the dump is truncated")
    return a


def mismatches(got, want, rtol):
    """rows x cols bool: missing on one side only, or further than rtol"""
    gm, wm = np.isnan(got), np.isnan(want)
    with np.errstate(invalid="ignore"):
        far = np.abs(got - want) > rtol * np.abs(want)
    return (gm != wm) | (~gm & ~wm & far)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("dump", help="file written by --preddump")
    parser.add_argument("model", nargs="?", help="the model the solver ran with")
    parser.add_argument("-f", "--features", default=os.path.join(here, "best_features.txt"))
    parser.add_argument("--rtol", type=float, default=1e-6, help="relative tolerance of a feature")
    parser.add_argument("--pred-rtol", type=float, default=1e-4, help="relative tolerance of a prediction")
    opts = parser.parse_args()

    raw_names = list(gen_pred_features.RAW)
    feats = gen_pred_features.read_features(opts.features)
    exprs = [compile(ccg.to_source(ast.parse(feat)).strip(), feat, "eval") for feat in feats]
    booster = None
    if opts.model:
        import xgboost as xgb
        booster = xgb.Booster(model_file=opts.model)
        booster.set_param({"nthread": 1})

    rows = 0
    bad_feat = np.zeros(len(feats), dtype=np.int64)
    bad_pred = 0
    worst = {}
    with open(opts.dump, "rb") as f:
        num_raw, num_feat = read_exact(f, np.uint32, 2)
        if num_raw != len(raw_names) or num_feat != len(feats):
            sys.exit("ERROR: the dump has %d raw columns and %d features, %s and gen_pred_features.py have %d and %d"
                     % (num_raw, num_feat, opts.features, len(raw_names), len(feats)))
        while True:
            head = np.fromfile(f, dtype=np.uint32, count=1)
            if len(head) == 0:
                break
            num = int(head[0])
            raw = read_exact(f, np.float64, num * num_raw).reshape(num, num_raw)
            got = read_exact(f, np.float32, num * num_feat).reshape(num, num_feat)
            preds = read_exact(f, np.float64, num)

            df = pd.DataFrame(raw, columns=raw_names)
            want = np.empty((num, num_feat), dtype=np.float64)
            with np.errstate(all="ignore"):
                for i, e in enumerate(exprs):
                    want[:, i] = eval(e, {"df": df})
                # as to_float() of predict_features_gen.h
                want[~np.isfinite(want) | (np.abs(want) > FLT_MAX)] = np.nan
                want = want.astype(np.float32)
            bad = mismatches(got, want, opts.rtol)
            bad_feat += bad.sum(axis=0)
            for r, c in zip(*np.nonzero(bad)):
                worst.setdefault(c, (got[r, c], want[r, c]))
            if booster:
                p = booster.inplace_predict(got, missing=np.nan)
                bad_pred += int(mismatches(preds, p.astype(np.float64), opts.pred_rtol).sum())
            rows += num

    if rows == 0:
        sys.exit("ERROR: the dump has no rows, the run was too short to reduce")
    for c in np.nonzero(bad_feat)[0]:
        print("FAILED: feature %d '%s' differs in %d of %d rows, e.g. C++ %r pandas %r"
              % (c, feats[c], bad_feat[c], rows, float(worst[c][0]), float(worst[c][1])))
    if bad_pred:
        print("FAILED: %d of %d predictions differ from the Python xgboost's" % (bad_pred, rows))
    if bad_feat.any() or bad_pred:
        sys.exit(1)
    print("OK: %d rows, %d features%s agree" % (rows, len(feats), " and the model's predictions" if booster else ""))


if __name__ == "__main__":
    main()

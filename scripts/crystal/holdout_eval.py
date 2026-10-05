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

# Offline hold-out: train on the frames of some instance dirs, report the
# ranking quality on the FAIR frames (evaldata) of others: per reduce,
# the share of the future use kept when keeping the best 25%/50% of the
# reduce candidates, model vs glue/size vs oracle, mean over the reduces
# of an instance, then over the instances. Minutes instead of the hours
# a solver A/B takes, so targets, objectives and feature lists can be
# compared first; the solver A/B is still the last word.
#
# usage: holdout_eval.py --train a-dir b-dir --test c-dir d-dir
#          [--target count|rel] [--objective squarederror|log|poisson]
#          [--weights none|strata|instance|family]
#          [-f best_features.txt] [--table used_later]

import argparse
import glob
import os
import sys

import numpy as np
import pandas as pd
import xgboost as xgb

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import helper

MISSING = np.nan


def load(dirs, table, what="cldata", suffix="-cut1-*.dat"):
    dfs = []
    for d in dirs:
        fs = glob.glob(os.path.join(d, "data-min.db-%s-%s-disc%s" % (what, table, suffix)))
        if not fs:
            print("no %s frame in %s, skipped" % (table, d))
            continue
        df = pd.read_pickle(fs[0])
        df["fname"] = os.path.basename(d.rstrip("/"))
        dfs.append(df)
    return pd.concat(dfs, ignore_index=True) if dfs else None


def prepare(df, feats):
    df = df.convert_dtypes(convert_integer=False, convert_string=False, convert_floating=False)
    helper.make_missing_into_nan(df)
    helper.add_features_from_list(df, feats)
    df.replace([np.inf, -np.inf], MISSING, inplace=True)
    X = df[feats].astype(np.float32).replace([np.inf, -np.inf], MISSING)
    return df, X


def kept(truth, order, frac):
    n = int(frac * len(truth))
    return 100.0 * truth[order[:n]].sum() / truth.sum() if truth.sum() > 0 else float("nan")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--train", nargs="+", required=True)
    parser.add_argument("--test", nargs="+", required=True)
    parser.add_argument("--table", default="used_later")
    parser.add_argument("--weights", default="family", choices=["none", "strata", "instance", "family"])
    parser.add_argument("--target", default="rel", choices=["count", "rel"])
    parser.add_argument("--objective", default="squarederror", choices=["squarederror", "log", "poisson"])
    parser.add_argument("-f", "--features", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "best_features.txt"))
    parser.add_argument("--estimators", type=int, default=40)
    parser.add_argument("--depth", type=int, default=5)
    parser.add_argument("--all", action="store_true", default=False,
                        help="score every clause, not only those not used since the reduce before (what reduce picks from)")
    opts = parser.parse_args()

    feats = helper.get_features(opts.features)
    count_label = "x.%s_disc" % opts.table
    label = count_label + ("_rel" if opts.target == "rel" else "")

    train = load(opts.train, opts.table)
    test = load(opts.test, opts.table, "evaldata", ".dat")
    if train is None or test is None:
        print("no data")
        exit(1)
    train, X = prepare(train, feats)
    test, Xt = prepare(test, feats)
    y = train[label].astype(float)
    objective = "reg:squarederror"
    if opts.objective == "log":
        y = np.log1p(y)
    elif opts.objective == "poisson":
        objective = "count:poisson"
    clf = xgb.XGBRegressor(objective=objective, n_estimators=opts.estimators, max_depth=opts.depth,
                           min_child_weight=10, random_state=0)
    clf.fit(X, y, sample_weight=helper.sample_weights(train, opts.weights))
    print("trained on %d rows of %d instances, target %s, objective %s, weights %s" % (
        len(train), train["fname"].nunique(), label, opts.objective, opts.weights))

    res = helper.ranking_per_reduce(test, clf.predict(Xt), count_label, cands=not opts.all)
    if res is None:
        print("no reduce with a used clause in the test frames")
        exit(1)
    pd.set_option("display.width", 200)
    print(res.round(1).to_string(index=False))
    m = res.drop(columns=["instance", "reduces"]).mean()
    print("mean over instances: " + "  ".join("%s %.1f" % (k, v) for k, v in m.items()))
    print("model-glue @25: %.1f  @50: %.1f" % (m["model@25"] - m["glue@25"], m["model@50"] - m["glue@50"]))

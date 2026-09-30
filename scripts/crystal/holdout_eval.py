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
# ranking quality (share of the future use kept when keeping the best
# 25%/50%, model vs glue/size vs oracle) on the frames of others, per
# held-out instance. Minutes instead of the hours a solver A/B takes, so
# targets, objectives and feature lists can be compared first; the
# solver A/B is still the last word.
#
# usage: holdout_eval.py --train a-dir b-dir --test c-dir d-dir
#          [--target count|rel] [--objective squarederror|log|poisson]
#          [-f best_features.txt] [--tier short] [--table used_later]

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


def load(dirs, table, tier):
    dfs = []
    for d in dirs:
        fs = glob.glob(os.path.join(d, "data-min.db-cldata-%s-%s-cut1-*.dat" % (table, tier)))
        if not fs:
            print("no %s %s frame in %s, skipped" % (table, tier, d))
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
    parser.add_argument("--tier", default="short")
    parser.add_argument("--target", default="count", choices=["count", "rel"])
    parser.add_argument("--objective", default="squarederror", choices=["squarederror", "log", "poisson"])
    parser.add_argument("-f", "--features", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "best_features.txt"))
    parser.add_argument("--estimators", type=int, default=40)
    parser.add_argument("--depth", type=int, default=5)
    parser.add_argument("--cands", action="store_true", default=True,
                        help="score only clauses not used since the reduce before (what reduce picks from)")
    opts = parser.parse_args()

    feats = helper.get_features(opts.features)
    count_label = "x.%s_%s" % (opts.table, opts.tier)
    label = count_label + ("_rel" if opts.target == "rel" else "")

    train = load(opts.train, opts.table, opts.tier)
    test = load(opts.test, opts.table, opts.tier)
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
    clf.fit(X, y)
    print("trained on %d rows of %d instances, target %s, objective %s" % (
        len(train), train["fname"].nunique(), label, opts.objective))

    pred = clf.predict(Xt)
    test["pred"] = pred
    rows = []
    for name, g in test.groupby("fname"):
        if opts.cands:
            g = g[g["rdb0.used"] < 30]
        truth = g[count_label].to_numpy(dtype=float)
        if len(truth) < 50 or truth.sum() <= 0:
            continue
        orders = {
            "model": np.argsort(-g["pred"].to_numpy(), kind="stable"),
            "glue": np.lexsort((g["rdb0.size"].to_numpy(), g["rdb0.glue"].fillna(1e9).to_numpy())),
            "oracle": np.argsort(-truth, kind="stable"),
        }
        r = {"instance": name[:34], "rows": len(g)}
        for frac in [0.25, 0.5]:
            for k, o in orders.items():
                r["%s@%d" % (k, 100*frac)] = kept(truth, o, frac)
        rows.append(r)
    res = pd.DataFrame(rows)
    pd.set_option("display.width", 200)
    print(res.round(1).to_string(index=False))
    m = res.drop(columns=["instance", "rows"]).mean()
    print("mean over instances: " + "  ".join("%s %.1f" % (k, v) for k, v in m.items()))
    print("model-glue @25: %.1f  @50: %.1f" % (m["model@25"] - m["glue@25"], m["model@50"] - m["glue@50"]))

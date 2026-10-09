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

# Offline hold-out: train on the frames of some instance dirs (or take
# a saved model) and replay the solver's reduce on the FAIR frames
# (evaldata) of others: the share of the future use the normal build
# keeps and what the model would keep in its place
# (helper.policy_per_reduce). Minutes instead of the hours a solver A/B
# takes, so targets, objectives and feature lists can be compared first;
# the solver A/B is still the last word.
#
# usage: holdout_eval.py (--train a-dir b-dir | --model predictor.json) --test c-dir d-dir
#          [--objective rank|squarederror|log|poisson]
#          [--target count|rel] [--weights none|strata|instance|family]   (not with rank)
#          [-f best_features.txt] [--table used_later]
#          [--rounds 1] [--remove 75] [--tier1 2]   the reduce that is replayed

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
        df["fname"] = os.path.realpath(d)
        dfs.append(df)
    return pd.concat(dfs, ignore_index=True) if dfs else None


def prepare(df, feats):
    df = df.convert_dtypes(convert_integer=False, convert_string=False, convert_floating=False)
    helper.make_missing_into_nan(df)
    helper.add_features_from_list(df, feats)
    df.replace([np.inf, -np.inf], MISSING, inplace=True)
    X = df[feats].astype(np.float32).replace([np.inf, -np.inf], MISSING)
    return df, X


def predict(model, X):
    booster = xgb.Booster()
    booster.load_model(model)
    return booster.predict(xgb.DMatrix(X, missing=MISSING))


def train(opts, feats, label, count_label):
    df = load(opts.train, opts.table)
    if df is None:
        print("no training data")
        exit(1)
    df, X = prepare(df, feats)
    params = dict(n_estimators=opts.estimators, max_depth=opts.depth, min_child_weight=10, random_state=0)
    if opts.objective == "rank":
        clf = helper.fit(xgb.XGBRanker(objective="rank:ndcg", **params), df, X, df[count_label])
    else:
        y = df[label].astype(float)
        if opts.objective == "log":
            y = np.log1p(y)
        objective = "count:poisson" if opts.objective == "poisson" else "reg:squarederror"
        clf = helper.fit(xgb.XGBRegressor(objective=objective, **params), df, X, y,
                         helper.sample_weights(df, opts.weights))
    print("trained on %d rows of %d instances, target %s, objective %s, weights %s" % (
        len(df), df["fname"].nunique(), label, opts.objective, opts.weights))
    return clf


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--train", nargs="+")
    parser.add_argument("--model", help="a saved model, instead of --train")
    parser.add_argument("--test", nargs="+", required=True)
    parser.add_argument("--table", default="used_later")
    parser.add_argument("--weights", default="family", choices=["none", "strata", "instance", "family"])
    parser.add_argument("--target", default="rel", choices=["count", "rel"])
    parser.add_argument("--objective", default="rank", choices=["rank", "squarederror", "log", "poisson"])
    parser.add_argument("-f", "--features", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "best_features.txt"))
    parser.add_argument("--estimators", type=int, default=40)
    parser.add_argument("--depth", type=int, default=5)
    parser.add_argument("--rounds", type=int, default=helper.ROUNDS, help="the replayed reduce: --reducerounds")
    parser.add_argument("--tier1", type=int, default=helper.TIER1_GLUE, help="the replayed reduce: --reducetier1glue")
    parser.add_argument("--remove", type=float, default=100 * helper.REMOVE, help="the replayed reduce: --reducetarget")
    opts = parser.parse_args()
    if (opts.train is None) == (opts.model is None):
        print("ERROR: give --train or --model")
        exit(1)

    feats = helper.get_features(opts.features)
    count_label = "x.%s_disc" % opts.table
    label = count_label + ("_rel" if opts.target == "rel" else "")

    test = load(opts.test, opts.table, "evaldata", ".dat")
    if test is None:
        print("no test data")
        exit(1)
    test, Xt = prepare(test, feats)
    if opts.model is not None:
        pred = predict(opts.model, Xt)
    else:
        pred = train(opts, feats, label, count_label).predict(Xt)

    res = helper.policy_per_reduce(test, pred, count_label, rounds=opts.rounds, remove=opts.remove / 100.0, tier1=opts.tier1)
    if res is None:
        print("no reduce with a used clause in the test frames")
        exit(1)
    pd.set_option("display.width", 200)
    print(res.round(1).to_string(index=False))
    m = res.drop(columns=["instance", "reduces"]).mean()
    print("mean over instances: " + "  ".join("%s %.1f" % (k, v) for k, v in m.items()))
    print("model minus normal: %+.2f (better on %d of %d), oracle minus normal: %+.2f" % (
        m["order"] - m["normal"], (res["order"] > res["normal"]).sum(), len(res), m["oracle"] - m["normal"]))
    print("clauses kept for the use the solver's own reduce holds, candidates by glue: %.1f%%, by the model: %.1f%%, by the oracle: %.1f%%" % (
        m["glue needs"], m["order needs"], m["oracle needs"]))
    sp = helper.best_split(test, pred, count_label, rounds=opts.rounds, remove=opts.remove / 100.0, tier1=opts.tier1)
    if sp is not None:
        print("the same number of candidates kept per run, split over its reduces with hindsight: %.1f%% of the use, the same share at every reduce: %.1f%%" % (sp[1], sp[0]))
    q = helper.candidate_quarters(test, pred, count_label, rounds=opts.rounds, tier1=opts.tier1)
    if q is not None:
        print("the candidates in quarters, best first: % used later, % of the candidates' use")
        print(q.round(1).to_string())

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

# Tests on the pandas frames cldata_gen_pandas.py made, run before any
# learning: dirty data has silently made bad models before (locked
# clauses, a glue sentinel, labels off by one reduce). Exit code 1 on any
# failed test; warnings for what is suspicious but not surely wrong.
#
# usage: check_frames.py [-f best_features.txt] frame.dat [frame.dat ...]
#   the frames of the three tiers of one table are cross-checked when
#   given together (short <= long <= forever for the same clause and reduce)

import argparse
import os
import re
import sys

import numpy as np
import pandas as pd

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import helper

CL_MAX_USED = 31
failed = []
warned = []


def fail(msg):
    failed.append(msg)
    print("FAIL: " + msg)


def warn(msg):
    warned.append(msg)
    print("warn: " + msg)


def check(cond, msg):
    if not cond:
        fail(msg)


def tier_of(fname):
    m = re.search(r"cldata-(used_later(?:_anc)?)-(short|long|forever|disc)-", fname)
    return (m.group(1), m.group(2)) if m else (None, None)


def check_frame(fname, df, feats):
    name = os.path.basename(fname)
    table, tier = tier_of(fname)
    check(table is not None, "%s: cannot tell table/tier from the name" % name)
    label = "x.%s_%s" % (table, tier)
    print("== %s: %d rows, %d columns" % (name, df.shape[0], df.shape[1]))
    check(df.shape[0] > 100, "%s: only %d rows" % (name, df.shape[0]))

    need = [label, "fname", "rdb0.glue", "rdb0.size", "rdb0.used", "rdb0.dump_no",
            "rdb0_common.conflicts", "rdb0.introduced_at_conflict",
            "sum_cl_use.clauseID", "cl.orig_glue", "rdb0.is_ternary_resolvent"]
    missing = [c for c in need if c not in df.columns]
    check(not missing, "%s: columns missing: %s" % (name, missing))
    if missing:
        return
    # the other tiers' labels would be a leak in a feature list, and the
    # rows differ per tier, so they must not be here
    others = [c for c in df.columns if c.startswith("x.used_later") and not c.startswith(label)]
    check(not others, "%s: other tiers' labels in the frame: %s" % (name, others))

    num = df.select_dtypes(include=[np.number])
    inf_cols = [c for c in num.columns if np.isinf(num[c].to_numpy(dtype=float)).any()]
    check(not inf_cols, "%s: inf in %s" % (name, inf_cols))

    y = df[label]
    check(y.notna().all(), "%s: NaN labels: %d" % (name, y.isna().sum()))
    check((y >= 0).all(), "%s: negative labels" % name)
    if "_anc" not in table and tier != "disc":
        check((y == y.round()).all(), "%s: non-integer use counts" % name)
    check(y.sum() > 0, "%s: all labels zero" % name)
    frac_pos = (y > 0).mean()
    if frac_pos < 0.02 or frac_pos > 0.98:
        warn("%s: %.1f%% of the labels are nonzero" % (name, 100*frac_pos))

    # the clause's own state
    g = df["rdb0.glue"]
    check((g.dropna() >= 1).all(), "%s: glue < 1" % name)
    check((df["rdb0.size"] >= 3).all(), "%s: long clauses of size < 3" % name)
    tern = df["rdb0.is_ternary_resolvent"] == 1
    # eager subsume marks 'delete next' with the glue sentinel, dumped as NULL
    nog = g.isna() & ~tern
    if nog.mean() > 0.3:
        warn("%s: %.0f%% of the non-ternary rows have no glue (eagerly subsumed)" % (name, 100*nog.mean()))
    bad = (g > df["rdb0.size"]).sum()
    if bad:
        warn("%s: glue > size on %d rows (glue is not updated when a clause shrinks)" % (name, bad))
    check(df["rdb0.used"].between(0, CL_MAX_USED).all(), "%s: rdb0.used outside 0..%d" % (name, CL_MAX_USED))
    check((df["cl.time_inside_solver"] >= 0).all(), "%s: negative time_inside_solver" % name)
    check((df["rdb0.introduced_at_conflict"] <= df["rdb0_common.conflicts"]).all(),
          "%s: introduced after the reduce it is dumped at" % name)
    # ternary resolvents have no learning-time data, everything else has
    check(df.loc[tern, "cl.orig_glue"].isna().all(), "%s: ternary resolvents with clause_stats" % name)
    check(df.loc[~tern, "cl.orig_glue"].notna().all(), "%s: learnt clauses without clause_stats" % name)
    check((df.loc[~tern, "cl.orig_size"] >= df.loc[~tern, "rdb0.size"]).all(),
          "%s: clause larger than when learnt" % name)
    # orig_glue is measured at learning, the trail changes afterwards
    up = (df.loc[~tern & g.notna(), "cl.orig_glue"] < g[~tern & g.notna()]).sum()
    if up:
        warn("%s: glue above orig_glue on %d rows" % (name, up))

    # one reduce = one set of rdb0_common values (rdb0.dump_no is per
    # clause, the reduce is identified by its conflict count)
    common = [c for c in df.columns if c.startswith("rdb0_common.")]
    nun = df.groupby(["fname", "rdb0_common.conflicts"])[common].nunique()
    varying = [c for c in common if (nun[c] > 1).any()]
    check(not varying, "%s: rdb0_common columns vary within a reduce: %s" % (name, varying))

    # features that carry nothing
    const = [c for c in num.columns if num[c].nunique(dropna=True) <= 1 and not c.startswith("x.")]
    if const:
        warn("%s: constant columns: %s" % (name, const))
    tiny = [c for c in num.columns if "per_time" in c and num[c].abs().max() < 1e-6]
    check(not tiny, "%s: rate columns that are always ~0: %s" % (name, tiny))

    # the labels must line up with the state: a clause used since the last
    # reduce (used == CL_MAX_USED) is used more in the future than one never
    # used. Off-by-one-reduce bugs break this
    recent = y[df["rdb0.used"] == CL_MAX_USED].mean()
    never = y[df["rdb0.used"] == 0].mean()
    msg = "%s: mean use of recently used clauses (%.2f) not above never used (%.2f)" % (name, recent, never)
    if "_anc" in table:
        # descendants' uses do not need the clause itself to be used
        if not recent > never:
            warn(msg)
    else:
        check(recent > never, msg)
    if (df["rdb0.used"] == 0).sum() < 100 or (df["rdb0.used"] == CL_MAX_USED).sum() < 100:
        warn("%s: few rows to check used-vs-label on" % name)

    # no future in the features, and every column they need is there
    # (after the few computed columns helper.py always adds)
    if feats:
        leak = [f for f in feats if "sum_cl_use" in f or "x." in f or "used_later" in f]
        check(not leak, "%s: features that look into the future: %s" % (name, leak))
        small = df.head(200).copy()
        helper.cldata_add_minimum_computed_features(small, False)
        for f in feats:
            for col in re.findall(r"[A-Za-z_][A-Za-z0-9_.]*", f):
                if "." in col and col not in small.columns:
                    fail("%s: feature column %s not in the frame" % (name, col))
    check(df["fname"].nunique() >= 1, "%s: no fname" % name)
    dup = df.duplicated(subset=["fname", "sum_cl_use.clauseID", "rdb0_common.conflicts"]).sum()
    check(dup == 0, "%s: %d duplicate (clause, reduce) rows" % (name, dup))
    return df


def cross_check(frames):
    """short <= long <= forever for the same clause at the same reduce"""
    by = {}
    for fname, df in frames.items():
        table, tier = tier_of(fname)
        by.setdefault(table, {})[tier] = df
    for table, tiers in by.items():
        seq = [t for t in ["short", "long", "forever"] if t in tiers]
        for a, b in zip(seq, seq[1:]):
            key = ["fname", "sum_cl_use.clauseID", "rdb0_common.conflicts"]
            la, lb = "x.%s_%s" % (table, a), "x.%s_%s" % (table, b)
            m = tiers[a][key + [la]].merge(tiers[b][key + [lb]], on=key)
            if len(m) < 50:
                warn("%s: only %d rows shared between %s and %s" % (table, len(m), a, b))
                continue
            bad = (m[la] > m[lb]).sum()
            check(bad == 0, "%s: %d rows where the %s use exceeds the %s use" % (table, bad, a, b))
            print("   %s: %s <= %s holds on %d shared rows" % (table, a, b, len(m)))
        # the discounted use weighs every use by at most 1 over two
        # half-lives (60k), so it is at most the 120k count, and a clause
        # used in the next 10k has a discounted use of at least 0.79 per use
        if "disc" in tiers and "forever" in tiers:
            key = ["fname", "sum_cl_use.clauseID", "rdb0_common.conflicts"]
            ld, lf = "x.%s_disc" % table, "x.%s_forever" % table
            m = tiers["disc"][key + [ld]].merge(tiers["forever"][key + [lf]], on=key)
            if len(m) >= 50:
                bad = (m[ld] > m[lf] + 1e-6).sum()
                check(bad == 0, "%s: %d rows where the discounted use exceeds the forever count" % (table, bad))
                print("   %s: disc <= forever holds on %d shared rows" % (table, len(m)))
        if "disc" in tiers and "short" in tiers:
            key = ["fname", "sum_cl_use.clauseID", "rdb0_common.conflicts"]
            ld, ls = "x.%s_disc" % table, "x.%s_short" % table
            m = tiers["disc"][key + [ld]].merge(tiers["short"][key + [ls]], on=key)
            if len(m) >= 50:
                bad = (m[ld] < 0.79 * m[ls] - 1e-6).sum()
                check(bad == 0, "%s: %d rows where the discounted use is below 0.79 x the short count" % (table, bad))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("frames", nargs="+")
    parser.add_argument("-f", "--features", default=None, help="best_features file to check against the frames")
    opts = parser.parse_args()
    feats = []
    if opts.features:
        with open(opts.features) as f:
            feats = [l.strip() for l in f if l.strip() and not l.startswith("#")]
    frames = {}
    for fname in opts.frames:
        df = pd.read_pickle(fname)
        frames[fname] = check_frame(fname, df, feats)
    cross_check({k: v for k, v in frames.items() if v is not None})
    print("%d failed, %d warnings" % (len(failed), len(warned)))
    sys.exit(1 if failed else 0)

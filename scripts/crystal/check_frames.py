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


def table_of(fname):
    m = re.search(r"(?:cl|eval)data-(used_later(?:_anc)?)-disc[-.]", fname)
    return m.group(1) if m else None


def check_frame(fname, df, feats):
    name = os.path.basename(fname)
    table = table_of(fname)
    check(table is not None, "%s: cannot tell the table from the name" % name)
    label = "x.%s_disc" % table
    print("== %s: %d rows, %d columns" % (name, df.shape[0], df.shape[1]))
    check(df.shape[0] > 100, "%s: only %d rows" % (name, df.shape[0]))
    fair = "evaldata-" in name

    need = [label, label + "_rel", "x.weight", "fname", "rdb0.glue", "rdb0.size", "rdb0.used", "rdb0.dump_no",
            "rdb0_common.conflicts", "rdb0.introduced_at_conflict",
            "sum_cl_use.clauseID", "cl.orig_glue", "rdb0.is_ternary_resolvent", "rdb0.gone"]
    missing = [c for c in need if c not in df.columns]
    check(not missing, "%s: columns missing: %s" % (name, missing))
    if missing:
        return
    # the other table's label would be a leak in a feature list
    others = [c for c in df.columns if c.startswith("x.used_later") and not c.startswith(label)]
    check(not others, "%s: another label in the frame: %s" % (name, others))

    num = df.select_dtypes(include=[np.number])
    inf_cols = [c for c in num.columns if np.isinf(num[c].to_numpy(dtype=float)).any()]
    check(not inf_cols, "%s: inf in %s" % (name, inf_cols))

    # the fair frame is what the solver has at the reduce
    gone = df["rdb0.gone"]
    check(gone.isin([0, 1]).all(), "%s: rdb0.gone not 0/1" % name)
    check(not fair or (gone == 0).all(), "%s: gone clauses in the fair frame" % name)
    check((gone[df["rdb0.dump_no"] == 0] == 0).all(), "%s: clauses gone before their first reduce" % name)

    y = df[label]
    check(y.notna().all(), "%s: NaN labels: %d" % (name, y.isna().sum()))
    check((y >= 0).all(), "%s: negative labels" % name)
    # the fair frame is a few reduces: they can all be before the first use
    if fair and y.sum() == 0:
        warn("%s: all labels zero" % name)
        return df
    check(y.sum() > 0, "%s: all labels zero" % name)
    # what a row stands for, and the rank among the clauses of the reduce
    w = df["x.weight"]
    check(w.notna().all() and (w >= 1).all() and np.isfinite(w).all(), "%s: x.weight below 1 or missing" % name)
    rel = df[label + "_rel"]
    check(rel.between(0, 1).all(), "%s: rank label outside 0..1" % name)
    check((rel[y == 0] == 0).all(), "%s: never-used rows with a rank above 0" % name)
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

    # the cost and where-the-search-is columns (solver commit 0616917e9)
    if "rdb0.visited" in df.columns:
        v = df["rdb0.visited"]
        # every propagation or conflict the clause caused was a visit first
        check((v >= df["rdb0.props_made"] + df["rdb0.conflicts_made"]).all(),
              "%s: visited below props_made + conflicts_made" % name)
        check((df["rdb0.sum_visited"] >= v).all(), "%s: sum_visited below this interval's visited" % name)
        check((df["rdb0.discounted_visited"] >= 0).all(), "%s: negative discounted_visited" % name)
        if v.sum() == 0:
            warn("%s: no clause was ever visited" % name)
        for c in ["rdb0.lit_act_rel", "rdb0.lit_vmtf_rel"]:
            check(df[c].between(0, 1).all(), "%s: %s outside 0..1" % (name, c))
        if df["rdb0.lit_act_rel"].max() == 0 and df["rdb0.lit_vmtf_rel"].max() == 0:
            fail("%s: no variable activity of any kind" % name)
        check((df["rdb0.num_assigned"] <= df["rdb0.size"]).all(), "%s: more literals assigned than the clause has" % name)
        check((df["rdb0.num_false_lev0"] <= df["rdb0.num_assigned"]).all(),
              "%s: more literals false at level 0 than assigned" % name)
    else:
        warn("%s: no rdb0.visited: gathered before the cost columns existed" % name)

    # one reduce = one set of rdb0_common values (rdb0.dump_no is per
    # clause, the reduce is identified by its conflict count)
    common = [c for c in df.columns if c.startswith("rdb0_common.")]
    nun = df.groupby(["fname", "rdb0_common.conflicts"])[common].nunique()
    varying = [c for c in common if (nun[c] > 1).any()]
    check(not varying, "%s: rdb0_common columns vary within a reduce: %s" % (name, varying))

    # features that carry nothing
    const = [c for c in num.columns if num[c].nunique(dropna=True) <= 1 and not c.startswith("x.") and c != "rdb0.gone"]
    if const:
        warn("%s: constant columns: %s" % (name, const))
    tiny = [c for c in num.columns if "per_time" in c and num[c].abs().max() < 1e-6]
    # the median clause may never propagate (f6bidw), the average one does
    med = [c for c in tiny if "median" in c]
    if med:
        warn("%s: median rates that are always 0: %s" % (name, med))
    tiny = [c for c in tiny if c not in med]
    check(not tiny, "%s: rate columns that are always ~0: %s" % (name, tiny))

    # the labels must line up with the state: a clause used since the last
    # reduce (used == CL_MAX_USED) is used more in the future than one never
    # used. Off-by-one-reduce bugs break this. Not on the fair frame: a
    # never used clause that is still there is one the reduce picked
    recent = y[df["rdb0.used"] == CL_MAX_USED].mean()
    never = y[df["rdb0.used"] == 0].mean()
    msg = "%s: mean use of recently used clauses (%.2f) not above never used (%.2f)" % (name, recent, never)
    if "_anc" in table or "evaldata" in name:
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
    print("%d failed, %d warnings" % (len(failed), len(warned)))
    sys.exit(1 if failed else 0)

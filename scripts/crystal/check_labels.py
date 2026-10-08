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

# The labels of the frames, computed a second time: for sampled rows the
# discounted future use is summed here from used_clauses[_anc] with code
# that shares nothing with the SQL of helper.fill_used_later, and must
# equal the frame's label. Needs the full data.db (KEEP_PROOF=1).
#
# usage: check_labels.py data.db frame.dat [frame.dat ...] [--halflife 4]

import argparse
import bisect
import os
import pickle
import sqlite3
import sys

import numpy as np


def label_of(uses, confls, at, halflife):
    """uses: (used_at, weight) of one clause. The reduce at 'at' is time i,
    a use between two reduces lies linearly in between; a use counts for
    two half-lives, halved every 'halflife' reduces."""
    i = confls.index(at)
    if i + 2 * halflife >= len(confls):
        return None
    end = confls[i + 2 * halflife]
    tot = 0.0
    for used_at, weight in uses:
        if used_at <= at or used_at > end:
            continue
        j = bisect.bisect_right(confls, used_at) - 1
        j = min(max(j, 0), len(confls) - 2)
        when = j + (used_at - confls[j]) / float(confls[j + 1] - confls[j])
        tot += weight * 0.5 ** ((when - i) / float(halflife))
    return tot


def check_frame(conn, fname, halflife, num, seed):
    base = os.path.basename(fname)
    table = "used_later_anc" if "used_later_anc" in base else "used_later"
    uses_table = "used_clauses_anc" if table == "used_later_anc" else "used_clauses"
    with open(fname, "rb") as f:
        df = pickle.load(f)
    lab = "x.%s_disc" % table
    confls = [r[0] for r in conn.execute("select distinct conflicts from reduceDB_common order by conflicts")]
    rows = df.sample(n=min(num, len(df)), random_state=seed)
    ids = sorted(set(int(x) for x in rows["sum_cl_use.clauseID"]))
    uses = {}
    for k in range(0, len(ids), 500):
        part = ids[k:k + 500]
        q = "select clauseID, used_at, weight from %s where clauseID in (%s)" % (uses_table, ",".join("?" * len(part)))
        for cid, used_at, weight in conn.execute(q, part):
            uses.setdefault(cid, []).append((used_at, weight))
    bad = 0
    nonzero = 0
    for cid, at, got in zip(rows["sum_cl_use.clauseID"], rows["rdb0_common.conflicts"], rows[lab]):
        want = label_of(uses.get(int(cid), []), confls, int(at), halflife)
        if want is None or not np.isclose(want, got, rtol=1e-5, atol=1e-7):
            bad += 1
            if bad <= 5:
                print("FAIL: %s: clause %d at %d: frame %s, recomputed %s" % (base, cid, at, got, want))
        nonzero += got > 0
    # the rank label follows the label within a reduce
    rel = "x.%s_disc_rel" % table
    disorder = 0
    for _, g in df.groupby("rdb0_common.conflicts"):
        g = g.sort_values(lab)
        disorder += int((np.diff(g[rel].to_numpy()) < -1e-9).sum())
    if disorder:
        print("FAIL: %s: %d rows whose rank label goes against the label" % (base, disorder))
    print("%s: %d rows recomputed (%d used), %d differ" % (base, len(rows), nonzero, bad))
    return bad + disorder, nonzero


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("db")
    parser.add_argument("frames", nargs="+")
    parser.add_argument("--halflife", type=int, default=4)
    parser.add_argument("--num", type=int, default=3000, help="rows recomputed per frame")
    parser.add_argument("--seed", type=int, default=0)
    opts = parser.parse_args()

    conn = sqlite3.connect(opts.db)
    bad = 0
    used = 0
    for fname in opts.frames:
        b, u = check_frame(conn, fname, opts.halflife, opts.num, opts.seed)
        bad += b
        used += u
    if used == 0:
        print("FAIL: no used row among the sampled ones, nothing was checked")
        bad += 1
    print("%d failed" % bad)
    sys.exit(1 if bad else 0)

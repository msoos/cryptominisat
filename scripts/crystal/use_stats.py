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

# When are the tracked clauses used in the proof? How far ahead of a
# reduce its clauses' uses lie (what a label horizon sees), and where
# in the run the uses are.
# Needs the labelled data.db (after clean_update_data.py).
#
# usage: use_stats.py data.db [--halflife 4]

import argparse
import sqlite3

import numpy as np


def pct(a, b):
    return 100.0 * a / b if b else 0.0


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("db")
    parser.add_argument("--halflife", type=int, default=4)
    opts = parser.parse_args()

    conn = sqlite3.connect(opts.db)
    confls = np.array([r[0] for r in conn.execute(
        "select distinct conflicts from reduceDB_common order by conflicts")], dtype=float)
    if len(confls) < 3:
        print("fewer than 3 reduces, no statistics")
        exit(0)
    # the k-th reduce is at k, linear in the conflicts in between
    rt = lambda x: np.interp(x, confls, np.arange(len(confls)))

    learnt = dict(conn.execute("select clauseID, min(introduced_at_conflict) from reduceDB group by clauseID"))
    uses = np.array(conn.execute("select clauseID, used_at, weight from used_clauses").fetchall(), dtype=float).reshape(-1, 3)
    n_cl = len(learnt)
    used_ids = set(int(x) for x in uses[:, 0])
    print("tracked clauses: %d, used in the proof: %d (%.1f%%), uses: %d" % (
        n_cl, len(used_ids & set(learnt)), pct(len(used_ids & set(learnt)), n_cl), len(uses)))
    if len(uses) == 0:
        exit(0)
    ok = np.array([int(x) in learnt for x in uses[:, 0]])
    uses = uses[ok]
    at = rt(uses[:, 1])
    born = rt(np.array([learnt[int(x)] for x in uses[:, 0]], dtype=float))

    # (reduce, later use of a clause the solver still had there) pairs by
    # distance in reduces: the reduces r with born <= r < at, the clause
    # not gone at r, see this use from at-r away
    alive = dict(conn.execute("select clauseID, max(conflicts) from reduceDB where gone = 0 group by clauseID"))
    last_r = np.floor(rt(np.array([alive.get(int(x), -1) for x in uses[:, 0]], dtype=float)))
    last_r[np.array([int(x) not in alive for x in uses[:, 0]])] = -1
    first_r = np.ceil(born)

    def pairs(lo, hi):
        # r in (at-hi, at-lo], first_r <= r <= last_r, r < at
        up = np.minimum(np.ceil(at - lo) - 1, last_r)
        down = np.maximum(np.floor(at - hi) + 1, first_r)
        return np.maximum(up - down + 1, 0).sum()

    tot = pairs(0, 1e9)
    edges = [0, 1, 2, 4, 8, 16, 1e9]
    print("the uses ahead of a reduce, by how many reduces ahead: " + ", ".join(
        "%s %.1f%%" % (("%d-%d" % (lo, hi)) if hi < 1e9 else "over %d" % lo, pct(pairs(lo, hi), tot))
        for lo, hi in zip(edges[:-1], edges[1:])))
    hor = 2 * opts.halflife
    print("within the label's horizon of %d reduces: %.1f%%" % (hor, pct(pairs(0, hor), tot)))

    run = at / (len(confls) - 1)
    print("uses by tenth of the run: " + " ".join("%.0f%%" % pct(((run >= k / 10.0) & (run < (k + 1) / 10.0 + (k == 9))).sum(), len(run))
                                                   for k in range(10)))

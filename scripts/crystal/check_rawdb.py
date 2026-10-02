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

# Pre-flight check of what the STATS build dumped, before the proof is
# processed (minutes to an hour on big instances): is the DB what the
# rest of the pipeline expects? Exit 1 on a failed test.
#
# usage: check_rawdb.py data.db-raw [--horizon 120000] [--dumpratio 0.1] [--proof data.frat]

import argparse
import os
import sqlite3
import sys

failed = []


def check(cond, msg):
    if not cond:
        failed.append(msg)
        print("FAIL: " + msg)


def warn(msg):
    print("warn: " + msg)


def one(c, q):
    return c.execute(q).fetchone()[0]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("db")
    parser.add_argument("--horizon", type=int, default=120000, help="the longest label horizon")
    parser.add_argument("--dumpratio", type=float, default=None, help="--cldatadumpratio of the run")
    parser.add_argument("--proof", default=None, help="the FRAT file the run wrote")
    opts = parser.parse_args()

    conn = sqlite3.connect(opts.db)
    c = conn.cursor()
    tables = set(r[0] for r in c.execute("select name from sqlite_master where type='table'"))
    for t in ["reduceDB", "reduceDB_common", "clause_stats", "cl_last_in_solver", "tags"]:
        check(t in tables, "table %s missing" % t)
    if failed:
        sys.exit(1)

    n_rdb = one(c, "select count(*) from reduceDB")
    n_cl = one(c, "select count(*) from clause_stats")
    n_red = one(c, "select count(*) from reduceDB_common")
    confl = one(c, "select max(conflicts) from reduceDB_common") or 0
    print("reduces %d, reduceDB rows %d, tracked learnt clauses %d, last reduce at %d conflicts" % (
        n_red, n_rdb, n_cl, confl))
    check(n_red >= 3, "fewer than 3 reduces: nothing to learn from")
    check(n_rdb > 1000, "only %d reduceDB rows" % n_rdb)
    check(n_cl > 100, "only %d tracked learnt clauses" % n_cl)
    if confl < opts.horizon:
        warn("the run ended at %d conflicts, the %d horizon will have no rows" % (confl, opts.horizon))

    # one row per (clause, reduce)
    dup = one(c, "select count(*) from (select clauseID, conflicts, count(*) n from reduceDB group by 1,2 having n > 1)")
    check(dup == 0, "%d (clause, reduce) pairs dumped more than once" % dup)
    # dump_no counts the clause's dumps: 0, 1, 2 ... each present
    for d in [0, 1, 2]:
        n = one(c, "select count(*) from reduceDB where dump_no = %d" % d)
        check(n > 0, "no row with dump_no %d (a double reset?)" % d)
    # a tracked clause's rows do not come before it was learnt
    bad = one(c, "select count(*) from reduceDB where conflicts < introduced_at_conflict")
    check(bad == 0, "%d rows before the clause was learnt" % bad)
    # the per-reduce aggregates are one row per reduce
    dupc = one(c, "select count(*) from (select conflicts, count(*) n from reduceDB_common group by 1 having n > 1)")
    check(dupc == 0, "%d reduces dumped twice in reduceDB_common" % dupc)
    # every reduceDB row has its reduceDB_common row
    orphan = one(c, "select count(*) from reduceDB r where not exists (select 1 from reduceDB_common m where m.conflicts = r.conflicts)")
    check(orphan == 0, "%d reduceDB rows without a reduceDB_common row" % orphan)
    # the cost columns, if this solver has them
    cols = [r[1] for r in c.execute("pragma table_info(reduceDB)")]
    if "visited" in cols:
        bad = one(c, "select count(*) from reduceDB where visited < props_made + conflicts_made")
        check(bad == 0, "%d rows with visited < props_made + conflicts_made" % bad)
        bad = one(c, "select count(*) from reduceDB where lit_act_rel < 0 or lit_act_rel > 1 or lit_vmtf_rel < 0 or lit_vmtf_rel > 1")
        check(bad == 0, "%d rows with an activity share outside 0..1" % bad)
    # the tracked share is roughly the dump ratio (locked clauses live
    # longer, so the per-reduce share drifts up: 3x is the alarm)
    if opts.dumpratio:
        all_learnt = one(c, "select avg(tot_cls_in_db) from reduceDB_common") or 0
        per_reduce = n_rdb / max(n_red, 1)
        if all_learnt > 0:
            share = per_reduce / all_learnt
            if share > 3 * opts.dumpratio or share < opts.dumpratio / 3:
                warn("tracked share per reduce %.3f vs dump ratio %.3f" % (share, opts.dumpratio))
    if opts.proof:
        check(os.path.exists(opts.proof) and os.path.getsize(opts.proof) > 0, "proof %s missing or empty" % opts.proof)
    print("%d failed" % len(failed))
    sys.exit(1 if failed else 0)

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

# Tests fix_up_xlrup.py: a hand-made proof with the expected uses written
# out, then random proofs (text and binary) on which xlrup_uses.cpp and
# the Python reference pass (--python) must fill the same tables.
#
# usage: test_xlrup_uses.py [--rounds N] [--real XLRUP RAWDB]
#   --real: also compare the two passes on a proof the solver wrote

import argparse
import os
import random
import shutil
import sqlite3
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
FIX = os.path.join(HERE, "fix_up_xlrup.py")


def leb(u):
    out = bytearray()
    while True:
        b = u & 0x7f
        u >>= 7
        if u:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)


def ivec(xs):
    return b"".join(leb(2 * abs(x) + (1 if x < 0 else 0)) for x in xs) + b"\0"


def idvec(xs):
    return b"".join(leb(2 * x) for x in xs) + b"\0"


def write_xlrup(fname, steps, binary):
    """steps: (kind, id, lits, hints or None); XLRUP has no 'o' or 'f' steps"""
    with open(fname, "wb") as f:
        for kind, cid, lits, hints in steps:
            if kind == "a":
                if binary:
                    f.write(b"a" + leb(2 * cid) + ivec(lits) + idvec(hints))
                else:
                    f.write(("%d %s0 %s0\n" % (cid, "".join("%d " % x for x in lits),
                                               "".join("%d " % x for x in hints))).encode())
            elif kind == "d":
                f.write(b"d" + idvec([cid]) if binary else ("%d d %d 0\n" % (cid, cid)).encode())


def write_db(fname, confl, updates):
    if os.path.exists(fname):
        os.unlink(fname)
    conn = sqlite3.connect(fname)
    conn.execute("create table set_id_confl (id bigint, conflicts bigint)")
    conn.execute("create table update_id (old_id bigint, new_id bigint)")
    conn.executemany("insert into set_id_confl values (?, ?)", confl)
    conn.executemany("insert into update_id values (?, ?)", updates)
    conn.commit()
    conn.close()


def run(proof, db, python):
    """returns (exit code, used_clauses rows, used_clauses_anc rows)"""
    cmd = [FIX, proof, db] + (["--python"] if python else [])
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        return p.returncode, None, None
    conn = sqlite3.connect(db)
    ret = [conn.execute("select clauseID, used_at, weight from %s order by rowid" % t).fetchall()
           for t in ("used_clauses", "used_clauses_anc")]
    conn.close()
    return 0, ret[0], ret[1]


def both(d, steps, confl, updates, expect=None, expect_fail=False):
    """every encoding and pass must agree (and match expect, if given)"""
    res = []
    for binary in (False, True):
        proof = os.path.join(d, "p.xlrup")
        write_xlrup(proof, steps, binary)
        for python in (False, True):
            db = os.path.join(d, "t.db")
            write_db(db, confl, updates)
            res.append(run(proof, db, python))
    if expect_fail:
        assert all(r[0] != 0 for r in res), "expected a failure, got %s" % [r[0] for r in res]
        return 0
    assert all(r[0] == 0 for r in res), "a run failed: %s" % [r[0] for r in res]
    for r in res[1:]:
        assert r == res[0], "the passes disagree:\n%s\n%s" % (res[0], r)
    if expect is not None:
        assert res[0][1] == expect[0], "used_clauses: %s" % res[0][1]
        assert res[0][2] == expect[1], "used_clauses_anc: %s" % res[0][2]
    return len(res[0][2])


def test_hand(d):
    # 1, 2 original; 3 tracked; 7 is not part of the proof; 6 is empty
    steps = [
        ("o", 1, [1, 2], None),
        ("o", 2, [-1, 2], None),
        ("a", 3, [2, 5], [1, 2]),
        ("a", 4, [2], [3, 1]),
        ("a", 5, [3], [4, 2]),
        ("a", 7, [4], [3]),
        ("d", 4, [2], None),
        ("a", 6, [], [5, 3]),
        ("f", 6, [], None),
    ]
    confl = [(3, 10), (4, 20), (5, 30), (6, 40), (7, 50)]
    anc = [(3, 20, 1.0), (3, 20, 0.5), (3, 20, 0.25), (3, 40, 1.0)]
    both(d, steps, confl, [(3, 3)], expect=([x for x in anc if x[2] == 1.0], anc))

    # an ID change: 4 is clause 3 strengthened, its uses count for 3
    steps = [
        ("o", 1, [1], None),
        ("a", 3, [2, 5], [1]),
        ("a", 4, [2], [3, 1]),
        ("a", 5, [], [4, 999]),
    ]
    both(d, steps, [(3, 10), (4, 20), (5, 30)], [(3, 3), (3, 4)],
         expect=([(3, 20, 1.0), (3, 30, 1.0)], [(3, 20, 1.0), (3, 30, 1.0)]))

    # a step without a conflict number counts nothing
    both(d, steps, [(3, 10), (5, 30)], [(3, 3), (3, 4)],
         expect=([(3, 30, 1.0)], [(3, 30, 1.0)]))

    # the weight halves down to 0.0625, then the line ends
    steps = [("o", 1, [1], None), ("a", 2, [1, 2], [1])]
    steps += [("a", i, [i], [i - 1]) for i in range(3, 10)]
    steps += [("a", 10, [], [9])]
    anc = [(2, 30, 1.0), (2, 30, 0.5), (2, 30, 0.25), (2, 30, 0.125), (2, 30, 0.0625)]
    both(d, steps, [(i, i * 10) for i in range(2, 11)], [(2, 2)], expect=(anc[:1], anc))

    # no empty clause: an error
    both(d, [("o", 1, [1], None), ("a", 2, [3], [1])], [(2, 10)], [(2, 2)], expect_fail=True)
    print("OK: hand-made proofs")


def random_proof(rng):
    n_orig = rng.randint(1, 20)
    n_add = rng.randint(1, 300)
    steps = [("o", i, [i, -i - 1], None) for i in range(1, n_orig + 1)]
    ids = list(range(1, n_orig + 1))
    confl = []
    updates = []
    tracked = []
    nid = n_orig
    c = 0
    for _ in range(n_add):
        nid += rng.randint(1, 3)
        hints = [rng.choice(ids[-40:]) for _ in range(rng.randint(1, 6))]
        if rng.random() < 0.03:
            hints.append(nid + 1000000)
        steps.append(("a", nid, [rng.randint(1, 50) * rng.choice([-1, 1]) for _ in range(rng.randint(1, 5))], hints))
        c += rng.randint(0, 3)
        if rng.random() < 0.95:
            confl.append((nid, c))
        if rng.random() < 0.3:
            updates.append((nid, nid))
            tracked.append(nid)
        elif tracked and rng.random() < 0.1:
            updates.append((rng.choice(tracked), nid))
            tracked.append(nid)
        if rng.random() < 0.1:
            steps.append(("d", rng.choice(ids), [1], None))
        ids.append(nid)
    nid += 1
    steps.append(("a", nid, [], [rng.choice(ids[-20:]) for _ in range(rng.randint(1, 8))]))
    confl.append((nid, c + 1))
    steps.append(("f", nid, [], None))
    return steps, confl, updates


def test_real(d, proof, rawdb):
    res = []
    for python in (False, True):
        db = os.path.join(d, "real.db")
        shutil.copy(rawdb, db)
        res.append(run(proof, db, python))
    assert res[0][0] == 0 and res[1][0] == 0, "a run failed: %s %s" % (res[0][0], res[1][0])
    assert res[0] == res[1], "the passes disagree on %s" % proof
    print("OK: %s, %d uses, %d with ancestors" % (proof, len(res[0][1]), len(res[0][2])))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--rounds", type=int, default=40)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--real", nargs=2, metavar=("XLRUP", "RAWDB"))
    opts = parser.parse_args()

    d = tempfile.mkdtemp(prefix="test-xlrup-uses-")
    try:
        test_hand(d)
        rng = random.Random(opts.seed)
        uses = 0
        for i in range(opts.rounds):
            uses += both(d, *random_proof(rng))
        assert uses > opts.rounds, "the random proofs have no uses"
        print("OK: %d random proofs, %d uses" % (opts.rounds, uses))
        if opts.real:
            test_real(d, opts.real[0], opts.real[1])
    finally:
        shutil.rmtree(d)
    sys.exit(0)

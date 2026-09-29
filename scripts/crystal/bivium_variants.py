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

# Makes UNSAT variants of a grain-of-salt Bivium CNF (SAT Competition 2020
# bivium-*.cnf.gz). The CNF has its guessed state bits and the keystream
# propagated into the clauses, so they cannot be changed: a variant keeps
# all of it and guesses N more state bits at random. A random guess is
# wrong and the keystream fixes the state, so the variant is UNSAT, and
# every guessed bit about halves the work.
#
# usage: bivium_variants.py base.cnf.gz -n 6 --seed 3 -o out.cnf

import argparse
import gzip
import random


def parse_ranges(txt):
    ret = []
    for part in txt.split(","):
        a, b = part.split("-")
        ret.extend(range(int(a), int(b)+1))
    return ret


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("base", help="the Bivium CNF, .gz or plain")
    parser.add_argument("-n", "--num", type=int, required=True, help="more state bits to guess")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("-o", "--out", required=True)
    parser.add_argument("--state", default="1-93,294-377", help="state variables")
    opts = parser.parse_args()

    opener = gzip.open if opts.base.endswith(".gz") else open
    clauses = []
    nvars = 0
    fixed = set()
    with opener(opts.base, "rt") as f:
        for line in f:
            line = line.strip()
            if not line or line[0] == "c":
                continue
            if line[0] == "p":
                nvars = int(line.split()[2])
                continue
            lits = line.split()
            assert lits[-1] == "0"
            if len(lits) == 2:
                fixed.add(abs(int(lits[0])))
            clauses.append(line)

    state = [v for v in parse_ranges(opts.state) if v not in fixed]
    rnd = random.Random(opts.seed)
    guess = sorted(rnd.sample(state, opts.num))
    units = ["%d 0" % (v if rnd.random() < 0.5 else -v) for v in guess]
    with open(opts.out, "w") as f:
        f.write("c %s, %d more guessed state bits, seed %d\n" % (opts.base.split("/")[-1], opts.num, opts.seed))
        f.write("p cnf %d %d\n" % (nvars, len(clauses)+len(units)))
        for c in clauses + units:
            f.write(c + "\n")

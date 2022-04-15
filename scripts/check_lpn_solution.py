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

import optparse
import random
import sys

lpnfile = sys.argv[1]
satout = sys.argv[2]

sat = None
sol = {}
with open(satout, "r") as f:
    for line in f:
        line = line.strip()
        if len(line) < 1:
            continue

        if line[0] == "s":
            if "s SATISFIABLE" in line:
                sat = True
                continue
            if "s UNSATISFIABLE" in line:
                sat = False
                continue
            print("ERROR: Nether SAT, nor UNSAT on solution line!")
            exit(-1)

        if line[0] == "v":
            line=line.strip()
            line=line[1:]
            for lit in line.split():
                lit = int(lit)
                var = abs(lit)
                if var == 0:
                    continue
                sol[var] = lit > 0

if sat is None:
    print("ERRROR could not recover solution from SAT output")
    exit(-1)

# print("Recovered SAT: ", sat)
# print("Recovered Solution : ", sol)

corr_sat = None
corr_sol = {}
with open(lpnfile, "r") as f:
    for line in f:
        line=line.strip()
        if "correct output" not in line:
            continue
        if "UNSAT" in line:
            corr_sat = False
            break
        corr_sat = True
        assert "SAT" in line

        line = line.split(":")[1]
        for lit in line.split()[1:]: # first is text "SAT"
            lit = int(lit)
            var = abs(lit)
            corr_sol[var] = lit > 0

if corr_sat is None:
    print("ERROR: could not recover correct solution from problem file")
    exit(-1)

if corr_sat is False:
    if sat == False:
        print("UNSAT both, all good!")
        exit(0)
    if sat == True:
        print("ERROR: buuuug!!! Should be UNSAT but it's SAT!!")
        exit(-1)

assert corr_sat == True
if sat == False:
    print("ERROR: buuuug!!! Should be SAT but it's UNSAT!!")
    exit(-1)

for v,val in corr_sol.items():
    if v not in sol:
        print("ERROR: Var %d is in problem, but not in solution" % v)
        exit(-1)
    if sol[v] != corr_sol[v]:
        print("ERROR: Var %d is %s in problem, but %s in solution" % (v, corr_sol[v], sol[v]))
        exit(-1)
    assert sol[v] == corr_sol[v]

print("OK, SAT, checked all solution values") #: %s" % corr_sol)
exit(0)

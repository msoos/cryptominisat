#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2022 Authors of CryptoMiniSat, see AUTHORS file
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


# hard example
# ----------------
# ../scripts/create_random_bnn.py 300 3000 100 4 > test.bnn
# ./cryptominisat5  --occsimp 0 --scc 0 test.bnn --presimp 0 --varelim 0 --verb 1 -n1 --printsol 0

# very hard example
#-------------
# ../scripts/create_random_bnn.py 300 2000 100 7 > test.bnn
# ./cryptominisat5  --occsimp 0 --scc 0 test.bnn --presimp 0 --varelim 0 --verb 4 -n1 --printsol 0
# appmc counting this:
# c [appmc] [   21.46 ] bounded_sol_count looking for   73 solutions -- hashes active: 128
# c [appmc] [   39.18 ] bounded_sol_count looking for   73 solutions -- hashes active: 256
# c [appmc] [  117.86 ] bounded_sol_count looking for   73 solutions -- hashes active: 512

# more constraints than above
# --------------
# ../scripts/create_random_bnn.py 400 2000 100 9 > test.bnn
#c [appmc+arjun] Total time: 214.48
#c [appmc] Number of solutions is: 33*2**257
#s SATISFIABLE
#s mc 7642277889662868897955685010573401918315818987932277226604200544522266556235776
# BUG in appmc: test.bnn-appmcbug (also, bug-appmc in approxmc/build directory)

# kinda interesting one to run
# ---------------
# ../scripts/create_random_bnn.py 20 100 50 9 > test.bnn
# c Total time (this thread) : 41.60
# c [appmc+arjun] Total time: 41.59
# c [appmc] Number of solutions is: 52*2**59




import random
import sys

n = 100 # number of constraints
v = 1000 # number of variables
s = 30 # max size of constraints
seed = 1

if len(sys.argv) != 1:
    if len(sys.argv) != 5:
        print("ERROR: must give: n v s seed")
        exit(-1)

    n = int(sys.argv[1])
    v = int(sys.argv[2])
    s = int(sys.argv[3])
    seed = int(sys.argv[4])

    assert n > 10
    assert v > 0

random.seed(seed)
print("c n=%d v=%d seed=%d" % (n, v, seed))




for x in range(n):
    sz = random.randint(1,s)

    # get input + output
    ks = []
    while len(ks) < sz+1:
        x = random.randint(1,v)
        if x in ks or -x in ks:
            continue
        else:
            ks.append(x)

    # get cutoff
    cutoff = random.randint(0, sz)

    # get output
    output  = ks[len(ks)-1]
    ks = ks[:-1]

    pr = "b "
    for k in ks:
        if random.choice([True, False]):
            pr+="-"
        pr += "%d " % k
    pr += "0 "
    pr += "%d " % cutoff
    pr += "%d" % output
    print("%s" % pr)


# add some clauses
for x in range(n):
    sz = random.randint(1,8)
    ks = []
    while len(ks) < sz:
        x = random.randint(1, v)
        if x in ks or -x in ks:
            continue

        if random.choice([True, False]):
            ks.append(x)
        else:
            ks.append(-x)

    pr = ""
    for k in ks:
        pr += "%d " % k
    print("%s0" % pr)

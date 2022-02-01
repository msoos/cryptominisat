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

import sys

if len(sys.argv) != 2:
    print("ERROR: must give: n v s seed")
    exit(-1)

fname = sys.argv[1]

def parse_lit(x):
    inv = 1
    if x[0] == '~':
        inv = -1
        x = x[1:]

    assert x[0] == 'x'
    v = int(x[1:])
    return v*inv

class Constr:
    def __init__(self, lhs, cutoff, rhs):
        self.lhs = lhs
        self.cutoff = cutoff
        self.rhs = rhs

    def __repr__(self):
        return "lhs: %s cutoff: %s rhs: %s" % (self.lhs, self.cutoff, self.rhs)

    def simple(self):
        for l in self.lhs:
            if l[0] == 1 or l[0] == -1:
                continue
            else:
                return False

        return True


def parse_constr(line):
    lhs = []
    cutoff = None
    rhs = None

    at_lhs = True
    at_cutoff = False
    at_rhs = False
    done = False
    line = line.split(' ')
    i = 0
    #print("line:", line)
    while i < len(line):
        #print("SO FAR -- lhs: ", lhs, "cutoff: ", cutoff, "rhs: ", rhs, "at_lhs:", at_lhs, "at_cutoff:", at_cutoff, "at_rhs:", at_rhs)

        k = line[i].strip()
        if k == '':
            i+=1
            continue

        if at_lhs:
            if k == ">=":
                at_lhs = False
                at_cutoff= True
                i+=1
                continue
            coeff = int(k)
            assert i+1 < len(line)
            i+=1
            k = line[i].strip()
            lit = parse_lit(k)
            lhs.append([coeff, lit])
            i+=1
            continue

        if at_cutoff:
            if k == "<->":
                at_cutoff= False
                at_rhs = True
                i+=1
                continue
            if k == ";":
                done = True
                break
            cutoff = int(k)
            i+=1
            continue

        if at_rhs:
            if k == ";":
                done = True
                break
            assert k[0] == 'x'
            rhs = parse_lit(k)
            i+=1
            continue

        assert False

    assert done

    #if rhs is not None:
        #print("lhs: ", lhs, "cutoff: ", cutoff, "rhs: ", rhs)
    #else:
        #print("lhs: ", lhs, "cutoff: ", cutoff)

    return Constr(lhs, cutoff, rhs)

ind = []
constr = []
with open(fname, "r") as f:
    # parse ind
    for line in f:
        line = line.strip()
        if len(line) == 0:
            continue
        if "* ind" in line:
            line = line[5:]
            for k in line.split(' '):
                k = k.strip()
                if k != "0" and k != '':
                    ind.append(int(k))
            continue

        if line[0] == '*':
            continue

        constr.append(parse_constr(line))

# deal with independent
pr = "c ind "
for i in ind:
    pr += "%d " % i
print("%s 0" % pr)

for c in constr:
    print("c doing constraint: ", c)
    if c.simple():
        pass
    else:
        print("c Can't deal with constraint: ", c)




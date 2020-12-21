#!/usr/bin/env python
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

import collections
import sys

toaddheader = []
toaddval = []
for x in range(1, len(sys.argv)):
    if x % 2 == 1:
        toaddheader.append(sys.argv[x])
    else:
        toaddval.append(sys.argv[x])


infilenames = []
infilenames.append("signals.csv")
infilenames.append("solveTimes_rev.csv")

# main data
files = {}


###############
# Fill file names
###############
allfiles = open("allFiles.csv", "r")
for l in allfiles:
    l = l.strip()
    files[l] = {"SAT":"?"}

###############
# Fill times, signals
###############
infiles = []
for x in infilenames:
    infiles.append(open(x, "r"))
for f,fname in zip(infiles, infilenames):
    for l in f:
        l = l.strip()
        l = l.split(" ")

        # print("appending to %s : %s" % (l[0], l[1]))
        files[l[0]][fname] = l[1]


###############
# Fill SAT/UNSAT
###############
with open("solvedUNSAT.csv") as f:
    for l in f:
        l = l.strip()
        files[l]["SAT"] = "UNSAT"

with open("solvedSAT.csv") as f:
    for l in f:
        l = l.strip()
        files[l]["SAT"] = "SAT"


###############
# Print header
###############
toprint = "fname,"
for x in range(len(infilenames)):
    toprint += infilenames[x].replace(".csv", "")
    if x+1 < len(infilenames):
        toprint += ","

for x in toaddheader:
    toprint+=","+x
toprint+=",SAT"

print(toprint)



###############
# Print lines, in order
###############
od = collections.OrderedDict(sorted(files.items()))
for k, v in od.items():
    toprint = ""
    for fname in infilenames+["SAT"]:
        if fname in v:
            toprint += "%s," % v[fname]
        else:
            toprint += "?,"

    for x in toaddval:
        toprint+= x + ","

    toprint = toprint.rstrip(",")
    print("%s,%s" % (k, toprint))

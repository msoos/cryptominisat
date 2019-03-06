#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2017  Mate Soos
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

infiles = []
infiles.append(open("memout.csv", "r"))
infiles.append(open("signals.csv", "r"))
infiles.append(open("solveTimes_rev.csv", "r"))
infiles.append(open("solved_sol.csv", "r"))
allfiles = open("allFiles.csv", "r")
files = {}
for l in allfiles:
    l = l.strip()
    files[l] = []

for f in infiles:
    for l in f:
        l = l.strip()
        l = l.split(" ")

        # print("appending to %s : %s" % (l[0], l[1]))
        files[l[0]].append(l[1])

print("fname,memout,signal,time,res")
od = collections.OrderedDict(sorted(files.items()))
for k, v in od.items():
    toprint = ""
    for datpoint in v:
        toprint += "%s," % datpoint
    toprint = toprint.rstrip(",")
    print("%s,%s" % (k, toprint))

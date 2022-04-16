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


from __future__ import with_statement  # Required in 2.5
from __future__ import print_function
import sys
import random

if len(sys.argv) !=4:
    print("ERROR: You must give 3 arguments: vars, Numxors and K")
    exit(-1)

vs = int(sys.argv[1])
numxors = int(sys.argv[2])
k = int(sys.argv[3])

print("c Will generate %d variable, %d XORs and then all variables's values sum will be at least %d" % (vs, numxors, k))

for i in range(numxors):
    thisxor = []
    for x in range(vs):
        if random.choice([True, False]):
            thisxor.append(x)
    print("c XOR:", thisxor)

    out = "x"
    if random.choice([True, False]):
        out += "-"
    for x in thisxor:
        out += "%d " % (x+1)
    out+="0\nc "
    print(out)

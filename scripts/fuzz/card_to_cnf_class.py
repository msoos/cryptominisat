#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2020  Gergely Kovasznai
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import re
from pysat.card import CardEnc, EncType
from pysat.formula import CNF, CNFPlus
import sys


class CardToCNF:
    def __init__(self):
        self.encoding = EncType.seqcounter

    def convert(self, infilename, outfilename):
        cnf = CNF()

        fin = open(infilename, "r")
        for line in fin:
            line = line.strip()

            # skip empty line
            if len(line) == 0:
                continue

            # skip header and comments
            if line[0] == 'c' or line[0] == 'p':
                continue

            if line[0] == 'x':
                # XOR clauses must be convert to CNF before
                assert(False)
            
            # convert cardinality constraint
            if re.search(r'(<=|>=)', line):
                lits, bound, is_atmost = self.parse_card(line)
                encode_func = CardEnc.atmost if is_atmost else CardEnc.atleast
                cnf.extend(
                    encode_func(
                        lits = lits,
                        bound = bound,
                        top_id = cnf.nv,
                        encoding = self.encoding
                    ).clauses
                )
                continue

            # simply add normal clause
            lits = self.parse_clause(line)
            cnf.append(lits)
        fin.close()
        
        with open(outfilename, 'w') as fp:
            cnf.to_fp(fp)

    def parse_card(self, line):
        assert re.search(r'^((-?\d+ )*)(<=|>=) (\d+)$', line)

        groups = line.split(' ')
        group_cnt = len(groups)

        return list(map(int, groups[:group_cnt - 2])), int(groups[group_cnt - 1]), groups[group_cnt - 2] == '<='

    def parse_clause(self, line):
        assert re.search(r'^(-?\d+ )*0$', line)

        groups = line.split(' ')

        return list(map(int, groups[:len(groups) - 1]))

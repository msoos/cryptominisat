#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2018, Marcel Bargull, Mate Soos
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from array import array
from collections import defaultdict
from itertools import chain
from time import time
import pycryptosat
import pycosat


def cms_setup(clauses, m):
    solver = pycryptosat.Solver()
    solver.add_clauses(clauses, max_var=m)
    return solver


def cms_solve(solver):
    return solver.solve()


def ps_setup(clauses, m):
    return pycosat.itersolve(clauses, vars=m)


def ps_solve(iter_sol):
    return next(iter_sol)


if __name__ == "__main__":
    n = 550*1000
    clause_list = list(chain.from_iterable(
        [(-x, x + n, x + int(n/2)), (-x, x + n, x + int(n/2))] for x in range(1, n+1)
    ))
    clause_array = array("i", chain.from_iterable(c + (0,) for c in clause_list))
    m = max(clause_array)

    all_times = defaultdict(list)
    my_list = ("CMS", cms_setup, cms_solve), ("Pycosat", ps_setup, ps_solve)
    #my_list = ("CMS", cms_setup, cms_solve), (None, None, None)
    for solver_type, setup, solve in list(my_list):
        if solver_type is None:
            continue

        my_type_list = ("list", clause_list), ("array", clause_array)
        #my_type_list = ("list", clause_list), (None, None)
        for clauses_type, clauses in (my_type_list):
            if clauses is None:
                continue

            for _ in range(5):
                t = time()
                solver = setup(clauses, m)
                all_times["setup {:<7} {:<5}".format(solver_type, clauses_type)].append(time() - t)
                t = time()
                solve(solver)
                all_times["solve {:<7} {:<5}".format(solver_type, clauses_type)].append(time() - t)
    for text, times in sorted(all_times.items()):
        print(text, *map("{:5.3f}".format, sorted(times)))

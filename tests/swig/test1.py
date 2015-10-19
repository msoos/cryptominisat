#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2014  Mate Soos
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
import copy
import random
from os.path import basename
import unittest
import _solver as solver

class TestSimple(unittest.TestCase):

    def setUp(self):
        self.a = solver.new_Solver()

    def tearDown(self):
        solver.delete_Solver(self.a)

    def test_free_vars(self):
        num = 100
        for _ in range(num):
            solver.Solver_new_external_var(self.a)

        self.assertEqual(solver.Solver_get_num_free_vars(self.a), num)


def run():
    print("sys.prefix: %s" % sys.prefix)
    print("sys.version: %s" % sys.version)
    try:
        print("solver version: %r" % solver.__version__)
    except AttributeError:
        pass
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestSimple))
    #suite.addTest(unittest.makeSuite(InitTester))
    #suite.addTest(unittest.makeSuite(TestSolve))

    runner = unittest.TextTestRunner(verbosity=2)
    return runner.run(suite)

if __name__ == '__main__':
    run()

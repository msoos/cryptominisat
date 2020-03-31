# -*- coding: utf-8 -*-
#
# CryptoMiniSat
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

from __future__ import unicode_literals
from __future__ import print_function
from array import array as _array
import sys
import unittest
import time


import pycryptosat
from pycryptosat import Solver


def array(typecode, initializer=()):
    return _array(str(typecode), initializer)


def check_clause(clause, solution):
    for lit in clause:
        var = abs(lit)
        if lit < 0:
            inverted = True
        else:
            inverted = False

        if solution[var] != inverted:
            return True


def check_solution(clauses, solution):
    for clause in clauses:
        if check_clause(clause, solution) is False:
            return False

    return True

# -------------------------- test clauses --------------------------------

# p cnf 5 3
# 1 -5 4 0
# -1 5 3 4 0
# -3 -4 0
clauses1 = [[1, -5, 4], [-1, 5, 3, 4], [-3, -4]]

# p cnf 2 2
# -1 0
# 1 0
clauses2 = [[-1], [1]]

# p cnf 2 3
# -1 2 0
# -1 -2 0
# 1 -2 0
clauses3 = [[-1, 2], [-1, -2], [1, -2]]

# -------------------------- actual unit tests ---------------------------


class TestXor(unittest.TestCase):

    def setUp(self):
        self.solver = Solver(threads=2)

    def test_wrong_args(self):
        self.assertRaises(TypeError, self.solver.add_xor_clause, [1, 2])
        self.assertRaises(ValueError, self.solver.add_xor_clause, [1, 0], True)
        self.assertRaises(
            ValueError, self.solver.add_xor_clause, [-1, 2], True)

    def test_binary(self):
        self.solver.add_xor_clause([1, 2], False)
        res, solution = self.solver.solve([1])
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, True, True))

    def test_unit(self):
        self.solver.add_xor_clause([1], False)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, False))

    def test_unit2(self):
        self.solver.add_xor_clause([1], True)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, True))

    def test_3_long(self):
        self.solver.add_xor_clause([1, 2, 3], False)
        res, solution = self.solver.solve([1, 2])
        self.assertEqual(res, True)
        # self.assertEqual(solution, (None, True, True, False))

    def test_3_long2(self):
        self.solver.add_xor_clause([1, 2, 3], True)
        res, solution = self.solver.solve([1, -2])
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, True, False, False))

    def test_long(self):
        for l in range(10, 30):
            self.setUp()
            toadd = []
            toassume = []
            solution_expected = [None]
            for i in range(1, l):
                toadd.append(i)
                solution_expected.append(False)
                if i != l - 1:
                    toassume.append(i * -1)

            self.solver.add_xor_clause(toadd, False)
            res, solution = self.solver.solve(toassume)
            self.assertEqual(res, True)
            self.assertEqual(solution, tuple(solution_expected))


class InitTester(unittest.TestCase):

    def test_wrong_args_to_solver(self):
        self.assertRaises(ValueError, Solver, threads=-1)
        self.assertRaises(ValueError, Solver, threads=0)
        self.assertRaises(ValueError, Solver, verbose=-1)
        self.assertRaises(ValueError, Solver, time_limit=-1)
        self.assertRaises(ValueError, Solver, confl_limit=-1)
        self.assertRaises(TypeError, Solver, threads="fail")
        self.assertRaises(TypeError, Solver, verbose="fail")
        self.assertRaises(TypeError, Solver, time_limit="fail")
        self.assertRaises(TypeError, Solver, confl_limit="fail")


class TestDump(unittest.TestCase):

    def setUp(self):
        self.solver = Solver()

    def test_max_glue_missing(self):
        self.assertRaises(TypeError,
                          self.solver.start_getting_small_clauses, 4)

    def test_one_dump(self):
        with open("tests/test.cnf", "r") as x:
            for line in x:
                line = line.strip()
                if "p" in line or "c" in line:
                    continue

                out = [int(x) for x in line.split()[:-1]]
                self.solver.add_clause(out)

        res, _ = self.solver.solve()
        self.assertEqual(res, True)

        self.solver.start_getting_small_clauses(4, max_glue=10)
        x = self.solver.get_next_small_clause()
        self.assertNotEquals(x, None)
        self.solver.end_getting_small_clauses()


class TestSolve(unittest.TestCase):

    def setUp(self):
        self.solver = Solver(threads=2)

    def test_wrong_args(self):
        self.assertRaises(TypeError, self.solver.add_clause, 'A')
        self.assertRaises(TypeError, self.solver.add_clause, 1)
        self.assertRaises(TypeError, self.solver.add_clause, 1.0)
        self.assertRaises(TypeError, self.solver.add_clause, object())
        self.assertRaises(TypeError, self.solver.add_clause, ['a'])
        self.assertRaises(
            TypeError, self.solver.add_clause, [[1, 2], [3, None]])
        self.assertRaises(ValueError, self.solver.add_clause, [1, 0])

    def test_no_clauses(self):
        for _ in range(7):
            self.assertEqual(self.solver.solve([]), (True, (None,)))

    def test_cnf1(self):
        for cl in clauses1:
            self.solver.add_clause(cl)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1, solution))

    def test_add_clauses(self):
        self.solver.add_clauses([[1], [-1]])
        res, solution = self.solver.solve()
        self.assertEqual(res, False)

    def test_add_clauses_wrong_zero(self):
        self.assertRaises(TypeError, self.solver.add_clause, [[1, 0], [-1]])

    def test_add_clauses_array_SAT(self):
        cls = array('i', [1, 2, 0, 1, 2, 0])
        self.solver.add_clauses(cls)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)

    def test_add_clauses_array_UNSAT(self):
        cls = array('i', [-1, 0, 1, 0])
        self.solver.add_clauses(cls)
        res, solution = self.solver.solve()
        self.assertEqual(res, False)

    def test_add_clauses_array_unterminated(self):
        cls = array('i', [1, 2, 0, 1, 2])
        self.assertRaises(ValueError, self.solver.add_clause, cls)

    def test_bad_iter(self):
        class Liar:

            def __iter__(self):
                return None
        self.assertRaises(TypeError, self.solver.add_clause, Liar())

    def test_get_conflict(self):
        self.solver.add_clauses([[-1], [2], [3], [-4]])
        assume = [-2, 3, 4]

        res, model = self.solver.solve(assumptions=assume)
        self.assertEqual(res, False)

        confl = self.solver.get_conflict()
        self.assertEqual(isinstance(confl, list), True)
        self.assertNotIn(3, confl)

        if 2 in confl:
            self.assertIn(2, confl)
        elif -4 in confl:
            self.assertIn(-4, confl)
        else:
            self.assertEqual(False, True, msg="Either -2 or 4 should be conflicting!")

        assume = [2, 4]
        res, model = self.solver.solve(assumptions=assume)
        self.assertEqual(res, False)

        confl = self.solver.get_conflict()
        self.assertEqual(isinstance(confl, list), True)
        self.assertNotIn(2, confl)
        self.assertIn(-4, confl)

    def test_cnf2(self):
        for cl in clauses2:
            self.solver.add_clause(cl)
        self.assertEqual(self.solver.solve(), (False, None))

    def test_cnf3(self):
        for cl in clauses3:
            self.solver.add_clause(cl)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses3, solution))

    def test_cnf1_confl_limit(self):
        for _ in range(1, 20):
            self.setUp()
            for cl in clauses1:
                self.solver.add_clause(cl)

            res, solution = self.solver.solve()
            self.assertTrue(res is None or check_solution(clauses1, solution))

    def test_by_re_curse(self):
        self.solver.add_clause([-1, -2, 3])
        res, _ = self.solver.solve()
        self.assertEqual(res, True)

        self.solver.add_clause([-5, 1])
        self.solver.add_clause([4, -3])
        self.solver.add_clause([2, 3, 5])
        res, _ = self.solver.solve()
        self.assertEqual(res, True)


class TestSolveTimeLimit(unittest.TestCase):

    def get_clauses(self):
        cls = []
        with open("tests/f400-r425-x000.cnf", "r") as f:
            for line in f:
                line = line.strip()
                if len(line) == 0:
                    continue
                if line[0] == "p":
                    continue
                if line[0] == "c":
                    continue
                line = line.split()
                line = [int(l.strip()) for l in line]
                assert line[-1] == 0
                cls.append(line[:-1])

        return cls


    def test_time(self):
        SAT_TIME_LIMIT = 1
        clauses = self.get_clauses() #returns a few hundred short clauses
        t0 = time.time()
        solver = Solver(threads=4, time_limit=SAT_TIME_LIMIT)
        solver.add_clauses(clauses)
        sat, sol = solver.solve()
        took_time = time.time() - t0

        # NOTE: the above CNF solves in about 1 hour.
        # So anything below 10min is good. Setting 2s would work... no most
        # systems, but not on overloaded CI servers
        self.assertLess(took_time, 4)

# ------------------------------------------------------------------------


def run():
    print("sys.prefix: %s" % sys.prefix)
    print("sys.version: %s" % sys.version)
    try:
        print("pycryptosat version: %r" % pycryptosat.__version__)
    except AttributeError:
        pass
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestXor))
    suite.addTest(unittest.makeSuite(InitTester))
    suite.addTest(unittest.makeSuite(TestSolve))
    suite.addTest(unittest.makeSuite(TestDump))
    suite.addTest(unittest.makeSuite(TestSolveTimeLimit))

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    n_errors = len(result.errors)
    n_failures = len(result.failures)

    if n_errors or n_failures:
        print('\n\nSummary: %d errors and %d failures reported\n'%\
            (n_errors, n_failures))

    print()

    sys.exit(n_errors+n_failures)


if __name__ == '__main__':
    run()

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

from array import array
import os
import sys
import unittest
import time


import pycryptosat
from pycryptosat import Solver

_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))+os.path.sep


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


class TestNbVars(unittest.TestCase):

    def setUp(self):
        self.solver = Solver()

    def test_zero_initially(self):
        self.assertEqual(self.solver.nb_vars(), 0)

    def test_grows_with_clauses(self):
        self.solver.add_clause([1, 2, 3])
        self.assertEqual(self.solver.nb_vars(), 3)

    def test_grows_to_max_var(self):
        self.solver.add_clause([1, -5, 4])
        self.assertEqual(self.solver.nb_vars(), 5)

    def test_no_shrink_on_second_add(self):
        self.solver.add_clause([1, 2, 3, 4, 5])
        self.solver.add_clause([1, 2])
        self.assertEqual(self.solver.nb_vars(), 5)

    def test_solution_length_matches_nb_vars(self):
        # solution tuple is (None, v1, v2, ..., vN), length = nb_vars + 1
        self.solver.add_clause([1, -3, 5])
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertEqual(len(solution), self.solver.nb_vars() + 1)
        self.assertIsNone(solution[0])


class TestIsSatisfiable(unittest.TestCase):

    def setUp(self):
        self.solver = Solver()

    def test_sat(self):
        self.solver.add_clause([1, 2])
        self.assertIs(self.solver.is_satisfiable(), True)

    def test_unsat(self):
        self.solver.add_clause([1])
        self.solver.add_clause([-1])
        self.assertIs(self.solver.is_satisfiable(), False)

    def test_no_clauses_is_sat(self):
        self.assertIs(self.solver.is_satisfiable(), True)


class TestIncremental(unittest.TestCase):
    """Adding clauses between solve() calls must work correctly."""

    def setUp(self):
        self.solver = Solver()

    def test_sat_then_unsat_after_new_clause(self):
        self.solver.add_clause([1])
        res, _ = self.solver.solve()
        self.assertEqual(res, True)

        self.solver.add_clause([-1])
        res, _ = self.solver.solve()
        self.assertEqual(res, False)

    def test_multiple_solve_calls_same_result(self):
        for cl in clauses1:
            self.solver.add_clause(cl)
        for _ in range(5):
            res, solution = self.solver.solve()
            self.assertEqual(res, True)
            self.assertTrue(check_solution(clauses1, solution))

    def test_assumptions_do_not_persist(self):
        # Solve with an assumption that forces UNSAT, then without it → SAT.
        self.solver.add_clause([1, 2])
        self.solver.add_clause([-1])   # forces var1 = False

        res, _ = self.solver.solve([-2])   # assume var2=False too → UNSAT
        self.assertEqual(res, False)

        res, solution = self.solver.solve()  # no assumption → SAT
        self.assertEqual(res, True)
        self.assertFalse(solution[1])        # var1 still False from clause
        self.assertTrue(solution[2])         # var2 must be True

    def test_add_xor_then_regular(self):
        # XOR(1,2) = True means exactly one of {1,2} is True.
        self.solver.add_xor_clause([1, 2], True)
        self.solver.add_clause([1])    # force var1 = True
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(solution[1])
        self.assertFalse(solution[2])  # XOR satisfied: 1 XOR 0 = 1

    def test_clause_count_grows(self):
        # nb_vars tracks the highest variable seen.
        self.assertEqual(self.solver.nb_vars(), 0)
        self.solver.add_clause([3])
        self.assertEqual(self.solver.nb_vars(), 3)
        self.solver.add_clause([7, -3])
        self.assertEqual(self.solver.nb_vars(), 7)


class TestSolveArgs(unittest.TestCase):
    """Per-call solve() keyword arguments."""

    def setUp(self):
        self.solver = Solver()

    def test_solve_bad_verbose(self):
        self.solver.add_clause([1])
        self.assertRaises(ValueError, self.solver.solve, None, -1)

    def test_solve_bad_time_limit(self):
        self.solver.add_clause([1])
        self.assertRaises(ValueError, self.solver.solve, [], 0, -1.0)

    def test_solve_bad_confl_limit(self):
        self.solver.add_clause([1])
        self.assertRaises(ValueError, self.solver.solve, [], 0, 1.0, -1)

    def test_solve_confl_limit_zero_hard_problem(self):
        # confl_limit=0 on a non-trivial formula should return (None, None).
        clauses = []
        with open(_MODULE_DIR + "f400-r425-x000.cnf") as f:
            for line in f:
                line = line.strip()
                if not line or line[0] in ("c", "p"):
                    continue
                nums = [int(x) for x in line.split()]
                assert nums[-1] == 0
                clauses.append(nums[:-1])
        self.solver.add_clauses(clauses)
        res, sol = self.solver.solve(confl_limit=0)
        self.assertIsNone(res)
        self.assertIsNone(sol)

    def test_per_call_limits_restored(self):
        # After a solve() with confl_limit=0, the next call without a limit
        # should still be able to find a solution (limits are not permanent).
        for cl in clauses1:
            self.solver.add_clause(cl)
        self.solver.solve(confl_limit=0)   # may or may not find solution
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1, solution))

    def test_assumption_unknown_variable_raises(self):
        self.solver.add_clause([1, 2])
        self.assertRaises(ValueError, self.solver.solve, [10])


class TestVersion(unittest.TestCase):

    def test_version_string(self):
        v = pycryptosat.__version__
        self.assertIsInstance(v, str)
        self.assertRegex(v, r'^\d+\.\d+\.\d+$')

    def test_version_constant(self):
        self.assertEqual(pycryptosat.__version__, pycryptosat.VERSION)


class TestXorMixed(unittest.TestCase):
    """XOR clauses combined with regular clauses."""

    def setUp(self):
        self.solver = Solver()

    def test_xor_unsat_via_regular_clauses(self):
        # XOR(1,2)=False means 1==2.  Force 1=T and 2=F → contradiction.
        self.solver.add_xor_clause([1, 2], False)
        self.solver.add_clause([1])
        self.solver.add_clause([-2])
        res, _ = self.solver.solve()
        self.assertEqual(res, False)

    def test_xor_system_unique_solution(self):
        # XOR(1,2)=True AND XOR(2,3)=True AND XOR(1,3)=False
        # → 1⊕2=1, 2⊕3=1, 1⊕3=0
        # Adding 1=True via unit clause → 2=False, 3=True.
        self.solver.add_xor_clause([1, 2], True)
        self.solver.add_xor_clause([2, 3], True)
        self.solver.add_xor_clause([1, 3], False)
        self.solver.add_clause([1])
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(solution[1])
        self.assertFalse(solution[2])
        self.assertTrue(solution[3])

    def test_xor_rhs_false_means_equal(self):
        # XOR(a,b)=False ↔ a==b. Force a=True → b must be True too.
        self.solver.add_xor_clause([1, 2], False)
        self.solver.add_clause([1])
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(solution[1])
        self.assertTrue(solution[2])


class TestGetConflict(unittest.TestCase):
    """get_conflict() behaviour in various situations."""

    def setUp(self):
        self.solver = Solver()

    def test_get_conflict_after_sat_is_empty(self):
        # After a SAT solve, the conflict set should be empty.
        self.solver.add_clause([1, 2])
        res, _ = self.solver.solve()
        self.assertEqual(res, True)
        self.assertEqual(self.solver.get_conflict(), [])

    def test_get_conflict_returns_list(self):
        self.solver.add_clause([-1])
        res, _ = self.solver.solve([-1])
        # -1 is forced False by the clause AND assumed False → SAT, not UNSAT
        # Let's force a real conflict: clause forces 1=False, assume 1=True
        res, _ = self.solver.solve([1])
        self.assertEqual(res, False)
        confl = self.solver.get_conflict()
        self.assertIsInstance(confl, list)
        self.assertIn(-1, confl)


class TestEdgeCases(unittest.TestCase):
    """Edge cases not covered elsewhere."""

    def setUp(self):
        self.solver = Solver()

    def test_empty_clause_makes_unsat(self):
        # An empty clause is always False → immediately UNSAT.
        self.solver.add_clause([])
        res, sol = self.solver.solve()
        self.assertEqual(res, False)
        self.assertIsNone(sol)

    def test_add_clauses_with_generator(self):
        # add_clauses() must accept any iterable, not just lists.
        def clause_gen():
            yield [1, 2]
            yield [-1, 3]
        self.solver.add_clauses(clause_gen())
        res, _ = self.solver.solve()
        self.assertEqual(res, True)

    def test_add_clauses_empty_input(self):
        # Adding zero clauses is a no-op; solver stays SAT.
        self.solver.add_clauses([])
        res, _ = self.solver.solve()
        self.assertEqual(res, True)

    def test_solve_per_call_verbose_does_not_crash(self):
        self.solver.add_clause([1])
        res, _ = self.solver.solve(verbose=0)
        self.assertEqual(res, True)

    def test_solve_per_call_time_limit(self):
        # A generous time_limit should still find the solution.
        self.solver.add_clause([1, 2])
        res, solution = self.solver.solve(time_limit=60.0)
        self.assertEqual(res, True)

    def test_solve_per_call_confl_limit_large(self):
        # A generous confl_limit should still find the solution.
        for cl in clauses1:
            self.solver.add_clause(cl)
        res, solution = self.solver.solve(confl_limit=100000)
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1, solution))

    def test_unit_clause_sets_variable(self):
        self.solver.add_clause([5])
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(solution[5])

    def test_negative_unit_clause_sets_variable_false(self):
        self.solver.add_clause([-3])
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertFalse(solution[3])

    def test_solve_assumptions_as_keyword(self):
        # assumptions can be passed as a keyword argument.
        self.solver.add_clause([1, 2])
        self.solver.add_clause([-1])
        res, _ = self.solver.solve(assumptions=[-2])
        self.assertEqual(res, False)

    def test_is_satisfiable_after_unsat(self):
        self.solver.add_clause([1])
        self.solver.add_clause([-1])
        self.assertIs(self.solver.is_satisfiable(), False)

    def test_nb_vars_after_xor_clause(self):
        self.solver.add_xor_clause([1, 2, 3], True)
        self.assertEqual(self.solver.nb_vars(), 3)


class TestSolveTimeLimit(unittest.TestCase):

    def get_clauses(self):
        cls = []
        with open(_MODULE_DIR+"f400-r425-x000.cnf", "r") as f:
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
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    for cls in (TestXor, InitTester, TestSolve, TestNbVars, TestIsSatisfiable,
                TestIncremental, TestSolveArgs, TestVersion, TestXorMixed,
                TestGetConflict, TestEdgeCases, TestSolveTimeLimit):
        suite.addTests(loader.loadTestsFromTestCase(cls))

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

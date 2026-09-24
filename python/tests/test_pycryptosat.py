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
    return any(solution[abs(lit)] == (lit > 0) for lit in clause)


def check_solution(clauses, solution):
    return all(check_clause(clause, solution) for clause in clauses)

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


# Every option settable via Solver(options=...) / set_option(), with sane
# non-default values. Keep in sync with the entries marked lib in
# src/conf_options.h, plus "polar".
EXPOSED_OPTIONS = {
    "seed": ["0", "42"],
    "mult": ["0.5", "1e-1"],
    "polar": ["true", "false", "rnd", "weight", "auto"],
    "branchstr": ["vsids", "vmtf"],
    "restart": ["0"],
    "stabilize": ["0"],
    "reduce": ["0"],
    "lucky": ["0"],
    "sls": ["0"],
    "rephase": ["0"],
    "target": ["0", "2"],
    "nonstop": ["1"],
    "schedsimp": ["0"],
    "presimp": ["0"],
    "occsimp": ["0"],
    "varelim": ["0"],
    "bva": ["0"],
    "distill": ["0"],
    "sweep": ["0"],
    "scc": ["0"],
    "intree": ["0"],
    "transred": ["0"],
    "breakid": ["0", "1"],
    "confbtwsimp": ["1", "100"],
    "schedule": ["scc-vrepl,sub-impl"],
    "preschedule": ["occ-bve"],
    "xor": ["0"],
    "maxxorsize": ["3", "12"],
    "xorfindtout": ["0", "10"],
    "maxxormat": ["10"],
    "xorgatemaxsize": ["3"],
    "maxmatrixrows": ["1", "5000"],
    "maxmatrixcols": ["1", "5000"],
    "minmatrixrows": ["1", "100"],
    "maxnummatrices": ["0", "3"],
    "autodisablegauss": ["0", "1"],
    "gaussusefulcutoff": ["0", "0.9"],
    "gaussmincalls": ["1"],
    "gausscheckevery": ["1"],
}

UNSIGNED_OPTIONS = ["seed", "confbtwsimp", "maxxorsize", "xorfindtout",
    "maxxormat", "xorgatemaxsize", "maxmatrixrows", "maxmatrixcols",
    "minmatrixrows", "maxnummatrices", "gaussmincalls", "gausscheckevery"]
SIGNED_OPTIONS = ["restart", "stabilize", "reduce", "lucky", "sls", "rephase",
    "target", "nonstop", "schedsimp", "presimp", "occsimp", "varelim", "bva",
    "distill", "sweep", "scc", "intree", "transred", "xor"]
BOOL_OPTIONS = ["autodisablegauss", "breakid"]
DOUBLE_OPTIONS = ["mult", "gaussusefulcutoff"]

# Command-line options that must NOT be settable from the library
NOT_EXPOSED_OPTIONS = ["verb", "threads", "maxsol", "xlrup", "printsol",
    "maxtime", "maxconfl", "occredmax", "reducetier1glue", "walkmineff",
    "savemem", "renumber", "printtimes", "cardfind", "gates", "r", "t"]


def planted_instance(num_vars, num_cls, num_xors, seed):
    """Random 3-SAT clauses and XORs, all satisfied by one hidden solution"""
    import random
    rnd = random.Random(seed)
    sol = [None] + [rnd.choice([True, False]) for _ in range(num_vars)]
    cls = []
    while len(cls) < num_cls:
        cl = [v if rnd.random() < 0.5 else -v
              for v in rnd.sample(range(1, num_vars+1), 3)]
        if any(sol[abs(lit)] == (lit > 0) for lit in cl):
            cls.append(cl)
    xors = []
    for _ in range(num_xors):
        vs = rnd.sample(range(1, num_vars+1), rnd.randint(2, 6))
        rhs = False
        for v in vs:
            rhs ^= sol[v]
        xors.append((vs, rhs))
    return cls, xors


def xors_satisfied(xors, solution):
    for vs, rhs in xors:
        val = False
        for v in vs:
            val ^= solution[v]
        if val != rhs:
            return False
    return True


class TestOptions(unittest.TestCase):

    def check_solves(self, **kwargs):
        solver = Solver(**kwargs)
        cls, xors = planted_instance(40, 150, 20, 1)
        solver.add_clauses(cls)
        for vs, rhs in xors:
            solver.add_xor_clause(vs, rhs)
        res, solution = solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(cls, solution))
        self.assertTrue(xors_satisfied(xors, solution))

        solver = Solver(**kwargs)
        solver.add_clauses(clauses2)
        self.assertEqual(solver.solve()[0], False)

    def test_every_exposed_option_solves_correctly(self):
        for name, values in EXPOSED_OPTIONS.items():
            for value in values:
                with self.subTest(option=name, value=value):
                    self.check_solves(options={name: value})

                    solver = Solver()
                    solver.set_option(name, value)
                    solver.add_clauses(clauses1)
                    res, solution = solver.solve()
                    self.assertEqual(res, True)
                    self.assertTrue(check_solution(clauses1, solution))

    def test_all_exposed_options_at_once(self):
        opts = {name: values[-1] for name, values in EXPOSED_OPTIONS.items()}
        opts["nonstop"] = "0"
        for threads in (1, 2):
            with self.subTest(threads=threads):
                self.check_solves(threads=threads, options=opts)

    def test_options_with_threads(self):
        for threads in (1, 2, 4):
            with self.subTest(threads=threads):
                self.check_solves(threads=threads, options={
                    "seed": "3", "polar": "rnd", "minmatrixrows": "1",
                    "autodisablegauss": "0", "branchstr": "vsids"})

                solver = Solver(threads=threads)
                solver.set_option("maxmatrixrows", "100")
                solver.set_option("seed", "9")
                solver.add_clauses(clauses1)
                self.assertEqual(solver.solve()[0], True)

    def test_gauss_on_xor_systems(self):
        opts = {"minmatrixrows": "1", "autodisablegauss": "0",
                "maxmatrixrows": "1000", "maxnummatrices": "10",
                "gaussmincalls": "1", "gausscheckevery": "1",
                "gaussusefulcutoff": "0"}
        for seed in range(10):
            with self.subTest(seed=seed):
                _, xors = planted_instance(30, 0, 25, seed)
                solver = Solver(options=opts)
                for vs, rhs in xors:
                    solver.add_xor_clause(vs, rhs)
                res, solution = solver.solve()
                self.assertEqual(res, True)
                self.assertTrue(xors_satisfied(xors, solution))

                # the sum of all XORs, with the wrong parity, contradicts them
                total = {}
                parity = False
                for vs, rhs in xors:
                    parity ^= rhs
                    for v in vs:
                        total[v] = not total.get(v, False)
                vs = [v for v, odd in total.items() if odd]
                if not vs:
                    continue
                solver = Solver(options=opts)
                for x_vs, rhs in xors:
                    solver.add_xor_clause(x_vs, rhs)
                solver.add_xor_clause(vs, not parity)
                self.assertEqual(solver.solve()[0], False)

    def test_seed_is_deterministic(self):
        cls, _ = planted_instance(60, 150, 0, 3)
        for seed in ("0", "1", "12345"):
            with self.subTest(seed=seed):
                models = []
                for _ in range(2):
                    solver = Solver(options={"seed": seed, "polar": "rnd"})
                    solver.add_clauses(cls)
                    res, solution = solver.solve()
                    self.assertEqual(res, True)
                    self.assertTrue(check_solution(cls, solution))
                    models.append(solution)
                self.assertEqual(models[0], models[1])

    def test_no_options(self):
        for opts in (None, {}):
            self.check_solves(options=opts)

    def test_not_exposed_options_rejected(self):
        for name in NOT_EXPOSED_OPTIONS:
            with self.subTest(option=name):
                self.assertRaises(ValueError, Solver, options={name: "1"})
                self.assertRaises(ValueError, Solver().set_option, name, "1")

    def test_bad_option_names(self):
        for name in ("", "nosuchoption", "--seed", "-r", "SEED", " seed",
                     "seed ", "gauss", "maxmatrixrow", "sééd"):
            with self.subTest(option=name):
                self.assertRaises(ValueError, Solver, options={name: "1"})
                self.assertRaises(ValueError, Solver().set_option, name, "1")

    def check_bad_values(self, names, values):
        for name in names:
            for value in values:
                with self.subTest(option=name, value=value):
                    self.assertRaises(ValueError, Solver,
                                      options={name: value})
                    self.assertRaises(ValueError, Solver().set_option,
                                      name, value)

    def test_bad_unsigned_values(self):
        self.check_bad_values(UNSIGNED_OPTIONS, [
            "", "abc", "-1", "1.5", "1e3", "10x", " 10", "0x10",
            "99999999999999999999999"])

    def test_bad_signed_values(self):
        self.check_bad_values(SIGNED_OPTIONS, [
            "", "abc", "1.5", "10x", " 1", "true",
            "99999999999999999999999"])

    def test_bad_bool_values(self):
        self.check_bad_values(BOOL_OPTIONS, ["", "abc", "true", "1.0"])

    def test_bad_double_values(self):
        self.check_bad_values(DOUBLE_OPTIONS, ["", "abc", "1.5x", "1,5"])

    def test_bad_polar_values(self):
        self.check_bad_values(["polar"], ["", "TRUE", "random", "1", "auto "])

    def test_good_numeric_formats(self):
        for name in DOUBLE_OPTIONS:
            for value in ("0", "1", "0.25", "1e-2", "-0.5"):
                with self.subTest(option=name, value=value):
                    Solver(options={name: value})
        for name in SIGNED_OPTIONS:
            for value in ("0", "1", "-1"):
                with self.subTest(option=name, value=value):
                    Solver(options={name: value})

    def test_conf_sanity_check(self):
        Solver(options={"maxxorsize": "12"})
        self.assertRaises(ValueError, Solver, options={"maxxorsize": "13"})
        self.assertRaises(ValueError, Solver().set_option, "maxxorsize", "13")

    def test_error_message_names_the_problem(self):
        with self.assertRaises(ValueError) as cm:
            Solver(options={"nosuchoption": "1"})
        self.assertIn("nosuchoption", str(cm.exception))
        with self.assertRaises(ValueError) as cm:
            Solver(options={"maxmatrixrows": "12abc"})
        self.assertIn("12abc", str(cm.exception))
        with self.assertRaises(ValueError) as cm:
            Solver(options={"polar": "sideways"})
        self.assertIn("sideways", str(cm.exception))

    def test_wrong_types(self):
        for opts in ([("seed", "1")], "seed=1", 1, ("seed", "1")):
            with self.subTest(options=opts):
                self.assertRaises(TypeError, Solver, options=opts)
        for name, value in (("seed", 1), ("seed", 1.0), ("seed", None),
                            ("seed", b"1"), (1, "1"), (None, "1"),
                            (b"seed", "1")):
            with self.subTest(name=name, value=value):
                self.assertRaises(TypeError, Solver, options={name: value})
                self.assertRaises(TypeError, Solver().set_option, name, value)

    def test_set_option_arguments(self):
        solver = Solver()
        solver.set_option(name="seed", value="7")
        solver.set_option("seed", value="8")
        self.assertRaises(TypeError, solver.set_option)
        self.assertRaises(TypeError, solver.set_option, "seed")
        self.assertRaises(TypeError, solver.set_option, "seed", "1", "2")
        self.assertRaises(TypeError, solver.set_option, nme="seed", value="1")

    def test_set_option_repeatedly(self):
        solver = Solver(threads=2, options={"maxmatrixrows": "10"})
        for name, values in EXPOSED_OPTIONS.items():
            if name == "nonstop":
                continue
            for value in values:
                solver.set_option(name, value)
        solver.add_clauses(clauses1)
        res, solution = solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1, solution))

    def test_solver_usable_after_bad_option(self):
        solver = Solver()
        self.assertRaises(ValueError, solver.set_option, "seed", "abc")
        self.assertRaises(ValueError, solver.set_option, "nosuchoption", "1")
        self.assertRaises(ValueError, solver.set_option, "maxxorsize", "13")
        solver.set_option("seed", "5")
        solver.add_clauses(clauses3)
        self.assertEqual(solver.solve(), (True, (None, False, False)))

    def test_set_option_too_late(self):
        def after_add_clause(s):
            s.add_clause([1, 2])

        def after_add_clauses(s):
            s.add_clauses([[1], [-2, 3]])

        def after_add_xor_clause(s):
            s.add_xor_clause([1, 2], True)

        def after_solve_empty(s):
            s.solve()

        def after_solve(s):
            s.add_clauses(clauses1)
            s.solve()

        def after_unsat(s):
            s.add_clauses(clauses2)
            s.solve()

        def after_solve_with_assumptions(s):
            s.add_clause([1, 2])
            s.solve([1])

        for step in (after_add_clause, after_add_clauses, after_add_xor_clause,
                     after_solve_empty, after_solve, after_unsat,
                     after_solve_with_assumptions):
            for threads in (1, 2):
                with self.subTest(step=step.__name__, threads=threads):
                    solver = Solver(threads=threads)
                    step(solver)
                    self.assertRaises(RuntimeError, solver.set_option,
                                      "seed", "1")
                    self.assertRaises(RuntimeError, solver.set_option,
                                      "maxmatrixrows", "10")

    def test_too_late_error_does_not_break_solver(self):
        solver = Solver()
        solver.add_clauses(clauses1)
        self.assertRaises(RuntimeError, solver.set_option, "seed", "1")
        res, solution = solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1, solution))
        solver.add_clause([-1])
        res, solution = solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1 + [[-1]], solution))

    def test_queries_do_not_make_it_too_late(self):
        solver = Solver()
        self.assertEqual(solver.nb_vars(), 0)
        solver.set_option("seed", "1")

    def test_docstrings(self):
        self.assertIn("options", Solver.__doc__)
        self.assertIn("set_option", Solver.set_option.__doc__)

    def test_option_names_match_library(self):
        names = pycryptosat.get_option_names()
        self.assertIsInstance(names, list)
        for name in names:
            self.assertIsInstance(name, str)
        self.assertEqual(len(names), len(set(names)))
        self.assertEqual(set(names), set(EXPOSED_OPTIONS))
        self.assertEqual(set(names), set(UNSIGNED_OPTIONS + SIGNED_OPTIONS
            + BOOL_OPTIONS + DOUBLE_OPTIONS + ["branchstr", "schedule",
            "preschedule", "polar"]))
        for name in NOT_EXPOSED_OPTIONS:
            self.assertNotIn(name, names)
        self.assertIn("get_option_names", pycryptosat.get_option_names.__doc__)

    def test_readme_lists_every_option(self):
        import re
        readme = _MODULE_DIR + ".." + os.path.sep + "README.md"
        if not os.path.exists(readme):
            self.skipTest("python/README.md not found")
        text = open(readme).read()
        start = text.index("## Solver options")
        end = text.index("\n## ", start + 1)
        listed = re.findall(r"^\| `(\w+)` \|", text[start:end], re.M)
        self.assertEqual(len(listed), len(set(listed)))
        self.assertEqual(set(listed), set(pycryptosat.get_option_names()))


THREAD_COUNTS = (2, 3, 4, 8)


def random_cnf(num_vars, num_cls, rnd, max_len=3):
    return [[v if rnd.random() < 0.5 else -v
             for v in rnd.sample(range(1, num_vars+1), rnd.randint(1, max_len))]
            for _ in range(num_cls)]


def brute_force_models(num_vars, clauses, xors=(), assumptions=()):
    """All models of the clauses, XORs and assumptions, by enumeration"""
    models = []
    for bits in range(1 << num_vars):
        sol = [None] + [bool((bits >> i) & 1) for i in range(num_vars)]
        if all(sol[abs(lit)] == (lit > 0) for lit in assumptions) \
                and check_solution(clauses, sol) \
                and xors_satisfied(xors, sol):
            models.append(tuple(sol))
    return models


def brute_force_sat(num_vars, clauses, xors=(), assumptions=()):
    for bits in range(1 << num_vars):
        sol = [None] + [bool((bits >> i) & 1) for i in range(num_vars)]
        if all(sol[abs(lit)] == (lit > 0) for lit in assumptions) \
                and check_solution(clauses, sol) \
                and xors_satisfied(xors, sol):
            return True
    return False


def pigeonhole(holes):
    pigeons = holes + 1
    var = lambda p, h: p*holes + h + 1
    cls = [[var(p, h) for h in range(holes)] for p in range(pigeons)]
    for h in range(holes):
        for p1 in range(pigeons):
            for p2 in range(p1+1, pigeons):
                cls.append([-var(p1, h), -var(p2, h)])
    return cls


class TestThreads(unittest.TestCase):

    def test_helpers_can_fail(self):
        self.assertFalse(check_solution([[1, -2], [2]], (None, False, True)))
        self.assertTrue(check_solution([[1, -2], [2]], (None, True, True)))
        self.assertFalse(xors_satisfied([([1, 2], True)], (None, True, True)))
        self.assertFalse(brute_force_sat(2, [[1], [-1, 2], [-2]]))
        self.assertEqual(len(brute_force_models(3, [[1, 2]])), 6)

    def check_result(self, solver, res, solution, num_vars, clauses,
                     xors=(), assumptions=()):
        expected = brute_force_sat(num_vars, clauses, xors, assumptions)
        self.assertEqual(res, expected)
        if res:
            self.assertTrue(check_solution(clauses, solution))
            self.assertTrue(xors_satisfied(xors, solution))
            for lit in assumptions:
                self.assertEqual(solution[abs(lit)], lit > 0)
        elif assumptions:
            confl = solver.get_conflict()
            for lit in confl:
                self.assertIn(-lit, assumptions)
            self.assertFalse(brute_force_sat(
                num_vars, clauses, xors, [-lit for lit in confl]))

    def test_random_cnf_against_brute_force(self):
        import random
        for threads in THREAD_COUNTS:
            for seed in range(20):
                with self.subTest(threads=threads, seed=seed):
                    rnd = random.Random(seed)
                    clauses = random_cnf(10, 45, rnd)
                    xors = [(rnd.sample(range(1, 11), rnd.randint(2, 4)),
                             rnd.random() < 0.5)
                            for _ in range(rnd.randint(0, 3))]
                    solver = Solver(threads=threads)
                    solver.add_clauses(clauses)
                    for vs, rhs in xors:
                        solver.add_xor_clause(vs, rhs)
                    res, solution = solver.solve()
                    self.check_result(solver, res, solution, 10, clauses, xors)

    def test_incremental_with_assumptions(self):
        import random
        for threads in THREAD_COUNTS:
            for seed in range(10):
                with self.subTest(threads=threads, seed=seed):
                    rnd = random.Random(seed)
                    solver = Solver(threads=threads)
                    clauses = [list(range(1, 11))]
                    solver.add_clauses(clauses)
                    for _ in range(8):
                        batch = random_cnf(10, 6, rnd)
                        solver.add_clauses(batch)
                        clauses += batch
                        for _ in range(3):
                            assumps = [v if rnd.random() < 0.5 else -v for v
                                       in rnd.sample(range(1, 11), rnd.randint(0, 4))]
                            res, solution = solver.solve(assumps)
                            self.check_result(solver, res, solution, 10,
                                              clauses, assumptions=assumps)
                        res, solution = solver.solve()
                        self.check_result(solver, res, solution, 10, clauses)
                        if not res:
                            break

    def test_enumerate_all_solutions(self):
        import random
        for threads in THREAD_COUNTS:
            for seed in range(5):
                with self.subTest(threads=threads, seed=seed):
                    rnd = random.Random(seed)
                    clauses = [list(range(1, 9))] + random_cnf(8, 12, rnd)
                    expected = set(brute_force_models(8, clauses))
                    solver = Solver(threads=threads)
                    solver.add_clauses(clauses)
                    found = set()
                    while True:
                        res, solution = solver.solve()
                        if not res:
                            break
                        sol = tuple(solution[:9])
                        self.assertNotIn(sol, found)
                        found.add(sol)
                        solver.add_clause([-v if sol[v] else v
                                           for v in range(1, 9)])
                    self.assertEqual(found, expected)

    def test_xor_systems(self):
        for threads in THREAD_COUNTS:
            for seed in range(5):
                with self.subTest(threads=threads, seed=seed):
                    clauses, xors = planted_instance(30, 60, 20, seed)
                    solver = Solver(threads=threads)
                    solver.add_clauses(clauses)
                    for vs, rhs in xors:
                        solver.add_xor_clause(vs, rhs)
                    res, solution = solver.solve()
                    self.assertEqual(res, True)
                    self.assertTrue(check_solution(clauses, solution))
                    self.assertTrue(xors_satisfied(xors, solution))

                    vs, rhs = xors[0]
                    solver.add_xor_clause(vs, not rhs)
                    self.assertEqual(solver.solve()[0], False)
                    self.assertEqual(solver.is_satisfiable(), False)

    def test_pigeonhole_unsat(self):
        for threads in THREAD_COUNTS:
            with self.subTest(threads=threads):
                solver = Solver(threads=threads)
                solver.add_clauses(pigeonhole(6))
                self.assertEqual(solver.solve(), (False, None))

    def test_confl_limit(self):
        for threads in THREAD_COUNTS:
            with self.subTest(threads=threads):
                solver = Solver(threads=threads)
                solver.add_clauses(pigeonhole(7))
                self.assertEqual(solver.solve(confl_limit=0), (None, None))
                self.assertEqual(solver.solve(), (False, None))

    def test_nb_vars_and_is_satisfiable(self):
        for threads in THREAD_COUNTS:
            with self.subTest(threads=threads):
                solver = Solver(threads=threads)
                self.assertEqual(solver.nb_vars(), 0)
                solver.add_clause([1, -7])
                self.assertEqual(solver.nb_vars(), 7)
                res, solution = solver.solve()
                self.assertEqual(res, True)
                self.assertEqual(len(solution), 8)
                self.assertEqual(solver.is_satisfiable(), True)
                solver.add_clauses([[-1], [7]])
                self.assertEqual(solver.solve()[0], False)
                self.assertEqual(solver.is_satisfiable(), False)

    def test_many_solvers(self):
        for i in range(40):
            threads = THREAD_COUNTS[i % len(THREAD_COUNTS)]
            solver = Solver(threads=threads)
            solver.add_clauses(clauses1)
            res, solution = solver.solve()
            self.assertEqual(res, True)
            self.assertTrue(check_solution(clauses1, solution))
            del solver

    def test_unused_solvers(self):
        for threads in THREAD_COUNTS:
            Solver(threads=threads)
            Solver(threads=threads).solve()


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
                TestGetConflict, TestEdgeCases, TestSolveTimeLimit,
                TestOptions, TestThreads):
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

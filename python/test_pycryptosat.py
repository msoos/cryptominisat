import sys
import copy
import random
from os.path import basename
import unittest

import os
print "our CWD is:", os.getcwd()

import pycryptosat
from pycryptosat import Solver

def check_clause(clause, solution) :
    for lit in clause:
        var = abs(lit)
        if lit < 0:
            inverted = True
        else :
            inverted = False

        if solution[var] != inverted :
            return True

def check_solution(clauses, solution) :
    for clause in clauses:
        if check_clause(clause, solution) == False :
            return False

    return True


# -------------------------- utility functions ---------------------------

#def read_cnf(path):
    #clauses = []
    #for line in open(path):
        #parts = line.split()
        #if not parts or parts[0] == 'c':
            #continue
        #if parts[0] == 'p':
            #assert len(parts) == 4
            #assert parts[1] == 'cnf'
            #n_vars, n_clauses = [int(n) for n in parts[2:4]]
            #continue
        #if parts[0] == '%':
            #break
        #assert parts[-1] == '0'
        #clauses.append([int(lit) for lit in parts[:-1]])
    #assert len(clauses) == n_clauses
    #return clauses, n_vars

#def process_cnf_file(path):
    #sys.stdout.write('%30s:  ' % basename(path))
    #sys.stdout.flush()

    #clauses, n_vars = read_cnf(path)
    #sys.stdout.write('vars: %6d   cls: %6d   ' % (n_vars, len(clauses)))
    #sys.stdout.flush()
    #n_sol = 0
    #for sol in itersolve(clauses):
        #sys.stdout.write('.')
        #sys.stdout.flush()
        #assert check_solution(clauses, sol)
        #n_sol += 1
    #sys.stdout.write("%d\n" % n_sol)
    #sys.stdout.flush()
    #return n_sol

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

class TestXor(unittest.TestCase) :
    def setUp(self) :
        self.solver = Solver(threads = 2);

    def test_wrong_args(self) :
        self.assertRaises(TypeError, self.solver.add_xor_clause, [1, 2])
        self.assertRaises(ValueError, self.solver.add_xor_clause, [1, 0], True)
        self.assertRaises(ValueError, self.solver.add_xor_clause, [-1, 2], True)

    def test_binary(self) :
        self.solver.add_xor_clause([1,2], False)
        res, solution = self.solver.solve([1])
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, True, True))

    def test_unit(self) :
        self.solver.add_xor_clause([1], False)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, False))

    def test_unit2(self) :
        self.solver.add_xor_clause([1], True)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, True))

    def test_3_long(self) :
        self.solver.add_xor_clause([1, 2, 3], False)
        res, solution = self.solver.solve([1, 2])
        self.assertEqual(res, True)
        #self.assertEqual(solution, (None, True, True, False))

    def test_3_long2(self) :
        self.solver.add_xor_clause([1, 2, 3], True)
        res, solution = self.solver.solve([1, -2])
        self.assertEqual(res, True)
        self.assertEqual(solution, (None, True, False, False))

    def test_long(self) :
        for l in range(10,30) :
            self.setUp()
            toadd = []
            toassume = []
            solution_expected = [None]
            for i in range(1,l) :
                toadd.append(i)
                solution_expected.append(False)
                if i != l-1 :
                    toassume.append(i*-1)

            self.solver.add_xor_clause(toadd, False)
            res, solution = self.solver.solve(toassume)
            self.assertEqual(res, True)
            self.assertEqual(solution, tuple(solution_expected))

class InitTester(unittest.TestCase):

    def test_wrong_args_to_solver(self):
        self.assertRaises(ValueError, Solver, threads = -1)
        self.assertRaises(ValueError, Solver, threads = 0)
        self.assertRaises(ValueError, Solver, verbose = -1)
        self.assertRaises(ValueError, Solver, confl_limit = -1)
        self.assertRaises(TypeError, Solver, threads = "fail")
        self.assertRaises(TypeError, Solver, verbose = "fail")
        self.assertRaises(TypeError, Solver, confl_limit = "fail")

class TestSolve(unittest.TestCase):
    def setUp(self) :
        self.solver = Solver(threads = 2)

    def test_wrong_args(self):
        self.assertRaises(TypeError, self.solver.add_clause, 'A')
        self.assertRaises(TypeError, self.solver.add_clause, 1)
        self.assertRaises(TypeError, self.solver.add_clause, 1.0)
        self.assertRaises(TypeError, self.solver.add_clause, object())
        self.assertRaises(TypeError, self.solver.add_clause, ['a'])
        self.assertRaises(TypeError, self.solver.add_clause, [[1, 2], [3, None]])
        self.assertRaises(ValueError, self.solver.add_clause, [1, 0])

    def test_no_clauses(self):
        for n in range(7):
            self.assertEqual(self.solver.solve([]), (True, (None,)))

    def test_cnf1(self):
        for cl in clauses1:
            self.solver.add_clause(cl)
        res, solution = self.solver.solve()
        self.assertEqual(res, True)
        self.assertTrue(check_solution(clauses1, solution))

    def test_bad_iter(self):
        class Liar:
            def __iter__(self): return None
        self.assertRaises(TypeError, self.solver.add_clause, Liar())

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
        for lim in range(1, 20):
            self.setUp()
            for cl in clauses1:
                self.solver.add_clause(cl)

            res, solution = self.solver.solve()
            self.assertTrue(res == None or check_solution(clauses1, solution))

# ------------------------------------------------------------------------

#def run():
    #print("sys.prefix: %s" % sys.prefix)
    #print("sys.version: %s" % sys.version)
    #try:
        #print("pycryptosat version: %r" % pycryptosat.__version__)
    #except AttributeError:
        #pass
    #suite = unittest.TestSuite()
    #suite.addTest(unittest.makeSuite(TestXor))
    #suite.addTest(unittest.makeSuite(TestSolve))

    #runner = unittest.TextTestRunner()
    #return runner.run(suite)

if __name__ == '__main__':
    unittest.main()

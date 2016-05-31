#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2016  Mate Soos
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

from __future__ import with_statement  # Required in 2.5
from __future__ import print_function
import optparse
import gzip
import re
import fnmatch

class solution_parser:
    def __init__(self):
        pass

    @staticmethod
    def parse_solution_from_output(output_lines, ignoreNoSolution = False):
        if len(output_lines) == 0:
            print("Error! SAT solver output is empty!")
            print("output lines: %s" % output_lines)
            print("Error code 500")
            exit(500)

        # solution will be put here
        satunsatfound = False
        vlinefound = False
        solution = {}
        conflict = None

        # parse in solution
        for line in output_lines:
            # skip comment
            if re.match('^conflict ', line):
                line = line.strip().split()[1:]
                conflict = [int(elem) for elem in line]
                continue

            if (re.match('^c ', line)):
                continue

            # solution
            if (re.match('^s ', line)):
                if (satunsatfound):
                    print("ERROR: solution twice in solver output!")
                    exit(400)

                if 'UNSAT' in line:
                    unsat = True
                    satunsatfound = True
                    continue

                if 'SAT' in line:
                    unsat = False
                    satunsatfound = True
                    continue

                print("ERROR: line starts with 's' but no SAT/UNSAT on line")
                exit(400)

            # parse in solution
            if (re.match('^v ', line)):
                vlinefound = True
                myvars = line.split(' ')
                for var in myvars:
                    var = var.strip()
                    if var == "" or var == 'v':
                        continue
                    if (int(var) == 0):
                        break
                    intvar = int(var)
                    solution[abs(intvar)] = (intvar >= 0)
        # print("Parsed values:", solution)

        if (ignoreNoSolution is False and
                (satunsatfound is False or (
                    unsat is False and vlinefound is False))):
            print("Error: Cannot find line starting with 's' or 'v' in output!")
            print(output_lines)
            print("Error code 500")
            exit(500)

        if (ignoreNoSolution is True and
                (satunsatfound is False or (
                    unsat is False and vlinefound is False))):
            print("Probably timeout, since no solution  printed. Could, of course, be segfault/assert fault, etc.")
            print("Making it look like an UNSAT, so no checks!")
            return (True, [])

        if (satunsatfound is False):
            print("Error: Cannot find if SAT or UNSAT. Maybe didn't finish running?")
            print(output_lines)
            print("Error code 500")
            exit(500)

        if (unsat is False and vlinefound is False):
            print("Error: Solution is SAT, but no 'v' line")
            print (output_lines)
            print("Error code 500")
            exit(500)

        return unsat, solution, conflict

    @staticmethod
    def check_regular_clause(line, solution):
            lits = line.split()
            final = False
            for lit in lits:
                numlit = int(lit)
                if numlit != 0:
                    if (abs(numlit) not in solution):
                        continue
                    if numlit < 0:
                        final |= ~solution[abs(numlit)]
                    else:
                        final |= solution[numlit]
                    if final is True:
                        break
            if final is False:
                print("Error: clause '%s' not satisfied." % line)
                print("Error code 100")
                exit(100)

    @staticmethod
    def check_xor_clause(line, solution):
        line = line.lstrip('x')
        lits = line.split()
        final = False
        for lit in lits:
            numlit = int(lit)
            if numlit != 0:
                if abs(numlit) not in solution:
                    print("Error: var %d not solved, but referred to in a xor-clause of the CNF" % abs(numlit))
                    print("Error code 200")
                    exit(200)
                final ^= solution[abs(numlit)]
                final ^= numlit < 0
        if final is False:
            print("Error: xor-clause '%s' not satisfied." % line)
            exit(-1)

    @staticmethod
    def test_found_solution(solution, fname, debugLibPart=None):
        if debugLibPart is None:
            print("Verifying solution for CNF file %s" % fname)
        else:
            print("Verifying solution for CNF file %s, part %d" %
                  (fname, debugLibPart))

        if fnmatch.fnmatch(fname, '*.gz'):
            f = gzip.open(fname, "r")
        else:
            f = open(fname, "r")
        clauses = 0
        thisDebugLibPart = 0

        for line in f:
            line = line.rstrip()

            # skip empty lines
            if len(line) == 0:
                continue

            # count debug lib parts
            if line[0] == 'c' and "Solver::solve" in line:
                thisDebugLibPart += 1

            # if we are over debugLibPart, exit
            if debugLibPart is not None and thisDebugLibPart >= debugLibPart:
                f.close()
                return

            # check solution against clause
            if line[0] != 'c' and line[0] != 'p':
                if line[0] != 'x':
                    solution_parser.check_regular_clause(line, solution)
                else:
                    solution_parser.check_xor_clause(line, solution)

                clauses += 1

        f.close()
        print("Verified %d original xor&regular clauses" % clauses)

def parse_arguments():
    class PlainHelpFormatter(optparse.IndentedHelpFormatter):

        def format_description(self, description):
            if description:
                return description + "\n"
            else:
                return ""

    usage = """usage: %prog solution cnf

For example:
%prog my_solution_file.out my_problem.cnf.gz"""
    parser = optparse.OptionParser(usage=usage, formatter=PlainHelpFormatter())
    parser.add_option("--verbose", "-v", action="store_true",
                      default=False, dest="verbose", help="Be more verbose")
    # parse options
    options, args = parser.parse_args()
    return options, args

if __name__ == "__main__":
    options, args = parse_arguments()
    print("Options are:", options)
    print("args are:", args)
    if len(args) != 2:
        print("ERROR: You must give exactly two parameters, one SOLUTION and one CNF")
        print("You gave {n} parameters".format(**{"n":len(args)}))
        exit(-1)

    sol_file = args[0]
    cnf_file = args[1]
    print("Verifying CNF file '{cnf}' against solution in file '{sol}'".format(
        **{"cnf":cnf_file, "sol":sol_file}))

    sol = {}
    with open(sol_file) as f:
        dat = f.read()

    dat = dat.split("\n")
    unsat, solution, conflict = solution_parser.parse_solution_from_output(dat)
    if unsat:
        print("Not checking UNSAT solution")
        exit(0)

    solution_parser.test_found_solution(solution, cnf_file)

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
from xor_to_cnf_class import *
from debuglib import *
import subprocess
import os
import stat
import time
import resource


def unique_file(fname_begin, fname_end=".cnf"):
        counter = 1
        while 1:
            fname = fname_begin + '_' + str(counter) + fname_end
            try:
                fd = os.open(
                    fname, os.O_CREAT | os.O_EXCL, stat.S_IREAD | stat.S_IWRITE)
                os.fdopen(fd).close()
                return fname
            except OSError:
                pass

            counter += 1

def setlimits():
    # sys.stdout.write("Setting resource limit in child (pid %d): %d s\n" %
    # (os.getpid(), options.maxtime))
    resource.setrlimit(resource.RLIMIT_CPU, (options.maxtime, options.maxtime))

class solution_parser:
    def __init__(self, options):
        self.options = options
        pass

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
                    solution_parser._check_regular_clause(line, solution)
                else:
                    solution_parser._check_xor_clause(line, solution)

                clauses += 1

        f.close()
        print("Verified %d original xor&regular clauses" % clauses)

    def check_unsat(self, fname):
        a = XorToCNF()
        tmpfname = unique_file("tmp_for_xor_to_cnf_convert")
        a.convert(fname, tmpfname)
        # execute with the other solver
        toexec = "lingeling -f %s" % tmpfname
        print("Solving with other solver: %s" % toexec)
        curr_time = time.time()
        try:
            p = subprocess.Popen(toexec.rsplit(),
                                 stdout=subprocess.PIPE,
                                 preexec_fn=setlimits)
        except OSError:
            print("ERROR: Probably you don't have lingeling installed!")
            raise

        consoleOutput2 = p.communicate()[0]
        os.unlink(tmpfname)

        # if other solver was out of time, then we can't say anything
        diff_time = time.time() - curr_time
        if diff_time > self.options.maxtime - self.options.maxtimediff:
            print("Other solver: too much time to solve, aborted!")
            return None

        # extract output from the other solver
        print("Checking other solver output...")
        otherSolverUNSAT, otherSolverSolution, _ = self.parse_solution_from_output(
            consoleOutput2.split("\n"))

        # check if the other solver agrees with us
        return otherSolverUNSAT

    def check_debug_lib(self, fname):
        largestPart = self._find_largest_debuglib_part(fname)
        for debugLibPart in range(1, largestPart + 1):
            fname_debug = "%s-debugLibPart%d.output" % (fname, debugLibPart)
            print("Checking debug lib part %s -- %s " % (debugLibPart, fname_debug))

            if (os.path.isfile(fname_debug) is False):
                print("Error: Filename to be read '%s' is not a file!" % fname_debug)
                exit(-1)

            # take file into mem
            f = open(fname_debug, "r")
            text = f.read()
            output_lines = text.splitlines()
            f.close()

            unsat, solution, conflict = self.parse_solution_from_output(output_lines)
            assumps = self._get_assumps(fname, debugLibPart)
            if unsat is False:
                print("debugLib is SAT")
                self._check_assumps_inside_solution(assumps, solution)
                self.test_found_solution(solution, fname, debugLibPart)
            else:
                print("debugLib is UNSAT")
                assert conflict is not None, "debugLibPart must create a conflict in case of UNSAT"
                self._check_assumps_inside_conflict(assumps, conflict)
                tmpfname = unique_file("tmp_for_extract_libpart")
                self._extract_lib_part(fname, debugLibPart, assumps, tmpfname)

                # check with other solver
                ret = self.check_unsat(tmpfname)
                if ret is None:
                    print("Cannot check, other solver took too much time")
                elif ret is True:
                    print("UNSAT verified by other solver")
                else:
                    print("Grave bug: SAT-> UNSAT : Other solver found solution!!")
                    exit(-1)
                os.unlink(tmpfname)

        self.remove_debuglib_files()

    def remove_debuglib_files(self):
        #removing debuglib files
        largestPart = self._find_largest_debuglib_part(fname)
        for debugLibPart in range(1, largestPart + 1):
            fname_debug = "%s-debugLibPart%d.output" % (fname, debugLibPart)
            os.unlink(fname_debug)

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

    def _extract_lib_part(self, fname, debug_num, assumps, tofile):
        fromf = open(fname, "r")
        thisDebugLibPart = 0
        maxvar = 0
        numcls = 0
        for line in fromf:
            line = line.strip()

            # ignore empty strings and headers
            if not line or line[0] == "p":
                continue

            # process (potentially special) comments
            if line[0] == "c":
                if "Solver::solve" in line:
                    thisDebugLibPart += 1

                continue

            # break out if we reached the debug lib part
            if thisDebugLibPart >= debug_num:
                break

            # count clauses and get max var number
            numcls += 1
            maxvar = max(maxvar, get_max_var_from_clause(line))

        fromf.close()

        # now we can create the new CNF file
        fromf = open(fname, "r")
        tof = open(tofile, "w")
        tof.write("p cnf %d %d\n" % (maxvar, numcls + len(assumps)))

        thisDebugLibPart = 0
        for line in fromf:
            line = line.strip()
            # skip empty lines and headers
            if not line or line[0] == "p":
                continue

            # parse up special header
            if line[0] == "c":
                if "Solver::solve" in line:
                    thisDebugLibPart += 1

                continue

            # break out if we reached the debug lib part
            if thisDebugLibPart >= debug_num:
                break

            tof.write(line + '\n')

        # add assumptions
        for lit in assumps:
            tof.write("%d 0\n" % lit)

        fromf.close()
        tof.close()

    def _get_assumps(self, fname, debugLibPart):
        f = open(fname, "r")

        thispart = 0
        solveline = None
        for line in f:
            if "Solver::solve" in line:
                thispart += 1
                if thispart == debugLibPart:
                    solveline = line
                    break
        f.close()

        assert solveline is not None
        ret = re.match("c.*Solver::solve\((.*)\)", solveline)
        assert ret is not None
        assumps = ret.group(1).strip().split()
        assumps = [int(x) for x in assumps]

        print("Assumptions: ", assumps)
        return assumps

    def _check_assumps_inside_conflict(self, assumps, conflict):
        for lit in conflict:
            if -1 * lit not in assumps:
                print("ERROR: Final conflict contains %s but assumps is %s" %(conflict, assumps))
                print("ERROR: lit ", lit, " is in conflict but its inverse is not is assumps!")
                exit(-100)

        print("OK, final conflict only contains elements from assumptions")

    def _check_assumps_inside_solution(self, assumps, solution):
        for lit in assumps:
            var = abs(lit)
            val = lit > 0
            if var in solution:
                if solution[var] != val:
                    print("Solution pinted has literal %s but assumptions contained the inverse: '%s'" % (-1 * lit, assumps))
                    exit(-100)

        print("OK, all assumptions inside solution")

    def _find_largest_debuglib_part(self, fname):
        largestPart = 0
        dirList2 = os.listdir(".")
        for fname_debug in dirList2:
            if fnmatch.fnmatch(fname_debug, "%s-debugLibPart*.output" % fname):
                largestPart += 1

        return largestPart

    @staticmethod
    def _check_regular_clause(line, solution):
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
    def _check_xor_clause(line, solution):
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
    parser.add_option("--tout", "-t", dest="maxtime", type=int, default=100,
                  help="Max time to run. Default: %default")
    parser.add_option("--textra", dest="maxtimediff", type=int, default=10,
                  help="Extra time on top of timeout for processing."
                  " Default: %default")
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

    print("Checking debug libs...")
    sol_parser = solution_parser(options)
    sol_parser.check_debug_lib(cnf_file)

    print("Checking console output...")
    sol = {}
    with open(sol_file) as f:
        dat = f.read()

    dat = dat.split("\n")
    unsat, solution, _ = sol_parser.parse_solution_from_output(dat)
    if not unsat:
        sol_parser.test_found_solution(solution, cnf_file)
        exit(0)

    # check with other solver
    ret = sol_parser.check_unsat(cnf_file)
    if ret is None:
        print("Other solver time-outed, cannot check")
    elif ret is True:
        print("UNSAT verified by other solver")
    else:
        print("Grave bug: SAT-> UNSAT : Other solver found solution!!")
        exit(-1)

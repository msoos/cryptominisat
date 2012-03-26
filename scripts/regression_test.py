#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import with_statement  # Required in 2.5
import subprocess
import os
import fnmatch
import gzip
import re
import commands
import getopt
import sys
import signal
import resource
import time
from subprocess import Popen, PIPE, STDOUT
#from optparse import OptionParser
import optparse

maxTime = 50
maxTimeLimit = 40

class PlainHelpFormatter(optparse.IndentedHelpFormatter):
    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog [options] --fuzz/--regtest/--checkdir/filetocheck"
desc = """Example usages:
* check already computed SAT solutions (UNSAT cannot be checked):
   python regression_test.py --checkdir ../../clusters/cluster93/
           --cnfdir ../../satcomp09/ -n 1

* check file 'MYFILE' multiple times for correct answer:
   python regression_test.py --file MYFILE --extraOptions="--nosubsume1 --noasymm"
           --numStart 20 --num 100"

* fuzz the solver with precosat as solution-checker:
   python regression_test.py --fuzz"""

parser = optparse.OptionParser(usage=usage, description=desc, formatter=PlainHelpFormatter())
parser.add_option("--numstart", dest="num_start_random_seed", default=0
                    , type = "int", metavar="SEEDSTART"
                    , help="Start randomize from this random seed. Default: %default"
                    )

parser.add_option("--num", dest="num_random_seeds", default=3
                    , type="int", metavar="SEEDS"
                    , help="Go through this many random seeds. Default: %default"
                    )

parser.add_option("--exec", metavar= "SOLVER", dest="solver"
                    , default="../build/cryptominisat"
                    , help="SAT solver executable. Default: %default"
                    )

parser.add_option("--extraopts", "-e", metavar= "OPTS", dest="extra_options"
                    , default=""
                    , help="Extra options to give to SAT solver"
                    )

parser.add_option("--verbose", "-v", action="store_true"
                    , default=False, dest="verbose"
                    , help="Print more output"
                    )

parser.add_option("-t", "--threads", dest="num_threads", metavar="NUM"
                    , default=1, type="int"
                    , help="Number of threads"
                    )

parser.add_option("--cnfdir", dest="dir_for_check"
                    , default=""
                    , help="Directory of CNF files checked against"
                    )

parser.add_option("-f", "--fuzz", dest="fuzz_test"
                    , default=False, action="store_true"
                    , help="Fuzz-test"
                    )

parser.add_option("--regtest", dest="regression_test"
                    , default=False, action="store_true"
                    , help="Regression test"
                    )
parser.add_option("--checkdir", metavar= "DIR", dest="testdir"
                    ,  help="Check solutions found here"
                    )
    #elif opt in ("--nodebuglib"):
    #self.needDebugLib = False

(options, args) = parser.parse_args()



def setlimits():
    sys.stderr.write("Setting resource limit in child (pid %d): %d s \n" % (os.getpid(), maxTime))
    resource.setrlimit(resource.RLIMIT_CPU, (maxTime, maxTime))

def unique_fuzz_file(file_name_begin):
    counter = 1
    while 1:
        file_name = file_name_begin + '_' + str(counter) + ".cnf"
        try:
            fd = os.open(file_name, os.O_CREAT | os.O_EXCL)
            return os.fdopen(fd), file_name
        except OSError:
            pass
        counter += 1

class Tester:
    def __init__(self):
        self.testDir = "../tests/"
        self.testDirNewVar = "../tests/newVar/"
        self.differentDirForCheck = \
            "/home/soos/Development/sat_solvers/satcomp09/"
        self.ignoreNoSolution = False
        self.needDebugLib = True

    def execute(self, fname, randomizeNum, newVar, needToLimitTime):
        if os.path.isfile(options.solver) != True:
            print "Error: Cannot find CryptoMiniSat executable. Searched in: '%s'" % \
                options.solver
            print "Error code 300"
            exit(300)

        command = "%s --randomize=%d " % (options.solver,randomizeNum)
        if (self.needDebugLib) :
            command += "--debuglib "
        if options.verbose == False:
            command += "--verbosity=0 "
        if (newVar) :
            command += "--debugnewvar "
        command += "--threads=%d " % options.num_threads
        command += options.extra_options + " "
        command += fname

        print "Executing: %s " % command
        #print time limit
        if options.verbose:
            print "CPU limit of parent (pid %d)" % os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)

        #if need time limit, then limit
        if (needToLimitTime) :
            p = subprocess.Popen(command.rsplit(), stdout=subprocess.PIPE, preexec_fn=setlimits)
        else:
            p = subprocess.Popen(command.rsplit(), stdout=subprocess.PIPE)


        #print time limit after child startup
        if options.verbose:
            print "CPU limit of parent (pid %d) after startup of child" % \
                os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)

        #Get solver output
        consoleOutput = p.communicate()[0]
        if options.verbose:
            print "CPU limit of parent (pid %d) after child finished executing" % \
                os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)

        return consoleOutput

    def parse_solution_from_output(self, output_lines):
        if len(output_lines) == 0:
            print "Error! SAT solver output is empty!"
            print "output lines: ", output_lines
            print "Error code 500"
            exit(500)


        #solution will be put here
        satunsatfound = False
        vlinefound = False
        value = {}

        #parse in solution
        for line in output_lines.split('\n'):
            #skip comment
            if (re.match('^c ', line)):
                continue;

            #solution
            if (re.match('^s ', line)):
                if (satunsatfound) :
                    print "ERROR: solution twice in solver output!"
                    exit(400)

                if 'UNSAT' in line:
                    unsat = True
                    satunsatfound = True
                    continue;

                if 'SAT' in line:
                    unsat = False
                    satunsatfound = True;
                    continue;

                print "ERROR: line starts with 's' but no SAT/UNSAT on line"
                exit(400)

            #parse in solution
            if (re.match('^v ', line)):
                vlinefound = True
                myvars = line.split(' ')
                for var in myvars:
                    if (var == 'v') : continue;
                    if (int(var) == 0) : break;
                    vvar = int(var)
                    value[abs(vvar)] = (vvar < 0) == False
        #print "Parsed values:", value

        if (self.ignoreNoSolution == False and
                (satunsatfound == False or (unsat == False and vlinefound == False))
                ):
            print "Error: Cannot find line starting with 's' or 'v' in output!"
            print output_lines
            print "Error code 500"
            exit(500)

        if (self.ignoreNoSolution == True and
                (satunsatfound == False or (unsat == False and vlinefound == False))
            ):
            print "Probably timeout, since no solution  printed. Could, of course, be segfault/assert fault, etc."
            print "Making it look like an UNSAT, so no checks!"
            return (True, [])

        if (satunsatfound == False) :
            print "Error: Cannot find if SAT or UNSAT. Maybe didn't finish running?"
            print output_lines
            print "Error code 500"
            exit(500)

        if (unsat == False and vlinefound == False) :
            print "Error: Solution is SAT, but no 'v' line"
            print output_lines
            print "Error code 500"
            exit(500)

        return (unsat, value)

    def check_regular_clause(self, line, value):
        lits = line.split()
        final = False
        for lit in lits:
            numlit = int(lit)
            if numlit != 0:
                if (abs(numlit) not in value): continue
                if numlit < 0:
                    final |= ~value[abs(numlit)]
                else:
                    final |= value[numlit]
                if final == True:
                    break
        if final == False:
            print "Error: clause '%s' not satisfied." % line
            print "Error code 100"
            exit(100)

    def check_xor_clause(self, line, value):
        line = line.lstrip('x')
        lits = line.split()
        final = False
        for lit in lits:
            numlit = int(lit)
            if numlit != 0:
                if abs(numlit) not in value:
                    print "Error: var %d not solved, but referred to in a xor-clause of the CNF" % abs(numlit)
                    print "Error code 200"
                    exit(200)
                final ^= value[abs(numlit)]
                final ^= numlit < 0
        if final == False:
            print "Error: xor-clause '%s' not satisfied." % line
            exit(-1)

    def test_found_solution(self, value, fname, debugLibPart=1000000):
        if debugLibPart == 1000000:
            print "Verifying solution for CNF file %s" % fname
        else:
            print "Verifying solution for CNF file %s, part %d" % (fname, debugLibPart)

        if fnmatch.fnmatch(fname, '*.gz'):
            f = gzip.open(fname, "r")
        else:
            f = open(fname, "r")
        clauses = 0
        thisDebugLibPart = 0

        for line in f:
            line = line.rstrip()

            #skip empty lines
            if len(line) == 0:
                continue

            #count debug lib parts
            if line[0] == 'c' and "Solver::solve()" in line:
                thisDebugLibPart += 1

            #if we are over, exit
            if thisDebugLibPart >= debugLibPart:
                f.close()
                return

            #check solution against clause
            if line[0] != 'c' and line[0] != 'p':
                if line[0] != 'x':
                    self.check_regular_clause(line, value)
                else:
                    self.check_xor_clause(line, value)

                clauses += 1

        f.close()
        print "Verified %d original xor&regular clauses" % clauses

    def check(self, fname, fnameCheck, randomizeNum=0, newVar=False,
              needSolve=True, needToLimitTime=False):

        consoleOutput = ""
        currTime = time.time()
        if needSolve:
            consoleOutput = self.execute(fname, randomizeNum, newVar, needToLimitTime)
        else:
            if (os.path.isfile(fname + ".out") == False) :
                print "ERROR! Solution file '%s' is not a file!" %(fname + ".out")
                exit(-1)
            f = open(fname + ".out", "r")
            consoleOutput = f.read()
            f.close()

        if needToLimitTime == True:
            diffTime = time.time() - currTime
            if diffTime > maxTimeLimit/options.num_threads:
                print "Too much time to solve, aborted!"
                return
            else:
                print "Not too much time: %f s" % (time.time() - currTime)

        print "filename: %s, random seed: %3d" % \
            (fname[:20], randomizeNum)

        if (self.needDebugLib) :
            largestPart = -1
            dirList2 = os.listdir(".")
            for fname_debug in dirList2:
                if fnmatch.fnmatch(fname_debug, "debugLibPart*.output"):
                    debugLibPart = int(fname_debug[fname_debug.index("t") + 1:fname_debug.rindex(".output")])
                    largestPart = max(largestPart, debugLibPart)

            for debugLibPart in range(1, largestPart + 1):
                fname_debug = "debugLibPart%d.output" % debugLibPart

                if (os.path.isfile(filename) == False) :
                    print "Error: Filename to be read '%s' is not a file!" % filename
                    print "Error code 400"
                    exit(400);
                f = open(fname_debug, "r")
                text = f.read()
                output_lines = text.splitlines()
                f.close()
                os.unlink(fname_debug)

                (unsat, value) = self.parse_solution_from_output(output_lines)
                if unsat == False:
                    self.test_found_solution(value, fnameCheck, debugLibPart)
                else:
                    print "Not examining part %d -- it is UNSAT" % (debugLibPart)

        print "Checking console output..."
        (unsat, value) = self.parse_solution_from_output(consoleOutput)
        otherSolverUNSAT = True
        if self.check_unsat and unsat:
            toexec = "../../lingeling-587f/lingeling %s" % fname
            print "Solving with other solver.."
            p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE,
                                 preexec_fn=setlimits)
            consoleOutput2 = p.communicate()[0]
            diffTime = time.time() - currTime
            if diffTime > maxTimeLimit:
                print "Other solver: too much time to solve, aborted!"
                return
            print "Checking other solver output..."
            (otherSolverUNSAT, otherSolverValue) = self.parse_solution_from_output(consoleOutput2)

        if unsat == True:
            if self.check_unsat == False:
                print "Cannot check -- output is UNSAT"
            else :
                if otherSolverUNSAT == False:
                    print "Grave bug: SAT-> UNSAT : Other solver found solution!!"
                    print "Console output: " , consoleOutput
                    exit()
                else:
                    print "UNSAT verified by other solver"

        else :
            self.test_found_solution(value, fnameCheck)

    def removeDebugLibParts(self) :
        dirList = os.listdir(".")
        for fname_unlink in dirList:
            if fnmatch.fnmatch(fname_unlink, 'debugLibPart*'):
                os.unlink(fname_unlink)
                None

    def fuzz_test(self) :
        fuzzers = ["build/cnf-fuzz-biere", "build/cnf-fuzz-nossum", "cnf-fuzz-brummayer.py"]
        directory = "../../cnf-utils/"
        for i in xrange(1,100000000):
            for fuzzer in fuzzers :
                fileopened, file_name = unique_fuzz_file("fuzzTest");
                fileopened.close()
                call = "{0}{1} > {2}".format(directory, fuzzer, file_name)
                out = commands.getstatusoutput(call)
                print "fuzzer ", fuzzer, " : ", out

                for seednum in range(options.num_start_random_seed, options.num_start_random_seed+options.num_random_seeds):
                    self.check(fname=file_name, fnameCheck=file_name,
                            randomizeNum=seednum, needToLimitTime=True)

                os.unlink(file_name)

    def checkDir(self) :
        self.ignoreNoSolution = True
        print "Checking already solved solutions"
        if testDirSet == False:
            print "When checking, you must give test dir"
            exit()
        else:
            print "You gave testdir (where solutions are):", self.testDir
        dirList = os.listdir(self.testDir)
        for fname in dirList:
            myMatch = ""
            if self.checkDirDifferent == True:
                myMatch = '*.cnf.gz.out'
            else:
                myMatch = '*.cnf.gz'
            if fnmatch.fnmatch(fname, myMatch):
                myDir = self.testDir
                if self.checkDirDifferent:
                    fname = fname[:len(fname) - 4]  #remove trailing .out
                    myDir = self.differentDirForCheck
                    self.check(fname=self.testDir + fname,
                               fnameCheck=myDir + fname, needSolve=False)

    def regression_test(self) :
        if testDirSet == False:
            dirList = os.listdir(self.testDirNewVar)
            if self.testDirNewVar == ".":
                self.testDirNewVar = ""
            for fname in dirList:
                if fnmatch.fnmatch(fname, '*.cnf.gz'):
                    for i in range(numStart, numStart+num):
                        self.check(fname=self.testDirNewVar + fname,
                                fnameCheck=self.testDirNewVar +
                                fname, randomizeNum=i, newVar=True)

        dirList = os.listdir(self.testDir)
        if self.testDir == ".":
            self.testDir = ""
        for fname in dirList:
            if fnmatch.fnmatch(fname, '*.cnf.gz'):
                for i in range(numStart, numStart+num):
                    self.check(fname=self.testDir + fname,
                               fnameCheck=self.testDir + fname,
                               randomizeNum=i, newVar=False)

    def checkFile(self, fname) :
        if os.path.isfile(fname) == False:
            print "Filename given '%s' is not a file!" % fname
            exit(-1)

        for seednum in range(options.num_start_random_seed, options.num_start_random_seed+options.num_random_seeds):
            print "Checking fname %s" % fname
            self.check(fname=fname, fnameCheck=fname, randomizeNum=seednum)

tester = Tester()

if len(args) == 1:
    print "Checking filename", args[0]
    tester.check_unsat = True
    tester.checkFile(args[0])


if (options.fuzz_test):
    tester.check_unsat = True
    tester.fuzz_test()

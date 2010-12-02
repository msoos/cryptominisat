#!/usr/bin/python
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

maxTime = 50
maxTimeLimit = 40

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

    sumTime = 0.0
    sumProp = 0
    verbose = False
    gaussUntil = 0
    testDir = "../tests/"
    testDirNewVar = "../tests/newVar/"
    cryptominisat = "../build/cryptominisat"
    speed = False
    checkDirDifferent = True
    differentDirForCheck = \
        "/home/soos/Development/sat_solvers/satcomp09/"
    ignoreNoSolution = False
    arminFuzzer = False
    extraOptions = ""
    needDebugLib = True
    numThreads = 4

    def __init__(self):
        self.sumTime = 0.0
        self.sumProp = 0
        self.verbose = False
        self.gaussUntil = 100
        self.testDir = "../tests/"
        self.testDirNewVar = "../tests/newVar/"
        self.cryptominisat = "../build/cryptominisat"
        self.checkDirOnly = False
        self.checkDirDifferent = False
        self.differentDirForCheck = \
            "/home/soos/Development/sat_solvers/satcomp09/"
        self.ignoreNoSolution = False
        self.arminFuzzer = False
        self.extraOptions = ""
        self.needDebugLib = True
        self.numThreads = 4

    def execute(self, fname, randomizeNum, newVar, needToLimitTime):
        if os.path.isfile(self.cryptominisat) != True:
            print "Error: Cannot find CryptoMiniSat executable. Searched in: '%s'" % \
                self.cryptominisat
            print "Error code 300"
            exit(300)

        command = "%s --randomize=%d " % (self.cryptominisat,randomizeNum)
        command += "--gaussuntil=%d " % self.gaussUntil
        if (self.needDebugLib) :
            command += "--debuglib "
        if self.verbose == False:
            command += "--verbosity=0 "
        if (newVar) :
            command += "--debugnewvar "
        command += "--threads=%d " % self.numThreads
        command += self.extraOptions + " "
        command += fname

        print "Executing: %s " % command
        if self.verbose:
            print "CPU limit of parent (pid %d)" % os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)
        if (needToLimitTime) :
            p = subprocess.Popen(command.rsplit(), stdout=subprocess.PIPE, preexec_fn=setlimits)
        else:
            p = subprocess.Popen(command.rsplit(), stdout=subprocess.PIPE)
        if self.verbose:
            print "CPU limit of parent (pid %d) after startup of child" % \
                os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)
        consoleOutput = p.communicate()[0]
        if self.verbose:
            print "CPU limit of parent (pid %d) after child finished executing" % \
                os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)

        return consoleOutput

    def parse_consoleOutput(self, consoleOutput):
        s3 = consoleOutput.splitlines()
        for l in s3:
            if "CPU time" in l:
                self.sumTime += float(l[l.index(":") + 1:l.rindex(" s")])
            if "propagations" in l:
                self.sumProp += int(l[l.index(":") + 1:l.rindex("(")])

    def read_found(self, filename):
        if (os.path.isfile(filename) == False) :
            print "Error: Filename to be read '%s' is not a file!" % filename
            print "Error code 400"
            exit(400);
        f = open(filename, "r")
        text = f.read()
        mylines = text.splitlines()
        f.close()

        if len(mylines) == 0:
            print "Error! CryptoMiniSat output is empty!"
            print "output lines: ", mylines
            print "Error code 500"
            exit(500)

        satunsatfound = False;

        value = {}
        for line in mylines:
            if 'UNSAT' in line:
                unsat = True
                satunsatfound = True
                continue;
            if 'SAT' in line:
                unsat = False
                satunsatfound = True;
                continue;
            if (re.match('^c ', line)):  continue;
            myvars = line.split(' ')
            for var in myvars:
                if (var == 'v') : continue;
                if (int(var) == 0) : break;
                vvar = int(var)
                value[abs(vvar)] = (vvar < 0) == False
        #print "Parsed values:", value
        if (satunsatfound == False) :
            print "Error: Cannot find if SAT or UNSAT. Maybe didn't finish running?"
            print "Error code 500"
            exit(500)

        os.unlink(filename)
        return (unsat, value)

    def read_found_output(self, output):
        lines = output.splitlines()

        if len(lines) == 0:
            if self.checkDirOnly == True:
                print "Solving probably timed out"
                return (True, {})
            else:
                print "Error! Output is empty!"
                print "output : ", output
                print "Error code 500"
                exit(500)

        value = {}
        unsat = False
        sLineFound = False
        vLineFound = False
        for line in lines:
            if line == 's UNSATISFIABLE':
                unsat = True
                sLineFound = True
            elif line == 's SATISFIABLE':
                unsat = False
                sLineFound = True
            elif re.match('^v ', line):
                vars = line.split(' ')
                vLineFound = True
                for var in vars:
                    if var == "v":
                        continue
                    vvar = int(var)
                    value[abs(vvar)] = (vvar < 0) == False

        if self.ignoreNoSolution == False and (sLineFound == False or
                unsat == False and vLineFound == False):
            print "Error: Cannot find line starting with 's' or 'v' in output!"
            print output
            print "Error code 500"
            exit(500)

        if self.ignoreNoSolution == True and (sLineFound == False or
                unsat == False and vLineFound == False):
            print "Probably timeout, since no solution  printed. Could, of course, be segfault/assert fault, etc."
            print "Making it look like an UNSAT, so no checks!"
            return (True, [])

        if self.verbose:
            print "Found sat ? ",  (not unsat)
            #for (k, v) in value.iteritems():
            #    print "var: %d, value: %s" % (k, v)

        return (unsat, value)

    def test_expect(self, unsat, value, expectOutputFile):
        if os.path.isfile(expectOutputFile) != True:
            return

        f = gzip.open(expectOutputFile, "r")
        text = f.read()
        f.close()

        indicated_value = {}
        indicated_unsat = False
        lines = text.splitlines()
        for line in lines:
            if 'UNSAT' in line:
                indicated_unsat = True
            elif 'SAT' in line:
                indicated_unsat = False
            else:
                stuff = line.split()
                indicated_value[int(stuff[0])] = stuff[1].lstrip().rstrip() == \
                    'true'

        if unsat != indicated_unsat:
            print "Error: UNSAT vs. SAT problem!"
            print "Error code 700"
            exit(700)
        else:
            for (k, v) in indicated_value.iteritems():
                if indicated_value[k] != value[k]:
                    print "Error: Problem of found values: values %d: '%s', '%s' don't match with those pre-indicated in solution file" % \
                        (k, value[k], indicated_value[k])
                    print "Error code 800"
                    exit(800)

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

    def test_found(self, unsat, value, fname, debugLibPart=1000000):
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
            if len(line) == 0:
                continue
            if line[0] == 'c' and "Solver::solve()" in line:
                thisDebugLibPart += 1
            if thisDebugLibPart >= debugLibPart:
                f.close()

                return
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
        None
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
            if diffTime > maxTimeLimit/self.numThreads:
                print "Too much time to solve, aborted!"
                return
            else:
                print "Not too much time: %d s" % (time.time() - currTime)

        self.parse_consoleOutput(consoleOutput)
        print "filename: %20s, exec: %3d, total props: %10d total time:%.2f" % \
            (fname[:20] + "....cnf.gz", randomizeNum, self.sumProp, self.sumTime)

        if (self.needDebugLib) :
            largestPart = -1
            dirList2 = os.listdir(".")
            for fname_debug in dirList2:
                if fnmatch.fnmatch(fname_debug, "debugLibPart*.output"):
                    debugLibPart = int(fname_debug[fname_debug.index("t") + 1:fname_debug.rindex(".output")])
                    largestPart = max(largestPart, debugLibPart)

            for debugLibPart in range(1, largestPart + 1):
                fname_debug = "debugLibPart%d.output" % debugLibPart

                (unsat, value) = self.read_found(fname_debug)
                if unsat == False:
                    self.test_found(unsat, value, fnameCheck, debugLibPart)
                else:
                    print "Not examining part %d -- it is UNSAT" % (debugLibPart)

        print "Checking console output..."
        (unsat, value) = self.read_found_output(consoleOutput)
        otherSolverUNSAT = True
        if self.arminFuzzer and unsat:
            toexec = "./precosat %s" % fname
            None
            print "Solving with other solver.."
            p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE,
                                 preexec_fn=setlimits)
            consoleOutput2 = p.communicate()[0]
            diffTime = time.time() - currTime
            if diffTime > maxTimeLimit:
                print "Other solver: too much time to solve, aborted!"
                return
            print "Checking other solver output..."
            (otherSolverUNSAT, otherSolverValue) = self.read_found_output(consoleOutput2)
        if unsat == True:
            if self.arminFuzzer == False:
                print "Cannot check -- output is UNSAT"
            elif self.arminFuzzer == True:
                if otherSolverUNSAT == False:
                    print "Grave bug: SAT-> UNSAT : Other solver found solution!!"
                    print "Console output: " , consoleOutput
                    exit()
                else:
                    print "UNSAT verified by other solver"
        self.test_expect(unsat, value, fname[:len(fname) - 6] + "output.gz")
        if unsat == False:
            self.test_found(unsat, value, fnameCheck)

    @staticmethod
    def usage():
        print "--num     (-n)     The number of times to randomize and solve the same instance. Default: 3"
        print "--numStart         The rand number we should start at (default 0)"
        print "--extraOptions     Add this as extra options to the solver (e.g. \"--novarelim\")"
        print "--verbose (-v)     Verbose output"
        print "--file    (-f)     The file to solve. Default: all files under ../tests/"
        print "--gauss   (-g)     Execute gaussian elimination until this depth. Default: 10000"
        print "--testdir (-t)     The directory where the files to test are. Default: \"../tests/\""
        print "--exe     (-e)     Where the cryptominisat executable is located. Default: \"../build/cryptominisat\""
        print "--speed   (-s)     Only solve, don't verify the result"
        print "--checkDirOnly(-c) Check all solutions in directory"
        print "--diffCheckDir(-d) Use with -c. The original files are at a different place"
        print "--ignore  (-i)     If no solution found, (timeout), ignore"
        print "--armin   (-a)     Use Armin Biere's fuzzer"
        print "--threads          Use this number of threads (default: 4)"
        print "--help    (-h)     Print this help screen"
        print ""
        print "Example usage:"
        print "1) check already computed SAT solutions (UNSAT cannot be checked):"
        print "   python regression_test.py -c -t ../../clusters/cluster93/ -d ../../satcomp09/ \\"
        print "        --ignore -n 1"
        print ""
        print "2) check file 'MYFILE' multiple times for correct answer:"
        print "   python regression_test.py --file MYFILE --extraOptions=\"--nosubsume1 --noasymm\" \\"
        print "       --numStart 20 --num 100"
        print ""
        print "3) fuzz the solver with precosat as solution-checker:"
        print "   python regression_test.py --armin -n 1"


    def main(self):
        try:
            (opts, args) = getopt.getopt((sys.argv)[1:],
                    "vscihag:n:f:t:e:d:", [
                "help",
                "checkDirOnly",
                "file=",
                "numStart=",
                "num=",
                "gauss=",
                "testdir=",
                "exe=",
                "speed",
                "verbose",
                "diffCheckDir",
                "ignore",
                "armin",
                "extraOptions=",
                "threads="
                ])
        except getopt.GetoptError, err:
            print str(err)
            self.usage()
            sys.exit(2)

        fname = None
        debugLib = False
        numStart = 0
        num = 3
        testDirSet = False
        for (opt, arg) in opts:
            if opt in ("-v", "--verbose"):
                self.verbose = True
            elif opt in ("-h", "--help"):
                self.usage()
                sys.exit()
            elif opt in ("-f", "--file"):
                fname = arg
            elif opt in ("-n", "--num"):
                num = int(arg)
            elif opt in ("--numStart"):
                numStart = int(arg)
            elif opt in ("-g", "--gauss"):
                self.gaussUntil = int(arg)
            elif opt in ("-t", "--testdir"):
                self.testDir = arg
                testDirSet = True
            elif opt in ("-e", "--exe"):
                self.cryptominisat = arg
            elif opt in ("-s", "--speed"):
                self.speed = True
            elif opt in ("--extraOptions"):
                self.extraOptions = arg
            elif opt in ("-c", "--checkDirOnly"):
                self.checkDirOnly = True
            elif opt in ("-d", "--diffCheckDir"):
                self.differentDirForCheck = arg
                self.checkDirDifferent = True
            elif opt in ("-i", "--ignore"):
                self.ignoreNoSolution = True
            elif opt in ("-a", "--armin"):
                self.arminFuzzer = True
                self.needDebugLib = False
            elif opt in ("--threads"):
                self.numThreads = int(arg)
            else:
                assert False, "unhandled option"

        dirList2 = os.listdir(".")
        for fname_unlink in dirList2:
            if fnmatch.fnmatch(fname_unlink, 'debugLibPart*'):
                os.unlink(fname_unlink)
                None

        if self.arminFuzzer:
            i = 0

            while i < 100000000:
                fileopened, file_name = unique_fuzz_file("fuzzTest");
                fileopened.close()
                commands.getstatusoutput("./fuzzsat > %s" %(file_name))

                for i3 in range(num):
                    self.check(fname=file_name, fnameCheck=file_name,
                               randomizeNum=i3, needToLimitTime=True)

                os.unlink(file_name)
                fileopened, file_name = unique_fuzz_file("fuzzTest");
                fileopened.close()
                commands.getstatusoutput("./cnffuzz > %s" % (file_name))

                for i3 in range(num):
                    self.check(fname=file_name, fnameCheck=file_name,
                               randomizeNum=i3, needToLimitTime=True)

                    i = i + 1
                os.unlink(file_name)

            exit()

        if self.checkDirOnly:
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
            exit()

        if fname == None:
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
        else:

            if os.path.isfile(fname) == False:
                print "Filename given '%s' is not a file!" % fname
                exit(-1)

            for i in range(numStart, numStart+num):
                print "Checking fname %s" % fname
                self.check(fname=fname, fnameCheck=fname, randomizeNum=i)

test = Tester()
test.main()
print "Everything went ok."
print "Exit code 0"
exit(0)

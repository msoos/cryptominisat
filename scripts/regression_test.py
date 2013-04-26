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
import struct
import random
from random import choice
from subprocess import Popen, PIPE, STDOUT
#from optparse import OptionParser
import optparse

maxTime = 300
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
   ./regression_test.py --checkdir ../../clusters/cluster93/
           --cnfdir ../../satcomp09/ -n 1

* check already computed SAT solutions (UNSAT cannot be checked):
   ./regression_test.py --check myfile.cnf
           --solution sol.txt

* fuzz the solver with precosat as solution-checker:
   ./regression_test.py --fuzz

* go through regression listdir
   ./regression_test.py --regtest --checkdir ../tests/
"""

parser = optparse.OptionParser(usage=usage, description=desc, formatter=PlainHelpFormatter())
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

#for fuzz-testing
parser.add_option("-f", "--fuzz", dest="fuzz_test"
                    , default=False, action="store_true"
                    , help="Fuzz-test"
                    )

#for regression testing
parser.add_option("--regtest", dest="regressionTest"
                    , default=False, action="store_true"
                    , help="Regression test"
                    )
parser.add_option("--testdir", dest="testDir"
                    , default= "../tests/"
                    , help="Directory where the tests are"
                    )

parser.add_option("--testdirNewVar", dest="testDirNewVar"
                    , default= "../tests/newVar/"
                    , help="Directory where the tests are"
                    )

parser.add_option("--drup", dest="drup"
                    , default= False, action="store_true"
                    , help="Directory where the tests are"
                    )

#check dir stuff
parser.add_option("--checksol", dest="checkSol"
                    , default=False, action="store_true"
                    , help="Check solution at specified dir against problems at specified dir"
                    )

parser.add_option("--soldir", dest="checkDirSol"
                    ,  help="Check solutions found here"
                    )
parser.add_option("--probdir", dest="checkDirProb"
                    , default="/home/soos/media/sat/examples/satcomp09/"
                    , help="Directory of CNF files checked against"
                    )
parser.add_option("--check", dest="checkFile"
                    , default=None
                    , help="Check this file"
                    )
parser.add_option("--sol", dest="solutionFile"
                    , default=None
                    , help="Against this solution"
                    )

(options, args) = parser.parse_args()



def setlimits():
    sys.stdout.write("Setting resource limit in child (pid %d): %d s\n" % (os.getpid(), maxTime))
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
        self.check_unsat = True
        self.testDir = options.testDir
        self.testDirNewVar = options.testDirNewVar

        self.ignoreNoSolution = False
        self.needDebugLib = True

    def random_options(self) :
        cmd = " "

        cmd += "--clbtwsimp %s " % random.randint(0,3)
        cmd += "--restart %s " % random.choice(["geom", "agility", "glue", "glueagility"])
        cmd += "--agilviollim %s " % random.randint(0,40)
        cmd += "--gluehist %s " % random.randint(1,500)
        cmd += "--updateglue %s " % random.randint(0,1)
        cmd += "--otfhyper %s " % random.randint(0,1)
        cmd += "--clean %s " % random.choice(["size", "glue", "activity", "propconfl"])
        cmd += "--preclean %s " % random.randint(0,1)
        cmd += "--precleanlim %s " % random.randint(0,10)
        cmd += "--precleantime %s " % random.randint(0,20000)
        cmd += "--clearstat %s " % random.randint(0,1)
        cmd += "--startclean %s " % random.randint(0,16000)
        cmd += "--maxredratio %s " % random.randint(2,20)
        cmd += "--dompickf %s " % random.randint(1,20)
        cmd += "--flippolf %s " % random.randint(1,3000)
        cmd += "--alwaysmoremin %s " % random.randint(0,1)
        cmd += "--rewardotfsubsume %s " % random.randint(0,100)
        cmd += "--bothprop %s " % random.randint(0,1)
        cmd += "--probemultip %s " % random.randint(0,10)
        cmd += "--cachesize %s " % random.randint(10, 100)
        cmd += "--calcreach %s " % random.randint(0,1)
        cmd += "--cachecutoff %s " % random.randint(0,2000)
        cmd += "--elimstrategy %s " % random.randint(0,1)
        cmd += "--elimcomplexupdate %s " % random.randint(0,1)
        cmd += "--occlearntmax %s " % random.randint(0,100)
        cmd += "--asymmte %s " % random.randint(0,1)
        cmd += "--noextbinsubs %s " % random.randint(0,1)
        cmd += "--extscc %s " % random.randint(0,1)
        cmd += "--vivif %s " % random.randint(0,1)
        cmd += "--sortwatched %s " % random.randint(0,1)
        cmd += "--recur %s " % random.randint(0,1)
        cmd += "--compsfrom %d " % random.randint(0,2)
        cmd += "--compsvar %d " % random.randint(20000,500000)
        cmd += "--compslimit %d " % random.randint(0,3000)
        cmd += "--preschedsimp %s " % random.randint(0,1)
        cmd += "--implicitmanip %s " % random.randint(0,1)

        #the most buggy ones, don't turn them off much, please
        if random.randint(0,1) == 1 :
            cmd += "--scc %s " % random.randint(0,1)
            cmd += "--schedsimplify %d " % random.randint(0,1)
            cmd += "--varelim %s " % random.randint(0,1)
            cmd += "--comps %s " % random.randint(0,1)
            cmd += "--subsume1 %s " % random.randint(0,1)
            cmd += "--block %s " % random.randint(0,1)
            cmd += "--probe %s " % random.randint(0,1)
            cmd += "--simplify %s " % random.randint(0,1)
            cmd += "--binpri %s " % random.randint(0,1)
            cmd += "--stamp %s " % random.randint(0,1)
            cmd += "--cache %s " % random.randint(0,1)
            cmd += "--otfsubsume %s " % random.randint(0,1)
            cmd += "--renumber %s " % random.randint(0,1)
            cmd += "--moreminim %s " % random.randint(0,1)



        return cmd

    def execute(self, fname, newVar=False, needToLimitTime=False, fnameDrup=None):
        if os.path.isfile(options.solver) != True:
            print "Error: Cannot find CryptoMiniSat executable. Searched in: '%s'" % \
                options.solver
            print "Error code 300"
            exit(300)

        #construct command
        command = options.solver
        command += self.random_options()
        if self.needDebugLib :
            command += "--debuglib "
        if options.verbose == False:
            command += "--verb 0 "
        if newVar :
            command += "--debugnewvar "
        command += "--threads=%d " % options.num_threads
        command += options.extra_options + " "
        command += fname
        if fnameDrup:
            command += " " + fnameDrup
        print "Executing: %s " % command

        #print time limit
        if options.verbose:
            print "CPU limit of parent (pid %d)" % os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)

        #if need time limit, then limit
        if (needToLimitTime) :
            p = subprocess.Popen(command.rsplit(), stderr=subprocess.STDOUT, stdout=subprocess.PIPE, preexec_fn=setlimits)
        else:
            p = subprocess.Popen(command.rsplit(), stderr=subprocess.STDOUT, stdout=subprocess.PIPE)


        #print time limit after child startup
        if options.verbose:
            print "CPU limit of parent (pid %d) after startup of child" % \
                os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)

        #Get solver output
        consoleOutput, err = p.communicate()
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
        for line in output_lines:
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

    def check(self, fname, fnameCheck, fnameDrup=None, newVar=False,
              needSolve=True, needToLimitTime=False):

        consoleOutput = ""
        currTime = time.time()

        #Do we need to solve the problem, or is it already solved?
        if needSolve:
            consoleOutput = self.execute(fname, newVar, needToLimitTime, fnameDrup=fnameDrup)
        else:
            if (os.path.isfile(fname + ".out") == False) :
                print "ERROR! Solution file '%s' is not a file!" %(fname + ".out")
                exit(-1)
            f = open(fname + ".out", "r")
            consoleOutput = f.read()
            f.close()

        #if time was limited, we need to know if we were over the time limit
        #and that is why there is no solution
        if needToLimitTime:
            diffTime = time.time() - currTime
            if diffTime > maxTimeLimit/options.num_threads:
                print "Too much time to solve, aborted!"
                return
            else:
                print "Within time limit: %f s" % (time.time() - currTime)

        print "filename: %s" % fname[:20]

        if (self.needDebugLib) :
            largestPart = -1
            dirList2 = os.listdir(".")
            for fname_debug in dirList2:
                if fnmatch.fnmatch(fname_debug, "debugLibPart*.output"):
                    debugLibPart = int(fname_debug[fname_debug.index("t") + 1:fname_debug.rindex(".output")])
                    largestPart = max(largestPart, debugLibPart)

            for debugLibPart in range(1, largestPart + 1):
                fname_debug = "debugLibPart%d.output" % debugLibPart
                print "Checking debug lib part ", debugLibPart

                if (os.path.isfile(fname_debug) == False) :
                    print "Error: Filename to be read '%s' is not a file!" % fname_debug
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
        (unsat, value) = self.parse_solution_from_output(consoleOutput.split("\n"))
        otherSolverUNSAT = True
        if self.check_unsat and unsat:
            toexec = "../../lingeling-587f/lingeling -f %s" % fname
            print "Solving with other solver.."
            currTime = time.time()
            p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE,
                                 preexec_fn=setlimits)
            consoleOutput2 = p.communicate()[0]
            diffTime = time.time() - currTime
            if diffTime > maxTimeLimit:
                print "Other solver: too much time to solve, aborted!"
                return
            print "Checking other solver output..."
            (otherSolverUNSAT, otherSolverValue) = self.parse_solution_from_output(consoleOutput2.split("\n"))

        if unsat == True:
            if fnameDrup:
                toexec = "drupcheck %s %s" % (fname, fnameDrup)
                print "Checking DRUP...: ", toexec
                p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE)
                                     #,preexec_fn=setlimits)
                consoleOutput2 = p.communicate()[0]
                diffTime = time.time() - currTime
                foundVerif = False
                drupLine = ""
                for line in consoleOutput2.split('\n') :
                    if len(line) > 1 and line[:2] == "s " :
                        #print "verif: " , line
                        foundVerif = True
                        if line[2:10] != "VERIFIED" and line[2:] != "TRIVIAL UNSAT" :
                            print "DRUP verification error, it says:", consoleOutput2
                        assert line[2:10] == "VERIFIED" or line[2:] == "TRIVIAL UNSAT", "DRUP didn't verify problem!"
                        drupLine = line

                if foundVerif == False:
                    print "verifier error! It says:", consoleOutput2
                    assert foundVerif, "Cannot find DRUP verification code!"
                else:
                    print "OK, DRUP says:", drupLine


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

    def callFromFuzzer(self, directory, fuzzer, file_name) :
        if (len(fuzzer) == 1) :
            call = "{0}{1} > {2}".format(directory, fuzzer[0], file_name)
        elif(len(fuzzer) == 2) :
            seed = struct.unpack("<L", os.urandom(4))[0]
            call = "{0}{1} {2} {3} > {4}".format(directory, fuzzer[0], fuzzer[1], seed, file_name)
        elif(len(fuzzer) == 3) :
            seed = struct.unpack("<L", os.urandom(4))[0]
            hashbits = (random.getrandbits(20) % 79) + 1
            call = "%s %s %d %s %d > %s" % (fuzzer[0], fuzzer[1], hashbits, fuzzer[2], seed, file_name)

        return call

    def fuzz_test(self) :
        fuzzers = [
            ["../../sha1-sat/build/sha1-gen --attack preimage --rounds 18 --cnf", "--hash-bits", "--seed"] \
            , ["build/cnf-fuzz-biere"] \
            #, ["build/cnf-fuzz-nossum"] \
            #, ["build/largefuzzer"] \
            , ["cnf-fuzz-brummayer.py"] \
            , ["multipart.py", "special"] \
            , ["build/sgen4 -unsat -n 50", "-s"] \
            #, ["build/sgen4 -sat -n 50", "-s"] \
        ]

        directory = "../../cnf-utils/"
        while True:
            for fuzzer in fuzzers :
                fileopened, file_name = unique_fuzz_file("fuzzTest");
                fileopened.close()
                fnameDrup = None
                if options.drup :
                    fnameDrup = "drupcheck"

                #should the multi-fuzzer be called?
                if len(fuzzer) == 2 and fuzzer[1] == "special":
                    #create N files
                    file_names_multi = []

                    #sometimes just fuzz with all SAT problems
                    if random.getrandbits(1)  == 1 :
                        fixed = 1
                    else:
                        fixed = 0

                    for i in range(random.randrange(2,4)) :
                        fileopened2, file_name2 = unique_fuzz_file("fuzzTest");
                        fileopened2.close()
                        file_names_multi.append(file_name2)

                        #chose a ranom fuzzer, not multipart
                        fuzzer2 = ["multipart.py", "special"]
                        while (fuzzer2[0] == "multipart.py") :
                            fuzzer2 = choice(fuzzers)

                        #sometimes fuzz with SAT problems only
                        if (fixed) :
                            fuzzer2 = fuzzers[0]

                        print "fuzzer2 used: ", fuzzer2
                        call = self.callFromFuzzer(directory, fuzzer2, file_name2)
                        print "calling sub-fuzzer:", call
                        out = commands.getstatusoutput(call)

                    #construct multi-fuzzer call
                    call = ""
                    call += directory
                    call += fuzzer[0]
                    call += " "
                    for name in file_names_multi :
                        call += " " + name
                    call += " > " + file_name

                else :
                    call = self.callFromFuzzer(directory, fuzzer, file_name)

                print "calling ", fuzzer, " : ", call
                out = commands.getstatusoutput(call)
                self.check(fname=file_name, fnameCheck=file_name, fnameDrup=fnameDrup,
                            needToLimitTime=True)

                os.unlink(file_name)

    def checkDir(self) :
        self.ignoreNoSolution = True
        print "Checking already solved solutions"

        #check if options.checkDir has bee set
        if options.checkDir == "":
            print "When checking, you must give test dir"
            exit()

        print "You gave testdir (where solutions are):", options.checkDirSol
        print "You gave CNF dir (where problems are) :", options.checkDirProb

        dirList = os.listdir(options.checkDir)
        for fname in dirList:

            if fnmatch.fnmatch(fname, '*.cnf.gz.out'):
                #add dir, remove trailing .out
                fnameCheck = options.checkDirProb + fname[:len(fname) - 4]

                #check now
                self.check(fname=self.testDir + fname, \
                   fnameCheck=fnameCheck, needSolve=False)

    def regressionTest(self) :

        if False:
            #first, test stuff with newVar
            dirList = os.listdir(self.testDirNewVar)
            for fname in dirList:
                if fnmatch.fnmatch(fname, '*.cnf.gz'):
                    self.check(fname=self.testDirNewVar + fname,
                                fnameCheck=self.testDirNewVar +
                                fname, newVar=True)

        dirList = os.listdir(self.testDir)

        #test stuff without newVar
        for fname in dirList:
            if fnmatch.fnmatch(fname, '*.cnf.gz'):
                self.check(fname=self.testDir + fname,
                fnameCheck=self.testDir + fname,
                newVar=False)

    def checkFile(self, problem, solution) :
        if os.path.isfile(problem) == False:
            print "Filename given '%s' is not a file!" % problem
            exit(-1)

        print "Checking CNF file '%s' against proposed solution '%s' " % (problem, solution)
        #self.check(fname=problem, fnameCheck=problem)
        output_lines = open(solution).readlines()
        (unsat, value) = self.parse_solution_from_output(output_lines)
        if not unsat:
            self.test_found_solution(value, problem)
        else:
            print "Cannot check, UNSAT"

tester = Tester()

if len(args) == 1:
    print "Checking filename", args[0]
    tester.check_unsat = True
    tester.checkFile(args[0])

if options.checkFile :
    tester.checkFile(options.checkFile, options.solutionFile)

if options.fuzz_test:
    tester.needDebugLib = False
    tester.check_unsat = True
    tester.fuzz_test()

if options.checkSol:
    tester.checkDir()

if options.regressionTest:
    tester.regressionTest()

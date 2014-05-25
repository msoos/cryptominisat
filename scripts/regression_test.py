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
import calendar
import glob
from xor_to_cnf_class import *

maxTime = 80
maxTimeDiff = 20

class PlainHelpFormatter(optparse.IndentedHelpFormatter):
    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog [options] --fuzz/--regtest/--checkdir/filetocheck"
desc = """Example usages:
* check already computed SAT solutions (UNSAT cannot be checked):
   ./regression_test.py --checkdir ../../clusters/cluster93/ --cnfdir ../../satcomp09/

* check already computed SAT solutions (UNSAT cannot be checked):
   ./regression_test.py -c myfile.cnf -s sol.txt

* fuzz the solver with fuzz-generator
   ./regression_test.py -f

* go through regression listdir
   ./regression_test.py --regtest --checkdir ../tests/cnfs/
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
                    , default= "../tests/cnfs/"
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
parser.add_option("-c", "--check", dest="checkFile"
                    , default=None
                    , help="Check this file"
                    )
parser.add_option("-s", "--sol", dest="solutionFile"
                    , default=None
                    , help="Against this solution"
                    )

(options, args) = parser.parse_args()



def setlimits():
    #sys.stdout.write("Setting resource limit in child (pid %d): %d s\n" % (os.getpid(), maxTime))
    resource.setrlimit(resource.RLIMIT_CPU, (maxTime, maxTime))

def unique_fuzz_file(file_name_begin):
    counter = 1
    while 1:
        file_name = file_name_begin + '_' + str(counter) + ".cnf"
        try:
            fd = os.open(file_name, os.O_CREAT | os.O_EXCL)
            os.fdopen(fd).close()
            return file_name
        except OSError:
            pass

        counter += 1

def file_exists(fname) :
    try:
       with open(fname):
           return True
    except IOError:
       return False

class Tester:
    def __init__(self):
        self.check_unsat = False
        self.ignoreNoSolution = False
        self.fuzzers = [
            ["../../sha1-sat/build/sha1-gen --attack preimage --rounds 18 --cnf", "--hash-bits", "--seed"] \
            , ["../../sha1-sat/build/sha1-gen --xor --attack preimage --rounds 18 --cnf", "--hash-bits", "--seed"] \
            , ["build/cnf-fuzz-biere"] \
            #, ["build/cnf-fuzz-nossum"] \
            #, ["build/largefuzzer"] \
            , ["cnf-fuzz-brummayer.py"] \
            , ["multipart.py", "special"] \
            , ["build/sgen4 -unsat -n 50", "-s"] \
            , ["cnf-fuzz-xor.py"] \
            , ["build/sgen4 -sat -n 50", "-s"] \
        ]
        self.fuzzer_directory = "../../cnf-utils/"

    def random_options(self) :
        cmd = " "

        cmd += "--clbtwsimp %s " % random.randint(0,3)
        cmd += "--restart %s " % random.choice(["geom", "agility", "glue", "glueagility"])
        cmd += "--agilviollim %s " % random.randint(0,40)
        cmd += "--gluehist %s " % random.randint(1,500)
        cmd += "--updateglue %s " % random.randint(0,1)
        cmd += "--otfhyper %s " % random.randint(0,1)
        cmd += "--clean %s " % random.choice(["size", "glue", "activity", "prconf"])
        cmd += "--preclean %s " % random.randint(0,1)
        cmd += "--precleanlim %s " % random.randint(0,10)
        cmd += "--precleantime %s " % random.randint(0,20000)
        cmd += "--clearstat %s " % random.randint(0,1)
        cmd += "--startclean %s " % random.randint(100,16000)
        cmd += "--maxredratio %s " % random.randint(2,20)
        cmd += "--dompickf %s " % random.randint(1,20)
        cmd += "--flippolf %s " % random.randint(1,3000)
        cmd += "--alwaysmoremin %s " % random.randint(0,1)
        cmd += "--rewardotfsubsume %s " % random.randint(0,100)
        cmd += "--bothprop %s " % random.randint(0,1)
        cmd += "--probemaxm %s " % random.choice([0,10,100,1000])
        cmd += "--cachesize %s " % random.randint(10, 100)
        cmd += "--calcreach %s " % random.randint(0,1)
        cmd += "--cachecutoff %s " % random.randint(0,2000)
        cmd += "--elimstrgy %s " % random.choice(["heuristic", "calculate"])
        cmd += "--elimcplxupd %s " % random.randint(0,1)
        cmd += "--occredmax %s " % random.randint(0,100)
        cmd += "--noextbinsubs %s " % random.randint(0,1)
        cmd += "--extscc %s " % random.randint(0,1)
        cmd += "--vivif %s " % random.randint(0,1)
        cmd += "--sortwatched %s " % random.randint(0,1)
        cmd += "--recur %s " % random.randint(0,1)
        cmd += "--compsfrom %d " % random.randint(0,2)
        cmd += "--compsvar %d " % random.randint(20000,500000)
        cmd += "--compslimit %d " % random.randint(0,3000)
        cmd += "--implicitmanip %s " % random.randint(0,1)
        cmd += "--occsimp %s " % random.randint(0,1)
        cmd += "--occirredmaxmb %s " % random.randint(0,10)
        cmd += "--occredmaxmb %s " % random.randint(0,10)
        cmd += "--implsubsto %s " % random.choice([0,10,1000])
        cmd += "--sync %d " % random.choice([100,1000,6000,100000])
        if self.num_threads > 1 :
            cmd += "--xor 0 "
            cmd += "--sql 0 "


        #the most buggy ones, don't turn them off much, please
        if random.randint(0,1) == 1 :
            opts = ["scc", "varelim" \
            , "comps", "strengthen", "probe"
            , "binpri", "stamp", "cache"
            , "otfsubsume", "renumber", "savemem"
            , "moreminim", "gates", "bva"
            , "gorshort", "gandrem", "gateeqlit", "schedsimp", "presimp"]
            if self.num_threads == 1 :
                opts.append("xor")

            for opt in opts:
                cmd += "--%s %d " % (opt, random.randint(0,1))

        return cmd

    def execute(self, fname, newVar=False, needToLimitTime=False, fnameDrup=None, extraOptions=""):
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
        command += "--threads %d " % self.num_threads
        command += options.extra_options + " "
        command += extraOptions
        command += fname
        if fnameDrup:
            command += " --drupexistscheck 0 " + fnameDrup
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
        solution = {}

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
                    solution[abs(vvar)] = (vvar >= 0)
        #print "Parsed values:", solution

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

        return (unsat, solution)

    def test_found_solution(self, solution, fname, debugLibPart=None):
        def check_regular_clause(line, solution):
            lits = line.split()
            final = False
            for lit in lits:
                numlit = int(lit)
                if numlit != 0:
                    if (abs(numlit) not in solution): continue
                    if numlit < 0:
                        final |= ~solution[abs(numlit)]
                    else:
                        final |= solution[numlit]
                    if final == True:
                        break
            if final == False:
                print "Error: clause '%s' not satisfied." % line
                print "Error code 100"
                exit(100)

        def check_xor_clause(line, solution):
            line = line.lstrip('x')
            lits = line.split()
            final = False
            for lit in lits:
                numlit = int(lit)
                if numlit != 0:
                    if abs(numlit) not in solution:
                        print "Error: var %d not solved, but referred to in a xor-clause of the CNF" % abs(numlit)
                        print "Error code 200"
                        exit(200)
                    final ^= solution[abs(numlit)]
                    final ^= numlit < 0
            if final == False:
                print "Error: xor-clause '%s' not satisfied." % line
                exit(-1)

        if debugLibPart == None:
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
            if line[0] == 'c' and "Solver::solve" in line:
                thisDebugLibPart += 1

            #if we are over debugLibPart, exit
            if debugLibPart != None and thisDebugLibPart >= debugLibPart:
                f.close()
                return

            #check solution against clause
            if line[0] != 'c' and line[0] != 'p':
                if line[0] != 'x':
                    check_regular_clause(line, solution)
                else:
                    check_xor_clause(line, solution)

                clauses += 1

        f.close()
        print "Verified %d original xor&regular clauses" % clauses

    def checkUNSAT(self, fname) :
        a = XorToCNF()
        tmpfname = unique_fuzz_file("tmp_for_xor_to_cnf_convert")
        a.convert(fname, tmpfname )
        #execute with the other solver
        toexec = "../../lingeling-587f/lingeling -f %s" % tmpfname
        print "Solving with other solver.."
        currTime = calendar.timegm(time.gmtime())
        p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE,
                             preexec_fn=setlimits)
        consoleOutput2 = p.communicate()[0]
        os.unlink(tmpfname)

        #if other solver was out of time, then we can't say anything
        diffTime = calendar.timegm(time.gmtime()) - currTime
        if diffTime > maxTime-maxTimeDiff:
            print "Other solver: too much time to solve, aborted!"
            return None

        #extract output from the other solver
        print "Checking other solver output..."
        (otherSolverUNSAT, otherSolverSolution) = self.parse_solution_from_output(consoleOutput2.split("\n"))

        #check if the other solver agrees with us
        return otherSolverUNSAT

    def extractLibPart(self, fname, debug_num, assumps, tofile) :
        fromf = open(fname, "r")
        thisDebugLibPart = 0
        maxvar = 0
        numcls = 0
        for line in fromf :
            line = line.strip()

            #ignore empty strings and headers
            if not line or line[0] == "p" :
                continue

            #process (potentially special) comments
            if line[0] == "c" :
                if "Solver::solve" in line:
                    thisDebugLibPart += 1

                continue

            #break out if we reached the debug lib part
            if thisDebugLibPart >= debug_num :
                break

            #count clauses and get max var number
            numcls += 1
            maxvar = max(maxvar, self.get_max_var_from_clause(line))

        fromf.close()

        #now we can create the new CNF file
        fromf = open(fname, "r")
        tof = open(tofile, "w")
        tof.write("p cnf %d %d\n" % (maxvar, numcls + len(assumps)))

        thisDebugLibPart = 0
        for line in fromf :
            line = line.strip()
            #skip empty lines and headers
            if not line or line[0] == "p" :
                continue

            #parse up special header
            if line[0] == "c" :
                if "Solver::solve" in line:
                    thisDebugLibPart += 1

                continue

            #break out if we reached the debug lib part
            if thisDebugLibPart >= debug_num :
                break

            tof.write(line + '\n')

        #add assumptions
        for lit in assumps:
            tof.write("%d 0\n" % lit)

        fromf.close()
        tof.close()

    def get_assumps(self, fname, debugLibPart) :
        f = open(fname, "r")

        thispart = 0
        solveline = None
        for line in f :
            if "Solver::solve" in line :
                thispart += 1
                if thispart == debugLibPart :
                    solveline = line
                    break
        f.close()

        assert solveline != None
        ret = re.match("c.*Solver::solve\((.*)\)", solveline)
        assert ret != None
        assumps = ret.group(1).strip().split()
        assumps = [int(x) for x in assumps]

        print "Assumptions: ", assumps
        return assumps

    def check_assumps_inside_solution(self, assumps, solution) :
        for lit in assumps:
            var = abs(lit)
            val = lit > 0
            if var in solution :
                if solution[var] != val :
                    print "Solution pinted has literal %s but assumptions contained the inverse: '%s'" % (-1*lit, assumps)
                    exit(-100)

        print "OK, all assumptions inside solution"

    def checkDebugLib(self, fname) :
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
                exit(400)

            #take file into mem
            f = open(fname_debug, "r")
            text = f.read()
            output_lines = text.splitlines()
            f.close()

            (unsat, solution) = self.parse_solution_from_output(output_lines)
            assumps = self.get_assumps(fname, debugLibPart)
            if unsat == False:
                print "debugLib is SAT"
                self.check_assumps_inside_solution(assumps, solution)
                self.test_found_solution(solution, fname, debugLibPart)
            else:
                print "debugLib is UNSAT"
                tmpfname = unique_fuzz_file("tempfile_for_extract_libpart")
                self.extractLibPart(fname, debugLibPart, assumps, tmpfname)

                #check with other solver
                ret = self.checkUNSAT(tmpfname)
                if ret == None :
                    print "Cannot check, other solver took too much time"
                elif ret == True :
                    print "UNSAT verified by other solver"
                else :
                    print "Grave bug: SAT-> UNSAT : Other solver found solution!!"
                    exit()

                #delete temporary file
                os.unlink(tmpfname)

    def check(self, fname, fnameSolution=None, fnameDrup=None, newVar=False,
              needSolve=True, needToLimitTime=False, checkAgainst=None, extraOptions=""):

        consoleOutput = ""
        if checkAgainst == None: checkAgainst = fname
        currTime = calendar.timegm(time.gmtime())

        #Do we need to solve the problem, or is it already solved?
        if needSolve:
            consoleOutput = self.execute(fname, newVar, needToLimitTime, fnameDrup=fnameDrup, extraOptions=extraOptions)
        else:
            if not os.path.isfile(fnameSolution) :
                print "ERROR! Solution file '%s' is not a file!" % fnameSolution
                exit(-1)
            f = open(fnameSolution, "r")
            consoleOutput = f.read()
            f.close()
            print "Read solution from file " , fnameSolution

        #if time was limited, we need to know if we were over the time limit
        #and that is why there is no solution
        if needToLimitTime:
            diffTime = calendar.timegm(time.gmtime()) - currTime
            if diffTime > (maxTime - maxTimeDiff)/self.num_threads:
                print "Too much time to solve, aborted!"
                return
            else:
                print "Within time limit: %3ds" % (calendar.timegm(time.gmtime()) - currTime)

        print "filename: %s" % fname

        #if library debug is set, check it
        if (self.needDebugLib) :
            self.checkDebugLib(checkAgainst)

        print "Checking console output..."
        (unsat, solution) = self.parse_solution_from_output(consoleOutput.split("\n"))
        otherSolverUNSAT = True

        if not unsat :
            self.test_found_solution(solution, checkAgainst)
            return;

        #it's UNSAT and we should not check, so exit
        if self.check_unsat == False:
            print "Cannot check -- output is UNSAT"
            return

        #it's UNSAT, let's check with DRUP
        if fnameDrup:
            toexec = "drat-trim %s %s" % (fname, fnameDrup)
            print "Checking DRUP...: ", toexec
            p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE)
                                 #,preexec_fn=setlimits)
            consoleOutput2 = p.communicate()[0]
            diffTime = calendar.timegm(time.gmtime()) - currTime

            #find verification code
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

            #Check whether we have found a verification code
            if foundVerif == False:
                print "verifier error! It says:", consoleOutput2
                assert foundVerif, "Cannot find DRUP verification code!"
            else:
                print "OK, DRUP says:", drupLine

        #check with other solver
        ret = self.checkUNSAT(checkAgainst)
        if ret == None :
            print "Other solver time-outed, cannot check"
        elif ret == True:
            print "UNSAT verified by other solver"
        else :
            print "Grave bug: SAT-> UNSAT : Other solver found solution!!"
            exit()


    def removeDebugLibParts(self) :
        dirList = os.listdir(".")
        for fname_unlink in dirList:
            if fnmatch.fnmatch(fname_unlink, 'debugLibPart*'):
                os.unlink(fname_unlink)
                None

    def callFromFuzzer(self, fuzzer, file_name) :
        if (len(fuzzer) == 1) :
            call = "{0}{1} > {2}".format(self.fuzzer_directory, fuzzer[0], file_name)
        elif(len(fuzzer) == 2) :
            seed = struct.unpack("<L", os.urandom(4))[0]
            call = "{0}{1} {2} {3} > {4}".format(self.fuzzer_directory, fuzzer[0], fuzzer[1], seed, file_name)
        elif(len(fuzzer) == 3) :
            seed = struct.unpack("<L", os.urandom(4))[0]
            hashbits = (random.getrandbits(20) % 79) + 1
            call = "%s %s %d %s %d > %s" % (fuzzer[0], fuzzer[1], hashbits, fuzzer[2], seed, file_name)

        return call

    def create_fuzz(self, fuzzer, file_name) :

        #handle special fuzzer
        file_names_multi = []
        if len(fuzzer) == 2 and fuzzer[1] == "special":

            #sometimes just fuzz with all SAT problems
            fixed = random.getrandbits(1)  == 1

            for i in range(random.randrange(2,4)) :
                file_name2 = unique_fuzz_file("fuzzTest");
                file_names_multi.append(file_name2)

                #chose a ranom fuzzer, not multipart
                fuzzer2 = ["multipart.py", "special"]
                while (fuzzer2[0] == "multipart.py") :
                    fuzzer2 = choice(self.fuzzers)

                #sometimes fuzz with SAT problems only
                if (fixed) :
                    fuzzer2 = self.fuzzers[0]

                print "fuzzer2 used: ", fuzzer2
                call = self.callFromFuzzer(fuzzer2, file_name2)
                print "calling sub-fuzzer:", call
                out = commands.getstatusoutput(call)

            #construct multi-fuzzer call
            call = ""
            call += self.fuzzer_directory
            call += fuzzer[0]
            call += " "
            for name in file_names_multi :
                call += " " + name
            call += " > " + file_name

            return call, file_names_multi

        #handle normal fuzzer
        else :
            return self.callFromFuzzer(fuzzer, file_name), []

    def file_len_no_comment(self, fname):
        i = 0;
        with open(fname) as f:
            for l in f :
                #ignore comments and empty lines and header
                if not l or l[0] == "c" or l[0] == "p":
                    continue
                i += 1

        return i

    def get_max_var_from_clause(self, line) :
        maxvar = 0
        #strip leading 'x'
        line2 = line.strip()
        if len(line2) > 0 and line2[0] == 'x' :
            line2 = line2[1:]

        for lit in line2.split() :
            num = 0
            try :
                num = int(lit)
            except ValueError:
                print "line '%s' contains a non-integer variable" % line2

            maxvar = max(maxvar, abs(num))

        return maxvar

    def generate_random_assumps(self, maxvar) :
        assumps = ""
        num = 0
        varsInside = set()

        #Half of the time, no assumptions at all
        if random.randint(0,1) == 1:
            return assumps

        #use a distribution so that few will be in assumps
        while (num < maxvar and random.randint(0,4) > 0) :

            #get a var that is not already inside the assumps
            thisVar = random.randint(1, maxvar)
            tries = 0
            while (thisVar in varsInside) :
                thisVar = random.randint(1, maxvar)
                tries += 1

                #too many tries, don't waste time
                if tries > 100:
                    return assumps

            varsInside.add(thisVar)

            #random sign
            num += 1
            if random.randint(0,1) :
                thisVar *= -1

            assumps += "%d " % thisVar

        return assumps

    def intersperse_with_debuglib(self, fname1, fname2) :

        #approx number of solve()-s to add
        if random.randint(0,1) == 1:
            numtodo = random.randint(0,10)
        else:
            numtodo = 0

        #based on length and number of solve()-s to add, intersperse
        #file with ::solve()
        file_len = self.file_len_no_comment(fname1)
        if numtodo > 0:
            nextToAdd = random.randint(1,(file_len/numtodo)*2+1)
        else :
            nextToAdd = file_len + 1

        fin = open(fname1, "r")
        fout = open(fname2, "w")
        at = 0
        maxvar = 0
        for line in fin :
            line = line.strip()

            #ignore comments (but write them out)
            if not line or line[0] == "c" or line[0] == 'p':
                fout.write(line + '\n')
                continue

            at += 1
            if at >= nextToAdd :
                assumps = self.generate_random_assumps(maxvar)
                #assumps = " "
                fout.write("c Solver::solve( %s )\n" % assumps)
                nextToAdd = at + random.randint(1,(file_len/numtodo)*2+1)

            #calculate max variable
            maxvar = max(maxvar, self.get_max_var_from_clause(line))

            #copy line over
            fout.write(line + '\n')
        fout.close()
        fin.close()


    def fuzz_test(self) :
        while True:
            fuzzer = random.choice(self.fuzzers)
            self.num_threads = random.choice([1, 2, 10])
            file_name = unique_fuzz_file("fuzzTest");
            self.drup = self.num_threads == 1 and random.choice([True, False])
            fnameDrup = None
            if self.drup :
                fnameDrup = unique_fuzz_file("fuzzTest");

            #create the fuzz file
            call, todel = self.create_fuzz(fuzzer, file_name)
            print "calling ", fuzzer, " : ", call
            out = commands.getstatusoutput(call)

            if not self.drup :
                self.needDebugLib = True
                self.delete_debuglibpart_files()
                file_name2 = unique_fuzz_file("fuzzTest");
                self.intersperse_with_debuglib(file_name, file_name2)
                os.unlink(file_name)
            else:
                self.needDebugLib = False
                file_name2 = file_name

            self.check(fname=file_name2, fnameDrup=fnameDrup, needToLimitTime=True)

            #remove temporary filenames
            os.unlink(file_name2)
            for name in todel :
                os.unlink(name)
            if fnameDrup != None :
                os.unlink(fnameDrup)
            for i in glob.glob(u'fuzz*'):
                os.unlink (i)

    def delete_debuglibpart_files(self):
        dirList = os.listdir(".")
        for fname in dirList:
            if fnmatch.fnmatch(fname, 'debugLibPart*'):
                os.unlink(fname)

    def checkDir(self) :
        self.ignoreNoSolution = True
        print "Checking already solved solutions"

        #check if options.checkDirSol has bee set
        if options.checkDirSol == "":
            print "When checking, you must give test dir"
            exit()

        print "You gave testdir (where solutions are):", options.checkDirSol
        print "You gave CNF dir (where problems are) :", options.checkDirProb

        for fname in os.listdir(options.checkDirSol):

            if fnmatch.fnmatch(fname, '*.cnf.gz.out'):
                #add dir, remove trailing .out
                fname = fname[:len(fname) - 4]
                fnameSol = options.checkDirSol + "/" + fname

                #check now
                self.check(fname=options.checkDirProb + "/" + fname, \
                   fnameSolution=fnameSol, needSolve=False)

    def regressionTest(self) :
        for fname in os.listdir(options.testDir):
            if fnmatch.fnmatch(fname, '*.cnf.gz'):
                self.check(fname=options.testDir + fname, newVar=False)

tester = Tester()

if options.checkFile :
    tester.check_unsat = True
    tester.needDebugLib = False
    tester.check(options.checkFile, options.solutionFile, needSolve=False)

if options.fuzz_test:
    tester.needDebugLib = False
    tester.check_unsat = True
    tester.fuzz_test()

if options.checkSol:
    tester.checkDir()

if options.regressionTest:
    tester.regressionTest()

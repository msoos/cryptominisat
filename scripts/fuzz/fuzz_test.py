#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2014  Mate Soos
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
# 02110-1301, USA.

from __future__ import with_statement  # Required in 2.5
from __future__ import print_function
import subprocess
import os
import fnmatch
import gzip
import re
import commands
import getopt
import sys
import signal
import time
import struct
import random
from random import choice
from subprocess import Popen, PIPE, STDOUT
# from optparse import OptionParser
import optparse
import glob
from verifier import *
from functools import partial

print("our CWD is: %s files here: %s" % (os.getcwd(), glob.glob("*")) )
sys.path.append(os.getcwd())
print("our sys.path is", sys.path)

from debuglib import *


class PlainHelpFormatter(optparse.IndentedHelpFormatter):

    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog [options] --fuzz/--regtest/--checkdir/filetocheck"
desc = """Fuzz the solver with fuzz-generator: ./fuzz_test.py
"""

parser = optparse.OptionParser(usage=usage, description=desc,
                               formatter=PlainHelpFormatter())
parser.add_option("--exec", metavar="SOLVER", dest="solver",
                  default="../../build/cryptominisat4",
                  help="SAT solver executable. Default: %default")

parser.add_option("--extraopts", "-e", metavar="OPTS",
                  dest="extra_options", default="",
                  help="Extra options to give to SAT solver")

parser.add_option("--verbose", "-v", action="store_true", default=False,
                  dest="verbose", help="Print more output")

# for fuzz-testing
parser.add_option("--seed", dest="fuzz_seed_start",
                  help="Fuzz test start seed. Otherwise, random seed is picked"
                  " (printed to console)", type=int)

parser.add_option("--fuzzlim", dest="fuzz_test_lim", type=int,
                  help="Number of fuzz tests to run"
                  )
parser.add_option("--novalgrind", dest="novalgrind", default=False,
                  action="store_true", help="No valgrind installed")

parser.add_option("--small", dest="small", default=False,
                  action="store_true",
                  help="Don't run 'large' fuzzer"
                  " (may mem-out on smaller systems)")

parser.add_option("--sqlite", dest="sqlite", default=False,
                  action="store_true", help="Test SQLite dumping")

parser.add_option("--gauss", dest="test_gauss", default=False,
                  action="store_true", help="Test gauss too")

parser.add_option("--tout", "-t", dest="maxtime", type=int, default=35,
                  help="Max time to run. Default: %default")

parser.add_option("--textra", dest="maxtimediff", type=int, default=10,
                  help="Extra time on top of timeout for processing."
                  " Default: %default")

(options, args) = parser.parse_args()


def fuzzer_call_failed():
    print("OOps, fuzzer executable call failed!")
    print("Did you build with cmake -DENABLE_TESTING=ON? Did you do git submodules init & update?")
    exit(-1)


class create_fuzz:

    def call_from_fuzzer(self, fuzzer, fname):
        seed = random.randint(0, 1000000)
        if len(fuzzer) == 1:
            call = "{0} {1} > {2}".format(fuzzer[0], seed, fname)
        elif len(fuzzer) == 2:
            call = "{0} {1} {2} > {3}".format(
                fuzzer[0], fuzzer[1], seed, fname)
        elif len(fuzzer) == 3:
            hashbits = (random.getrandbits(20) % 80) + 1
            call = "%s %s %d %s %d > %s" % (
                fuzzer[0], fuzzer[1], hashbits, fuzzer[2], seed, fname)
        else:
            assert False, "Fuzzer must have at most 2 arguments"

        return call

    def create_fuzz_file(self, fuzzer, fuzzers, fname):
        # handle special fuzzer
        fnames_multi = []
        if len(fuzzer) == 2 and fuzzer[1] == "special":

            # sometimes just fuzz with all SAT problems
            fixed = random.getrandbits(1) == 1

            for i in range(random.randrange(2, 4)):
                fname2 = unique_file("fuzzTest")
                fnames_multi.append(fname2)

                # chose a ranom fuzzer, not multipart
                fuzzer2 = ["multipart.py", "special"]
                while os.path.basename(fuzzer2[0]) == "multipart.py":
                    fuzzer2 = choice(fuzzers)

                # sometimes fuzz with SAT problems only
                if (fixed):
                    fuzzer2 = fuzzers[0]

                print("fuzzer2 used: %s" % fuzzer2)
                call = self.call_from_fuzzer(fuzzer2, fname2)
                print("calling sub-fuzzer: %s" % call)
                status, _ = commands.getstatusoutput(call)
                if status != 0:
                    fuzzer_call_failed()

            # construct multi-fuzzer call
            call = ""
            call += fuzzer[0]
            call += " "
            for name in fnames_multi:
                call += " " + name
            call += " > " + fname

            return call, fnames_multi

        # handle normal fuzzer
        else:
            return self.call_from_fuzzer(fuzzer, fname), []


def file_exists(fname):
    try:
        with open(fname):
            return True
    except IOError:
        return False


def print_version():
    command = options.solver + " --version"
    p = subprocess.Popen(command.rsplit(), stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    consoleOutput, err = p.communicate()
    print("Version values: %s" % consoleOutput.strip())

fuzzers_noxor = [
    ["../../build/tests/sha1-sat/sha1-gen --attack preimage --rounds 20",
     "--hash-bits", "--seed"],
    ["../../build/tests/sha1-sat/sha1-gen --attack preimage --zero --message-bits 400 --rounds 8 --hash-bits 60",
     "--seed"],
    # ["build/cnf-fuzz-nossum"],
    ["../../build/tests/cnf-utils/largefuzzer"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/sgen4 -unsat -n 50", "-s"],
    ["../../build/tests/cnf-utils//sgen4 -sat -n 50", "-s"],
    ["../../utils/cnf-utils/cnf-fuzz-brummayer.py", "-s"],
    ["../../utils/cnf-utils/cnf-fuzz-xor.py", "--seed"],
    ["../../utils/cnf-utils/multipart.py", "special"]
]
fuzzers_xor = [
    ["../../utils/cnf-utils/xortester.py", "--seed"],
    ["../../build/tests/sha1-sat/sha1-gen --xor --attack preimage --rounds 21",
     "--hash-bits", "--seed"],
]


class Tester:

    def __init__(self):
        self.ignoreNoSolution = False
        self.extra_options_if_supported = self.list_options_if_supported(
            ["xor", "autodisablegauss"])
        self.sol_parser = solution_parser(options)

    def list_options_if_supported(self, tocheck):
        ret = []
        for elem in tocheck:
            if self.option_supported(elem):
                ret.append(elem)

        return ret

    def option_supported(self, option_name):
        command = options.solver
        command += " --hhelp"
        p = subprocess.Popen(
            command.rsplit(), stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
        consoleOutput, err = p.communicate()

        for l in consoleOutput.split("\n"):
            tmp_option_name = "--" + option_name
            if tmp_option_name in l:
                return True

        return False

    def random_options(self, preproc=False):
        cmd = " --zero-exit-status "

        if random.choice([True, False]):
            cmd += " --reconf %d " % random.choice([3, 6, 7, 12, 13, 14])
            cmd += " --undef %d " % random.choice([0, 1])
            cmd += " --reconfat %d " % random.randint(0, 2)
            cmd += "--burst %d " % random.choice([0, 100, random.randint(0, 10000)])
            cmd += "--keepguess %s " % random.randint(0, 10)
            cmd += "--restart %s " % random.choice(
                ["geom", "glue", "luby"])
            cmd += "--adjustglue %f " % random.choice([0, 0.5, 0.7, 1.0])
            cmd += "--gluehist %s " % random.randint(1, 500)
            cmd += "--updateglueonanalysis %s " % random.randint(0, 1)
            cmd += "--otfhyper %s " % random.randint(0, 1)
            # cmd += "--clean %s " % random.choice(["size", "glue", "activity",
            # "prconf"])
            cmd += "--cacheformoreminim %d " % random.choice([0, 1, 1, 1, 1])
            cmd += "--stampformoreminim %d " % random.choice([0, 1, 1, 1, 1])
            cmd += "--alwaysmoremin %s " % random.randint(0, 1)
            cmd += "--rewardotfsubsume %s " % random.randint(0, 100)
            cmd += "--bothprop %s " % random.randint(0, 1)
            cmd += "--probemaxm %s " % random.choice([0, 10, 100, 1000])
            cmd += "--cachesize %s " % random.randint(10, 100)
            cmd += "--cachecutoff %s " % random.randint(0, 2000)
            cmd += "--elimstrgy %s " % random.choice(["heuristic", "calculate"])
            cmd += "--elimcplxupd %s " % random.randint(0, 1)
            cmd += "--occredmax %s " % random.randint(0, 100)
            cmd += "--noextbinsubs %s " % random.randint(0, 1)
            cmd += "--extscc %s " % random.randint(0, 1)
            cmd += "--distill %s " % random.randint(0, 1)
            cmd += "--sortwatched %s " % random.randint(0, 1)
            cmd += "--recur %s " % random.randint(0, 1)
            cmd += "--compsfrom %d " % random.randint(0, 2)
            cmd += "--compsvar %d " % random.randint(20000, 500000)
            cmd += "--compslimit %d " % random.randint(0, 3000)
            cmd += "--implicitmanip %s " % random.randint(0, 1)
            cmd += "--occsimp %s " % random.randint(0, 1)
            cmd += "--occirredmaxmb %s " % random.randint(0, 10)
            cmd += "--occredmaxmb %s " % random.randint(0, 10)
            cmd += "--skipresol %d " % random.choice([1, 1, 1, 0])
            cmd += "--implsubsto %s " % random.choice([0, 10, 1000])
            cmd += "--sync %d " % random.choice([100, 1000, 6000, 100000])
            cmd += "-m %0.12f " % random.gammavariate(0.4, 2.0)
            # gammavariate gives us sometimes very low values, sometimes large

            if options.sqlite:
                cmd += "--sql 2 "
                cmd += "--sqlrestfull %d " % random.choice([0, 1])
                cmd += "--sqlresttime %d " % random.choice([0, 1])

        # the most buggy ones, don't turn them off much, please
        if random.choice([True, False]):
            opts = ["scc", "varelim", "comps", "strengthen", "probe", "intree",
                    "binpri", "stamp", "cache", "otfsubsume",
                    "renumber", "savemem", "moreminim", "gates", "bva",
                    "gorshort", "gandrem", "gateeqlit", "schedsimp", "presimp",
                    "elimcoststrategy"]

            opts.extend(self.extra_options_if_supported)

            for opt in opts:
                cmd += "--%s %d " % (opt, random.randint(0, 1))

            def create_rnd_sched(string_list):
                opts = string_list.split(",")
                opts = [a.strip(" ") for a in opts]
                opts = list(set(opts))
                if options.verbose:
                    print("available schedule options: %s" % opts)

                sched = []
                for i in range(int(random.gammavariate(12, 0.7))):
                    sched.append(random.choice(opts))

                if "autodisablegauss" in self.extra_options_if_supported and options.test_gauss:
                    sched.append("occ-gauss")

                return sched

            cmd += self.add_schedule_options(create_rnd_sched, preproc)

        return cmd

    def add_schedule_options(self, create_rnd_sched, preproc):
        cmd = ""

        sched_opts = "handle-comps,"
        sched_opts += "scc-vrepl, cache-clean, cache-tryboth,"
        sched_opts += "sub-impl, intree-probe, probe,"
        sched_opts += "sub-str-cls-with-bin, distill-cls, scc-vrepl, sub-impl,"
        sched_opts += "str-impl, cache-clean, sub-str-cls-with-bin, distill-cls, scc-vrepl,"
        sched_opts += "occ-backw-sub-str, occ-xor, occ-clean-implicit, occ-bve, occ-bva, occ-gates,"
        sched_opts += "check-cache-size, renumber"

        sched = ",".join(create_rnd_sched(sched_opts))
        if sched != "" and not preproc:
            cmd += "--schedule %s " % sched

        sched = ",".join(create_rnd_sched(sched_opts))
        if sched != "":
            cmd += "--preschedule %s " % sched

        return cmd

    def execute(self, fname, fname2=None, fixed_opts="", rnd_opts=None):
        if os.path.isfile(options.solver) is not True:
            print("Error: Cannot find CryptoMiniSat executable.Searched in: '%s'" %
                  options.solver)
            print("Error code 300")
            exit(300)

        for f in glob.glob("%s-debugLibPart*.output" % fname):
            os.unlink(f)

        # construct command
        command = ""
        if not options.novalgrind and random.randint(0, 10) == 0:
            command += "valgrind -q --leak-check=full  --error-exitcode=9 "
        command += options.solver
        if rnd_opts is None:
            rnd_opts = self.random_options()
        command += rnd_opts
        if self.needDebugLib:
            command += "--debuglib %s " % fname
        if options.verbose is False:
            command += "--verb 0 "
        command += "--threads %d " % self.num_threads
        command += options.extra_options + " "
        command += fixed_opts + " "
        if fname is not None:
            command += fname
        if fname2:
            command += " %s --savedstate %s-savedstate.dat " % (fname2, fname2)

        print("Executing: %s " % command)

        # print time limit
        if options.verbose:
            print("CPU limit of parent (pid %d)" % os.getpid(), resource.getrlimit(resource.RLIMIT_CPU))

        # if need time limit, then limit
        err_fname = unique_file("%s_err" % fname, ".out")
        err_file = open(err_fname, "w")
        p = subprocess.Popen(
            command.rsplit(), stderr=err_file, stdout=subprocess.PIPE, preexec_fn=partial(setlimits, options.maxtime))

        # print time limit after child startup
        if options.verbose:
            print("CPU limit of parent (pid %d) after startup of child" %
                  (os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)))

        # Get solver output
        consoleOutput, err = p.communicate()
        retcode = p.returncode
        err_file.close()
        with open(err_fname, "r") as err_file:
            found_something = False
            for line in err_file:
                print("Error line while executing: %s" % line.strip())
                if "std::_Ios_Fmtflags" in line or "mzd.h" in line or "lexical_cast.hpp" in line:
                    pass
                else:
                    found_something = True

            if found_something:
                exit(-1)

        os.unlink(err_fname)

        if options.verbose:
            print("CPU limit of parent (pid %d) after child finished executing" %
                  (os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)))

        return consoleOutput, retcode

    def check(self, fname, fname2=None,
              checkAgainst=None,
              fixed_opts="", dump_output_fname=None,
              rnd_opts=None):

        consoleOutput = ""
        if checkAgainst is None:
            checkAgainst = fname
        curr_time = time.time()

        # Do we need to solve the problem, or is it already solved?
        consoleOutput, retcode = self.execute(
            fname, fname2=fname2,
            fixed_opts=fixed_opts, rnd_opts=rnd_opts)

        # if time was limited, we need to know if we were over the time limit
        # and that is why there is no solution
        diff_time = time.time() - curr_time
        if diff_time > (options.maxtime - options.maxtimediff) / self.num_threads:
            print("Too much time to solve, aborted!")
            return None

        print("Within time limit: %.2f s" % diff_time)
        print("filename: %s" % fname)

        # if library debug is set, check it
        if (self.needDebugLib):
            self.sol_parser.check_debug_lib(checkAgainst)

        if retcode != 0:
            print("Return code is not 0, error!")
            exit(-1)

        print("Checking console output...")
        unsat, solution, _ = self.sol_parser.parse_solution_from_output(
            consoleOutput.split("\n"), self.ignoreNoSolution)

        # preprocessing
        if dump_output_fname is not None:
            f = open(dump_output_fname, "w")
            f.write(consoleOutput)
            f.close()
            return True

        if not unsat:
            self.sol_parser.test_found_solution(solution, checkAgainst)
            return

        # it's UNSAT, let's check with DRAT
        if fname2:
            toexec = "drat-trim %s %s" % (fname, fname2)
            print("Checking DRAT...: ", toexec)
            p = subprocess.Popen(toexec.rsplit(), stdout=subprocess.PIPE)
            consoleOutput2 = p.communicate()[0]
            diff_time = time.time() - curr_time

            # find verification code
            foundVerif = False
            dratLine = ""
            for line in consoleOutput2.split('\n'):
                if len(line) > 1 and line[:2] == "s ":
                    # print("verif: " , line)
                    foundVerif = True
                    if line[2:10] != "VERIFIED" and line[2:] != "TRIVIAL UNSAT":
                        print("DRAT verification error, it says: %s" % consoleOutput2)
                    assert line[2:10] == "VERIFIED" or line[
                        2:] == "TRIVIAL UNSAT", "DRAT didn't verify problem!"
                    dratLine = line

            # Check whether we have found a verification code
            if foundVerif is False:
                print("verifier error! It says: %s" % consoleOutput2)
                assert foundVerif, "Cannot find DRAT verification code!"
            else:
                print("OK, DRAT says: %s" % dratLine)

        # check with other solver
        ret = self.sol_parser.check_unsat(checkAgainst)
        if ret is None:
            print("Other solver time-outed, cannot check")
        elif ret is True:
            print("UNSAT verified by other solver")
        else:
            print("Grave bug: SAT-> UNSAT : Other solver found solution!!")
            exit()

    def fuzz_test_one(self):
        print("\n--- NORMAL TESTING ---")
        self.num_threads = random.choice([1, 2, 4])
        self.drat = self.num_threads == 1 and random.choice([True, False])
        if self.drat:
            fuzzers = fuzzers_drat
        else:
            fuzzers = fuzzers_nodrat
        fuzzer = random.choice(fuzzers)

        fname = unique_file("fuzzTest")
        fname_drat = None
        if self.drat:
            fname_drat = "%s-drat" % fname

        # create the fuzz file
        cf = create_fuzz()
        call, todel = cf.create_fuzz_file(fuzzer, fuzzers, fname)
        print("calling %s" % call)
        status, _ = commands.getstatusoutput(call)
        if status != 0:
            fuzzer_call_failed()

        if not self.drat:
            self.needDebugLib = True
            interspersed_fname = unique_file("fuzzTest")
            seed_for_inters = random.randint(0, 1000000)
            intersperse(fname, interspersed_fname, seed_for_inters)
            print("Interspersed: ./intersperse.py %s %s %d" % (fname,
                                                               interspersed_fname,
                                                               seed_for_inters))
            os.unlink(fname)
        else:
            self.needDebugLib = False
            interspersed_fname = fname

        self.check(fname=interspersed_fname, fname2=fname_drat)

        # remove temporary filenames
        os.unlink(interspersed_fname)
        for name in todel:
            os.unlink(name)
        if fname_drat:
            os.unlink(fname_drat)

    def delete_file_no_matter_what(self, fname):
        try:
            os.unlink(fname)
        except:
            pass

    def fuzz_test_preproc(self):
        print("\n--- PREPROC TESTING ---")
        tester.needDebugLib = False
        fuzzer = random.choice(fuzzers_drat)
        self.num_threads = 1
        fname = unique_file("fuzzTest")
        self.drat = False

        # create the fuzz file
        cf = create_fuzz()
        call, todel = cf.create_fuzz_file(fuzzer, fuzzers_nodrat, fname)
        print("calling %s : %s" % (fuzzer, call))
        status, _ = commands.getstatusoutput(call)
        if status != 0:
            fuzzer_call_failed()

        rnd_opts = self.random_options(preproc=True)

        # preprocess
        simp = "%s-simplified.cnf" % fname
        self.delete_file_no_matter_what(simp)
        curr_time = time.time()
        console, retcode = self.execute(fname, fname2=simp,
                                        rnd_opts=rnd_opts,
                                        fixed_opts="--preproc 1")

        diff_time = time.time() - curr_time
        if diff_time > (options.maxtime - options.maxtimediff) / self.num_threads:
            print("Too much time to solve, aborted!")
        else:
            print("Within time limit: %.2f s" % diff_time)
            if retcode != 0:
                print("Return code is not 0, error!")
                exit(-1)

            solution = "%s-solution.txt" % fname
            ret = self.check(fname=simp, dump_output_fname=solution)
            if ret is not None:
                # didn't time out, so let's reconstruct the solution
                savedstate = "%s-savedstate.dat" % simp
                self.check(fname=solution, checkAgainst=fname,
                           fixed_opts="--preproc 2 --savedstate %s" % savedstate,
                           rnd_opts=rnd_opts)
                os.unlink(savedstate)
                os.unlink(solution)

        # remove temporary filenames
        os.unlink(fname)
        for name in todel:
            os.unlink(name)


def filter_large_fuzzer(dat):
    f = []
    for x in dat:
        okay = True
        for y in x:
            if "large" in y:
                okay = False

        if okay:
            f.append(x)

    return f

global fuzzers_drat
global fuzzers_nodrat
fuzzers_drat = fuzzers_noxor
fuzzers_nodrat = fuzzers_noxor + fuzzers_xor
if options.small:
    fuzzers_drat = filter_large_fuzzer(fuzzers_drat)
    fuzzers_nodrat = filter_large_fuzzer(fuzzers_nodrat)

print_version()
tester = Tester()
tester.needDebugLib = False
num = 0
rnd_seed = options.fuzz_seed_start
if rnd_seed is None:
    rnd_seed = random.randint(0, 1000*1000*100)

while True:
    toexec = "./fuzz_test.py --fuzzlim 1 --seed %d" % rnd_seed
    if options.novalgrind:
        toexec += " --novalgrind"
    if options.small:
        toexec += " --small"

    print("To re-create fuzz-test below: %s" % toexec)

    random.seed(rnd_seed)
    if random.choice([True, False]):
        tester.fuzz_test_preproc()
    else:
        tester.fuzz_test_one()
    rnd_seed += 1
    num += 1
    if options.fuzz_test_lim is not None and num >= options.fuzz_test_lim:
        exit(0)

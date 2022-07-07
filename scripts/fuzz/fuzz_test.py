#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
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
import sys
import time
import random
from random import choice
import optparse
import glob
import resource
from verifier import *
from functools import partial

print("our CWD is: %s files here: %s" % (os.getcwd(), glob.glob("*")))
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


def set_up_parser():
    parser = optparse.OptionParser(usage=usage, description=desc,
                                   formatter=PlainHelpFormatter())
    parser.add_option("--exec", metavar="SOLVER", dest="solver",
                      default="../../build/cryptominisat5",
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
    parser.add_option("--dovalgrind", dest="novalgrind", default=True,
                      action="store_false", help="Use valgrind installed -- NOT recommended, compile with SANTIZE instead!")

    parser.add_option("--valgrindfreq", dest="valgrind_freq", type=int,
                      default=10, help="1 out of X times valgrind will be used. Default: %default in 1")

    parser.add_option("--gauss", dest="gauss", default=False,
                      action="store_true",
                      help="Concentrate fuzzing gauss")
    parser.add_option("--sampling", dest="only_sampling", default=False,
                      action="store_true",
                      help="Concentrate fuzzing sampling variables")
    parser.add_option("--maxth", "-m", dest="max_threads", default=20,
                      type=int, help="Max number of threads")

    parser.add_option("--tout", "-t", dest="maxtime", type=int, default=25,
                      help="Max time to run. Default: %default")

    parser.add_option("--textra", dest="maxtimediff", type=int, default=5,
                      help="Extra time on top of timeout for processing."
                      " Default: %default")

    return parser


def fuzzer_call_failed(fname):
    print("OOps, fuzzer executable call failed!")
    print("Did you build with cmake -DENABLE_TESTING=ON? Did you do git submodules init & update?")
    print("Here is the output:")

    print("**** ----- ****")
    with open(fname, "r") as a:
        for x in a:
            print(x.strip())
    print("**** ----- ****")
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

            for _ in range(random.randrange(2, 4)):
                fname2 = unique_file("fuzzTest-multipart")
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
                status = subprocess.call(call, shell=True)
                if status != 0:
                    fuzzer_call_failed(fname2)

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
    if options.verbose:
        print("Executing: %s" % command)
    p = subprocess.Popen(command.rsplit(), stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE, universal_newlines=True)

    consoleOutput, err = p.communicate()
    print("Version values: %s" % consoleOutput.strip())


class Tester:

    def __init__(self):
        self.ignoreNoSolution = False
        self.extra_opts_supported = self.list_options_if_supported(
            ["xor", "autodisablegauss", "sql", "breakid", "predshort"])
        self.sol_parser = solution_parser(options)
        self.sqlitedbfname = None
        self.only_sampling = False
        self.sampling_vars = []
        self.this_gauss_on = False
        self.num_threads = 1
        self.novalgrind = options.novalgrind

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
            command.rsplit(), stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
            universal_newlines=True)

        consoleOutput, err = p.communicate()

        for l in consoleOutput.split("\n"):
            tmp_option_name = "--" + option_name
            if tmp_option_name in l:
                return True

        return False

    def create_rnd_sched(self, string_list):
        opts = string_list.split(",")
        opts = [a.strip(" ") for a in opts]
        opts = sorted(list(set(opts)))
        if options.verbose:
            print("available schedule options: %s" % opts)

        sched = []
        for _ in range(int(random.gammavariate(12, 0.7))):
            sched.append(random.choice(opts))

        # just so that XOR is really found and used, so we can fuzz it
        if "autodisablegauss" in self.extra_opts_supported:
            sched.append("occ-xor")

        return sched

    def rnd_schedule_all(self):
        sched_opts = "scc-vrepl,"
        sched_opts += "sub-impl, intree-probe,"
        sched_opts += "sub-str-cls-with-bin, distill-cls, scc-vrepl, sub-impl,"
        sched_opts += "sub-cls-with-bin,"
        sched_opts += "str-impl, sub-str-cls-with-bin, distill-cls, scc-vrepl,"
        sched_opts += "occ-backw-sub-str, occ-backw-sub, occ-xor, occ-clean-implicit, occ-bve, occ-bva,"
        sched_opts += "renumber, must-renumber,"
        sched_opts += "card-find, breakid,"
        sched_opts += "occ-lit-rem, distill-bins, occ-resolv-subs,occ-rem-with-orgates"

        # type of schedule
        cmd = ""
        sched = self.create_rnd_sched(sched_opts)
        sched = ",".join(sched)

        if sched != "":
            cmd += "--schedule %s " % sched

        sched = ",".join(self.create_rnd_sched(sched_opts))
        if sched != "":
            cmd += "--preschedule %s " % sched

        return cmd

    def random_options(self):
        self.sqlitedbfname = None
        cmd = " --zero-exit-status "

        # disable gauss when gauss is compiled in but asked not to be used
        #if not self.this_gauss_on and "autodisablegauss" in self.extra_opts_supported:
            #cmd += "--gauss 0 "

        cmd += "--presimp %d " % random.choice([1]*10+[0])
        if not options.gauss:
            cmd += "--confbtwsimp %d " % random.choice([100, 1000])
            cmd += "--nextm %f " % random.choice([0.1, 0.01, 0.001])
            cmd += "--everylev1 %d " % random.choice([122, 1222, 12222])
            cmd += "--everylev2 %d " % random.choice([133, 1333, 14444])

        if "breakid" in self.extra_opts_supported:
            cmd += "--breakid %d " % random.choice([1]*10+[0])
            cmd += "--breakideveryn %d " % random.choice([1]*10+[3])
            cmd += "--breakidcls %d " % random.choice([0, 1, 2, 3, 10]+[50]*4)
            cmd += "--breakidtime %d " % random.choice([10000]*5+[1])

        if options.gauss:
            sls = 0
            cmd += "--autodisablegauss %s " % random.choice([0]*15+[1])

            # Don't always use M4RI -- let G-J do toplevel, so fuzzing is more complete
            cmd += "--m4ri %d " % random.choice([0, 0, 0, 0, 1])

            # "Maximum number of matrixes to treat.")
            cmd += "--maxnummatrices %s " % int(random.gammavariate(1.5, 20.0))

        # SLS
        cmd += "--sls %d " % random.choice([0, 1])
        cmd += "--slsgetphase %d " % random.choice([0, 0, 0, 1])
        cmd += "--yalsatmems %d " % random.choice([1, 2, 5])
        cmd += "--walksatruns %d " % random.choice([2, 15, 20])

        # polarities
        cmd += "--polar %s " % random.choice(["true", "false", "rnd", "auto"])

        cmd += "--mustrenumber %d " % random.choice([0, 1])
        cmd += "--diffdeclevelchrono %d " % random.choice([1, random.randint(1, 1000), -1])
        cmd += "--bva %d " % random.choice([1, 1, 1, 0])
        cmd += "--bvaeveryn %d " % random.choice([1, random.randint(1, 20)])

        if self.only_sampling:
            cmd += "--onlysampling "
            cmd += "--sampling "
            cmd += ",".join(["%s" % x for x in self.sampling_vars]) + " "

        if random.choice([True, False]):
            cmd += "--varsperxorcut %d " % random.randint(4, 6)
            cmd += "--tern %d " % random.choice([0, 1])
            cmd += "--terntimelim %d " % random.choice([1, 10, 100])
            cmd += "--ternkeep %d " % random.choice([0, 0.001, 0.5, 300])
            cmd += "--terncreate %d " % random.choice([0, 0.001, 0.5, 300])
            cmd += "--locgmult %.12f " % random.gammavariate(0.5, 0.7)
            cmd += "--varelimover %d " % random.gammavariate(1, 20)
            cmd += "--memoutmult %0.12f " % random.gammavariate(0.05, 10)
            cmd += "--verb %d " % random.choice([0, 0, 0, 0, 1, 2])
            cmd += "--detachxor %d " % random.choice([0, 1, 1, 1, 1])
            cmd += "--restart %s " % random.choice(
                ["geom", "glue", "luby"])
            cmd += "--adjustglue %f " % random.choice([0, 0.5, 0.7, 1.0])
            cmd += "--gluehist %s " % random.randint(1, 500)
            cmd += "--updateglueonanalysis %s " % random.randint(0, 1)
            # cmd += "--clean %s " % random.choice(["size", "glue", "activity",
            # "prconf"])
            cmd += "--occredmax %s " % random.randint(0, 100)
            cmd += "--distill %s " % random.randint(0, 1)
            cmd += "--recur %s " % random.randint(0, 1)
            cmd += "--varelimcheckres %s " % random.randint(0, 1)
            cmd += "--implicitmanip %s " % random.randint(0, 1)
            cmd += "--occsimp %s " % random.randint(0, 1)
            cmd += "--occirredmaxmb %s " % random.gammavariate(0.2, 5)
            cmd += "--occredmaxmb %s " % random.gammavariate(0.2, 5)
            cmd += "--implsubsto %s " % random.choice([0, 10, 1000])
            cmd += "--sync %d " % random.choice([100, 1000, 6000, 100000])
            cmd += "-m %0.12f " % random.gammavariate(0.1, 5.0)
            cmd += "--maxsccdepth %d " % random.choice([0, 1, 100, 100000])

            # more more minim
            cmd += "--moremoreminim %d " % random.choice([1, 1, 1, 0])
            cmd += "--moremorealways %d " % random.choice([1, 1, 1, 0])

            if self.this_gauss_on:
                if random.randint(0,1000) < 10:
                    # Set maximum no. of rows for gaussian matrix."
                    cmd += "--maxmatrixrows %s " % int(random.gammavariate(1, 15.0))

                    # "Set minimum no. of rows for gaussian matrix.
                    cmd += "--minmatrixrows %s " % int(random.gammavariate(1, 15.0))

            if "sql" in self.extra_opts_supported and random.randint(0, 3) > 0 and self.num_threads == 1:
                cmd += "--sql 2 "
                self.sqlitedbfname = unique_file("fuzz", ".sqlitedb")
                cmd += "--sqlitedb %s " % self.sqlitedbfname
                cmd += "--sqlitedboverwrite 1 "
                cmd += "--cldatadumpratio %0.3f " % random.choice([0.9, 0.1, 0.7])

        # the most buggy ones, don't turn them off much, please
        if random.choice([1, 0, 0]):
            opts = ["scc", "varelim", "strengthen", "intree",
                    "renumber", "savemem", "moreminim", "gates",
                    "schedsimp", "otfhyper"]

            if "xor" in self.extra_opts_supported:
                opts.append("xor")

            for opt in opts:
                if opt == "xor" and self.this_gauss_on:
                    continue

                cmd += "--%s %d " % (opt, random.choice([0, 1, 1, 1, 1]))

            cmd += self.rnd_schedule_all()

        return cmd

    def execute(self, fname, fname_frat=None, fixed_opts="", rnd_opts=None):
        if os.path.isfile(options.solver) is not True:
            print("Error: Cannot find CryptoMiniSat executable.Searched in: '%s'" %
                  options.solver)
            print("Error code 300")
            exit(300)

        for f in glob.glob("%s-debugLibPart*.output" % fname):
            os.unlink(f)

        # construct command
        command = ""
        if not self.novalgrind and random.randint(1, options.valgrind_freq) == 1:
            command += "valgrind -q --leak-check=full  --error-exitcode=9 "
        command += options.solver
        if rnd_opts is None:
            rnd_opts = self.random_options()
        command += rnd_opts
        if self.needDebugLib:
            command += "--debuglib %s " % fname
        command += "--threads %d " % self.num_threads
        command += options.extra_options + " "
        command += fixed_opts + " "
        if fname is not None:
            command += "--input %s " % fname
        if fname_frat:
            command += " --frat %s " % fname_frat

        if options.verbose:
            print("Executing before doalarm/pipes: %s " % command)

        # print time limit
        if options.verbose:
            print("CPU limit of parent (pid %d)" % os.getpid(), resource.getrlimit(resource.RLIMIT_CPU))

        # if need time limit, then limit
        err_fname = unique_file("err", ".out")
        out_fname = unique_file("out", ".out")

        if self.num_threads == 1:
            time_limit = partial(setlimits, options.maxtime)
        else:
            time_limit = None

        myalarm = "./doalarm -t real %d "% options.maxtime
        toexec = myalarm + command + " 1>" + out_fname + " 2> " + err_fname
        print("Executing: " + toexec)
        retcode = os.system(toexec)

        # print time limit after child startup
        #if options.verbose:
            #print("CPU limit of parent (pid %d) after startup of child: %s secs" %
                  #(os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)))

        # Get solver output
        consoleOutput = ""
        with open(out_fname, "r") as f:
            for line in f:
                consoleOutput += line
        #retcode = p.returncode
        #err_file.close()
        with open(err_fname, "r") as err_file:
            found_something = False
            for line in err_file:
                line = line.strip()
                if line == "":
                    continue

                if options.verbose:
                    print("ERR line: ", line)

                # let's not care about leaks for the moment
                if "LeakSanitizer: detected memory leaks" in line:
                    break

                # don't error out on issues related to UBSAN/ASAN
                # of clang of other projects
                noprob = ["std::_Ios_Fmtflags", "mzd.h", "lexical_cast.hpp",
                      "MersenneTwister.h", "boost::any::holder", "~~~~~",
                      "SUMMARY", "process memory map", "==========="]

                ok = False
                for x in noprob:
                    if x in line:
                        ok = True

                if not ok:
                    found_something = True
                    errline = line

            if found_something:
                print("Error line while executing: %s" % errline.strip())
                exit(-1)

        os.unlink(err_fname)
        if self.sqlitedbfname is not None:
            os.unlink(self.sqlitedbfname)

        if options.verbose:
            print("CPU limit of parent (pid %d) after child finished executing: %s" %
                  (os.getpid(), resource.getrlimit(resource.RLIMIT_CPU)))

        os.unlink(out_fname)
        return consoleOutput, retcode

    def check(self, fname, fname_frat=None,
              checkAgainst=None,
              fixed_opts="",
              rnd_opts=None):

        consoleOutput = ""
        if checkAgainst is None:
            checkAgainst = fname
        curr_time = time.time()

        # Do we need to solve the problem, or is it already solved?
        consoleOutput, retcode = self.execute(
            fname, fname_frat=fname_frat,
            fixed_opts=fixed_opts, rnd_opts=rnd_opts)

        # if time was limited, we need to know if we were over the time limit
        # and that is why there is no solution
        diff_time = time.time() - curr_time
        if diff_time > (options.maxtime - options.maxtimediff):
            print("Too much time to solve, aborted!")
            return None

        print("Within time limit: %.2f s" % diff_time)
        print("filename: %s" % fname)

        if options.verbose:
            print(consoleOutput)

        if retcode != 0:
            print("Return code of CryptoMiniSat is not 0, it is: %d -- error!" % retcode)
            exit(-1)

        # if library debug is set, check it
        if (self.needDebugLib):
            must_check_unsat = True
            if options.gauss:
                must_check_unsat = random.choice([False]*15+[True])
            self.sol_parser.check_debug_lib(checkAgainst, must_check_unsat)

        print("Checking console output...")
        unsat, solution, _ = self.sol_parser.parse_solution_from_output(
            consoleOutput.split("\n"), self.ignoreNoSolution)

        if not unsat:
            if len(self.sampling_vars) != 0:
                self.sol_parser.sampling_vars_solution_check(fname, self.sampling_vars, solution)
            else:
                self.sol_parser.test_found_solution(solution, checkAgainst)

            return

        # it's UNSAT, let's check with FRAT
        if fname_frat:
            toexec = "./frat-rs elab {fname_frat} {cnf} -v"
            toexec = toexec.format(cnf=fname, fname_frat=fname_frat)
            print("Checking with FRAT.. ", toexec)
            p = subprocess.Popen(toexec.rsplit(),
                    stdout=subprocess.PIPE,
                    universal_newlines=True)

            consoleOutput2 = p.communicate()[0]
            diff_time = time.time() - curr_time

            # find verification code
            foundVerif = False
            fratLine = ""
            for line in consoleOutput2.split('\n'):
                if len(line) >= 8:
                    if line[0:8] == "VERIFIED":
                        fratLine = line
                        foundVerif = True

            # Check whether we have found a verification code
            if foundVerif is False:
                print("verifier error! It says: %s" % consoleOutput2)
                assert foundVerif, "Cannot find FRAT verification code!"
            else:
                print("OK, FRAT says: %s" % fratLine)

        if options.gauss:
            if random.choice([True, True, True, True, False]):
                print("NOT verifying with other solver, it's Gauss so too expensive")
                return

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
        print("--- NORMAL TESTING ---")
        self.num_threads = random.choice([1]+[random.randint(2,4)])
        self.num_threads = min(options.max_threads, self.num_threads)
        self.this_gauss_on = "autodisablegauss" in self.extra_opts_supported

        # frat turns off a bunch of systems, like symmetry breaking so use it about 50% of time
        self.frat = self.num_threads == 1 and (random.randint(0, 10) < 5)

        self.sqlitedbfname = None
        self.only_sampling = random.choice([True, False, False, False, False]) and not self.frat

        if options.only_sampling:
            self.frat = False
            self.only_sampling = True

        if self.frat:
            fuzzers = fuzzers_frat
        elif options.gauss:
            fuzzers = fuzzers_xor
        else:
            fuzzers = fuzzers_nofrat
        fuzzer = random.choice(fuzzers)

        fname = unique_file("fuzzTest")
        fname_frat = None
        if self.frat:
            fname_frat = unique_file("fuzzTest-frat")

        # create the fuzz file
        cf = create_fuzz()
        call, todel = cf.create_fuzz_file(fuzzer, fuzzers, fname)
        print("calling %s" % call)
        status = subprocess.call(call, shell=True)
        if status != 0:
            fuzzer_call_failed(fname)

        shuf_seed = random.randint(1, 100000)
        fname_shuffled = unique_file("fuzzTest")
        print("calling ./shuffle.py %s %s %s" % (fname, fname_shuffled, shuf_seed))
        shuffle_cnf(fname, fname_shuffled, shuf_seed)
        os.unlink(fname)
        fname = fname_shuffled

        if not self.frat and not self.only_sampling:
            print("->Multipart test")
            self.needDebugLib = True
            interspersed_fname = unique_file("fuzzTest-interspersed")
            seed_for_inters = random.randint(0, 1000000)
            intersperse(fname, interspersed_fname, seed_for_inters)
            print("Interspersed: ./intersperse.py %s %s %d" % (fname,
                                                               interspersed_fname,
                                                               seed_for_inters))
            os.unlink(fname)
        else:
            self.needDebugLib = False
            interspersed_fname = fname

        # calculate sampling vars
        self.sampling_vars = []
        if self.only_sampling:
            max_vars = self.sol_parser.max_vars_in_file(fname)
            assert max_vars > 0

            self.sampling_vars = []
            myset = {}
            for _ in range(random.randint(1, 50)):
                x = random.randint(1, max_vars)
                if x not in myset:
                    self.sampling_vars.append(x)
                    myset[x] = 1

            # don't do it for 0-length sampling vars
            if len(self.sampling_vars) == 0:
                self.only_sampling = False

        self.check(fname=interspersed_fname, fname_frat=fname_frat)

        # remove temporary filenames
        os.unlink(interspersed_fname)
        if fname_frat:
            os.unlink(fname_frat)
        for name in todel:
            os.unlink(name)

    def delete_file_no_matter_what(self, fname):
        try:
            os.unlink(fname)
        except:
            pass

fuzzers_noxor = [
    ["../../build/tests/sha1-sat/sha1-gen --nocomment --attack preimage --rounds 20",
     "--hash-bits", "--seed"],
    ["../../build/tests/sha1-sat/sha1-gen --nocomment --attack preimage --zero "
        "--message-bits 400 --rounds 8 --hash-bits 60",
     "--seed"],
    # ["build/cnf-fuzz-nossum"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/cnf-fuzz-biere"],
    ["../../build/tests/cnf-utils/makewff -cnf 3 250 1080", "-seed"],
    ["../../build/tests/cnf-utils/sgen4 -unsat -n 50", "-s"],
    ["../../build/tests/cnf-utils//sgen4 -sat -n 50", "-s"],
    ["../../utils/cnf-utils/cnf-fuzz-brummayer.py", "-s"],
    ["../../utils/cnf-utils/cnf-fuzz-xor.py", "--seed"],
    ["../../utils/cnf-utils/multipart.py", "special"]
]

fuzzers_xor = [
    ["../../utils/cnf-utils/xortester.py --varsmin 40", "--seed"],
    ["../../utils/cnf-utils/xortester.py --varsmin 60", "--seed"],
    ["../../utils/cnf-utils/xortester.py --varsmin 80", "--seed"],
    ["../../utils/cnf-utils/xortester.py --varsmin 100", "--seed"],
    ["../../utils/cnf-utils/xortester.py --varsmin 200", "--seed"],
    ["../../utils/cnf-utils/spacer_test.py", "--seed"],
    ["../../build/tests/sha1-sat/sha1-gen --xor --attack preimage --rounds 21",
     "--hash-bits", "--seed"],
]


if __name__ == "__main__":
    if not os.path.isdir("out"):
        print("Directory for outputs, 'out' not present, creating it.")
        os.mkdir("out")

    # parse options
    parser = set_up_parser()
    (options, args) = parser.parse_args()
    if options.valgrind_freq <= 0:
        print("Valgrind Frequency must be at least 1")
        exit(-1)

    fuzzers_frat = fuzzers_noxor
    fuzzers_nofrat = fuzzers_noxor + fuzzers_xor

    print_version()
    tester = Tester()
    tester.needDebugLib = False
    num = 0
    rnd_seed = options.fuzz_seed_start
    if rnd_seed is None:
        rnd_seed = random.randint(0, 1000*1000*100)

    while True:
        toexec = "./fuzz_test.py --fuzzlim 1 --seed %d " % rnd_seed
        if not options.novalgrind:
            toexec += "--dovalgrind "
        if options.valgrind_freq:
            toexec += "--valgrindfreq %d " % options.valgrind_freq
        if options.gauss:
            toexec += "--gauss "
        if options.only_sampling:
            toexec += "--sampling "
        toexec += "-m %d " % options.max_threads
        toexec += "-t %d " % options.maxtime

        print("")
        print("")
        print("--> To re-create fuzz-test below: %s" % toexec)

        random.seed(rnd_seed)
        tester.fuzz_test_one()
        rnd_seed += 1
        num += 1
        if options.fuzz_test_lim is not None and num >= options.fuzz_test_lim:
            exit(0)

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

from __future__ import print_function
import os
import glob
import sys
import optparse
import subprocess
import time
import gzip
import threading


class PlainHelpFormatter(optparse.IndentedHelpFormatter):

    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog [options] cryptominisat5-binary testfile(s)"
desc = """Test solver against some problems
"""

parser = optparse.OptionParser(usage=usage, description=desc,
                               formatter=PlainHelpFormatter())

parser.add_option("--verbose", "-v", action="store_true", default=False,
                  dest="verbose", help="Print more output")
parser.add_option("--threads", "-t", default=4, type=int,
                  dest="threads", help="Number of threads")

(options, args) = parser.parse_args()
print_lock = threading.Lock()
todo_lock = threading.Lock()

if len(args) < 1:
    print("ERROR: You must call this script with at least one argument, the cryptominisat5 binary")
    exit(-1)

if len(args) < 2:
    print("ERROR: You must call this script with at least one file to check")
    exit(-1)

cms4_exe = args[0]
if not os.path.isfile(cms4_exe):
    print("CryptoMiniSat executable you gave, '%s' is not a file. Exiting" % cms4_exe)
    exit(-1)

if not os.access(cms4_exe, os.X_OK):
    print("CryptoMiniSat executable you gave, '%s' is not executable. Exiting." % cms4_exe)
    exit(-1)


def go_through_cnf(f):
    maxvar = 0
    for line in f:
        line = line.strip()
        if len(line) == 0:
            continue
        if line[0] == "p":
            line = line.split()
            assert line[1].strip() == "cnf"
            assert line[2].isdigit()
            return int(line[2])

    assert False


def find_num_vars(fname):

    try:
        with gzip.open(fname, 'rb') as f:
            maxvar = go_through_cnf(f)
    except IOError:
        with open(fname, 'rb') as f:
            maxvar = go_through_cnf(f)

    return maxvar


minisat_exe = os.getcwd() + "/minisat/build/release/bin/minisat"
todo = []
exitnum = 0


class MyThread(threading.Thread):
    def __init__(self, threadID, extraopts):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.extraopts = extraopts

    def run(self):
        global todo
        global exitnum
        while len(todo) > 0:
            with todo_lock:
                fname = todo[0]
                todo = todo[1:]

            if options.verbose:
                with print_lock:
                    print("Thread %d pikced up %s" % (self.threadID, fname))

            ret = self.test_velim_one_file(fname)

            with todo_lock:
                exitnum |= ret

        if options.verbose:
            with print_lock:
                print("Finished thread %d" % self.threadID)

    def test_velim_one_file(self, fname):
        orig_num_vars = find_num_vars(fname)

        simp_fname = "simp.out-%d" % self.threadID
        try:
            os.unlink(simp_fname)
        except:
            pass

        toprint = ""

        toexec = [cms4_exe, "--zero-exit-status", "--preproc", "1", "--verb", "0"]
        toexec.extend(self.extraopts)
        toexec.extend([fname, simp_fname])

        toprint += "Executing: %s\n" % toexec

        start = time.time()
        cms_out_fname = "cms-%s.out" % os.path.split(fname)[1]
        try:
            with open(cms_out_fname, "w") as f:
                subprocess.check_call(" ".join(toexec), stdout=f, shell=True)
        except subprocess.CalledProcessError:
            # memory out????
            toprint += "*** ERROR CryptoMiniSat errored out!\n"
            with print_lock:
                print(toprint)
            return 1
        t_cms = time.time()-start
        num_vars_after_cms_preproc = find_num_vars(simp_fname)

        start = time.time()
        toexec = [minisat_exe, fname]
        toprint += "Executing: %s\n" % toexec
        minisat_out_fname = "minisat_elim_data.out-%d" % self.threadID
        try:
            with open(minisat_out_fname, "w") as f:
                subprocess.check_call(" ".join(toexec), stdout=f, shell=True)
        except subprocess.CalledProcessError:
            # probably solved it
            # can't really test.
            toprint += "** Minisat errored out...\n"
            with print_lock:
                print(toprint)
            return 0
        t_msat = time.time()-start

        var_elimed = None
        num_vars_after_ms_preproc = None
        with open(minisat_out_fname, "r") as f:
            for line in f:
                line = line.strip()
                if "num-vars-eliminated" in line:
                    var_elimed = int(line.split()[1])
                if "num-free-vars" in line:
                    num_vars_after_ms_preproc = int(line.split()[1])

        assert var_elimed is not None, "Couldn't find var-elimed line, out: %s" % toprint
        assert num_vars_after_ms_preproc is not None, "Couldn't find num-free-vars line, out: %s" % toprint

        toprint += "-> orig num vars: %d\n" % orig_num_vars
        toprint += "-> T-cms : %-4.2f free vars after: %-9d\n" % (t_cms, num_vars_after_cms_preproc)
        toprint += "-> T-msat: %-4.2f free vars after: %-9d\n" % (t_msat, num_vars_after_ms_preproc)
        diff = num_vars_after_cms_preproc - num_vars_after_ms_preproc
        limit = float(orig_num_vars)*0.05
        if diff < limit*8 and t_msat > t_cms*3 and t_msat > 20:
            toprint += " * MiniSat didn't timeout, but we did, acceptable difference.\n"
            return 0

        if diff > limit:
            toprint += "*** ERROR: No. vars difference %d is more than 5%% " % diff
            toprint += "of original no. of vars, %d\n" % limit
            return 1

        if t_cms > (t_msat*2 + 8):
            toprint += "*** ERROR: Time difference %d is too big!\n" % (t_cms-t_msat)
            return 1

        toprint += "------------------[ thread %d ]------------------------" % self.threadID

        with print_lock:
            print(toprint)

        return 0


def test(extraopts):
    exitnum = 0
    global todo
    todo = args[1:]

    threads = []
    for i in range(options.threads):
        threads.append(MyThread(i, extraopts))

    for t in threads:
        t.start()

    for t in threads:
        t.join()

    if exitnum == 0:
        print("ALL PASSED")
        subprocess.check_call("rm *.out", shell=True)
    else:
        print("SOME CHECKS FAILED")

    return exitnum

test(["--preschedule", "occ-bve,must-renumber"])

exit(exitnum)

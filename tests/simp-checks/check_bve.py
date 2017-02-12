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

(options, args) = parser.parse_args()


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
        if line[0] == "c" or line[0] == "p":
            continue

        line = line.split()
        for lit in line:
            if lit.isdigit():
                if abs(int(lit)) > maxvar:
                    maxvar = abs(int(lit))

    return maxvar


def find_num_vars(fname):

    try:
        with gzip.open(fname, 'rb') as f:
            maxvar = go_through_cnf(f)
    except IOError:
        with open(fname, 'rb') as f:
            maxvar = go_through_cnf(f)

    return maxvar


def test_velim_one_file(fname, extraopts):
    orig_num_vars = find_num_vars(fname)

    simp_fname = "simp.out"
    try:
        os.unlink(simp_fname)
    except:
        pass

    toexec = [cms4_exe, "--zero-exit-status", "--preproc", "1"]
    toexec.extend(extraopts)
    toexec.extend([fname, simp_fname])

    print("Executing: %s" % toexec)

    start = time.time()
    cms_out_fname = "cms-%s.out" % os.path.split(fname)[1]
    with open(cms_out_fname, "w") as f:
        subprocess.check_call(toexec, stdout=f)
    t_cms = time.time()-start
    num_vars_after_cms_preproc = find_num_vars(simp_fname)

    start = time.time()
    toexec = [minisat_exe, fname]
    print("Executing: %s" % toexec)
    with open("minisat_elim_data.out", "w") as f:
        subprocess.check_call(toexec, stdout=f)
    t_msat = time.time()-start

    var_elimed = None
    num_vars_after_ms_preproc = None
    with open("minisat_elim_data.out", "r") as f:
        for line in f:
            line = line.strip()
            if "num-vars-eliminated" in line:
                var_elimed = int(line.split()[1])
            if "num-free-vars" in line:
                num_vars_after_ms_preproc = int(line.split()[1])

    assert var_elimed is not None, "Couldn't find var-elimed line"
    assert num_vars_after_ms_preproc is not None, "Couldn't find num-free-vars line"

    print("-> orig num vars: %d" % orig_num_vars)
    print("-> T-cms : %-4.2f free vars after: %-9d" % (t_cms, num_vars_after_cms_preproc))
    print("-> T-msat: %-4.2f free vars after: %-9d" % (t_msat, num_vars_after_ms_preproc))
    diff = num_vars_after_cms_preproc - num_vars_after_ms_preproc
    limit = float(orig_num_vars)*0.05
    if diff < limit*8 and t_msat > t_cms*3 and t_msat > 20:
        print(" * MiniSat didn't timeout, but we did, acceptable difference.")
        return 0

    if diff > limit:
        print("*** ERROR: No. vars difference %d is more than 5%% of original no. of vars, %d" % (diff, limit))
        return 1

    if t_cms > (t_msat*2 + 8):
        print("*** ERROR: Time difference %d is too big!" % (t_cms-t_msat))
        return 1

    return 0

minisat_exe = os.getcwd() + "/minisat/build/minisat"


def test(extraopts):
    exitnum = 0
    for fname in args[1:]:
        exitnum |= test_velim_one_file(fname, extraopts)

    if exitnum == 0:
        print("ALL PASSED")
        subprocess.check_call("rm *.out", shell=True)
    else:
        print("SOME CHECKS FAILED")

    return exitnum

exitnum = 0
exitnum |= test(["--preschedule", "occ-bve, must-renumber"])

exit(exitnum)

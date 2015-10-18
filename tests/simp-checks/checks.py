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

import os
import glob
import sys
import optparse
import subprocess
import time

class PlainHelpFormatter(optparse.IndentedHelpFormatter):

    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog [options] cryptominisat4-binary testfile(s)"
desc = """Test solver against some problems
"""

parser = optparse.OptionParser(usage=usage, description=desc,
                               formatter=PlainHelpFormatter())

parser.add_option("--verbose", "-v", action="store_true", default=False,
                  dest="verbose", help="Print more output")

parser.add_option("--local", "-l", action="store_true", default=False,
                  dest="local", help="Not doing it on a remote server")

(options, args) = parser.parse_args()


if len(args) < 1:
    print "ERROR: You must call this script with at least one argument, the cryptominisat4 binary"
    exit(-1)

if len(args) < 2:
    print "ERROR: You must call this script with at least one file to check"
    exit(-1)

cms4_exe = args[0]
if not os.path.isfile(cms4_exe):
    print "CryptoMiniSat executable you gave, '%s' is not a file. Exiting" % cms4_exe
    exit(-1)

if not os.access(cms4_exe, os.X_OK):
    print "CryptoMiniSat executable you gave, '%s' is not executable. Exiting." % cms4_exe
    exit(-1)

def clone_and_make_minisat():
    print "Cloning and making minisat..."
    with open("minisat_build.out", "w") as f:
        subprocess.check_call("git clone --depth 1 --no-single-branch https://github.com/msoos/minisat.git".split(), stdout=f, stderr=f)
        os.chdir("minisat")
        subprocess.check_call("git checkout remotes/origin/only_elim_and_subsume".split(), stdout=f, stderr=f)
        subprocess.check_call("git checkout -b only_elim_and_subsume".split(), stdout=f, stderr=f)
        subprocess.check_call("make".split(), stdout=f, stderr=f)
        minisat_exe = os.getcwd() + "/build/release/bin/minisat"
        os.chdir("..")

    print "Done."
    return minisat_exe

def test_velim_one_file(fname):
    simp_fname = "simp.out"
    try:
        os.unlink(simp_fname)
    except:
        pass

    toexec = "%s --zero-exit-status -p1 %s %s" % (cms4_exe, fname, simp_fname)
    print "Executing: %s" % toexec

    start = time.time()
    cms_out_fname = "cms-%s.out" % os.path.split(fname)[1]
    with open(cms_out_fname, "w") as f:
        subprocess.check_call(toexec.split(), stdout=f)
    t_cms = time.time()-start

    start = time.time()
    with open("minisat_elim_data.out", "w") as f:
        subprocess.check_call([minisat_exe, simp_fname], stdout=f)
    t_msat = time.time()-start
    var_elimed = None
    with open("minisat_elim_data.out", "r") as f:
        for line in f:
            line = line.strip()
            if "num-vars-eliminated" in line:
                var_elimed = int(line.split()[1])

    assert var_elimed is not None, "Couldn't find var-elimed line"
    if var_elimed > 30:
        print "FAILED file %s" % fname
        exitnum = 1
    else:
        print "PASSED file %s" % fname
        exitnum = 0

    print "-> T-cms: %.2f T-msat: %.2f msat-bve: %d\n" % (t_cms, t_msat, var_elimed)
    return exitnum

if not options.local:
    minisat_exe = clone_and_make_minisat()
else:
    minisat_exe = os.getcwd() + "/minisat/build/release/bin/minisat"

exitnum = 0
for fname in args[1:]:
    exitnum |= test_velim_one_file(fname)

if exitnum == 0:
    print "ALL PASSED"
    subprocess.check_call("rm *.out", shell=True)
else:
    print "SOME CHECKS FAILED"

exit(exitnum)
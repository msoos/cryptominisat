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
import sys
import subprocess


def query_yes_no(question, default="no"):
    """Ask a yes/no question via raw_input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
        It must be "yes" (the default), "no" or None (meaning
        an answer is required of the user).

    The "answer" return value is True for "yes" or False for "no".
    """
    valid = {"yes": True, "y": True, "ye": True,
             "no": False, "n": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "
                             "(or 'y' or 'n').\n")

num = 15
ignore = "2,4,5,0,8,11,9,10"
ignore_elems = {}
for x in ignore.split(","):
    x = x.strip()
    if x == "":
        continue

    x = int(x)
    ignore_elems[x] = True

#os.rm -f
subprocess.call("rm outs/*", shell=True)
toexec = "./reconf.py -n %d -i %s  -f outs/out  ~/media/sat/out/satcomp091113/reconf*/*stdout*" % (num, ignore)
f = open("output", "w")
subprocess.call(toexec, shell=True, stdout=f)
f.close()

for i in range(num):
    if i in ignore_elems:
        continue

    print "reconf with %d" % i
    subprocess.call("cp outs/reconf.names outs/out%d.names" % i, shell=True)
    subprocess.call("c5.0 -u 20 -f outs/out%d -r > outs/out%d.c50.out" % (i, i),
                    shell=True)

subprocess.call("./tocpp.py -i %s -n %d > ../../src/features_to_reconf.cpp" % (ignore, num),
                shell=True)

subprocess.call("sed -i 's/red-/red_cl_distrib./g' ../../src/features_to_reconf.cpp",
                shell=True)

upload = query_yes_no("Upload to AWS?")
if upload:
    subprocess.call("aws s3 cp ../../src/features_to_reconf.cpp s3://msoos-solve-data/solvers/", shell=True)
    print "Uploded to AWS"
else:
    print "Not uploaded to AWS"



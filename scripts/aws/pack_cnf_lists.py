#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys

if len(sys.argv) == 1:
    print("""Usage example:
./{name} satcomp14_updated satcomp16_updated > satcomp1416_updated
""".format(name=sys.argv[0]))
    exit(-1)

files = {}
for a in range(len(sys.argv)-1):
    with open(sys.argv[a+1], "r") as f:
        for l in f:
            l = l.strip()
            fname = l[l.find("/")+1:]
            files[fname] = l

for a,b in files.iteritems():
    print(b)

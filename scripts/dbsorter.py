#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

#print "file: %s" % sys.argv[1]

def myabs(a) :
    return(abs(a))

f = open(sys.argv[1])
for line in f :
    if line[0] == "d" or line[0] == "p" or line[0] == "c":
        continue

    sp = [int(a) for a in line.split()]
    sp.sort(key=myabs)
    for i in sp :
        if i != 0 :
            sys.stdout.write("%s " % i)
    print "0"

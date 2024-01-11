#!/usr/bin/python

import os
import sys

if len(sys.argv) != 2:
    print("Must give proof")
    exit(-1)

cls = {}
xls = {}
line_no = 0
with open(sys.argv[1], "r") as f:
    for line in f:
        line_no+=1
        line = line.strip()
        if len(line) == 0:
            print("EMTPY LINE!!")
            exit(-1)
        # imply cl from x
        if line[0] == 'i':
            cl_xs = line[1:].split('l')
            assert len(cl_xs) == 2

            cl = cl_xs[0].split()
            id = int(cl[0])
            cl = cl[1:]
            if id in cls:
                print("ERROR, implied cl ", id, " already in cls")
                print("line no: ", line_no)
                print("line: ", line)
                exit(-1)
            cl = [int(l) for l in cl]
            cls[id] = sorted(cl)

            xs = cl_xs[1].split()
            xs = xs[:-1]
            for x in xs:
                if int(x) not in xls:
                    print("ERROR, cl implied by x ", x , " not in xls")
                    print("line no: ", line_no)
                    print("line: ", line)
                    exit(-1)
                assert x in xls
            continue

        if line[0] == 'a' or line[0] == 'o':
            assert line[1] == ' '
            line = line[2:]
            x_or_cl = line.split()
            assert len(x_or_cl) >= 2
            if x_or_cl[0].strip() == 'x':
                x = line[1:].split()
                id = int(x[0])
                ls = [int(l) for l in x[1:]]
                if id in xls:
                    print("ERROR, added/orig x ", id, " already in xls")
                    print("line no: ", line_no)
                    print("line: ", line)
                    exit(-1)
                assert id not in xls
                xls[id] = sorted(ls)
            else:
                c = str(line).split()
                id = int(c[0])
                ls = [int(l) for l in c[1:]]
                if id in cls:
                    print("ERROR, added/orig CL ", id, " already in cls")
                    print("line no: ", line_no)
                    print("line: ", line)
                    exit(-1)
                assert id not in xls
                xls[id] = sorted(ls)

        if line[0] == 'd' or line[0] == 'f':
            assert line[1] == ' '
            line = line[2:]
            x_or_cl = line.split()
            assert len(x_or_cl) >= 2
            if x_or_cl[0].strip() == 'x':
                x = line[1:].split()
                id = int(x[0])
                ls = [int(l) for l in x[1:]]
                if id not in xls:
                    print("ERROR, deleted/finalized x ", id, " not in xls")
                    print("line no: ", line_no)
                    print("line: ", line)
                    exit(-1)
                assert id in xls
                xls.pop(id)
            else:
                c = str(line).split()
                id = int(c[0])
                ls = [int(l) for l in c[1:]]
                if id in cls:
                    print("ERROR, deleted/finalized CL ", id, " not in cls")
                    print("line no: ", line_no)
                    print("line: ", line)
                    exit(-1)
                assert id in xls
                xls.pop(id)




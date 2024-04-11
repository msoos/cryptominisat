#!/usr/bin/python
import sys
import debuglib

if len(sys.argv) < 3:
    print("You must give at least 2 arguments: input file, output file")
    exit(-1)

if len(sys.argv) >= 5:
    print("You can only give at most 3 arguments, 3rd being the seed")
    exit(-1)

seed = 0
if len(sys.argv) > 3:
    seed = int(sys.argv[3])

debuglib.shuffle_cnf(sys.argv[1], sys.argv[2], seed)

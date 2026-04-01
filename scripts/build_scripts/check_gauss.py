#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import optparse
import subprocess
import copy
import calendar
import time


def check_one_conflict(orig_cnf, clause):

    newf = copy.deepcopy(orig_cnf)
    newf += "\n"
    for lit in clause:
        newf += "%d 0\n" % (-1*lit)

    toexec = "lingeling -f"
    if options.verbose:
        print("conflict check -- solving with other solver: %s" % toexec)
    curr_time = calendar.timegm(time.gmtime())
    p = subprocess.Popen(toexec.rsplit(),
                         stdout=subprocess.PIPE,
                         stdin=subprocess.PIPE)
    console_out = p.communicate(input=newf)[0]
    t = calendar.timegm(time.gmtime()) - curr_time
    console_out = map(str.strip, console_out.split('\n'))

    unsat = False
    for line in console_out:
        if "UNSATISFIABLE" in line:
            unsat = True

    if not unsat:
        print("OOOps, this is the issue: %s" % clause)
        exit(-1)


def get_clauses_to_verify(fname):
    clauses = []
    with open(fname, "r") as f:
        for line in f:
            line = line.strip()
            if "clause:" not in line:
                continue

            lits = line.split(",")[0].split(" ")[3:][:-1]
            lits = map(int, lits)
            clauses.append(lits)
            if options.verbose:
                print("lits: %s" % lits)

    return clauses


def get_xors_to_verify(fname):
    xors = []
    with open(fname, "r") as f:
        for line in f:
            line = line.strip()
            if line[:2] != "x ":
                continue

            vs = line[2:].split("=")[0].split("+")
            vs = map(str.strip, vs)
            vs = map(int, vs)
            rhs = line[2:].split("=")[1].strip()
            if rhs == "true":
                rhs = True
            elif rhs == "false":
                rhs = False
            else:
                print "ERROR: RHS is neither 'true' nor 'false'"
                exit(-1)
            if options.verbose:
                print("vars: %s rhs: %s" % (vs, rhs))

            xors.append([vs, rhs])
    return xors


def xor_to_clauses(vs, rhs):
    clauses = []
    for i in xrange(2**len(vs)):
        if bin(i).count("1") % 2 != rhs and len(vs) % 2 == 1:
            continue
        if bin(i).count("1") % 2 == rhs and len(vs) % 2 == 0:
            continue

        clause = []
        for v, pos in zip(vs, xrange(100000)):
            k = v
            if (i >> pos) & 1 == 0:
                k *= -1

            clause.append(k)

        clauses.append(clause)

    if options.verbose:
        print("XOR: %s rhs %s" % (vs, rhs))
        for cl in clauses:
            print(" --> %s" % cl)
    return clauses


def check():
    # Read in orignal file
    origf = ""
    with open(cnffname, "r") as f:
        origf = f.read()

    # get data from 'outfname'
    clauses = get_clauses_to_verify(outfname)
    xors = get_xors_to_verify(outfname)

    print("Calculating XORs' in clause version....")
    xor_clauses = []
    for x in xors:
        xor_clauses.extend(xor_to_clauses(x[0], x[1]))

    print("Checking XORs....")
    for clause in xor_clauses:
        if options.verbose:
            print("Checking (previously XOR) clause %s" % clause)
        check_one_conflict(origf, clause)
    print("XORs all OK")

    print("Checking propagations and conflicts...")
    for clause in clauses:
        if options.verbose:
            print("Checking clause %s" % clause)
        check_one_conflict(origf, clause)
    print("All props and conflicts verify.")


if __name__ == "__main__":
    usage = """usage: %prog [options] CNF gauss_output
Where gauss_output is the outptut of gauss with lines such as:
(0) prop clause: -43 84 27 143 -12 151 , rhs:1
(0) confl clause: -133 102 146 -149 8 -16 -172, rhs: 1

The system will verify each and every clause and check if it's a direct
consequence of the CNF. It exits on the first wrong clause it finds.
"""
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    (options, args) = parser.parse_args()

    if len(args) != 2:
        print("ERROR: You must give CNF and output file")
        exit(-1)

    cnffname = args[0]
    outfname = args[1]
    print("CNF: %s output file: %s" % (cnffname, outfname))

    check()

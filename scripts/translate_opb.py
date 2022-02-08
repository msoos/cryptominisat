#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2022 Authors of CryptoMiniSat, see AUTHORS file
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

import sys
import subprocess

def parse_lit(x):
    inv = 1
    if x[0] == '~':
        inv = -1
        x = x[1:]

    assert x[0] == 'x'
    v = int(x[1:])
    return v*inv

def leq_translate(c):
    #if len(c.lhs) == 1 and c.lhs[0][0] == 1 and c.cutoff == 0:
        #print("%d %d 0" % (-c.lhs[0][1], c.rhs))
        #print("%d %d 0" % (c.lhs[0][1], -c.rhs))
        #return
    #if len(c.lhs) == 1 and c.lhs[0][0] == -1 and c.cutoff == 1:
        #print("%d %d 0" % (-c.lhs[0][1], c.rhs))
        #print("%d %d 0" % (c.lhs[0][1], -c.rhs))
        #return

    #if len(c.lhs) == 1 and c.rhs == None and c.cutoff == 1:
        #if c.lhs[0][0] == 1:
            #print("%d 0" % c.lhs[0][1])
            #return
        #sys.stderr.write("ERRROOOR\n")
        #exit(-1)

    ins = []
    for l in c.lhs:
        if l[0] == 1:
            ins.append(l[1])
            continue
        else:
            ins.append(-l[1])
            c.cutoff+=1

    pr = "b "
    for l in ins:
        pr += "%d " % l

    pr += "0 "
    pr += "%d " % c.cutoff

    if c.rhs is not None:
        pr += "%d " % c.rhs
    print("%s" % pr)

class Constr:
    def __init__(self, lhs, cutoff, rhs, sign):
        self.lhs = lhs
        self.cutoff = cutoff
        self.rhs = rhs
        self.sign = sign

    def __repr__(self):
        return "lhs: %s cutoff: %s sign: %s rhs: %s" % (self.lhs, self.cutoff, self.sign, self.rhs)

    def simple(self):
        if self.sign != "=" and self.sign != ">=":
            return

        for l in self.lhs:
            if l[0] == 1 or l[0] == -1:
                continue
            else:
                return False

        return True

    def translate(self):
        print("c Doing simple:", self)
        assert self.simple()
        if self.sign == ">=":
            leq_translate(self)
        elif self.sign == "=":
            leq_translate(self)
            c = Constr(self.lhs, self.cutoff, self.rhs, self.sign)
            c.cutoff = len(c.lhs) - c.cutoff
            for i in range(len(c.lhs)):
                c.lhs[i][0] = -c.lhs[i][0]
            leq_translate(c)
        else:
            print("ERRROR")
            exit(-1)





def parse_constr(line):
    lhs = []
    cutoff = None
    rhs = None
    sign = ""

    at_lhs = True
    at_cutoff = False
    at_rhs = False
    done = False
    line = line.split(' ')
    i = 0
    #print("line:", line)
    while i < len(line):
        #print("SO FAR -- lhs: ", lhs, "cutoff: ", cutoff, "rhs: ", rhs, "at_lhs:", at_lhs, "at_cutoff:", at_cutoff, "at_rhs:", at_rhs)

        k = line[i].strip()
        if k == '':
            i+=1
            continue

        if at_lhs:
            if k == ">=" or k == "=":
                sign = k
                at_lhs = False
                at_cutoff= True
                i+=1
                continue
            #else:
                #sys.stderr.write("ERRROOOR: input has:" + k)
                #exit(-1)
            coeff = int(k)
            assert i+1 < len(line)
            i+=1
            k = line[i].strip()
            lit = parse_lit(k)
            lhs.append([coeff, lit])
            i+=1
            continue

        if at_cutoff:
            if k == "<->":
                at_cutoff= False
                at_rhs = True
                i+=1
                continue
            if k == ";":
                done = True
                break
            cutoff = int(k)
            i+=1
            continue

        if at_rhs:
            if k == ";":
                done = True
                break
            assert k[0] == 'x'
            rhs = parse_lit(k)
            i+=1
            continue

        assert False

    assert done

    #if rhs is not None:
        #print("lhs: ", lhs, "cutoff: ", cutoff, "rhs: ", rhs)
    #else:
        #print("lhs: ", lhs, "cutoff: ", cutoff)

    return Constr(lhs, cutoff, rhs, sign)


def do_pbsugar(constr, maxvar):
    with open("sugarin", "w") as f:
        f.write("* #variable= %d #constraint= 1\n" % maxvar)
        for i in constr.lhs:
            f.write("%d x%d " % (i[0],i[1]))
        f.write(" >= %d ;\n" % constr.cutoff)
    assert constr.rhs is None

    args = ("./pbsugar", "-map", "x", "-n", "sugarin", "-sat", "sugarout")
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    popen.wait()
    output = popen.stdout.read()
    #print("c pbsugar output: ", output)

    # get map
    varmap_end = 0
    varmap = {}
    with open("x", "r") as f:
        first = True
        for line in f:
            if first:
                num_max = int(line.strip())
                assert num_max == maxvar # or it had to use some extra vars
                first = False
                continue
            line=line.strip().split(' ')
            assert line[0][0] == 'x'
            new = int(line[1])
            orig = int(line[0][1:])
            varmap[new] = orig
            if new > varmap_end:
                varmap_end = new

    towrite = ""
    with open("sugarout", "r") as f:
        for line in f:
            if "p cnf" in line:
                continue
            line = line.strip().split()
            for lit in line:
                lit = int(lit)
                sign = lit < 0
                var = abs(lit)
                if var == 0:
                    towrite+= '0'
                    print(towrite)
                    towrite = ""
                    continue

                if var in varmap:
                    # part of original formula
                    var2 = varmap[var]
                else:
                    # fresh variable by pbsugar
                    assert var > varmap_end
                    maxvar+=1
                    var2 = maxvar

                if sign:
                    lit2 = -var2
                else:
                    lit2 = var2
                towrite+="%d " % lit2

    return maxvar


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("ERROR: must give ONE input, the OPB file")
        exit(-1)

    fname = sys.argv[1]

    # parse input
    ind = []
    constr = []
    maxvar = None
    with open(fname, "r") as f:
        # parse ind
        for line in f:
            line = line.strip()
            if len(line) == 0:
                continue

            if "#variable" in line:
                maxvar = int(line.split()[2])
                print("c maxvar: ", maxvar)
                continue

            if "* ind" in line:
                line = line[5:]
                for k in line.split(' '):
                    k = k.strip()
                    if k != "0" and k != '':
                        ind.append(int(k))
                continue

            if line[0] == '*':
                continue

            constr.append(parse_constr(line))

    # deal with independent
    pr = "c ind "
    for i in ind:
        pr += "%d " % i
    print("%s 0" % pr)

    # create output
    for c in constr:
        print("c doing constraint: ", c)
        if c.simple():
            c.translate()
        else:
            print("c Using pbsugar to deal with: ", c)
            maxvar = do_pbsugar(c, maxvar)



# -*- coding: utf-8 -*-
import commands
import os
import fnmatch
import gzip
import re
import getopt, sys

def check_regular_clause(line, maxvar):
  lits = line.split()
  for lit in lits :
    numlit = int(lit)
    var = abs(numlit)
    if (var == 0) : break
    maxvar = max(var, maxvar)
  return maxvar
  
def doit(fname):
  print "Examining CNF file %s" %(fname)
  
  f = gzip.open(fname, "r")
  maxvar = 0
  clauses = 0
  for line in f :
    #print "Examining line '%s'" %(line)
    line = line.rstrip()
    if (len(line) == 0) : continue
    if (line[0] != 'c' and line[0] != 'p') :
      if (line[0] != 'x') :
        maxvar = check_regular_clause(line, maxvar)
      else :
        print "OOOOPS! -- xor-clause"
        exit(-1);
        #self.check_xor_clause(line)
      clauses += 1

  f.close()
  print "Num clauses: %d maxvar: %d" %(clauses, maxvar)

  fname2 = fname[:len(fname)-7] + "-DIMACS.cnf.gz"
  fout = gzip.open(fname2, 'w')
  f = gzip.open(fname, "r")
  fout.write("p cnf %d %d\n" %(maxvar, clauses))
  for line in f :
        fout.write(line)
  fout.close()
  f.close()

dirList=os.listdir(".")
for fname in dirList:
  if fnmatch.fnmatch(fname, '*.cnf.gz'):
      doit(fname)

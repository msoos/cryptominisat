# -*- coding: utf-8 -*-
from __future__ import with_statement # Required in 2.5
import subprocess
import os
import fnmatch
import gzip
import re
import commands
import getopt, sys
import signal
import resource
import time
from subprocess import Popen, PIPE, STDOUT

maxTime = 20
maxTimeLimit = 17

def setlimits():
  sys.stderr.write("Setting resource limit in child (pid %d): %d s \n" %(os.getpid(), numsecs))
  resource.setrlimit(resource.RLIMIT_CPU, (maxTime, maxTime))

def addClause(variables) :
    assert(len(variables)>1)
    for i in range(0, len(variables)-1) :
        print "%d %d 0" %(int(variables[i]), -1*int(variables[i+1]))

#for arg in sys.argv:
#    print arg[1]
if (len(sys.argv) == 1) :
    print "c using origProblem.cnf"
    toexec= "./saucy -c origProblem.cnf 2>&1"
else:
    toexec= "./saucy --timeout=5 -c %s > myout 2>&1" %(sys.argv[1])

print "toexec:", toexec
p = subprocess.Popen(toexec.rsplit(), shell=True, preexec_fn=setlimits)
consoleOutput = p.communicate()[0]

f = open("myout", "r");
consoleOutput = f.read()
f.close()

myset = set()
mylines = consoleOutput.splitlines()
for line in mylines :
    thisLineDone = False
    line = line.rstrip();
    if (line.find("Error") != -1) :
        sys.stderr.write("c Error in saucy: %s", line)
        exit(-1)
    if (line.find("duplicate") != -1) :
        sys.stderr.write("c Duplicate in saucy");
        exit(-1)
    if (line == "") : continue
    print "c --------------------------------"
    tuples = line.split('(')
    for tup in tuples:
        tup = tup.rstrip()
        tup = tup.rstrip(")")
        if (tup == "") : continue
        #print "tuple:", tup
        variables = tup.split(' ')
        negated = ""
        for var in variables :
            negated += str(-1*int(var)) + " "
        negated = negated.rstrip(' ')
        #print "negated: ", negated
        if (negated in myset) :
            #print "FOUND!"
            addClause(variables)
            thisLineDone = True
            break
        else :
            myset.add(tup)
            #print "added:", tup


if (len(mylines) == 0) :
    sys.stderr.write("Error! saucy output is empty!");
    exit(-1)

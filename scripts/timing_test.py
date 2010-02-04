# -*- coding: utf-8 -*-
import commands
import os
import getopt, sys
import re

problemDir = "../satfiles/"
problem = ""
rangeStart = 0
rangeEnd = 0

def testOneFile(exe, fname) :
  sumtime = 0.0
  sumprop = 0
  myexec = "%s \"%s%s\""%(exe, problemDir, fname)
  s2 =  commands.getoutput(myexec)
  #print "%s \"%s%s\""%(exe, problemDir, fname)

  #print s2
  
  satisfiablePattern = re.compile("^s SATISFIABLE")
  unsatisfiablePattern = re.compile("^s UNSATISFIABLE")
  solved = False

  s2 = s2.splitlines()
  for l in s2:
    if "CPU time" in l:
      time = float(l[l.index(":")+1:l.rindex(" s")])
      sumtime += time
    if "propagations" in l:
      prop = int(l[l.index(":")+1:l.rindex("(")])
      sumprop += prop
    if (satisfiablePattern.match(l)):
        solved = True
        sat = True
    if (unsatisfiablePattern.match(l)) :
        solved = True
        sat = False
  
  if (solved == False) :
      print "Cannot find line starting with s!!!"
      print myexec
      print s2
      exit(-1)
        
  print "exe: %s file: %s, total props: %10d total time:%.2f" %(exe, fname, sumprop, sumtime)
  return (sumprop, sumtime, sat);


def usage():
  print "--problem (-p)     Name of the problem. E.g. grain, bivium"
  print "--dir     (-d)     Directory it is found in (default: ../satfiles/)"
  print "--result  (-r)     Filename where the results will be stored"
  print "--help    (-h)     Print this help screen"
  print "--rangeStart       Start of the range to solve"
  print "--rangeEnd         End of the range to solve (default = 0)"

resultFile = "results"

try:
  opts, args = getopt.getopt(sys.argv[1:], "p:d:r:", ["help", "problem=", "directory=", "result=", "rangeStart=", "rangeEnd="])
except getopt.GetoptError, err:
  print str(err)
  usage()
  sys.exit(2)

for opt, arg in opts:
    if opt in ("-h", "--help"):
        usage()
        sys.exit()
    elif opt in ("-p", "--problem"):
        problem = arg
    elif opt in ("-d", "--directory"):
        problemDir = arg
    elif opt in ("-r", "--result"):
        resultFile = arg
    elif opt in ("--rangeStart"):
        rangeStart = int(arg)
    elif opt in ("--rangeEnd"):
        rangeEnd = int(arg)
    else:
        assert False, "unhandled option"

if (problem == "") :
  print "Problem name must be given!"
  #usage()
  sys.exit()

if (rangeStart == 0) :
  print "--rangeStart=x must be given"
  sys.exit()

print "Problem name  : %s" %(problem)
print "Directory name: %s" %(problemDir)
print "Result file   : %s" %(resultFile)


for i in range (rangeStart, rangeEnd, -2):
  dirList=os.listdir(problemDir)
  bigSumTime1 = 0.0
  bigSumProp1 = 0
  bigSumTime2 = 0.0
  bigSumProp2 = 0
  execnum = 0
  for fname in dirList:
    mystr = "%s-%d-" %(problem, i)
    if (mystr in fname):
      execnum += 1
      (tmpProp, tmpTime, sat1) = testOneFile("../build/cryptominisat_ext.sh -d ../build/ -g 100 -v 0 -z -q -c -n ", fname)
      bigSumProp1 += tmpProp;
      bigSumTime1 += tmpTime;
      
      (tmpProp, tmpTime, sat2) = testOneFile("../build/cryptominisat_ext.sh -d ../build/ -v 0 -z -q -c -n ", fname)
      bigSumProp2 += tmpProp;
      bigSumTime2 += tmpTime;

      if (sat1 != sat2) :
          print "Satisfiablilities don't match!!!"
          exit(-1)

  if (execnum == 0) :
    continue

  timingFile = open(resultFile, 'a')
  print "given help bits: %d, ext1, total props: %10d total time:%.2f" %(i, bigSumProp1, bigSumTime1)
  timingFile.write("gauss\t%s\t%d\t%d\t%d\t%.2f\n" %(problem, execnum, i, bigSumProp1, bigSumTime1));
  print "given help bits: %d, ext2, total props: %10d total time:%.2f" %(i, bigSumProp2, bigSumTime2)
  timingFile.write("nogauss\t%s\t%d\t%d\t%d\t%.2f\n" %(problem, execnum, i, bigSumProp2, bigSumTime2));
  timingFile.close()


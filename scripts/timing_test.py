# -*- coding: utf-8 -*-
import commands
import os
import getopt, sys

problemDir = "../satfiles/"
problem = ""

def testOneFile(exe, fname) :
  sumtime = 0.0
  sumprop = 0
  s2 =  commands.getoutput("%s \"%s%s\""%(exe, problemDir, fname))
  #print "%s \"%s%s\""%(exe, problemDir, fname)

  #print s2
  
  s2 = s2.splitlines()
  for l in s2:
    if "CPU time" in l:
      time = float(l[l.index(":")+1:l.rindex(" s")])
      sumtime += time
    if "propagations" in l:
      prop = int(l[l.index(":")+1:l.rindex("(")])
      sumprop += prop
  print "exe: %s file: %s, total props: %10d total time:%.2f" %(exe, fname, sumprop, sumtime)
  return (sumprop, sumtime);


def usage():
  print "--problem (-p)     Name of the problem. E.g. grain, bivium"
  print "--dir     (-d)     Directory it is found in (default: ../satfiles/)"
  print "--result  (-r)     Filename where the results will be stored"
  print "--help    (-h)     Print this help screen"

resultFile = "results"

try:
  opts, args = getopt.getopt(sys.argv[1:], "p:d:r:", ["help", "problem=", "directory=", "result="])
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
    else:
        assert False, "unhandled option"

if (problem == "") :
  print "Problem name must be given!"
  #usage()
  sys.exit()

print "Problem name  : %s" %(problem)
print "Directory name: %s" %(problemDir)
print "Result file   : %s" %(resultFile)

timingFile = open(resultFile, 'a')

for i in range (60, 0, -2):
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
      (tmpProp, tmpTime) = testOneFile("./cryptominisat_ext.sh", fname)
      bigSumProp1 += tmpProp;
      bigSumTime1 += tmpTime;
      
      (tmpProp, tmpTime) = testOneFile("./cryptominisat_ext2.sh", fname)
      bigSumProp2 += tmpProp;
      bigSumTime2 += tmpTime;

  if (execnum == 0) :
    continue
  print "given help bits: %d, ext1, total props: %10d total time:%.2f" %(i, bigSumProp1, bigSumTime1)
  timingFile.write("gauss\t%s\t%d\t%d\t%d\t%.2f\n" %(problem, execnum, i, bigSumProp1, bigSumTime1));
  print "given help bits: %d, ext2, total props: %10d total time:%.2f" %(i, bigSumProp2, bigSumTime2)
  timingFile.write("nogauss\t%s\t%d\t%d\t%d\t%.2f\n" %(problem, execnum, i, bigSumProp2, bigSumTime2));


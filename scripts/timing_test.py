# -*- coding: utf-8 -*-
import commands
import os
# call MiniSat

mydir = "../satfiles/"
problem = "grain"

def testOneFile(exe, fname) :
  sumtime = 0.0
  sumprop = 0
  s2 =  commands.getoutput("%s \"%s%s\""%(exe, mydir,fname))

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

for i in range (110, 0, -2):
  dirList=os.listdir(mydir)
  bigSumTime1 = 0.0
  bigSumProp1 = 0
  bigSumTime2 = 0.0
  bigSumProp2 = 0
  for fname in dirList:
    mystr = "%s-%d" %(problem, i)
    if (mystr in fname):
      (tmpProp, tmpTime) = testOneFile("./cryptominisat_ext.sh", fname)
      bigSumProp1 += tmpProp;
      bigSumTime1 += tmpTime;
      
      (tmpProp, tmpTime) = testOneFile("./cryptominisat_ext2.sh", fname)
      bigSumProp2 += tmpProp;
      bigSumTime2 += tmpTime;

  print "given help bits: %d, ext1, total props: %10d total time:%.2f" %(i, bigSumProp1, bigSumTime1)
  print "given help bits: %d, ext2, total props: %10d total time:%.2f" %(i, bigSumProp2, bigSumTime2)

    




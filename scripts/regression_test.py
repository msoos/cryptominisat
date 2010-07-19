# -*- coding: utf-8 -*-
import commands
import os
import fnmatch
import gzip
import re
import getopt, sys


class Tester:

  sumTime = 0.0
  sumProp = 0
  verbose = False
  gaussUntil = 100
  testDir = "../tests/"
  testDirNewVar = "../tests/newVar/"
  cryptominisat = "../build/cryptominisat"
  speed = False
  checkDirDifferent = True
  differentDirForCheck = "/home/soos/Development/sat_solvers/satcomp09/"
  ignoreNoSolution = False
  arminFuzzer = False
  
  def __init__(self):
    self.sumTime = 0.0
    self.sumProp = 0
    self.verbose = False
    self.gaussUntil = 100
    self.testDir = "../tests/"
    self.testDirNewVar = "../tests/newVar/"
    self.cryptominisat = "../build/cryptominisat"
    self.speed = False
    self.checkDirOnly = False
    self.checkDirDifferent = False
    self.differentDirForCheck = "/home/soos/Development/sat_solvers/satcomp09/"
    self.ignoreNoSolution = False
    self.arminFuzzer = False

  def execute(self, fname, i, newVar):
    if (os.path.isfile(self.cryptominisat) != True) :
            print "Cannot file CryptoMiniSat executable. Searched in: '%s'" %(self.cryptominisat)
            exit()

    command = "%s --randomize=%d --debuglib "%(self.cryptominisat, i)
    if (newVar) :
        command += "--debugnewvar "
    command += "--gaussuntil=%d "%(self.gaussUntil);
    if (self.verbose == False) :
        command += "--verbosity=0 ";
    command += "\"%s\""%(fname);
    print "Executing: %s" %(command)
    consoleOutput =  commands.getoutput(command)
     
    if (self.verbose) :
      print consoleOutput
       
    return consoleOutput
       
  def parse_consoleOutput(self, consoleOutput):
    s3 = consoleOutput.splitlines()
    for l in s3:
        if "CPU time" in l:
            self.sumTime += float(l[l.index(":")+1:l.rindex(" s")])
        if "propagations" in l:
            self.sumProp += int(l[l.index(":")+1:l.rindex("(")])


  def read_found(self, of):
    f = open(of, "r")
    text = f.read()
    mylines = text.splitlines()
    f.close()

    if (len(mylines) == 0) :
      print "Error! CryptoMiniSat output is empty!"
      exit(-1)

    unsat = False
    if ('UNSAT' in mylines[0]) :
      unsat = True
    elif ('SAT' in mylines[0]) :
      unsat = False
    else :
      print "Error! Cannot find if SAT or UNSAT. Maybe didn't finish running?"
      exit(-1)

    value = {}
    if (len(mylines) > 1) :
      vars = mylines[1].split(' ')
      for var in vars:
        vvar = int(var)
        value[abs(vvar)] = ((vvar < 0) == False)

    os.unlink(of)
    return (unsat, value)


  def read_found_output(self, output):
    lines = output.splitlines()

    if (len(lines) == 0) :
      if (self.checkDirOnly == True) :
        print "Solving probably timed out"
        return (True, {})
      else :
        print "Error! CryptoMiniSat output is empty!"
        exit(-1)

    value = {}
    unsat = False
    sLineFound = False
    vLineFound = False
    for line in lines:
      if (line == 's UNSATISFIABLE') :
        unsat = True
        sLineFound = True
      elif (line == 's SATISFIABLE') :
        unsat = False
        sLineFound = True
      elif (re.match('^v ',line)) :
        vars = line.split(' ')
        vLineFound = True
        for var in vars:
            if (var == "v") :
              continue
            vvar = int(var)
            value[abs(vvar)] = ((vvar < 0) == False)
    
    if (self.ignoreNoSolution == False and (sLineFound == False or (unsat == False and vLineFound == False))) :
        print "Cannot find line starting with 's' or 'v' in output!";
        print output
        exit()
    if (self.ignoreNoSolution == True and (sLineFound == False or (unsat == False and vLineFound == False))) :
        print "Probably timeout, since no solution  printed. Could, of course, be segfault/assert fault, etc."
        print "Making it look like an UNSAT, so no checks!"
        return (True,[])
    
    #print "FOUND:"
    #print "unsat: %d" %(unsat)
    #for k, v in value.iteritems():
    #    print "var: %d, value: %s" %(k,v)
    
    return (unsat, value)


  def test_expect(self, unsat, value, expectOutputFile):
    if (os.path.isfile(expectOutputFile) != True) : return
    
    f = gzip.open(expectOutputFile, "r")
    text = f.read()
    f.close()
    
    indicated_value = {}
    indicated_unsat = False
    lines = text.splitlines()
    for line in lines :
      if ('UNSAT' in line) :
          indicated_unsat = True
      elif ('SAT' in line) :
          indicated_unsat = False
      else :
          #print "line: %s" %(line)
          stuff = line.split()
          indicated_value[int(stuff[0])] = (stuff[1].lstrip().rstrip() == 'true')

    
    #print "INDICATED:"
    #print "unsat: %d" %(indicated_unsat)
    if (unsat != indicated_unsat) :
        print "UNSAT vs. SAT problem!"
        exit()
    else :
        for k, v in indicated_value.iteritems():
            #print "var: %d, value: %s" %(k,v)
            if (indicated_value[k] != value[k]) :
              print "Problem of found values: values %d: '%s', '%s' don't match!" %(k, value[k], indicated_value[k])
              exit()

  def check_regular_clause(self, line, value):
    lits = line.split()
    final = False
    for lit in lits :
      numlit = int(lit)
      if (numlit != 0) :
        if (numlit < 0) : final |= ~value[abs(numlit)]
        else : final |= value[numlit]
        if (final == True) : break
    if (final == False) :
      print "Error: clause '%s' not satisfied." %(line)
      exit(-1)
  
  def check_xor_clause(self, line, value):
    line = line.lstrip('x')
    lits = line.split()
    final = False
    for lit in lits :
      numlit = int(lit)
      if (numlit != 0) :
        if (abs(numlit) not in value) :
          print "Error: var %d not solved, but referred to in a xor-clause of the CNF" %(abs(numlit))
          exit(-1)
        final ^= value[abs(numlit)]
        final ^= (numlit < 0)
    if (final == False) :
      print "Error: xor-clause '%s' not satisfied." %(line)
      exit(-1)

  def test_found(self, unsat, value, fname, debugLibPart = 1000000):
    if (debugLibPart == 1000000) :
      print "Examining CNF file %s" %(fname)
    else :
      print "Examining CNF file %s, part %d" %(fname, debugLibPart)
    
    if fnmatch.fnmatch(fname, '*.gz') :
      #tmpfname = "gunzip -c " + fname
      #f = os.popen(tmpfname, 'r')
      f = gzip.open(fname, 'r')
    else :
      f = open(fname, "r")
    clauses = 0
    thisDebugLibPart = 0
    for line in f :
      #print "Examining line '%s'" %(line)
      line = line.rstrip()
      if (len(line) == 0) : continue
      if (line[0] == 'c' and ("Solver::solve()" in line)) :
        thisDebugLibPart += 1
      if (thisDebugLibPart >= debugLibPart) :
        f.close()
        #print "Verified %d original xor&regular clauses" %(clauses)
        return
      if (line[0] != 'c' and line[0] != 'p') :
        if (line[0] != 'x') :
          self.check_regular_clause(line, value)
        else :
          self.check_xor_clause(line, value)
        
        clauses += 1
      
    f.close()
    #print "Verified %d original xor&regular clauses" %(clauses)
    
  def check(self, fname, fnameCheck, i, newVar, needSolve = True):
    consoleOutput = "";
    if (needSolve) :
        consoleOutput = self.execute(fname, i, newVar)
    else :
        f = open(fname + ".out", "r")
        consoleOutput = f.read()
        f.close()
    
    self.parse_consoleOutput(consoleOutput)
    print "filename: %20s, exec: %3d, total props: %10d total time:%.2f" %(fname[:20]+"....cnf.gz", i, self.sumProp, self.sumTime)
    
    if (self.speed == True) :
        return

    largestPart = -1
    dirList2 = os.listdir(".")
    for fname_debug in dirList2:
      if fnmatch.fnmatch(fname_debug, "debugLibPart*.output"):
        debugLibPart = int(fname_debug[fname_debug.index("t")+1:fname_debug.rindex(".output")])
        if (largestPart < debugLibPart) :
          largestPart = debugLibPart
    
    for debugLibPart in range(1,largestPart+1) :
      fname_debug = "debugLibPart%d.output" %(debugLibPart)
      #print "debugLibPart: %d, %s" %(debugLibPart, fname_debug)
      unsat, value = self.read_found(fname_debug)
      if (unsat == False) :
        self.test_found(unsat, value, fnameCheck, debugLibPart)
      else :
        print "Not examining part %d -- it is UNSAT" %(debugLibPart)
    
    #print "Checking against solution %s" %(of)
    #unsat, value = self.read_found(of)
    unsat, value = self.read_found_output(consoleOutput)
    otherSolverUNSAT = True
    if (self.arminFuzzer) :
      toexec = "./precosat %s" %(fname);
      #print "executing: %s" %(toexec)
      consoleOutput2 = commands.getoutput(toexec);
      #print "precosat output:"
      #print consoleOutput2
      otherSolverUNSAT, otherSolverValue = self.read_found_output(consoleOutput2)
    if (unsat == True) : 
      if (self.arminFuzzer == False) :
        print "Cannot check -- output is UNSAT"
      elif (self.arminFuzzer == True) :
        if (otherSolverUNSAT == False) :
          print "Grave bug: SAT-> UNSAT : Other solver found solution!!"
          exit()
        else : print "UNSAT verified by other solver"
    self.test_expect(unsat, value, fname[:len(fname)-6] + "output.gz")
    if (unsat == False) : 
        self.test_found(unsat, value, fnameCheck)

  @staticmethod
  def usage():
    print "--num     (-n)     The number of times to randomize and solve the same instance. Default: 3"
    print "--verbose (-v)     Verbose output"
    print "--file    (-f)     The file to solve. Default: all files under ../tests/"
    print "--gauss   (-g)     Execute gaussian elimination until this depth. Default: 10000"
    print "--testdir (-t)     The directory where the files to test are. Default: \"../tests/\""
    print "--exe     (-e)     Where the cryptominisat executable is located. Default: \"../build/cryptominisat\""
    print "--speed   (-s)     Only solve, don't verify the result"
    print "--checkDirOnly(-c) Check all solutions in directory"
    print "--diffCheckDir(-d) Use with -c. The original files are at a different place"
    print "--ignore  (-i)     If no solution found, (timeout), ignore"
    print "--armin   (-a)     Use Armin Biere's fuzzer"
    print "--help    (-h)     Print this help screen"

  def main(self):
    try:
      opts, args = getopt.getopt(sys.argv[1:], "vscihag:n:f:t:e:d:", ["help", "checkDirOnly", "file=", "num=", "gauss=", "testdir=", "exe=", "speed", "verbose", "diffCheckDir", "ignore", "armin"])
    except getopt.GetoptError, err:
      print str(err)
      self.usage()
      sys.exit(2)
    
    fname = None
    debugLib = False
    num = 3
    testDirSet = False
    for opt, arg in opts:
        if opt in ("-v", "--verbose"):
            self.verbose = True
        elif opt in ("-h", "--help"):
            self.usage()
            sys.exit()
        elif opt in ("-f", "--file"):
            fname = arg
        elif opt in ("-n", "--num"):
            num = int(arg)
        elif opt in ("-g", "--gauss"):
            self.gaussUntil = int(arg)
        elif opt in ("-t", "--testdir"):
            self.testDir = arg
            testDirSet = True
        elif opt in ("-e", "--exe"):
            self.cryptominisat = arg
        elif opt in ("-s", "--speed"):
            self.speed = True
        elif opt in ("-c", "--checkDirOnly"):
            self.checkDirOnly = True
        elif opt in ("-d", "--diffCheckDir"):
            self.differentDirForCheck = arg
            self.checkDirDifferent = True
        elif opt in ("-i", "--ignore"):
            self.ignoreNoSolution = True
        elif opt in ("-a", "--armin"):
            self.arminFuzzer = True
        else:
            assert False, "unhandled option"

    dirList2=os.listdir(".")
    for fname_unlink in dirList2:
        if fnmatch.fnmatch(fname_unlink, 'debugLibPart*'):
          os.unlink(fname_unlink);
          
    if (self.arminFuzzer) :
      for i in range(100) :
        commands.getoutput("./fuzzer > fuzzTest");
        for i2 in range(3):
          self.check("fuzzTest", "fuzzTest", i2, False)
      exit()
    
    if (self.checkDirOnly) :
        print "Checking already solved solutions"
        if (testDirSet == False) :
            print "When checking, you must give test dir"
            exit()
        dirList=os.listdir(self.testDir)
        for fname in dirList:
          myMatch = ""
          if (self.checkDirDifferent == True) :
            myMatch = '*.cnf.gz.out'
          else:
            myMatch = '*.cnf.gz'
          if fnmatch.fnmatch(fname, myMatch):
            myDir = self.testDir
            if (self.checkDirDifferent) :
              fname = fname[:len(fname)-4] #remove trailing .out
              myDir = self.differentDirForCheck
              self.check(self.testDir + fname, myDir + fname, 0, False, False)
        exit()
      

    if (fname == None) :
      if (testDirSet == False) :
        dirList=os.listdir(self.testDirNewVar)
        if (self.testDirNewVar == ".") :
          self.testDirNewVar = ""
        for fname in dirList:
          if fnmatch.fnmatch(fname, '*.cnf.gz'):
            for i in range(num):
              self.check(self.testDirNewVar + fname, self.testDirNewVar + fname, i, True)
     
      dirList=os.listdir(self.testDir)
      if (self.testDir == ".") :
        self.testDir = ""
      for fname in dirList:
        if fnmatch.fnmatch(fname, '*.cnf.gz'):
          for i in range(num):
            self.check(self.testDir + fname, self.testDir + fname, i, False)
            
    else:
      if (os.path.isfile(fname) == False) :
        print "Filename given '%s' is not a file!" %(fname)
        exit(-1)
      print "Checking fname %s" %(fname)
      
      for i in range(num):
        self.check(fname, fname, i, False)


test = Tester()
test.main()



# -*- coding: utf-8 -*-
import commands
import os
import fnmatch
import gzip
import getopt, sys

global testdir
testdir = "../tests/"


class Tester:

  greedyUnbound = False
  sumTime = 0.0
  sumProp = 0
  verbose = False
  gaussUntil = 10000
  
  def __init__(self):
    self.greedyUnbound = False
    self.sumTime = 0.0
    self.sumProp = 0
    self.verbose = False
    self.gaussUntil = 10000


  def execute(self, fname, i, of):
    if (os.path.isfile(of)) : os.unlink(of)
    cryptominisat = "../build/cryptominisat"
    if (os.path.isfile(cryptominisat) != True) :
            print "Cannot file CryptoMiniSat executable. Searched in: '%s'" %(cryptominisat)
            exit()

    command = "%s -randomize=%d "%(cryptominisat, i)
    if (self.greedyUnbound) :
        command += "-greedyUnbound "
    command += "-gaussuntil=%d \"%s\" %s"%(self.gaussUntil, fname, of)
    print "Executing: %s" %(command)
    consoleOutput =  commands.getoutput(command)
    
    if (os.path.isfile(of) != True) :
       print "OOops, output was not produced by CryptoMiniSat! Error!"
       print "Error log:"
       print consoleOutput
       exit()
     
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
      print "Error! MiniSat output is empty!"
      exit(-1);

    unsat = False
    if ('UNSAT' in mylines[0]) :
        unsat = True
    elif ('SAT' in mylines[0]) :
        unsat = False
    else :
        print "Error! Maybe didn't finish running?"
        exit(-1)

    value = {}
    if (len(mylines) > 1) :
        vars = mylines[1].split(' ')
        for var in vars:
             vvar = int(var)
             value[abs(vvar)] = ((vvar < 0) == False)
       
    #print "FOUND:"
    #print "unsat: %d" %(unsat)
    #for k, v in value.iteritems():
    #    print "var: %d, value: %s" %(k,v)
    
    os.unlink(of)
    return (unsat, value)


  def test_expect(self, unsat, value, expectOutputFile):
    if (os.path.isfile(expectOutputFile) != True) : return
    
    f = gzip.open(expectOutputFile, "r")
    text = f.read()
    f.close()
    
    indicated_value = {}
    indicated_unsat = False
    mylines = text.splitlines()
    for line in mylines :
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
        if (abs(numlit) not in value) :
          if (self.greedyUnbound == False) :
            print "var %d not solved, but referred to in a clause in the CNF" %(abs(numlit))
            exit(-1)
        else :
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

  def test_found(self, unsat, value, fname):
    f = gzip.open(fname, "r")
    line = f.readline()
    clauses = 0
    while line:
      #print "Examining line '%s'" %(line)
      line = line.rstrip()
      if (line[0] != 'c' and line[0] != 'p') :
        if (line[0] != 'x') :
          self.check_regular_clause(line, value)
        else :
          self.check_xor_clause(line, value);
        
        clauses += 1
          
      line = f.readline()
      
    print "Verified %d original xor&regular clauses" %(clauses)
      
    f.close();
    
  def check(self, fname, i):
    of = "outputfile"
    consoleOutput = self.execute(fname, i, of)
    self.parse_consoleOutput(consoleOutput)
    print "filename: %20s, exec: %3d, total props: %10d total time:%.2f" %(fname[:20]+"....cnf.gz", i, self.sumProp, self.sumTime)
    
    unsat, value = self.read_found(of)
    self.test_expect(unsat, value, fname[:len(fname)-6] + "output.gz")
    if (unsat == False) : 
      self.test_found(unsat, value, fname)

  @staticmethod
  def usage():
    print "--num     (-n)     The number of times to randomize and solve the same instance. Default: 3"
    print "--file    (-f)     The file to solve. Default: all files under ../tests/"
    print "--unbound (-u)     Greedily unbound variables after solving. The CNF will still remain SAT"
    print "--gauss   (-g)     Execute gaussian elimination until this depth. Default: 10000"
    print "--help    (-h)     Print this help screen"

  def main(self):
    try:
      opts, args = getopt.getopt(sys.argv[1:], "huvg:n:f:", ["help", "file=", "num=", "unbound", "gauss="])
    except getopt.GetoptError, err:
      print str(err)
      self.usage()
      sys.exit(2)
    
    fname = None
    num = 3
    for opt, arg in opts:
        if opt == "-v":
            self.verbose = True
        elif opt in ("-h", "--help"):
            self.usage()
            sys.exit()
        elif opt in ("-f", "--file"):
            fname = arg
        elif opt in ("-n", "--num"):
            num = int(arg)
        elif opt in ("-u", "--unbound"):
            self.greedyUnbound = True
        elif opt in ("-g", "--gauss"):
            self.gaussUntil = int(arg)
        else:
            assert False, "unhandled option"

    if (fname == None) :
      dirList=os.listdir(testdir)
      for fname in dirList:
        if fnmatch.fnmatch(fname, '*.cnf.gz'):
          for i in range(num):
            self.check(testdir + fname, i);
            
    else:
      if (os.path.isfile(fname) == False) :
        print "Filename given '%s' is not a file!" %(fname)
        exit(-1)
      
      for i in range(num):
        self.check(fname, i)


test = Tester()
test.main()



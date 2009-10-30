# -*- coding: utf-8 -*-
import commands
import os
import fnmatch
import gzip

sumt2 = 0.0
sumprop = 0
testdir = "../tests/"
dirList=os.listdir(testdir)
for fname in dirList:
    if fnmatch.fnmatch(fname, '*.cnf.gz'):
      for i in range(3):
          of = "outputfile"
          if (os.path.isfile(of)) : os.unlink(of)
          cryptominisat = "../build/cryptominisat"
          if (os.path.isfile(cryptominisat) != True) :
                  print "Cannot file CryptoMiniSat executable. Searched in: '%s'" %(cryptominisat)
                  exit()

          command = "%s -randomize=%d -gaussuntil=10 \"%s%s\" %s"%(cryptominisat, i, testdir, fname, of)
          print "Executing: %s" %(command)
          s2 =  commands.getoutput(command)
          if (os.path.isfile(of) != True) :
             print "OOops, output was not produced by CryptoMiniSat! Error!"
             exit()
          #print s2

          s2 = s2.splitlines()
          for l in s2:
              if "CPU time" in l:
                  t2 = float(l[l.index(":")+1:l.rindex(" s")])
                  sumt2 += t2
              if "propagations" in l:
                  p = int(l[l.index(":")+1:l.rindex("(")])
                  sumprop += p


          print "filename: %20s, exec: %3d, total props: %10d total time:%.2f" %(fname[:20]+"....cnf.gz", i, sumprop, sumt2)

          f = open(of, "r")
          text = f.read()
          mylines = text.splitlines()
          f.close()

          unsat = 0
          if ('UNSAT' in mylines[0]) :
              unsat = 1
          elif ('SAT' in mylines[0]) :
              unsat = 0
          else :
              print "Problem! Maybe didn't finish running? OOOppss!"
              exit(-1)

          value = {}
          if (len(mylines) > 1) :
              vars = mylines[1].split(' ')
              for var in vars:
                   vvar = int(var)
                   if (vvar < 0) :
                      value[abs(vvar)] = 'false'
                   else :
                      value[abs(vvar)] = 'true'
          
             
          #print "FOUND:"
          #print "unsat: %d" %(unsat)
          #for k, v in value.iteritems():
          #    print "var: %d, value: %s" %(k,v)
          
          
          myfname = testdir + fname[:len(fname)-6]
          myfname += "output.gz"
          f = gzip.open(myfname, "r")
          text = f.read()
          f.close()
          
          indicated_value = {}
          indicated_unsat = 0
          mylines = text.splitlines()
          for line in mylines :
            if ('UNSAT' in line) :
                indicated_unsat = 1
            elif ('SAT' in line) :
                indicated_unsat = 0
            else :
                #print "line: %s" %(line)
                stuff = line.split()
                indicated_value[int(stuff[0])] = stuff[1].lstrip().rstrip()
        
          
          #print "INDICATED:"
          #print "unsat: %d" %(indicated_unsat)
          if (unsat != indicated_unsat) :
              print "UNSAT vs. SAT problem!"
              os.unlink(of)
              exit()
          else :
              for k, v in indicated_value.iteritems():
                  #print "var: %d, value: %s" %(k,v)
                  if (indicated_value[k] != value[k]) :
                    print "Problem of found values: values %d: '%s', '%s' don't match!" %(k, value[k], indicated_value[k])
                    os.unlink(of)
                    exit()
          
          os.unlink(of)




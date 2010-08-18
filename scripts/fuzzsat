#!/usr/bin/env python

# Copyright 2009 Robert Daniel Brummayer
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import random, getopt, sys, signal

VAR = 0
AND = 1
OR = 2
IFF = 3
XOR = 4

g_version = "0.1"
g_min_num_vars = 1
g_max_num_vars = 100
g_num_vars = 0
g_min_rclauses_perc = 1 
g_max_rclauses_perc = 10
g_rclauses_perc = 0
g_min_rclause_len = 2
g_max_rclause_len = 6 
g_min_refs = 1
g_id = 0
g_nodes = {}
g_taut = None 
g_multilit = None 
g_hash = {}
g_unfinished1 = {} #variables that have to be referenced >= g_min_refs times
g_unfinished2 = {} #non-referenced nodes that are no variables

class Node:
  def __init__(self, kind, n0=None, n1=None, neg_n0=False, neg_n1=False):
    self.kind = kind
    self.n0 = n0
    self.n1 = n1
    self.neg_n0 = neg_n0
    self.neg_n1 = neg_n1
    self.refs = 0
    self.mark = 0 
    self.cnf_id = 0

def usage():
  print "********************************************************************************"
  print "*              FuzzSAT " + g_version + "                                                     *"
  print "*              Fuzzing Tool for CNF                                            *"
  print "*              written by Robert Daniel Brummayer, 2009                        *" 
  print "********************************************************************************" 
  print ""
  print "usage: fuzzsat [<option>...]"
  print ""
  print "where <option> is one of the following:"
  print ""
  print "  -h              print usage information and exit"
  print "  -V              print version and exit"
  print ""
  print "  -t              allow tautological clauses"
  print "  -m              allow multiple occurrences"
  print "                  of literals in clauses"
  print ""
  print "  -i <v>          use min <v> input variables          (default     1)"
  print "  -I <v>          use max <v> input variables          (default   100)"
  print "  -r <r>          use min <r> references               (default     1)"
  print ""
  print "  -p <p>          use min <p> % random clauses         (default     1)"
  print "  -P <p>          use max <p> % random clauses         (default    10)"
  print "  -l <l>          set min random clause length to <l>  (default     2)"
  print "  -L <l>          set max random clause length to <l>  (default     6)"
  print ""

def sighandler(signume, frame):
  sys.exit(0)

signal.signal (signal.SIGINT, sighandler)
signal.signal (signal.SIGTERM, sighandler)
signal.signal (signal.SIGQUIT, sighandler)
signal.signal (signal.SIGHUP, sighandler)

try:
  opts, args = getopt.getopt(sys.argv[1:], "hVtmi:I:r:p:P:l:L:")
except getopt.GetoptError, err:
  print str(err)
  usage()
  sys.exit(1)

for o, a in opts:
  if o in ("-h"):
    usage()
    sys.exit(0)
  if o in ("-V"):
    print g_version
    sys.exit(0)
  if o in ("-t"):
    g_taut = True
  elif o in ("-m"):
    g_multilit = True
  elif o in ("-i"):
    g_min_num_vars = int(a)
    if g_min_num_vars < 1:
      print "minimum number of variables must not be < 1"
      sys.exit(1)
  elif o in ("-r"):
    g_min_refs = int(a)
    if g_min_refs < 1:
      print "minimum number of references must not be < 1"
      sys.exit(1)
  elif o in ("-p"):
    g_min_rclauses_perc = int(a)
    if g_min_rclauses_perc < 0:
      print "minimum percentage of random clauses must not be < 0"
      sys.exit(1)
    if g_min_rclauses_perc > 100:
      print "minimum percentage of random clauses must not be > 100"
      sys.exit(1)
  elif o in ("-l"):
    g_min_rclause_len = int(a)
    if g_min_rclause_len < 1:
      print "minimum random clause length must not be < 1"
      sys.exit(1)
  elif o in ("-I"):
    g_max_num_vars = int(a)
    if g_max_num_vars < 1:
      print "maximum number of variables must not be < 1"
      sys.exit(1)
  elif o in ("-P"):
    g_max_rclauses_perc = int(a)
    if g_max_rclauses_perc < 0:
      print "maximum percentage of random clauses must not be < 0"
      sys.exit(1)
    if g_max_rclauses_perc > 100:
      print "maximum percentage of random clauses must not be > 100"
      sys.exit(1)
  elif o in ("-L"):
    g_max_rclause_len = int(a)
    if g_max_rclause_len < 1:
      print "maximum random clause length must not be < 1"
      sys.exit(1)
 
if g_min_num_vars > g_max_num_vars:
  print "minimum number of variables must not be > maximum"
  sys.exit(1)
if g_min_rclauses_perc > g_max_rclauses_perc:
  print "minimum percentage of random clauses must not be > maximum"
  sys.exit(1)
if g_min_rclause_len > g_max_rclause_len:
  print "minimum random clause length must not be > maximum"
  sys.exit(1)

g_num_vars = random.randint (g_min_num_vars, g_max_num_vars);
for i in range (0, g_num_vars):
  g_unfinished1[str(i)] = g_nodes[str(i)] = Node (VAR)
  g_id += 1

while len(g_unfinished1) > 0:
  op = random.randint (AND, IFF)
  id0 = random.randint (0, g_id - 1)
  id1 = random.randint (0, g_id - 1)
  n0 = g_nodes[str(id0)]
  n1 = g_nodes[str(id1)]
  neg0 = random.randint (0, 1) == 1
  neg1 = random.randint (0, 1) == 1

  if n0.refs >= g_min_refs - 1:
    if g_unfinished1.has_key(str(id0)):
      del g_unfinished1[str(id0)]
  if g_unfinished2.has_key (str(id0)):
    del g_unfinished2[str(id0)]

  if n1.refs >= g_min_refs - 1:
    if g_unfinished1.has_key(str(id1)):
      del g_unfinished1[str(id1)]
  if g_unfinished2.has_key (str(id1)):
    del g_unfinished2[str(id1)]

  n0.refs += 1
  n1.refs += 1
  g_unfinished2[str(g_id)] = g_nodes[str(g_id)] = Node (op, n0, n1, neg0, neg1)
  g_id += 1

# combine non-referenced nodes to one boolean root
while len(g_unfinished2.keys()) > 1:
  keys = g_unfinished2.keys()
  len_keys = len(keys)

  op = random.randint (AND, IFF)
  id0 = random.randint (0, len_keys - 1)
  id1 = random.randint (0, len_keys - 1)
  while id0 == id1:
    id1 = random.randint (0, len_keys - 1)
  neg0 = random.randint (0, 1) == 1
  neg1 = random.randint (0, 1) == 1

  key0 = keys[id0]
  n0 = g_nodes[key0]
  key1 = keys[id1]
  n1 = g_nodes[key1]
  del g_unfinished2[key0]
  del g_unfinished2[key1]
  g_unfinished2[str(g_id)] = g_nodes[str(g_id)] = Node (op, n0, n1, neg0, neg1)
  g_id += 1

assert (len(g_unfinished2) == 1)

#tseitin transformation
stack = list()
cnf_id = 1
num_clauses = 0
cnf = ""
root = g_unfinished2.values()[0]
stack.append (root)
while len(stack) > 0:
  cur = stack.pop()
  assert (cur != None)
  assert (cur.mark >= 0 and cur.mark <= 2)

  if cur.mark == 2:
    continue

  if cur.kind == VAR: 
    assert (cur.cnf_id == 0)
    cur.cnf_id = cnf_id
    cnf_id += 1
    cur.mark = 2
  else:
    if cur.mark == 0:
      cur.mark = 1
      stack.append (cur)
      stack.append (cur.n1)
      stack.append (cur.n0)
    else:
      assert (cur.mark == 1)
      cur.mark = 2
      assert (cur.cnf_id == 0)
      x = cur.cnf_id = cnf_id
      cnf_id += 1
      y = cur.n0.cnf_id
      assert (y != 0)
      if cur.neg_n0:
        y = -y
      z = cur.n1.cnf_id
      assert (z != 0)
      if cur.neg_n1:
        z = -z
       
      if cur.kind == AND:
        if g_taut or -x != -y:
          cnf += str(-x)
          if g_multilit or -x != y: 
            cnf += " " + str(y)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or -x != -z:
          cnf += str(-x)
          if g_multilit or -x != z: 
            cnf += " " + str(z)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (-y != z and -y != -x and -z != -x):
          cnf += str(-y)
          if g_multilit or -y != -z:
            cnf += " " + str(-z)
          if g_multilit or (-y != x and -z != x):
            cnf += " " + str(x)
          cnf += " 0\n"
          num_clauses += 1
      elif cur.kind == OR:
        if g_taut or x != y:
          cnf += str(x)
          if g_multilit or x != -y: 
            cnf += " " + str(-y)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or x != z:
          cnf += str(x)
          if g_multilit or x != -z: 
            cnf += " " + str(-z)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (y != -z and y != x and z != x):
          cnf += str(y)
          if g_multilit or y != z:
            cnf += " " + str(z)
          if g_multilit or (y != -x and z != -x):
            cnf += " " + str(-x)
          cnf += " 0\n"
          num_clauses += 1
      elif cur.kind == IFF:
        if g_taut or (-y != -z and -y != x and z != x):
          cnf += str(-y)
          if g_multilit or -y != z:
            cnf += " " + str(z)
          if g_multilit or (-y != -x and z != -x):
            cnf += " " + str(-x)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (y != z and y != x and -z != x):
          cnf += str(y)
          if g_multilit or y != -z:
            cnf += " " + str(-z)
          if g_multilit or (y != -x and -z != -x):
            cnf += " " + str(-x)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (-y != z and -y != -x and -z != -x):
          cnf += str(-y)
          if g_multilit or -y != -z:
            cnf += " " + str(-z)
          if g_multilit or (-y != x and -z != x):
            cnf += " " + str(x)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (y != -z and y != -x and z != -x):
          cnf += str(y)
          if g_multilit or y != z:
            cnf += " " + str(z)
          if g_multilit or (y != x and z != x):
            cnf += " " + str(x)
          cnf += " 0\n"
          num_clauses += 1
      else:
        assert (cur.kind == XOR)
        if g_taut or (-y != -z and -y != -x and z != -x):
          cnf += str(-y)
          if g_multilit or -y != z:
            cnf += " " + str(z)
          if g_multilit or (-y != x and z != x):
            cnf += " " + str(x)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (y != z and y != -x and -z != -x):
          cnf += str(y)
          if g_multilit or y != -z:
            cnf += " " + str(-z)
          if g_multilit or (y != x and -z != x):
            cnf += " " + str(x)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (-y != z and -y != x and -z != x):
          cnf += str(-y)
          if g_multilit or -y != -z:
            cnf += " " + str(-z)
          if g_multilit or (-y != -x and -z != -x):
            cnf += " " + str(-x)
          cnf += " 0\n"
          num_clauses += 1
        if g_taut or (y != -z and y != x and z != x):
          cnf += str(y)
          if g_multilit or y != z:
            cnf += " " + str(z)
          if g_multilit or (y != -x and z != -x):
            cnf += " " + str(-x)
          cnf += " 0\n"
          num_clauses += 1

assert (len(g_nodes) == g_id)
g_rclauses_perc = random.randint(g_min_rclauses_perc, g_max_rclauses_perc)
num_rclauses = int(round((g_rclauses_perc / 100.0) * num_clauses))
for i in range(0, num_rclauses):
  clause_len = random.randint(g_min_rclause_len, g_max_rclause_len)
  assert (clause_len > 0)
  g_hash.clear()
  for j in range (0, clause_len):
    id = random.randint(0, g_id - 1)
    x = g_nodes[str(id)].cnf_id
    assert (x != 0)
    if random.randint(0, 1) == 1:
      x = -x
    if (g_taut or not str(-x) in g_hash) and \
       (g_multilit or not str(x) in g_hash):
      cnf += str(x) + " "
      g_hash[str(x)] = True
  cnf += "0\n"
num_clauses += num_rclauses

print "c generated by FuzzSAT"
print "p cnf " + str(cnf_id - 1) + " " + str(num_clauses + 1)
sys.stdout.write (cnf)
assert (root.cnf_id != 0)
print str(root.cnf_id) + " 0"

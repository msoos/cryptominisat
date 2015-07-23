#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import gzip
import re
import ntpath

f = open("outs/out0.rules")
num_conds = 0
cond_no = 0
num_rules = 0
rule_no = 0
string = ""
for line in f:
    if "id=" in line:
        continue

    line = line.strip()
    line = line.split(" ")
    dat = {}
    for elem in line:
        elems = elem.split("=")
        elems = [e.strip("\"") for e in elems]
        # print "elems:", elems
        dat[elems[0]] = elems[1]

    if "conds" in dat:
        assert num_conds == cond_no
        num_conds = int(dat["conds"])
        rule_class = dat["class"]
        cond_no = 0
        continue

    if "entries" in dat:
        continue

    if "rules" in dat:
        num_rules = int(dat["rules"])

    if "default" in dat:
        default = dat["default"]
        if default == "+":
            print "double output = %f;" %(1.0)
        else:
            print "double output = %f;" %(0.0)
        continue

    #process rules
    if cond_no == 0:
        string = "if "
    else:
        string +=" &&\n    "

    # print "dat:", dat
    string +="(%s %s %s)" % (dat["att"], dat["result"], dat["cut"])
    cond_no+= 1

    #end rules
    if cond_no == num_conds:
        string +="\n{"
        print string

        string = ""
        string += "    output = "
        if rule_class == "+":
            string += "1.0;"
        else:
            string += "0.0;"

        print string
        print "}"
        rule_no += 1

    # print dat

print "num_rules:", num_rules
print "rule_no:", rule_no
assert num_rules == rule_no
print "default is:", default
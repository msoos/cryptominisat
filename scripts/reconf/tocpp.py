#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import gzip
import re
import ntpath

from optparse import OptionParser
parser = OptionParser()
parser.add_option("-n", "--num",
                  dest="num", type=int,
                  help="Number of reconfs")

(options, args) = parser.parse_args()

if options.num is None:
    print "ERROR: You must give the number of reconfs"
    exit(-1)

print """
#ifndef _FEATURES_TO_RECONF_H_
#define _FEATURES_TO_RECONF_H_

#include "features.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {
"""

for i in range(options.num):
    print "double get_score%d(const Features& feat);" %i

print """
int get_reconf_from_features(const Features& feat, const int verb)
{
\tdouble best_score = 0.0;
\tint best_val = 0;
\tdouble score;
"""

for i in range(options.num):
    print """
\tscore = get_score%d(feat);
\tif (verb >= 2)
\t\tcout << "c Score for reconf %d is " << score << endl;
\tif (best_score < score) {
\t\tbest_score = score;
\t\tbest_val = %d;
\t}
""" % (i, i, i)

print """
\tif (verb >= 2)
\t\tcout << "c Winning reconf is " << best_val << endl;
\treturn best_val;
}

"""

def read_one_rule(rule_num) :
    f = open("outs/out%d.rules" % rule_num)
    num_conds = 0
    cond_no = 0
    num_rules = 0
    rule_no = 0
    string = ""

    print """
double get_score%d(const Features& feat)
{""" % rule_num
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
            confidence = float(dat["confidence"]);
            continue

        if "entries" in dat:
            continue

        if "rules" in dat:
            num_rules = int(dat["rules"])

        if "default" in dat:
            default = dat["default"]
            if default == "+":
                print "\tdouble default_val = %.2f;\n" %(1.0)
            else:
                print "\tdouble default_val = %.2f;\n" %(0.0)

            print "\tdouble total_plus = 0.0;"
            print "\tdouble total_neg = 0.0;"
            continue

        #process rules
        if cond_no == 0:
            string = "\tif ("
        else:
            string +=" &&\n\t\t"

        # print "dat:", dat
        string +="(feat.%s %s %.5f)" % (dat["att"], dat["result"], float(dat["cut"]))
        cond_no+= 1

        #end rules
        if cond_no == num_conds:
            string +=")\n\t{"
            print string

            string = ""
            if rule_class == "+":
                string += "\t\ttotal_plus += %.3f;" % confidence
            else:
                string += "\t\ttotal_neg += %.3f;" % confidence

            print string
            print "\t}"
            rule_no += 1

        # print dat

    print "\t// num_rules:", num_rules
    print "\t// rule_no:", rule_no
    assert num_rules == rule_no
    print "\t// default is:", default
    print """
\tif (total_plus == 0.0 && total_neg == 0.0) {
\t\treturn default_val;
\t}
\treturn total_plus - total_neg;
}
"""



for i in range(options.num):
    read_one_rule(i)

print """
} //end namespace

#endif //_FEATURES_TO_RECONF_H_

"""


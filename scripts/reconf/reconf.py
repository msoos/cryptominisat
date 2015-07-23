#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import gzip
import re
import ntpath

from optparse import OptionParser
parser = OptionParser()
parser.add_option("-f", "--file",
                  dest="outfname", type=str,
                  help="print final values to this file")
parser.add_option("-r", "--reconf",
                  dest="reconf", type=int,
                  help="print final values to this file")
parser.add_option("-p", "--plusminus",
                  dest="plusminus", default=False,
                  action="store_true",
                  help="print final element as +/-")


(options, args) = parser.parse_args()
# print "args:", args

if options.reconf is None:
    print "You must give --reconf"
    exit(-1)


def parse_features_line(line):
    line = re.sub("c.*features. ", "", line)
    line = line.strip().split(" ")
    # print line
    dat = {}

    name = ""
    for elem, i in zip(line, xrange(1000)):
        elem = elem.strip(":").strip(",")
        if i % 2 == 0:
            name = elem
            continue

        if name == "(numVars/(1.0*numClauses)":
            name = "var_cl_ratio"
        dat[name] = elem
        # print name, " -- ", elem
        name = ""

#    for name, val in dat.items():
#        print "%s:\t\tcontinious" % name

    return dat


def nobody_could_solve_it(reconf_score):
    for r_s_elem in reconf_score:
        if r_s_elem[1] != 0:
            return False

    return True


def all_above_fixed_score(reconf_score, score_limit):
    for x in reconf_score:
        if x[1] < score_limit:
            return False

    return True


def print_features_and_scores(fname, features, reconfs_scores):
    r_s = sorted(reconfs_scores, key=lambda x: x[1])[::-1]
    best_reconf = r_s[0][0]
    best_reconf_score = r_s[0][1]
    print r_s

    if nobody_could_solve_it(r_s):
        print "%s Nobody could solve it" % fname
        return -1, False

    if all_above_fixed_score(r_s, 4500):
        print "%s All above score" % (fname)
        return -2, False

    #calculate final array
    final_array = [0.0]*12
    val = 1.0
    best_score = r_s[0][1]
    for conf_score, i in zip(r_s, xrange(100)):
        diff = abs(conf_score[1]-best_score)
        best_score = conf_score[1]
        val -= float(diff)/2000.0
        if diff > 0:
            val -= 0.1

        if val < 0.0 or conf_score[1] == 0:
            val = 0.0

        if conf_score[1] > 0:
            final_array[conf_score[0]] = val

    #print final string
    string = ""
    #string += "%s," % (fname)
    for name, val in features.items():
        string += "%s," % val

    string += "||"
    if not options.plusminus:
        string += "%.3f" % final_array[options.reconf]
        string += "||"
        for a in final_array:
            string += "%.1f " % a
    else:
        if abs(final_array[options.reconf]-0.5) < 0.05:
            string += "+"
        else:
            string += "-"

    print fname, string
    if outf:
        outf.write(string.replace("||", "") + "\n")
    only_this_could_solve_it = r_s[1][1] == 0
    return best_reconf, only_this_could_solve_it


def parse_file(fname):
    f = gzip.open(fname, 'rb')
    #print "fname orig:", fname
    fname_clean = re.sub("cnf.gz-.*", "cnf.gz", fname)
    fname_clean = ntpath.split(fname_clean)[1]
    reconf = 0

    satisfiable = None
    features = None
    score = 0
    for line in f:
        line = line.strip()
        #print "parsing line:", line
        if features is None and "features" in line and "numClauses" in line:
            features = parse_features_line(line)

        if "Total time" in line:
            time_used = line.strip().split(":")[1].strip()
            score = int(round(float(time_used)))
            #score -= score % 1000
            score = 5000-score

        if "reconfigured" in line:
            reconf = line.split("to config")[1].strip()
            reconf = int(reconf)

        if "s SATIS" in line:
            satisfiable = True

        if "s UNSATIS" in line:
            satisfiable = False

    #if satisfiable == True:
    #    score = 0

    return fname_clean, reconf, features, score

all_files = set()
all_files_scores = {}
all_files_features = {}
for x in args:
    print "# parsing infile:", x
    fname, reconf, features, score = parse_file(x)
    if fname in all_files:
        if all_files_features[fname] != features:
            print "ERROR different features extracted for fname", fname
            print "orig:", all_files_features[fname]
            print "new: ", features
        assert all_files_features[fname] == features
    else:
        all_files.add(fname)
        all_files_features[fname] = features
        all_files_scores[fname] = []

    #print "fname:", fname
    all_files_scores[fname].append([reconf, score])

    sys.stdout.write(".")
    sys.stdout.flush()

print "END--------"
print "all files:", all_files
print ""
outf = None
if options.outfname:
    outf = open(options.outfname, "w")

best_reconf = {}
only_this = {}
for fname in all_files:
    #print "fname:", fname
    if all_files_features[fname] is not None:
        best, only_this_could_solve_it = print_features_and_scores(fname, all_files_features[fname], all_files_scores[fname])

        if best not in best_reconf:
            best_reconf[best] = 1
        else:
            best_reconf[best] = best_reconf[best] + 1

        if only_this_could_solve_it:
            if best not in only_this:
                only_this[best] = 1
            else:
                only_this[best] = only_this[best] + 1

        print ""

print "best reconfs: ", best_reconf
print "uniquely solved by: ", only_this


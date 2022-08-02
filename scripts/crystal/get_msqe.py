#!/bin/python3

import sys
import os

assert len(sys.argv) >= 2


tiers = {"short":[], "long":[], "forever":[]}

for mydir in sys.argv[1:]:
    if not os.path.isdir(mydir):
        print("WARN: %s is not a directory, skipping" % mydir)
        continue
    print(mydir)
    print("========")
    for tier in ["short", "long", "forever"]:
        for table in ["used_later", "used_later_anc"]:
            fname = "%s/out-%s-%s" %(mydir, table, tier)
            if not os.path.isfile(fname):
                print("ERROR: the file '%s' does not exist!" % fname)
                exit(-1)

            test = False
            all_dump_no = False
            with open(fname, "r")  as f:
                out = "Tier %-9s " % tier
                for line in f:
                    line = line.strip()
                    if " test data" in line:
                        test = True

                    if not test:
                        continue

                    if "ALL dump_no" in line:
                        all_dump_no = True

                    if not all_dump_no:
                        continue

                    if "train+test" in line:
                        break

                    if "train data" in line:
                        break

                    assert test
                    if "Mean squared error is" in line:
                        mydat = line.split(" ")
                        val = float(mydat[5])
                        tiers[tier].append(["%s-%s" % (mydir, table), val])
                        out += line
                        print(out)
                        break

for tier in ["short", "long", "forever"]:
    print("TIER: ", tier)
    top =  sorted(tiers[tier], key=lambda x: x[1], reverse=False)
    for t in top:
        print(t)

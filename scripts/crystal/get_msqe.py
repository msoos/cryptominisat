#!/bin/python3

import sys

assert len(sys.argv) >= 2

for dir in sys.argv[1:]:
    print(dir)
    print("========")
    for tier in ["short", "long", "forever"]:
        test = False
        all_dump_no = False
        with open("%s/out_%s" %(dir, tier), "r")  as f:
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
                    out += line
                    print(out)
                    break

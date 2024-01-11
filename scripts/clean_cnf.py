#!/usr/bin/python
import sys

if len(sys.argv) != 3:
    print("You must give 2 arguments: input file, output file")
    exit(-1)

def shuffle_cnf(fname1, fname2):
    with open(fname1, "r") as f1:
        headers = []
        lines = []
        for line in f1:
            line = line.strip()
            if len(line) == 0:
                continue
            if line[0] == 'p':
                headers.append(line)
            elif line[0] == 'c':
                continue
            elif line[0] == 'x':
                line = line[1:]
                line = [e.strip() for e in line.split()]
                assert line[-1] == "0"
                line = line[:-1] # remove 0
                line = sorted(set(line))
                l = "x " + " ".join([str(e) for e in line]) + " 0"
                lines.append(l)
            else:
                line = [e.strip() for e in line.split()]
                assert line[-1] == "0"
                line = line[:-1] # remove 0
                line = sorted(set(line))
                l = " ".join([str(e) for e in line]) + " 0"
                lines.append(l)

    with open(fname2, "w") as f2:
        for line in headers:
            f2.write(line+"\n")
        for line in lines:
            f2.write(line+"\n")

shuffle_cnf(sys.argv[1], sys.argv[2])

#!/usr/bin/python
import sys

if len(sys.argv) != 3:
    print("You must give 2 arguments: input file, output file")
    exit(-1)

def check_duplicate_invert(l):
    m = list(l)
    m2 = set()
    for a in m:
        a = int(a)
        a = abs(a)
        if abs(a) in m2:
            # print("NOTE! inverted line: ", l)
            return False
        m2.add(abs(a))

    return True

def update_maxv( maxv, line):
    l = [abs(int(e)) for e in line]
    return max(maxv, max(l))

def shuffle_cnf(fname1, fname2):
    comments = []
    lines = []
    maxv = 0
    with open(fname1, "r") as f1:
        for line in f1:
            line = line.strip()
            orig_line = str(line)
            if len(line) == 0:
                continue
            if line[0] == 'p':
                continue
            elif line[0] == 'c':
                comments.append(line)
                continue
            elif line[0] == 'x':
                line = line[1:]
                line = [e.strip() for e in line.split()]
                assert line[-1] == "0"
                line = line[:-1] # remove 0
                elems = len(line)
                line = set(line)
                if len(line) != elems:
                    # print("NOTE: duplicate line: ", orig_line)
                    pass
                ok = check_duplicate_invert(line)
                if ok:
                    line = sorted(line)
                    maxv = update_maxv(maxv, line)
                    l = "x " + " ".join([str(e) for e in line]) + " 0"
                    lines.append(l)
            else:
                line = [e.strip() for e in line.split()]
                assert line[-1] == "0"
                line = line[:-1] # remove 0
                elems = len(line)
                line = set(line)
                if len(line) != elems:
                    print("NOTE: duplicate line: ", orig_line)
                ok = check_duplicate_invert(line)
                if ok:
                    line = sorted(line)
                    maxv = update_maxv(maxv, line)
                    l = " ".join([str(e) for e in line]) + " 0"
                    lines.append(l)

    with open(fname2, "w") as f2:
        f2.write("p cnf %d %d\n" % (maxv, len(lines)))
        for line in comments:
            f2.write(line+"\n")
        for line in lines:
            f2.write(line+"\n")

shuffle_cnf(sys.argv[1], sys.argv[2])

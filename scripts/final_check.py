#!/usr/bin/python

import os
import sys

if len(sys.argv) != 2:
    print("Must give proof")
    exit(-1)

cls_store = {}
xls_store = {}
line_no = 0
with open(sys.argv[1], "r") as f:
    for line in f:
        orig_line = str(line)
        line_no+=1
        if line_no % 100000 == 99999:
            print("line: ", line_no)
        line = line.strip()
        if len(line) == 0:
            print("EMTPY LINE!!")
            exit(-1)

        x = None
        cl = None
        xid = None
        xls = None
        cls = None
        cl_xs = None
        xs = None
        ls = None
        id = None
        if line[0] == 'i':
            assert line[1] == ' '
            assert len(line) > 2
            line = line[2:]
            if line[0] == 'x':
                # imply x from cls
                cl_xs = line[1:].split('l')
                assert len(cl_xs) == 2

                x = cl_xs[0].split()
                xid = int(x[0])
                x = x[1:]  # remove id
                x = [int(l) for l in x]
                if xid in xls_store:
                    print("ERROR, implied x ", xid, " already in xls. lits:", x)
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                assert x[-1] == 0
                assert xid not in xls_store
                xls_store[xid] = sorted(x)

                cls = cl_xs[1].split()
                cls = cls[:-1] # remove trailing 0 id
                cls = [int(id) for id in cls]
                for id in cls_store:
                    # TODO assert all the same length!
                    if id not in cls_store:
                        print("ERROR, clause used to imply ", xid, " id: ", id, " not in xls_store")
                        print("line no: ", line_no)
                        print("line: ", orig_line)
                        exit(-1)
                    assert id in cls_store
                pass
            else:
                # imply cl from x
                cl_xs = line.split('l')
                assert len(cl_xs) == 2

                cl = cl_xs[0].split()
                id = int(cl[0])
                cl = cl[1:] # remove id
                if id in cls_store:
                    print("ERROR, implied cl ", id, " already in cls_store")
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                cl = [int(l) for l in cl]
                assert cl[-1] == 0
                assert id not in cls_store
                cls_store[id] = sorted(cl)

                xs = cl_xs[1].split()
                xs = xs[:-1] # remove trailing 0 id
                assert len(xs) == 1
                xid = int(xs[0])
                if xid not in xls_store:
                    print("ERROR, cl implied by x ", xid , " not in xls_store")
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                assert xid in xls_store
            continue

        if line[0] == 'a' or line[0] == 'o':
            assert len(line) > 2
            assert line[1] == ' '
            line = line[2:]
            x_or_cl = line.split()
            assert len(x_or_cl) >= 2
            if x_or_cl[0].strip() == 'x':
                x = x_or_cl[1:]
                xid = int(x[0])
                x = x[1:]
                x = [l.strip() for l in x]
                if "l" in x:
                    at = x.index("l")
                    xids = x[at+1:]
                    xids=xids[:-1] # trailing 0
                    xids=[int(sub_xid) for sub_xid in xids]
                    for sub_xid in xids:
                        if sub_xid not in xls_store:
                            print("orig/add of XCL has 'l' and the cl is not in the store")
                            print("xcl: ", xid, " missing id: ", sub_xid)
                            print("line no: ", line_no)
                            print("line: ", orig_line)
                        assert sub_xid in xls_store
                    x = x[:at]
                ls = [int(l) for l in x[1:]]
                if xid in xls_store:
                    print("ERROR, added/orig x ", id, " already in xls_store")
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                assert xid not in xls_store
                xls_store[xid] = sorted(ls)
            else:
                c = list(x_or_cl)
                id = int(c[0])
                c = c[1:]
                c = [l.strip() for l in c]
                if "l" in c:
                    at = c.index("l")
                    ids = c[at+1:]
                    ids=ids[:-1] # trailing 0
                    ids=[int(sub_id) for sub_id in ids]
                    for sub_id in ids:
                        if sub_id not in cls_store:
                            print("orig/add of CL has 'l' and the cl is not in the store")
                            print("cl: ", id, " missing id: ", sub_id)
                            print("line no: ", line_no)
                            print("line: ", orig_line)
                        assert sub_id in cls_store
                    c = c[:at]
                ls = [int(l) for l in c[1:]]
                if id in cls_store:
                    print("ERROR, added/orig CL ", id, " already in cls_store")
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                assert id not in cls_store
                cls_store[id] = sorted(ls)
            continue

        if line[0] == 'd' or line[0] == 'f':
            assert line[1] == ' '
            line = line[2:]
            x_or_cl = line.split()
            assert len(x_or_cl) >= 2
            if x_or_cl[0].strip() == 'x':
                x = x_or_cl[1:]
                xid = int(x[0])
                x = x[1:]  # remove id
                ls = [int(l) for l in x]
                if xid not in xls_store:
                    print("ERROR, deleted/finalized x ", xid, " not in xls_store")
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                assert xid in xls_store
                xls_store.pop(xid)
            else:
                c = list(x_or_cl)
                id = int(c[0])
                c = c[1:]  # remove id
                ls = [int(l) for l in c]
                if id not in cls_store:
                    print("ERROR, deleted/finalized CL ", id, " not in cls_store")
                    print("line no: ", line_no)
                    print("line: ", orig_line)
                    exit(-1)
                assert id in cls_store
                cls_store.pop(id)
            continue

        if line[0] == 'c':
            continue

        print("cannot understand: ", line)

err = False
for id, lits in xls_store.items():
    print("ERROR, x ID: ", id, " not finalized. lits: ", lits)
    err = True

for id, lits in cls_store.items():
    print("ERROR, cl ID: ", id, " not finalized. lits: ", lits)
    err = True

if err :exit(-1)
exit(0)


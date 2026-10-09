#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# Reads the XLRUP proof the solver wrote (every clause addition carries its full
# hint chain, text or binary) and fills `used_clauses` and
# `used_clauses_anc`: which tracked clause was used at which conflict to
# derive a clause that is part of the final UNSAT proof.
#
# The proof is trimmed here: starting from the empty clause, only steps
# reachable through hints count. A clause is "used" when its ID is in the
# hint chain of such a step. used_clauses_anc also credits ancestors: a
# clause derived from a tracked clause counts as its child with weight
# 0.5, grandchildren 0.25, and so on, down to 0.05.
#
# The pass over the proof is xlrup_uses.cpp (built here on first use); the
# same pass in Python is --python: the reference test_xlrup_uses.py
# compares against, and what --verbose traces.

import sqlite3
import argparse
import array
import os
import subprocess
import sys
import time
import mmap

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
USE_DTYPE = np.dtype([("id", "<i8"), ("at", "<i8"), ("w", "<f8")])


def build_xlrup_uses():
    src = os.path.join(HERE, "xlrup_uses.cpp")
    exe = os.path.join(HERE, "xlrup_uses")
    if not os.path.exists(exe) or os.path.getmtime(exe) < os.path.getmtime(src):
        tmp = "%s.%d" % (exe, os.getpid())
        subprocess.check_call(["g++", "-O2", "-std=c++17", "-Wall", "-Wextra", "-o", tmp, src])
        os.rename(tmp, exe)
    return exe

cl_to_conflict = {}
new_id_to_old_id = {}


class XLRUPFile:
    """Random access to the clause addition steps of an XLRUP file, text or binary."""

    def __init__(self, fname):
        self.f = open(fname, "rb")
        self.data = mmap.mmap(self.f.fileno(), 0, access=mmap.ACCESS_READ)
        first = self.data[0:1]
        if first.isdigit():
            self.binary = False
        elif first in (b'a', b'd'):
            self.binary = True
        else:
            self.binary = self.data[1:2] != b' '
        print("XLRUP file is %s" % ("binary" if self.binary else "text"))
        self.offsets = array.array('q') # offset of each clause addition, in proof order
        self.ids = array.array('q')     # its ID
        self.empty_cl = None
        self.index()

    # ---- binary encoding: LEB128 unsigned, ids as 2*id, lits as 2*|l|+sign
    def unum(self, pos):
        res = 0
        mul = 0
        while True:
            c = self.data[pos]
            pos += 1
            res |= (c & 0x7f) << mul
            mul += 7
            if c & 0x80 == 0:
                return res, pos

    def id_list(self, pos):
        ret = []
        while True:
            u, pos = self.unum(pos)
            if u == 0:
                return ret, pos
            ret.append(u >> 1)

    def skip_lists(self, pos, num):
        # no 00 byte inside a number, so a list ends at its first 00
        for _ in range(num):
            pos = self.data.find(b'\x00', pos) + 1
        return pos

    def index_binary(self):
        data = self.data
        pos = 0
        n = len(data)
        # lists following the tag, the record's own id being part of the first
        xkinds = {0x6f: 1, 0x61: 3, 0x64: 1, 0x63: 2, 0x69: 2} # o a d c i
        while pos < n:
            k = data[pos]
            if k == 0x61: # a
                cid, p = self.unum(pos + 1)
                self.offsets.append(pos)
                self.ids.append(cid >> 1)
                if data[p] == 0:
                    self.empty_cl = cid >> 1
                pos = self.skip_lists(p, 2)
            elif k == 0x64: # d
                pos = self.skip_lists(pos + 1, 1)
            elif k == 0x78 and data[pos + 1] in xkinds: # x
                pos = self.skip_lists(pos + 2, xkinds[data[pos + 1]])
            else:
                print("ERROR: unknown XLRUP record at byte %d" % pos)
                exit(-1)

    def index_text(self):
        data = self.data
        pos = 0
        n = len(data)
        while pos < n:
            end = data.find(b'\n', pos)
            if end == -1:
                end = n
            # 'ID lits 0 hints 0', but not the deletion 'ID d ids 0'
            if 0x30 <= data[pos] <= 0x39:
                sp = data.find(b' ', pos, end)
                if data[sp + 1] != 0x64:
                    cid = int(data[pos:sp])
                    self.offsets.append(pos)
                    self.ids.append(cid)
                    if data[sp + 1:sp + 3] == b'0 ':
                        self.empty_cl = cid
            pos = end + 1

    def index(self):
        t = time.time()
        if self.binary:
            self.index_binary()
        else:
            self.index_text()
        print("Indexed %d add steps T: %-3.2f s" % (len(self.offsets), time.time() - t))
        if self.empty_cl is None:
            print("ERROR: no empty clause in the proof. Was the instance UNSAT? Use --xor 0")
            exit(-1)
        # ID -> position in offsets/ids (IDs are assigned increasingly)
        self.max_id = max(self.ids) if len(self.ids) else 0
        self.pos_of_id = array.array('q', [-1]) * (self.max_id + 1)
        for i, cid in enumerate(self.ids):
            self.pos_of_id[cid] = i

    def hints(self, i):
        """Hint chain of the i-th add step."""
        pos = self.offsets[i]
        if self.binary:
            u, pos = self.unum(pos + 1)
            ret, pos = self.id_list(self.skip_lists(pos, 1))
            return ret
        else:
            end = self.data.find(b'\n', pos)
            toks = self.data[pos:end].split()
            assert toks[-1] == b'0'
            return [int(x) for x in toks[toks.index(b'0', 1) + 1:-1]]

    def mark_proof(self):
        """Backward pass: which add steps derive the empty clause."""
        t = time.time()
        marked = bytearray(len(self.offsets))
        todo = [self.pos_of_id[self.empty_cl]]
        marked[todo[0]] = 1
        while todo:
            i = todo.pop()
            for h in self.hints(i):
                if h > self.max_id:
                    continue
                j = self.pos_of_id[h]
                if j >= 0 and not marked[j]: # original clauses are not add steps
                    marked[j] = 1
                    todo.append(j)
        print("Marked %d of %d add steps as part of the proof T: %-3.2f s" % (
            sum(marked), len(self.offsets), time.time() - t))
        return marked


class Query:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.cl_used = []
        self.cl_used_total = 0
        self.children_set = 0
        self.num_steps = 0
        self.no_confl = 0

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

    def delete_tbls(self, table):
        queries = """
DROP TABLE IF EXISTS `{table}`;
create table `{table}` ( `clauseID` bigint(20) NOT NULL, `used_at` bigint(20) NOT NULL, `weight` float NOT NULL);
""".format(table=table)

        for l in queries.split('\n'):
            if opts.verbose:
                print("Running query: ", l)
            self.c.execute(l)

    def get_conflicts(self):
        self.c.execute("select id, conflicts from set_id_confl;")
        for ID, confl in self.c:
            if ID in cl_to_conflict:
                print("ERROR: ID %d in set_id_confl twice. Old value: %d new value: %d" % (
                    ID, cl_to_conflict[ID], confl))
                exit(-1)
            cl_to_conflict[ID] = confl
        print("Got ID-conflict data: %d IDs" % len(cl_to_conflict))

    def get_updates(self):
        # old_id == new_id marks a tracked clause; later rows follow its ID
        # changes (strengthening etc.) so uses of any of its IDs count
        self.c.execute("select old_id, new_id from update_id order by old_id;")
        for old_id, new_id in self.c:
            if old_id in new_id_to_old_id:
                real_old_id = new_id_to_old_id[old_id][0]
            else:
                real_old_id = old_id

            new_id_to_old_id[new_id] = [real_old_id, 1.0, None]
        print("Got ID-update data: %d IDs tracked" % len(new_id_to_old_id))

    def dump_used_clauses(self):
        for table in ("used_clauses_anc", "used_clauses"):
            if table == "used_clauses":
                todump = filter(lambda c: c[1] == 1.0, self.cl_used)
            else:
                todump = self.cl_used

            self.c.executemany("""
            INSERT INTO %s (
            `clauseID`,
            `weight`,
            `used_at`)
            VALUES (?, ?, ?);""" % table, todump)

        self.cl_used = []

    def deal_with_chain(self, hints, final_resolvent_ID, tracked_already, confl):
        for chain_ID in hints:
            if chain_ID not in new_id_to_old_id:
                continue
            data = new_id_to_old_id[chain_ID]
            if opts.verbose:
                print("Cl ID %8d used for Cl ID %8d -> tracked as %s" % (
                    chain_ID, final_resolvent_ID, data))
            if data[2] is None:
                # this is an original parent
                count_as_confl = confl
                assert data[1] == 1.0
            else:
                # this is a child
                count_as_confl = data[2]
                assert data[1] < 1.0

            new_item = [data[0], data[1], count_as_confl]
            self.cl_used.append(new_item)
            self.cl_used_total += 1
            if opts.verbose:
                print("--> USED: %s" % new_item)
            if len(self.cl_used) > 10000:
                self.dump_used_clauses()

            # This chain_ID is tracked. Let's track Clause "final_resolvent_ID", it will be a child
            # NOTE: we only track it as ONE of the children... which is not ideal.
            if tracked_already:
                if opts.verbose:
                    print("-----> Can't track Cl ID %d as child, already tracked either as MAIN or as a child" % final_resolvent_ID)
                continue

            # final_resolvent_ID is not tracked already. We'll track it according to its ancestor
            chain_ID_upd = data[0]
            val = 0.5*data[1]
            if data[2] is not None:
                # This is a child of a child, so parent's conf needs to be bumped
                anc_confl = data[2]
            else:
                # Children of this will need confl to be bumped
                anc_confl = confl

            # Don't bother if it's less than 0.05
            if val <= 0.05:
                continue

            if opts.verbose:
                print("-----> Therefore, we will track ID %d with val %f to count as ID %d, confl %d" % (final_resolvent_ID, val, chain_ID_upd, anc_confl))
            new_id_to_old_id[final_resolvent_ID] = [chain_ID_upd, val, anc_confl]
            tracked_already = True
            self.children_set += 1

    def fix_up_xlrup_fast(self, xlrupfile, dbfname):
        """the proof pass in C++: IDs out, uses back"""
        exe = build_xlrup_uses()
        base = "%s.xlrupuses-%d" % (dbfname, os.getpid())
        files = [base + x for x in (".confl", ".tracked", ".out")]
        try:
            self.c.execute("select id, conflicts from set_id_confl")
            confl = np.array(self.c.fetchall(), dtype="<i8").reshape(-1, 2)
            if len(np.unique(confl[:, 0])) != len(confl):
                print("ERROR: an ID is in set_id_confl twice")
                exit(-1)
            confl.tofile(files[0])
            print("Got ID-conflict data: %d IDs" % len(confl))
            if len(confl):
                sz = os.path.getsize(xlrupfile)
                print("proof: %.1f MB, %.0f bytes per conflict" % (sz / 1e6, sz / max(confl[:, 1].max(), 1)))
            del confl
            self.get_updates()
            np.array([[k, v[0]] for k, v in new_id_to_old_id.items()], dtype="<i8").reshape(-1, 2).tofile(files[1])
            sys.stdout.flush()
            if subprocess.call([exe, xlrupfile] + files) != 0:
                exit(-1)
            uses = np.fromfile(files[2], dtype=USE_DTYPE)
        finally:
            for f in files:
                if os.path.exists(f):
                    os.unlink(f)
        q = "INSERT INTO %s (`clauseID`, `used_at`, `weight`) VALUES (?, ?, ?)"
        plain = uses[uses["w"] == 1.0]
        for table, rows in (("used_clauses_anc", uses), ("used_clauses", plain)):
            for i in range(0, len(rows), 500000):
                self.c.executemany(q % table, rows[i:i + 500000].tolist())

    def fix_up_xlrup(self, xlrup):
        marked = xlrup.mark_proof()
        t = time.time()
        for i in range(len(xlrup.offsets)):
            if not marked[i]:
                continue
            final_resolvent_ID = xlrup.ids[i]
            self.num_steps += 1
            tracked_already = final_resolvent_ID in new_id_to_old_id
            if opts.verbose:
                print("-------------****----------------------")
                print("MAIN: Cl ID %8d tracked: %s" % (final_resolvent_ID, tracked_already))

            if final_resolvent_ID not in cl_to_conflict:
                self.no_confl += 1
                if opts.verbose:
                    print("MAIN: ID %8d not in set_id_confl, skipping" % final_resolvent_ID)
                continue

            confl = cl_to_conflict[final_resolvent_ID]
            self.deal_with_chain(xlrup.hints(i), final_resolvent_ID, tracked_already, confl)
        print("Forward pass T: %-3.2f s" % (time.time() - t))


if __name__ == "__main__":
    usage = """usage: %(prog)s [opts] XLRUP SQLITEDB

Adds used_clauses and used_clauses_anc to the SQLite database"""

    parser = argparse.ArgumentParser(usage=usage)
    parser.add_argument("xlrupfile", type=str, metavar='XLRUP',
                        help="XLRUP proof written by the solver, text (--xlrup 1) or binary (--xlrup 2)")
    parser.add_argument("sqlitedb", type=str, metavar='SQLITEDB')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output (implies --python)")
    parser.add_argument("--python", action="store_true", default=False,
                        help="The slow reference pass in Python instead of xlrup_uses.cpp")

    opts = parser.parse_args()

    print("Using XLRUP file %s" % opts.xlrupfile)
    print("Using sqlite3db file %s" % opts.sqlitedb)

    t = time.time()
    if not (opts.python or opts.verbose):
        with Query(opts.sqlitedb) as q:
            q.delete_tbls("used_clauses")
            q.delete_tbls("used_clauses_anc")
            q.fix_up_xlrup_fast(opts.xlrupfile, opts.sqlitedb)
        print("T: %-3.2f s" % (time.time() - t))
        exit(0)

    xlrup = XLRUPFile(opts.xlrupfile)
    with Query(opts.sqlitedb) as q:
        q.delete_tbls("used_clauses")
        q.delete_tbls("used_clauses_anc")
        q.get_conflicts()
        q.get_updates()
        q.fix_up_xlrup(xlrup)
        q.dump_used_clauses()

        print("Proof steps in proof:   %10d" % q.num_steps)
        print("Steps w/o conflict no.: %10d" % q.no_confl)
        print("Total num uses:         %10d" % q.cl_used_total)
        print("Children set:           %10d" % q.children_set)
        if q.num_steps == 0:
            print("ERROR: no proof steps found")
            exit(-1)
    print("T: %-3.2f s" % (time.time() - t))

    exit(0)

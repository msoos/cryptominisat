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

# Reads the FRAT proof the solver wrote (every 'a' step carries its full
# hint chain, ascii or binary) and fills `used_clauses` and
# `used_clauses_anc`: which tracked clause was used at which conflict to
# derive a clause that is part of the final UNSAT proof.
#
# The proof is trimmed here: starting from the empty clause, only steps
# reachable through hints count. A clause is "used" when its ID is in the
# hint chain of such a step. used_clauses_anc also credits ancestors: a
# clause derived from a tracked clause counts as its child with weight
# 0.5, grandchildren 0.25, and so on, down to 0.05.

import sqlite3
import argparse
import array
import time
import mmap

cl_to_conflict = {}
new_id_to_old_id = {}


class FratFile:
    """Random access to the 'a' steps of a FRAT file, ascii or binary."""

    def __init__(self, fname):
        self.f = open(fname, "rb")
        self.data = mmap.mmap(self.f.fileno(), 0, access=mmap.ACCESS_READ)
        self.binary = self.data[1:2] not in (b' ', b'\n')
        print("FRAT file is %s" % ("binary" if self.binary else "ascii"))
        self.offsets = array.array('q') # offset of each 'a' step, in proof order
        self.ids = array.array('q')     # its ID
        self.empty_cl = None
        self.index()

    # ---- binary encoding, as frat-rs: LEB128 unsigned, lits as 2*|l|+sign
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

    def ivec(self, pos):
        ret = []
        while True:
            u, pos = self.unum(pos)
            if u == 0:
                return ret, pos
            ret.append(-(u >> 1) if u & 1 else (u >> 1))

    def skip_ivec(self, pos):
        while True:
            c = self.data[pos]
            pos += 1
            if c == 0:
                return pos

    def index_binary(self):
        data = self.data
        pos = 0
        n = len(data)
        while pos < n:
            k = data[pos]
            pos += 1
            if k in (0x61, 0x6f, 0x64, 0x66, 0x69): # a o d f i
                start = pos - 1
                cid, pos = self.unum(pos)
                if cid == 0: # step header, e.g. 'i 0' before 'x'
                    continue
                if k == 0x61:
                    self.offsets.append(start)
                    self.ids.append(cid)
                    if data[pos] == 0:
                        self.empty_cl = cid
                pos = self.skip_ivec(pos)
            elif k == 0x6c: # l
                pos = self.skip_ivec(pos)
            elif k == 0x72: # r: pairs until 0
                while True:
                    u, pos = self.unum(pos)
                    if u == 0:
                        break
                    u, pos = self.unum(pos)
            elif k == 0x74: # t
                u, pos = self.unum(pos)
                u, pos = self.unum(pos)
            elif k == 0x78: # x
                u, pos = self.unum(pos)
                pos = self.skip_ivec(pos)
            elif k == 0x63: # c
                while data[pos] != 0:
                    pos += 1
                pos += 1
            else:
                print("ERROR: unknown FRAT step '%s' at byte %d. Use --xor 0 (no XOR/BNN steps)"
                      % (chr(k), pos - 1))
                exit(-1)

    def index_ascii(self):
        data = self.data
        pos = 0
        n = len(data)
        while pos < n:
            end = data.find(b'\n', pos)
            if end == -1:
                end = n
            if data[pos] == 0x61: # 'a'
                sp = data.find(b' ', pos + 2, end)
                cid = int(data[pos + 2:sp])
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
            self.index_ascii()
        print("Indexed %d add steps T: %-3.2f s" % (len(self.offsets), time.time() - t))
        if self.empty_cl is None:
            print("ERROR: no empty clause in the proof. Was the instance UNSAT?")
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
            pos = self.skip_ivec(pos)
            assert self.data[pos] == 0x6c, "add step without hints, need a FRAT with full chains"
            ret, pos = self.ivec(pos + 1)
            return ret
        else:
            end = self.data.find(b'\n', pos)
            line = self.data[pos:end].split(b' l ')
            assert len(line) == 2, "add step without hints, need a FRAT with full chains"
            ret = [int(x) for x in line[1].split()]
            assert ret[-1] == 0
            return ret[:-1]

    def mark_proof(self):
        """Backward pass: which add steps derive the empty clause."""
        t = time.time()
        marked = bytearray(len(self.offsets))
        todo = [self.pos_of_id[self.empty_cl]]
        marked[todo[0]] = 1
        while todo:
            i = todo.pop()
            for h in self.hints(i):
                if h < 0 or h > self.max_id: # RAT hints are negative
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

    def fix_up_frat(self, frat):
        marked = frat.mark_proof()
        t = time.time()
        for i in range(len(frat.offsets)):
            if not marked[i]:
                continue
            final_resolvent_ID = frat.ids[i]
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
            self.deal_with_chain(frat.hints(i), final_resolvent_ID, tracked_already, confl)
        print("Forward pass T: %-3.2f s" % (time.time() - t))


if __name__ == "__main__":
    usage = """usage: %(prog)s [opts] FRAT SQLITEDB

Adds used_clauses and used_clauses_anc to the SQLite database"""

    parser = argparse.ArgumentParser(usage=usage)
    parser.add_argument("fratfile", type=str, metavar='FRAT',
                        help="FRAT proof written by the solver (--xlrup 0), ascii or binary")
    parser.add_argument("sqlitedb", type=str, metavar='SQLITEDB')
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    opts = parser.parse_args()

    print("Using FRAT file %s" % opts.fratfile)
    print("Using sqlite3db file %s" % opts.sqlitedb)

    t = time.time()
    frat = FratFile(opts.fratfile)
    with Query(opts.sqlitedb) as q:
        q.delete_tbls("used_clauses")
        q.delete_tbls("used_clauses_anc")
        q.get_conflicts()
        q.get_updates()
        q.fix_up_frat(frat)
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

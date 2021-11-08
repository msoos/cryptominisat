#!/usr/bin/python3
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

import unittest

def inv(l):
    assert len(l) > 0
    if l[0] == "-":
        return l[1:]
    else:
        return "-"+l

def check_clause_sanity(c):
    for i1 in range(len(c)):
        l1 = c[i1]
        for i2 in range(i1+1, len(c)):
            l2 = c[i2]
            if l1 == l2:
                print("ERROR: Same literal twice in clause: %s", l1)
                assert False

            if l1 == inv(l2):
                print("ERROR: Inverted literal in clause: %s ", l1)
                assert False

    return True


def can_resolve(c1, c2):
    check_clause_sanity(c1)
    check_clause_sanity(c2)

    # find inverted
    resolve_on = None
    for a in c1:
        for b in c2:
            if a==inv(b):
                if resolve_on is not None:
                    return None
                resolve_on = a

    return resolve_on

def resolve(c1, c2):
    resolve_on = can_resolve(c1,c2)
    assert resolve_on is not None, "Cannot resolve the two clauses!"

    ret = []
    for a in c1:
        if a != resolve_on:
            ret.append(a)

    for b in c2:
        if b != inv(resolve_on) and b not in ret:
            ret.append(b)

    ret = sorted(ret)
    return ret


def ham_w(num):
    w = 0
    for i in range(16):
        w += (num>>i)&1

    return w


def gen_cls(x):
    check_clause_sanity(x)

    ret = []
    for i in range(1<<len(x)):
        if ham_w(i)%2 == (len(x)%2):
            continue

        cl = []
        for i2 in range(len(x)):
            l = x[i2]
            if i>>i2&1 == 1:
                cl.append(inv(l))
            else:
                cl.append(l)

        ret.append(cl)

    return ret


class TestMethods(unittest.TestCase):

    def test_inv(self):
        self.assertEqual(inv("a"), "-a")
        self.assertEqual(inv("-a"), "a")
        self.assertEqual(inv("x1"), "-x1")
        self.assertEqual(inv("-x1"), "x1")

    def test_sanity_good(self):
        self.assertTrue(check_clause_sanity(["a"]))
        self.assertTrue(check_clause_sanity(["a", "b"]))
        self.assertTrue(check_clause_sanity(["a", "-b"]))

    def test_sanity(self):
        with self.assertRaises(AssertionError):
            check_clause_sanity(["a", "a"])
        with self.assertRaises(AssertionError):
            check_clause_sanity(["a", "-a"])

    def tets_gen_cls(self):
        self.assertEqual(gen_cls(["a"]), [["a"]])
        self.assertEqual(gen_cls(["-a"]), [["-a"]])
        self.assertEqual(gen_cls(["a", "b"]), [["a", "b"], ["-a", "-b"]])
        self.assertEqual(gen_cls(["a", "-b"]), [["a", "-b"], ["-a", "b"]])
        self.assertEqual(
            gen_cls(["a", "b", "c"]),
            [["a", "b", "c"], ["-a", "-b", "c"], ["-a", "b", "-c"], ["a", "-b", "-c"]])

    def test_ham_w(self):
        self.assertEqual(ham_w(1), 1)
        self.assertEqual(ham_w(0), 0)
        self.assertEqual(ham_w(2), 1)
        self.assertEqual(ham_w(3), 2)
        self.assertEqual(ham_w(9), 2)
        self.assertEqual(ham_w(8), 1)
        self.assertEqual(ham_w(11), 3)

    def test_resolve(self):
        with self.assertRaises(AssertionError):
            resolve(["-a", "b"], ["-a", "c"])

        with self.assertRaises(AssertionError):
            resolve(["-a", "-b"], ["a", "b"])

        self.assertEqual(resolve(["a", "b", "c"], ["-a", "b"]), ["b", "c"])

def num_inside(cl,lits):
    num = 0
    for l in cl:
        for l2 in lits:
            if l == l2:
                num+=1

    return num


def find_most_inside(xors, to_prove):
    most_inside = 0
    most_inside_cl = None
    for i_x in range(len(xors)):
        x = xors[i_x]
        cls = gen_cls(x)
        for i_c in range(len(cls)):
            cl = cls[i_c]
            n = num_inside(cl, to_prove)
            if n > most_inside:
                most_inside = n
                most_inside_cl = cl
                most_inside_at = i_x

    return most_inside, most_inside_cl, most_inside_at


def find_resolvent_most_inside(xors, cl_other, to_prove):
    most_inside = 0
    most_inside_cl = None
    for i_x in range(len(xors)):
        x = xors[i_x]
        cls = gen_cls(x)
        for i in range(len(cls)):
            cl = cls[i]
            if can_resolve(cl,cl_other) is None:
                continue

            n = num_inside(cl, to_prove)
            if n > most_inside:
                most_inside = n
                most_inside_cl = cl
                most_inside_at = i_x

    return most_inside, most_inside_cl, most_inside_at

def prove(xors, to_prove):
    to_prove = sorted(to_prove)

    ret = []
    most_inside, cl, most_inside_at = find_most_inside(xors, to_prove)
    assert most_inside > 0
    print("cl picked:", cl)
    xors.remove(xors[most_inside_at])

    while cl != to_prove:
        most_inside, cl2, most_inside_at = find_resolvent_most_inside(xors, cl, to_prove)
        assert most_inside > 0
        print("cl picked:", cl2)

        cl = resolve(cl, cl2)
        cl = sorted(cl)
        print("resolvent:", cl)

if __name__ == '__main__':
    # XORs to add together (we know this)
    xors = [
        ["a", "b", "c"],
        ["a", "b", "d"]
    ]

    # so c+d = 0
    # so c=1 d=0 must be banned
    # generate clause
    to_prove = ["-c", "d"]


    print("xors:", xors)
    print("to_prove:", to_prove)
    prove(xors, to_prove)
    print("----------------")
    print("----------------")


    print(gen_cls(["v1", "-v2"]))
    print(gen_cls(["v1", "v2", "v3"]))
    print("----------------")
    print("----------------")

    unittest.main()


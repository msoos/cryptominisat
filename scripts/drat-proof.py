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

xors = [
    ["a", "b", "c"],
    ["a", "b", "d"]
]

# to prove impossible
prove = ["c", "d"]

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

def resolve(c1, c2):
    check_clause_sanity(c1)
    check_clause_sanity(c2)

    # find inverted
    resolve_on = None
    for a in c1:
        for b in c2:
            if a==inv(b):
                assert resolve_on is None, "Two inverted literals in the 2 clauses!"
                resolve_on = a

    assert resolve_on is not None, "Must resolve on a literal!"

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

if __name__ == '__main__':
    print(gen_cls(["v1", "v2", "v3"]))
    unittest.main()


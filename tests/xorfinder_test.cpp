/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "gtest/gtest.h"

#include <fstream>

#include "src/solver.h"
#include "src/xorfinder.h"
#include "src/solverconf.h"
#include "src/occsimplifier.h"
using namespace CMSat;
#include "test_helper.h"

struct xor_finder : public ::testing::Test {
    xor_finder()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
        occsimp = s->occsimplifier;
    }
    ~xor_finder()
    {
        delete s;
    }
    Solver* s = NULL;
    OccSimplifier* occsimp = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(xor_finder, find_none)
{
    s->add_clause_outside(str_to_cl("1, 2"));
    s->add_clause_outside(str_to_cl("-1, -2"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.grab_mem();
    finder.find_xors();
    EXPECT_EQ(s->xorclauses.size(), 0U);
}

TEST_F(xor_finder, find_tri_1)
{
    s->add_clause_outside(str_to_cl("1, 2, 3"));
    s->add_clause_outside(str_to_cl("-1, -2, 3"));
    s->add_clause_outside(str_to_cl("-1, 2, -3"));
    s->add_clause_outside(str_to_cl("1, -2, -3"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_contains(s->xorclauses, "1, 2, 3 = 1");
}

TEST_F(xor_finder, find_tri_2)
{
    s->add_clause_outside(str_to_cl("-1, 2, 3"));
    s->add_clause_outside(str_to_cl("1, -2, 3"));
    s->add_clause_outside(str_to_cl("1, 2, -3"));
    s->add_clause_outside(str_to_cl("-1, -2, -3"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3 = 0");
}

TEST_F(xor_finder, find_tri_3)
{
    s->add_clause_outside(str_to_cl("-1, 2, 3"));
    s->add_clause_outside(str_to_cl("1, -2, 3"));
    s->add_clause_outside(str_to_cl("1, 2, -3"));
    s->add_clause_outside(str_to_cl("-1, -2, -3"));

    s->add_clause_outside(str_to_cl("1, 2, 3"));
    s->add_clause_outside(str_to_cl("-1, -2, 3"));
    s->add_clause_outside(str_to_cl("-1, 2, -3"));
    s->add_clause_outside(str_to_cl("1, -2, -3"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_contains(s->xorclauses, "1, 2, 3 = 0");
    check_xors_contains(s->xorclauses, "1, 2, 3 = 1");
}


TEST_F(xor_finder, find_4_1)
{
    s->add_clause_outside(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, 3, -4"));

    s->add_clause_outside(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4 = 0;");
}

TEST_F(xor_finder, find_4_4)
{
    s->add_clause_outside(str_to_cl("-1, -2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, -3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, 2,  -3, 4"));
    s->add_clause_outside(str_to_cl("-1, 2,  3, -4"));
    s->add_clause_outside(str_to_cl("1, -2,  3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("1, 2, 3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4 = 1");
}

/*
 * These tests only work if the matching is non-exact
 * i.e. if size is not checked for equality
 */
TEST_F(xor_finder, find_4_2)
{
    s->add_clause_outside(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, 3"));

    s->add_clause_outside(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4 = 0;");
}

TEST_F(xor_finder, find_4_3)
{
    s->add_clause_outside(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, 3"));

    s->add_clause_outside(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outside(str_to_cl("-1, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4 = 0;");
}

/*
//Finder pruning is too strong and we don't find this one
TEST_F(xor_finder, find_5_2)
{
    s->add_clause_outside(str_to_cl("-1, -2, 3, 4, 5"));
    s->add_clause_outside(str_to_cl("-1, 2, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));

    s->add_clause_outside(str_to_cl("1, -2, -3, 4, 5"));
    s->add_clause_outside(str_to_cl("1, -2, 3, -4, 5"));
    s->add_clause_outside(str_to_cl("1, -2, 3, 4, -5"));

    s->add_clause_outside(str_to_cl("1, 2, -3, -4, 5"));
    s->add_clause_outside(str_to_cl("1, 2, -3, 4, -5"));

    s->add_clause_outside(str_to_cl("1, 2, 3, -4, -5"));

    //

    s->add_clause_outside(str_to_cl("1, -2, -3, -4, -5"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, -4, -5"));
    s->add_clause_outside(str_to_cl("-1, -2, 3, -4, -5"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, 4, -5"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, -4, 5"));

    s->add_clause_outside(str_to_cl("1, 2, 3, 4, 5"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4, 5 = 1;");
}*/

TEST_F(xor_finder, find_4_5)
{
    s->add_clause_outside(str_to_cl("-1, -2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, -3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, 2,  -3, 4"));
    s->add_clause_outside(str_to_cl("-1, 2,  3, -4"));
    s->add_clause_outside(str_to_cl("1, -2,  3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("1, 2, 3"));

    s->add_clause_outside(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outside(str_to_cl("1, 2, 3"));

    s->add_clause_outside(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outside(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outside(str_to_cl("-1, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4 = 1; 1, 2, 3, 4 = 0");
}
/***
 * Specialty, non-matching XOR test end
*/

TEST_F(xor_finder, find_5_1)
{
    s->add_clause_outside(str_to_cl("-1, -2, 3, 4, 5"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, 4, 5"));
    s->add_clause_outside(str_to_cl("-1, 2, 3, -4, 5"));
    s->add_clause_outside(str_to_cl("-1, 2, 3, 4, -5"));

    s->add_clause_outside(str_to_cl("1, -2, -3, 4, 5"));
    s->add_clause_outside(str_to_cl("1, -2, 3, -4, 5"));
    s->add_clause_outside(str_to_cl("1, -2, 3, 4, -5"));

    s->add_clause_outside(str_to_cl("1, 2, -3, -4, 5"));
    s->add_clause_outside(str_to_cl("1, 2, -3, 4, -5"));

    s->add_clause_outside(str_to_cl("1, 2, 3, -4, -5"));

    //

    s->add_clause_outside(str_to_cl("1, -2, -3, -4, -5"));
    s->add_clause_outside(str_to_cl("-1, 2, -3, -4, -5"));
    s->add_clause_outside(str_to_cl("-1, -2, 3, -4, -5"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, 4, -5"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, -4, 5"));

    s->add_clause_outside(str_to_cl("1, 2, 3, 4, 5"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 2, 3, 4, 5 = 1;");
}


//we don't find 6-long, too expensive
/*TEST_F(xor_finder, find_6_0)
{
    s->add_clause_outside(str_to_cl("1, -7, -3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("-1, 7, -3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("-1, -7, 3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("1, 7, 3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("-1, -7, -3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("1, 7, -3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("1, -7, 3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("-1, 7, 3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("-1, -7, -3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("1, 7, -3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("1, -7, 3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("-1, 7, 3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("1, -7, -3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("-1, 7, -3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("-1, -7, 3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("1, 7, 3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("-1, -7, -3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("1, 7, -3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("1, -7, 3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("-1, 7, 3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("1, -7, -3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("-1, 7, -3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("-1, -7, 3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("1, 7, 3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("1, -7, -3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("-1, 7, -3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("-1, -7, 3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("1, 7, 3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("-1, -7, -3, 4, 5, 9"));
    s->add_clause_outside(str_to_cl("1, 7, -3, 4, 5, 9"));
    s->add_clause_outside(str_to_cl("1, -7, 3, 4, 5, 9"));
    s->add_clause_outside(str_to_cl("-1, 7, 3, 4, 5, 9"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "1, 7, 3, 4, 5, 9 = 0;");
}

TEST_F(xor_finder, find_6_1)
{
    s->add_clause_outside(str_to_cl("-6, -7, -3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("6, 7, -3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("6, -7, 3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("-6, 7, 3, -4, -5, -9"));
    s->add_clause_outside(str_to_cl("6, -7, -3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("-6, 7, -3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("-6, -7, 3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("6, 7, 3, 4, -5, -9"));
    s->add_clause_outside(str_to_cl("6, -7, -3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("-6, 7, -3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("-6, -7, 3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("6, 7, 3, -4, 5, -9"));
    s->add_clause_outside(str_to_cl("-6, -7, -3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("6, 7, -3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("6, -7, 3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("-6, 7, 3, 4, 5, -9"));
    s->add_clause_outside(str_to_cl("6, -7, -3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("-6, 7, -3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("-6, -7, 3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("6, 7, 3, -4, -5, 9"));
    s->add_clause_outside(str_to_cl("-6, -7, -3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("6, 7, -3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("6, -7, 3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("-6, 7, 3, 4, -5, 9"));
    s->add_clause_outside(str_to_cl("-6, -7, -3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("6, 7, -3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("6, -7, 3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("-6, 7, 3, -4, 5, 9"));
    s->add_clause_outside(str_to_cl("6, -7, -3, 4, 5, 9"));
    s->add_clause_outside(str_to_cl("-6, 7, -3, 4, 5, 9"));
    s->add_clause_outside(str_to_cl("-6, -7, 3, 4, 5, 9"));
    s->add_clause_outside(str_to_cl("6, 7, 3, 4, 5, 9"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(s->xorclauses, "6, 7, 3, 4, 5, 9 = 1;");
}*/

struct xor_finder2 : public ::testing::Test {
    xor_finder2()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
        occsimp = s->occsimplifier;
        finder = new XorFinder(occsimp, s);
        finder->grab_mem();
    }
    ~xor_finder2()
    {
        delete s;
        delete finder;
    }
    Solver* s = NULL;
    OccSimplifier* occsimp = NULL;
    std::atomic<bool> must_inter;
    XorFinder* finder;
};


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

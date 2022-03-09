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
#include "src/toplevelgaussabst.h"
#ifdef USE_M4RI
#include "src/toplevelgauss.h"
#endif

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
        #ifdef USE_M4RI
        topLevelGauss = new TopLevelGauss(s);
        #endif
    }
    ~xor_finder2()
    {
        delete s;
        delete finder;
        #ifdef USE_M4RI
        delete topLevelGauss;
        #endif
    }
    Solver* s = NULL;
    OccSimplifier* occsimp = NULL;
    std::atomic<bool> must_inter;
    XorFinder* finder;
    #ifdef USE_M4RI
    TopLevelGaussAbst *topLevelGauss;
    #endif
};


TEST_F(xor_finder2, clean_v1)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0;");
    finder->move_xors_without_connecting_vars_to_unused();
    EXPECT_EQ(s->xorclauses.size(), 0u);
}

TEST_F(xor_finder2, clean_v2)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 4, 5, 6 = 0");
    finder->move_xors_without_connecting_vars_to_unused();
    EXPECT_EQ(s->xorclauses.size(), 2u);
}

TEST_F(xor_finder2, clean_v3)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 4, 5, 6 = 0; 10, 11, 12, 13 = 1");
    finder->move_xors_without_connecting_vars_to_unused();
    EXPECT_EQ(s->xorclauses.size(), 2u);
}

TEST_F(xor_finder2, clean_v4)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 4, 5, 6 = 0; 10, 11, 12, 13 = 1; 10, 15, 16, 17 = 0");
    finder->move_xors_without_connecting_vars_to_unused();
    EXPECT_EQ(s->xorclauses.size(), 4u);
}

TEST_F(xor_finder2, xor_1)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 1; 1, 4, 5, 6 = 0;");
    finder->xor_together_xors(s->xorclauses);
    check_xors_eq(s->xorclauses, "2, 3, 4, 5, 6 = 1 c 1;");
}

TEST_F(xor_finder2, xor_2)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 4, 5, 6 = 0;");
    finder->xor_together_xors(s->xorclauses);
    check_xors_eq(s->xorclauses, "2, 3, 4, 5, 6 = 0 c 1;");
}

TEST_F(xor_finder2, xor_3)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 10, 4, 5, 6 = 0;");
    finder->xor_together_xors(s->xorclauses);
    check_xors_eq(s->xorclauses, "1, 2, 3 = 0; 10, 4, 5, 6 = 0;");
}

TEST_F(xor_finder2, xor_4)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 4, 5, 6 = 0;"
        "1, 9, 10, 11 = 0;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 3u);
}

TEST_F(xor_finder2, xor_5)
{
    s->xorclauses = str_to_xors(
        "2, 3, 1 = 0;"
        "1, 5, 6, 4 = 0;"
        "4, 10, 11 = 0;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 1u);
    check_xors_contains(s->xorclauses, "2, 3, 5, 6, 10, 11 = 0 c 1, 4");
}

TEST_F(xor_finder2, xor_6)
{
    s->xorclauses = str_to_xors(
        "1, 2 = 0;"
        "1, 4 = 0;"
        "6, 7 = 0;"
        "6, 10 = 1");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 2u);
    check_xors_eq(s->xorclauses, "2, 4 = 0 c 1; 7, 10 = 1 c 6");
}

TEST_F(xor_finder2, xor_7)
{
    s->xorclauses = str_to_xors(
        "1, 2 = 0 c 10;"
        "1, 4 = 0 c 11;"
        "6, 7 = 0;"
        "6, 9 = 1 c 18");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 2u);
    check_xors_eq(s->xorclauses,
        "2, 4 = 0 c 10, 11, 1;"
        "7, 9 = 1 c 6, 18");
}

TEST_F(xor_finder2, xor_8)
{
    s->xorclauses = str_to_xors(
        "1, 2 = 0 c 10;"
        "1, 4, 7 = 0 c 11;"
        "7, 8, 9 = 0 c 12;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 1u);
    check_xors_eq(s->xorclauses,
        "2, 4, 8, 9 = 0 c 10, 11, 12, 1, 7;");
}


TEST_F(xor_finder2, dont_xor_together_when_clash_more_than_one)
{
    s->xorclauses = str_to_xors(
        "1, 2, 3, 4 = 0;"
        "1, 2, 5, 6 = 0;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 2u);
    check_xors_eq(s->xorclauses,
      "1, 2, 3, 4 = 0;"
      "1, 2, 5, 6 = 0;");
}

TEST_F(xor_finder2, clash_1)
{
    s->xorclauses = str_to_xors("1, 2, 3, 4 = 0; 1, 2, 5, 6= 0;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 2u);
    check_xors_eq(s->xorclauses, "1, 2, 3, 4 = 0; 1, 2, 5, 6= 0;");
}

TEST_F(xor_finder2, dont_remove_xors)
{
    s->xorclauses = str_to_xors("1, 2 = 0; 1, 2= 0;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 1U);
}

TEST_F(xor_finder2, dont_remove_xors2)
{
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 2, 3= 0;");
    finder->xor_together_xors(s->xorclauses);
    EXPECT_EQ(s->xorclauses.size(), 1U);
}

/*TEST_F(xor_finder2, xor_pure_unit_unsat)
{
    s->xorclauses = str_to_xors("1 = 0; 1 = 1;");
    finder->xor_together_xors(s->xorclauses);
    bool ret = finder->add_new_truths_from_xors(s->xorclauses);
    EXPECT_FALSE(ret);
}

TEST_F(xor_finder2, xor_add_truths)
{
    s->xorclauses = str_to_xors("1, 2 = 0; 1 = 1; 2 = 0");
    bool ret = finder->xor_together_xors(s->xorclauses);
    EXPECT_TRUE(ret);
    EXPECT_EQ(s->xorclauses.size(), 1U);

    ret = finder->add_new_truths_from_xors(s->xorclauses);
    EXPECT_FALSE(ret);
}

TEST_F(xor_finder2, xor_unit)
{
    s->xorclauses = str_to_xors("1 = 0; 1, 2, 3 = 1; 3 = 1");
    finder->xor_together_xors(s->xorclauses);
    bool ret = finder->add_new_truths_from_xors(s->xorclauses);
    EXPECT_TRUE(ret);
    EXPECT_EQ(s->xorclauses.size(), 0u);
}*/

#ifdef USE_M4RI
TEST_F(xor_finder2, xor_unit2_2)
{
    s->add_clause_outside(str_to_cl("-4"));
    s->xorclauses = str_to_xors("1, 2, 3 = 0; 1, 2, 3, 4 = 1;");
    vector<Lit> out_changed_occur;
    bool ret = topLevelGauss->toplevelgauss(s->xorclauses, &out_changed_occur);
    EXPECT_FALSE(ret);
}
#endif

// TEST_F(xor_finder2, xor_binx)
// {
//     s->xorclauses = str_to_xors("1, 2, 5 = 0; 1, 3 = 0; 2 = 0");
//     bool ret = finder->xor_together_xors(s->xorclauses);
//     if (ret) {
//         ret &= finder->add_new_truths_from_xors(s->xorclauses);
//     }
//     EXPECT_TRUE(ret);
//     EXPECT_EQ(s->xorclauses.size(), 0u);
//     check_red_cls_eq(s, "5, -3; -5, 3");
// }
//
// TEST_F(xor_finder2, xor_binx_inv_not_found)
// {
//     s->xorclauses = str_to_xors("3, 1 = 1; 1, 3 = 0;");
//     bool ret = finder->xor_together_xors(s->xorclauses);
//     if (ret) {
//         ret &= finder->add_new_truths_from_xors(s->xorclauses);
//     }
//     EXPECT_TRUE(ret);
// }

TEST_F(xor_finder2, xor_recur_bug)
{
    s->xorclauses = str_to_xors("3, 7, 9 = 0; 1, 3, 4, 5 = 1;");
    bool ret = finder->xor_together_xors(s->xorclauses);
    EXPECT_TRUE(ret);
    check_xors_eq(s->xorclauses, "7, 9, 1, 4, 5 = 1 c 3;");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

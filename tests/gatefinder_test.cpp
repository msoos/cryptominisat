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

#include <set>
using std::set;

#include "src/solver.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct gatefinder_test : public ::testing::Test {
    gatefinder_test()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
    }
    ~gatefinder_test()
    {
        delete s;
    }

    Solver* s;
    std::atomic<bool> must_inter;
};

//BY-BY 1

TEST_F(gatefinder_test, orgate_2)
{
    s->new_vars(20);
    s->add_clause_outside(str_to_cl("1, -2"));
    s->add_clause_outside(str_to_cl("1, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));

    const auto gs = s->get_recovered_or_gates();
    EXPECT_EQ( 1, gs.size());
    EXPECT_EQ( Lit(0, false), gs[0].rhs);
}

TEST_F(gatefinder_test, orgate_3)
{
    s->new_vars(20);
    s->add_clause_outside(str_to_cl("1, -2"));
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("1, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));

    const auto gs = s->get_recovered_or_gates();
    EXPECT_EQ( 1, gs.size());
    EXPECT_EQ( Lit(0, false), gs[0].rhs);
}

TEST_F(gatefinder_test, orgate_4)
{
    s->new_vars(20);
    s->add_clause_outside(str_to_cl("1, -2"));
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("1, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));

    const auto gs = s->get_recovered_or_gates();
    EXPECT_EQ( 1, gs.size());
    const auto tmp = gs[0].get_lhs();
    EXPECT_TRUE(find_lit(tmp, "2"));
    EXPECT_TRUE(find_lit(tmp, "3"));
}


TEST_F(gatefinder_test, orgate_5)
{
    s->new_vars(20);
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("-1, -3"));
    s->add_clause_outside(str_to_cl("1, 2, 3"));

    const auto gs = s->get_recovered_or_gates();
    EXPECT_EQ( str_to_lit("-1"), gs[0].rhs);

    EXPECT_EQ( 1, gs.size());
    const auto tmp = gs[0].get_lhs();
    EXPECT_TRUE(find_lit(tmp, "2"));
    EXPECT_TRUE(find_lit(tmp, "3"));
}


TEST_F(gatefinder_test, orgate_6)
{
    s->new_vars(20);
    s->add_clause_outside(str_to_cl("-1, 2"));
    s->add_clause_outside(str_to_cl("-1, 3"));
    s->add_clause_outside(str_to_cl("1, -2, -3"));

    const auto gs = s->get_recovered_or_gates();
    EXPECT_EQ( str_to_lit("-1"), gs[0].rhs);

    EXPECT_EQ( 1, gs.size());
    const auto tmp = gs[0].get_lhs();
    EXPECT_TRUE(find_lit(tmp, "-2"));
    EXPECT_TRUE(find_lit(tmp, "-3"));
}

//a = 1
//f = 2
//g = 3
//x = 4

// -a V  f V -x
// -a V  g V  x
//  a V -f V -x
//  a V -g V  x
TEST_F(gatefinder_test, itegate_1)
{
    s->new_vars(20);
    s->add_clause_outside(str_to_cl("-1, 2, -4"));
    s->add_clause_outside(str_to_cl("-1, 3, 4"));
    s->add_clause_outside(str_to_cl("1, -2, -4"));
    s->add_clause_outside(str_to_cl("1, -3, 4"));

    const auto gs = s->get_recovered_ite_gates();

    EXPECT_EQ( 2, gs.size());
    const auto tmp = gs[0].get_all();
    EXPECT_TRUE(find_lit(tmp, "-2"));
    EXPECT_TRUE(find_lit(tmp, "-3"));
    EXPECT_TRUE(find_lit(tmp, "-4"));
    EXPECT_TRUE(find_lit(tmp, "1"));
}


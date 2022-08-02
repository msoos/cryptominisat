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

#include "cryptominisat5/cryptominisat.h"
#include "src/solverconf.h"
#include "test_helper.h"
#include "src/lucky.h"
using namespace CMSat;
#include <vector>
using std::vector;


struct lucky : public ::testing::Test {
    lucky()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
        l = new Lucky(s);
    }
    ~lucky()
    {
        delete l;
        delete s;
    }
    Solver* s = NULL;
    Lucky* l = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(lucky, pos1)
{
    s->add_clause_outside(str_to_cl("1, -12"));
    s->add_clause_outside(str_to_cl("2, -3, -4, -5, -6"));
    s->add_clause_outside(str_to_cl("2, -3, -4, -5, -7"));

    EXPECT_EQ(l->check_all(true), true);
}

TEST_F(lucky, pos2)
{
    s->add_clause_outside(str_to_cl("1"));
    s->add_clause_outside(str_to_cl("-9, -2, -5, 6"));
    s->add_clause_outside(str_to_cl("-8, -4, -5, 7"));

    EXPECT_EQ(l->check_all(true), true);
}

TEST_F(lucky, not_pos)
{
    s->add_clause_outside(str_to_cl("-1, -2, -3, -4"));
    s->add_clause_outside(str_to_cl("2, 3, 4, 5, 7"));
    s->add_clause_outside(str_to_cl("12, 13, 14, 15, 17"));

    EXPECT_EQ(l->check_all(true), false);
}

TEST_F(lucky, neg)
{
    s->add_clause_outside(str_to_cl("1, 2, 3, -4"));
    s->add_clause_outside(str_to_cl("2, 3, -4, 5, 7"));
    s->add_clause_outside(str_to_cl("12, 13, 14, 15, -17"));

    EXPECT_EQ(l->check_all(false), true);
}

TEST_F(lucky, neg2)
{
    s->add_clause_outside(str_to_cl("1,-4"));
    s->add_clause_outside(str_to_cl("2, -4"));
    s->add_clause_outside(str_to_cl("12, -4"));

    EXPECT_EQ(l->check_all(false), true);
}

TEST_F(lucky, fwd1)
{
    s->add_clause_outside(str_to_cl("1, -4"));
    s->add_clause_outside(str_to_cl("4, 5"));
    s->add_clause_outside(str_to_cl("-6, 7"));
    s->add_clause_outside(str_to_cl("7, -8"));

    EXPECT_EQ(l->search_fwd_sat(true), true);
}

TEST_F(lucky, fwd1_fail)
{
    s->add_clause_outside(str_to_cl("1, -4"));
    s->add_clause_outside(str_to_cl("-1, -4"));
    s->add_clause_outside(str_to_cl("-1, 4"));

    EXPECT_EQ(l->search_fwd_sat(true), false);
}

TEST_F(lucky, horn)
{
    s->add_clause_outside(str_to_cl("-1, -4, -5, 6"));
    s->add_clause_outside(str_to_cl("-1, -2, -3, 5"));
    s->add_clause_outside(str_to_cl("-5, -6, 7"));

    EXPECT_EQ(l->horn_sat(true), true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

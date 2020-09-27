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
#include "src/cardfinder.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct card_finder : public ::testing::Test {
    card_finder()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 1;
        s = new Solver(&conf, &must_inter);
        s->new_vars(50);
        finder =  new CardFinder(s);
    }
    ~card_finder()
    {
        delete finder;
        delete s;
    }
    Solver* s = NULL;
    CardFinder* finder = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(card_finder, find_none)
{
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("-3, -4"));

    finder->find_cards();
    EXPECT_EQ(finder->get_cards().size(), 0U);
}

TEST_F(card_finder, find_one)
{
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("-1, -3"));
    s->add_clause_outside(str_to_cl("-2, -3"));

    finder->find_cards();
    EXPECT_EQ(finder->get_cards().size(), 1U);
}

TEST_F(card_finder, find_one_4)
{
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("-1, -3"));
    s->add_clause_outside(str_to_cl("-1, -4"));
    s->add_clause_outside(str_to_cl("-2, -3"));
    s->add_clause_outside(str_to_cl("-2, -4"));
    s->add_clause_outside(str_to_cl("-3, -4"));

    finder->find_cards();
    ASSERT_EQ(finder->get_cards().size(), 1U);
    vector<Lit> lits = finder->get_cards()[0];
    std::sort(lits.begin(), lits.end());
    EXPECT_EQ(lits, str_to_cl("1,2,3,4"));
}

TEST_F(card_finder, find_two)
{
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("-1, -3"));
    s->add_clause_outside(str_to_cl("-1, -4"));
    s->add_clause_outside(str_to_cl("-2, -3"));
    s->add_clause_outside(str_to_cl("-2, -4"));
    s->add_clause_outside(str_to_cl("-3, -4"));

    s->add_clause_outside(str_to_cl("-11, -21"));
    s->add_clause_outside(str_to_cl("-11, -31"));
    s->add_clause_outside(str_to_cl("-11, -41"));
    s->add_clause_outside(str_to_cl("-21, -31"));
    s->add_clause_outside(str_to_cl("-21, -41"));
    s->add_clause_outside(str_to_cl("-31, -41"));

    finder->find_cards();
    ASSERT_EQ(finder->get_cards().size(), 2U);
}


TEST_F(card_finder, find_large)
{
    s->add_clause_outside(str_to_cl("-1, -2"));
    s->add_clause_outside(str_to_cl("-1, -3"));
    s->add_clause_outside(str_to_cl("-1, -4"));
    s->add_clause_outside(str_to_cl("-2, -3"));
    s->add_clause_outside(str_to_cl("-2, -4"));
    s->add_clause_outside(str_to_cl("-3, -4"));

    s->add_clause_outside(str_to_cl("4, -5"));
    s->add_clause_outside(str_to_cl("4, -6"));
    s->add_clause_outside(str_to_cl("4, -7"));
    s->add_clause_outside(str_to_cl("-5, -6"));
    s->add_clause_outside(str_to_cl("-5, -7"));
    s->add_clause_outside(str_to_cl("-6, -7"));

    s->add_clause_outside(str_to_cl("-11, -12"));
    s->add_clause_outside(str_to_cl("-11, -13"));
    s->add_clause_outside(str_to_cl("-11, -14"));
    s->add_clause_outside(str_to_cl("-12, -13"));
    s->add_clause_outside(str_to_cl("-12, -14"));
    s->add_clause_outside(str_to_cl("-13, -14"));

    s->add_clause_outside(str_to_cl("14, -15"));
    s->add_clause_outside(str_to_cl("14, -16"));
    s->add_clause_outside(str_to_cl("14, -17"));
    s->add_clause_outside(str_to_cl("-15, -16"));
    s->add_clause_outside(str_to_cl("-15, -17"));
    s->add_clause_outside(str_to_cl("-16, -17"));

    s->add_clause_outside(str_to_cl("1, 11"));
    s->add_clause_outside(str_to_cl("1, -20"));
    s->add_clause_outside(str_to_cl("11, -20"));

    finder->find_cards();
    ASSERT_EQ(finder->get_cards().size(), 1U);
}

TEST_F(card_finder, find_two_product)
{
    //c x1..x9
    //c rows: x 20..x22
    //c cols: x 30..x32
    //c --------------
    s->add_clause_outside(str_to_cl("-1, 20"));
    s->add_clause_outside(str_to_cl("-1, 30"));
    s->add_clause_outside(str_to_cl("-2, 21"));
    s->add_clause_outside(str_to_cl("-2, 30"));
    s->add_clause_outside(str_to_cl("-3, 22"));
    s->add_clause_outside(str_to_cl("-3, 30"));
    s->add_clause_outside(str_to_cl("-4, 20"));
    s->add_clause_outside(str_to_cl("-4, 31"));
    s->add_clause_outside(str_to_cl("-5, 21"));
    s->add_clause_outside(str_to_cl("-5, 31"));
    s->add_clause_outside(str_to_cl("-6, 22"));
    s->add_clause_outside(str_to_cl("-6, 31"));
    s->add_clause_outside(str_to_cl("-7, 20"));
    s->add_clause_outside(str_to_cl("-7, 32"));
    s->add_clause_outside(str_to_cl("-8, 21"));
    s->add_clause_outside(str_to_cl("-8, 32"));
    s->add_clause_outside(str_to_cl("-9, 22"));
    s->add_clause_outside(str_to_cl("-9, 32"));

    //c ------ rows
    s->add_clause_outside(str_to_cl("-20, -21"));
    s->add_clause_outside(str_to_cl("-20, -22"));
    s->add_clause_outside(str_to_cl("-21, -22"));

    //c ------ cols
    s->add_clause_outside(str_to_cl("-30, -31"));
    s->add_clause_outside(str_to_cl("-30, -32"));
    s->add_clause_outside(str_to_cl("-31, -32"));

    finder->find_cards();
    ASSERT_EQ(finder->get_cards().size(), 3U);

    vector<Lit> lits;
    lits = finder->get_cards()[0];
    EXPECT_EQ(lits, str_to_cl("20, 21, 22"));

    lits = finder->get_cards()[1];
    EXPECT_EQ(lits, str_to_cl("30, 31, 32"));

    lits = finder->get_cards()[2];
    EXPECT_EQ(lits, str_to_cl("1, 2, 3, 4, 5, 6, 7, 8, 9"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

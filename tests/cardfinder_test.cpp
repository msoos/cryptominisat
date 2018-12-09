/******************************************
Copyright (c) 2016, Mate Soos

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
    s->add_clause_outer(str_to_cl("-1, -2"));
    s->add_clause_outer(str_to_cl("-3, -4"));

    finder->find_cards();
    EXPECT_EQ(finder->get_cards().size(), 0U);
}

TEST_F(card_finder, find_one)
{
    s->add_clause_outer(str_to_cl("-1, -2"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->add_clause_outer(str_to_cl("-2, -3"));

    finder->find_cards();
    EXPECT_EQ(finder->get_cards().size(), 1U);
}

TEST_F(card_finder, find_one_4)
{
    s->add_clause_outer(str_to_cl("-1, -2"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->add_clause_outer(str_to_cl("-1, -4"));
    s->add_clause_outer(str_to_cl("-2, -3"));
    s->add_clause_outer(str_to_cl("-2, -4"));
    s->add_clause_outer(str_to_cl("-3, -4"));

    finder->find_cards();
    ASSERT_EQ(finder->get_cards().size(), 1U);
    vector<Lit> lits = finder->get_cards()[0];
    std::sort(lits.begin(), lits.end());
    EXPECT_EQ(lits, str_to_cl("1,2,3,4"));
}

TEST_F(card_finder, find_two)
{
    s->add_clause_outer(str_to_cl("-1, -2"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->add_clause_outer(str_to_cl("-1, -4"));
    s->add_clause_outer(str_to_cl("-2, -3"));
    s->add_clause_outer(str_to_cl("-2, -4"));
    s->add_clause_outer(str_to_cl("-3, -4"));

    s->add_clause_outer(str_to_cl("-11, -21"));
    s->add_clause_outer(str_to_cl("-11, -31"));
    s->add_clause_outer(str_to_cl("-11, -41"));
    s->add_clause_outer(str_to_cl("-21, -31"));
    s->add_clause_outer(str_to_cl("-21, -41"));
    s->add_clause_outer(str_to_cl("-31, -41"));

    finder->find_cards();
    ASSERT_EQ(finder->get_cards().size(), 2U);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

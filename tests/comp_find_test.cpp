/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "gtest/gtest.h"

#include <fstream>

#include "src/solver.h"
#include "src/compfinder.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct comp_finder : public ::testing::Test {
    comp_finder()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        conf.doCache = false;
        s = new Solver(&conf, &must_inter);
        s->new_vars(20);
        finder = new CompFinder(s);
    }
    ~comp_finder()
    {
        delete finder;
        delete s;
    }
    CompFinder *finder;
    Solver* s = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(comp_finder, find_1_1)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    finder->find_components();

    EXPECT_EQ(finder->getNumComps(), 1U);
}

TEST_F(comp_finder, find_1_2)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("3, -4, 5"));
    s->add_clause_outer(str_to_cl("7, -5"));
    s->add_clause_outer(str_to_cl("7, 10, 15"));
    s->add_clause_outer(str_to_cl("15, 4, 2"));

    finder->find_components();

    EXPECT_EQ(finder->getNumComps(), 1U);
}

TEST_F(comp_finder, find_2_1)
{
    s->add_clause_outer(str_to_cl("1, 2"));

    s->add_clause_outer(str_to_cl("3, -4, 5"));
    s->add_clause_outer(str_to_cl("7, -5"));
    s->add_clause_outer(str_to_cl("7, 10, 15"));
    s->add_clause_outer(str_to_cl("15, 4"));

    finder->find_components();

    EXPECT_EQ(finder->getNumComps(), 2U);
}

TEST_F(comp_finder, find_2_2)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("1, 14, 17"));

    s->add_clause_outer(str_to_cl("3, -4, 5"));
    s->add_clause_outer(str_to_cl("7, -5"));
    s->add_clause_outer(str_to_cl("7, 10, 15"));
    s->add_clause_outer(str_to_cl("15, 4"));

    finder->find_components();

    EXPECT_EQ(finder->getNumComps(), 2U);
}

TEST_F(comp_finder, find_5_1)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("3, 4, 5"));

    s->add_clause_outer(str_to_cl("6, -7"));
    s->add_clause_outer(str_to_cl("6, -8"));

    s->add_clause_outer(str_to_cl("10, -12, 11"));
    s->add_clause_outer(str_to_cl("10, -17, 9"));

    s->add_clause_outer(str_to_cl("13, -14, 15, -16"));
    s->add_clause_outer(str_to_cl("13, -14, -16"));

    s->add_clause_outer(str_to_cl("-18, -17"));
    s->add_clause_outer(str_to_cl("-18, 20"));

    finder->find_components();

    EXPECT_EQ(finder->getNumComps(), 5U);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
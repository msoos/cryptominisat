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
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct undef : public ::testing::Test {
    undef()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        conf.doCache = false;
        s = new Solver(&conf, &must_inter);
        s->conf.greedy_undef = true;
    }
    ~undef()
    {
        delete s;
    }
    Solver* s = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(undef, replace)
{
    s->new_vars(2);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    uint32_t undef = count_num_undef_in_solution(s);
    EXPECT_EQ(undef, 0);
}

TEST_F(undef, simple_1)
{
    s->new_vars(2);
    s->add_clause_outer(str_to_cl("1, 2"));
    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    uint32_t undef = count_num_undef_in_solution(s);
    EXPECT_EQ(undef, 1);
}

TEST_F(undef, simple_2)
{
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    uint32_t undef = count_num_undef_in_solution(s);
    EXPECT_EQ(undef, 2);
}

TEST_F(undef, simple_2_mult)
{
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    for(size_t i = 0; i < 20; i++) {
        lbool ret = s->solve_with_assumptions();
        EXPECT_EQ(ret, l_True);
        uint32_t undef = count_num_undef_in_solution(s);
        EXPECT_EQ(undef, 2);
    }
}

//TODO add test for multiple solve() calls
//TODO add test for varelim->solve->varelim->solve etc. calls

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

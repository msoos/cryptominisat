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

#include <set>
using std::set;

#include "src/solver.h"
#include "src/comphandler.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct comp_handle : public ::testing::Test {
    comp_handle()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
        s->testing_fill_assumptions_set();
        chandle = s->compHandler;
    }
    ~comp_handle()
    {
        delete s;
    }

    Solver* s;
    CompHandler* chandle = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(comp_handle, handle_1_comp)
{
    s->add_clause_outer(str_to_cl("1, -2, 3"));

    chandle->handle();
    EXPECT_TRUE(s->okay());
    EXPECT_EQ(chandle->get_num_vars_removed(), 0);
}

TEST_F(comp_handle, handle_2_comps)
{
    s->add_clause_outer(str_to_cl("1, -2, 3"));

    s->add_clause_outer(str_to_cl("9, 4, 5"));
    s->add_clause_outer(str_to_cl("5, 6, 7"));

    chandle->handle();
    EXPECT_TRUE(s->okay());
    EXPECT_EQ(chandle->get_num_vars_removed(), 3);
    EXPECT_EQ(chandle->get_num_components_solved(), 1);
}

TEST_F(comp_handle, handle_3_comps)
{
    s->add_clause_outer(str_to_cl("1, -2, 3"));

    s->add_clause_outer(str_to_cl("9, 4, 5"));
    s->add_clause_outer(str_to_cl("5, 6, 7"));

    s->add_clause_outer(str_to_cl("19, 14, 15"));
    s->add_clause_outer(str_to_cl("15, 16, 17"));

    chandle->handle();
    EXPECT_TRUE(s->okay());
    EXPECT_EQ(chandle->get_num_components_solved(), 2);
    EXPECT_EQ(chandle->get_num_vars_removed(), 8);
}

TEST_F(comp_handle, check_solution_zero_lev_assign)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, 2"));
    s->add_clause_outer(str_to_cl("1, -2"));

    s->add_clause_outer(str_to_cl("11, 12"));
    s->add_clause_outer(str_to_cl("-11, 12"));
    s->add_clause_outer(str_to_cl("11, -12"));

    s->add_clause_outer(str_to_cl("19, 14, 15"));
    s->add_clause_outer(str_to_cl("15, 16, 17"));
    s->add_clause_outer(str_to_cl("17, 16, 18, 14"));
    s->add_clause_outer(str_to_cl("17, 18, 13"));

    chandle->handle();
    EXPECT_TRUE(s->okay());
    EXPECT_EQ(chandle->get_num_components_solved(), 2);
    EXPECT_EQ(chandle->get_num_vars_removed(), 0);
    vector<lbool> solution(s->nVarsOuter(), l_Undef);
    chandle->addSavedState(solution);
    check_zero_assigned_lits_contains(s, "1");
    check_zero_assigned_lits_contains(s, "1");
    check_zero_assigned_lits_contains(s, "11");
    check_zero_assigned_lits_contains(s, "12");
}

TEST_F(comp_handle, check_solution_non_zero_lev_assign)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("11, 12"));
    s->add_clause_outer(str_to_cl("-11, 12"));

    s->add_clause_outer(str_to_cl("20, 22"));
    s->add_clause_outer(str_to_cl("-24, 22"));

    s->add_clause_outer(str_to_cl("19, 14, 15"));
    s->add_clause_outer(str_to_cl("15, 16, 17"));
    s->add_clause_outer(str_to_cl("17, 16, 18, 14"));
    s->add_clause_outer(str_to_cl("17, 18, 13"));

    chandle->handle();
    EXPECT_TRUE(s->okay());
    EXPECT_EQ(chandle->get_num_components_solved(), 3);
    EXPECT_EQ(chandle->get_num_vars_removed(), 7);
    vector<lbool> solution(s->nVarsOuter(), l_Undef);
    chandle->addSavedState(solution);
    EXPECT_TRUE(clause_satisfied("1, 2", solution));
    EXPECT_TRUE(clause_satisfied("-1, 2", solution));
    EXPECT_TRUE(clause_satisfied("11, 12", solution));
    EXPECT_TRUE(clause_satisfied("-11, 12", solution));
    EXPECT_TRUE(clause_satisfied("20, 22", solution));
    EXPECT_TRUE(clause_satisfied("-24, 22", solution));
}

TEST_F(comp_handle, check_unsat)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, 2"));
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    s->add_clause_outer(str_to_cl("19, 14, 15"));
    s->add_clause_outer(str_to_cl("15, 16, 17"));
    s->add_clause_outer(str_to_cl("17, 16, 18, 14"));
    s->add_clause_outer(str_to_cl("17, 18, 13"));

    bool ret = chandle->handle();
    EXPECT_FALSE(ret);
    EXPECT_FALSE(s->okay());
    EXPECT_EQ(chandle->get_num_components_solved(), 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

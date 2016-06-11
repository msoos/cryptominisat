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
        s->conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;
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

    uint32_t num_undef = count_num_undef_in_solution(s);
    EXPECT_EQ(num_undef, 0u);
}

TEST_F(undef, simple_1)
{
    s->new_vars(2);
    s->add_clause_outer(str_to_cl("1, 2"));
    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    uint32_t num_undef = count_num_undef_in_solution(s);
    EXPECT_EQ(num_undef, 1u);
}

TEST_F(undef, simple_2)
{
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    uint32_t num_undef = count_num_undef_in_solution(s);
    EXPECT_EQ(num_undef, 2u);
}

TEST_F(undef, simple_2_mult)
{
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    for(size_t i = 0; i < 20; i++) {
        lbool ret = s->solve_with_assumptions();
        EXPECT_EQ(ret, l_True);
        uint32_t num_undef = count_num_undef_in_solution(s);
        EXPECT_EQ(num_undef, 2u);
    }
}

TEST_F(undef, simple_2_mult_novarelim)
{
    s->conf.verbosity = 0;
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("-1, -2, -3"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->conf.perform_occur_based_simp = 0;
    for(size_t i = 0; i < 20; i++) {
        lbool ret = s->solve_with_assumptions();
        EXPECT_EQ(ret, l_True);
        uint32_t num_undef = count_num_undef_in_solution(s);
        EXPECT_EQ(num_undef, 2u);
    }
}

TEST_F(undef, simple_2_ind_no)
{
    s->conf.verbosity = 0;
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("-1, -2, -3"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->conf.independent_vars = new std::vector<uint32_t>();
    s->conf.perform_occur_based_simp = 0;

    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    uint32_t num_undef = count_num_undef_in_solution(s);
    EXPECT_EQ(num_undef, 0u);
    delete s->conf.independent_vars;
}

TEST_F(undef, simple_2_ind_1)
{
    s->conf.verbosity = 0;
    s->new_vars(3);
    s->add_clause_outer(str_to_cl("-1, -2, -3"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->conf.independent_vars = new std::vector<uint32_t>();
    s->conf.perform_occur_based_simp = 0;
    s->conf.independent_vars->push_back(1); //i.e. var 2

    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    EXPECT_EQ(s->model_value(1), l_Undef);
    delete s->conf.independent_vars;
}

TEST_F(undef, simple_2_ind_renumber)
{
    s->conf.verbosity = 0;
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("-4, -2, -3"));
    s->add_clause_outer(str_to_cl("-4, -3"));
    s->add_clause_outer(str_to_cl("-1"));
    s->conf.independent_vars = new std::vector<uint32_t>();
    s->conf.perform_occur_based_simp = 0;
    s->conf.independent_vars->push_back(1); //i.e. var 2

    //since '1' has been set but renumer NOT called
    EXPECT_EQ(s->nVars(), 4u);
    s->clear_order_heap();
    s->renumber_variables();
    s->rebuildOrderHeap();

    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    //since '1' has been set and renumber called
    EXPECT_EQ(s->nVars(), 3u);

    EXPECT_EQ(s->model_value(1), l_Undef);
    delete s->conf.independent_vars;
}

TEST_F(undef, simple_2_ind_renumber_empty_indep)
{
    s->conf.verbosity = 0;
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("-4, -2, -3"));
    s->add_clause_outer(str_to_cl("-4, -3"));
    s->add_clause_outer(str_to_cl("-1"));
    s->conf.independent_vars = new std::vector<uint32_t>();
    s->conf.perform_occur_based_simp = 0;

    //since '1' has been set but renumer NOT called
    EXPECT_EQ(s->nVars(), 4u);
    s->clear_order_heap();
    s->renumber_variables();
    s->rebuildOrderHeap();

    lbool ret = s->solve_with_assumptions();
    EXPECT_EQ(ret, l_True);

    //since '1' has been set and renumber called
    EXPECT_EQ(s->nVars(), 3u);

    //Since indep vars is empty, all is set
    for(size_t i = 0; i < 4; i++) {
        EXPECT_NE(s->model_value(i), l_Undef);
    }
    delete s->conf.independent_vars;
}

//TODO add test for multiple solve() calls
//TODO add test for varelim->solve->varelim->solve etc. calls

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

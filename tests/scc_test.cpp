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
#include "src/sccfinder.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

TEST(scc_test, find_1)
{
    SolverConf conf;
    conf.doCache = false;

    Solver s(&conf, new std::atomic<bool>(false));
    s.new_vars(2);
    s.add_clause_outer(str_to_cl("1, 2"));
    s.add_clause_outer(str_to_cl("-1, -2"));

    SCCFinder scc(&s);
    scc.performSCC();
    EXPECT_EQ(scc.get_binxors().size(), 1);
}

TEST(scc_test, find_2)
{
    SolverConf conf;
    conf.doCache = false;

    Solver s(&conf, new std::atomic<bool>(false));
    s.new_vars(4);
    s.add_clause_outer(str_to_cl("1, 2"));
    s.add_clause_outer(str_to_cl("-1, -2"));

    s.add_clause_outer(str_to_cl("3, 4"));
    s.add_clause_outer(str_to_cl("-3, -4"));

    SCCFinder scc(&s);
    scc.performSCC();
    EXPECT_EQ(scc.get_binxors().size(), 2);
}

TEST(scc_test, find_circle_3)
{
    SolverConf conf;
    conf.doCache = false;

    Solver s(&conf, new std::atomic<bool>(false));
    s.new_vars(4);
    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("2, -3"));
    s.add_clause_outer(str_to_cl("3, -1"));

    SCCFinder scc(&s);
    scc.performSCC();
    EXPECT_EQ(scc.get_binxors().size(), 3);
}

TEST(scc_test, find_two_circle2_3)
{
    SolverConf conf;
    conf.doCache = false;

    Solver s(&conf, new std::atomic<bool>(false));
    s.new_vars(6);
    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("2, -3"));
    s.add_clause_outer(str_to_cl("3, -1"));

    s.add_clause_outer(str_to_cl("4, -5"));
    s.add_clause_outer(str_to_cl("5, -6"));
    s.add_clause_outer(str_to_cl("6, -4"));

    SCCFinder scc(&s);
    scc.performSCC();
    EXPECT_EQ(scc.get_binxors().size(), 6);
}

TEST(scc_test, find_1_diff)
{
    SolverConf conf;
    conf.doCache = false;

    Solver s(&conf, new std::atomic<bool>(false));
    s.new_vars(2);
    s.add_clause_outer(str_to_cl("1, 2"));
    s.add_clause_outer(str_to_cl("-1, -2"));
    s.add_clause_outer(str_to_cl("1, -2"));

    SCCFinder scc(&s);
    scc.performSCC();
    EXPECT_EQ(scc.get_binxors().size(), 1);
}

TEST(scc_test, find_0)
{
    SolverConf conf;
    conf.doCache = false;

    Solver s(&conf, new std::atomic<bool>(false));
    s.new_vars(4);
    s.add_clause_outer(str_to_cl("1, 2"));
    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("3, -4"));

    SCCFinder scc(&s);
    scc.performSCC();
    EXPECT_EQ(scc.get_binxors().size(), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

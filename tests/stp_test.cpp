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

#include "cryptominisat4/cryptominisat.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

void add_clauses_for_simp_check(SATSolver& s)
{
    s.new_vars(4);

    // 1 = 2
    s.add_clause(str_to_cl("1, -2"));
    s.add_clause(str_to_cl("-1, 2"));

    // 3 = 4
    s.add_clause(str_to_cl("3, -4"));
    s.add_clause(str_to_cl("-3, 4"));

    //no elimination
    s.add_clause(str_to_cl("3, 2"));
    s.add_clause(str_to_cl("4, 1"));
}

TEST(stp_test, no_simp_at_startup)
{
    SATSolver s;
    s.set_no_simplify_at_startup();
    add_clauses_for_simp_check(s);

    s.solve();
    auto eq_xors = s.get_all_binary_xors();
    EXPECT_EQ(eq_xors.size(), 0U);
}

TEST(stp_test, simp_at_startup)
{
    SATSolver s;
    add_clauses_for_simp_check(s);

    s.solve();
    auto eq_xors = s.get_all_binary_xors();
    EXPECT_EQ(eq_xors.size(), 1U);
}

TEST(stp_test, set_num_threads_true)
{
    SATSolver s;
    s.set_num_threads(5);
    s.new_vars(2);
    s.add_clause(str_to_cl("1,2"));
    s.add_clause(str_to_cl("1,-2"));

    lbool ret = s.solve();
    EXPECT_EQ(ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_True);
}

TEST(stp_test, set_num_threads_false)
{
    SATSolver s;
    s.set_no_simplify_at_startup();
    s.set_num_threads(5);
    s.new_vars(2);
    s.add_clause(str_to_cl("1,2"));
    s.add_clause(str_to_cl("1,-2"));
    s.add_clause(str_to_cl("-1,2"));
    s.add_clause(str_to_cl("-1,-2"));
    lbool ret = s.solve();
    EXPECT_EQ(ret, l_False);
}

TEST(stp_test, default_polar_false)
{
    SATSolver s;
    s.set_no_simplify_at_startup();
    s.set_default_polarity(false);
    s.new_vars(4);
    s.add_clause(str_to_cl("-1, -2, -3, -4"));
    lbool ret = s.solve();
    EXPECT_EQ(ret, l_True);
    for(size_t i = 0; i < 4; i++) {
        EXPECT_EQ(s.get_model()[0], l_False);
    }
}

TEST(stp_test, default_polar_true)
{
    SATSolver s;
    s.set_no_simplify_at_startup();
    s.set_default_polarity(true);
    s.new_vars(4);
    s.add_clause(str_to_cl("1, 2, 3, 4"));
    lbool ret = s.solve();
    EXPECT_EQ(ret, l_True);
    for(size_t i = 0; i < 4; i++) {
        EXPECT_EQ(s.get_model()[0], l_True);
    }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

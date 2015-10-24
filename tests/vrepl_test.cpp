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
#include "src/varreplacer.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

TEST(vrepl_test, find_one_1)
{
    SolverConf conf;
    conf.doCache = false;
    Solver s(&conf, new bool);
    s.new_vars(20);
    VarReplacer& repl = *s.varReplacer;

    s.add_clause_outer(str_to_cl("1, 2"));
    s.add_clause_outer(str_to_cl("-1, -2"));

    s.add_clause_outer(str_to_cl("1, 3, 4, 5"));
    s.add_clause_outer(str_to_cl("2, 3, 4, 5"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 1);
    EXPECT_EQ(s.get_num_long_irred_cls(), 2);
    std::string exp = "-2, 3, 4, 5;  2, 3, 4, 5";
    EXPECT_TRUE(check_irred_cls_eq(s, exp));
}

TEST(vrepl_test, find_one_2)
{
    SolverConf conf;
    conf.doCache = false;
    Solver s(&conf, new bool);
    s.new_vars(20);
    VarReplacer& repl = *s.varReplacer;

    s.add_clause_outer(str_to_cl("1, -3"));
    s.add_clause_outer(str_to_cl("-1, 3"));

    s.add_clause_outer(str_to_cl("1, 4, 5"));
    s.add_clause_outer(str_to_cl("2, 3, 4, 5"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 1);
    std::string exp = "3, 4, 5;  2, 3, 4, 5";
    EXPECT_TRUE(check_irred_cls_eq(s, exp));
}

TEST(vrepl_test, remove_lit)
{
    SolverConf conf;
    conf.doCache = false;
    Solver s(&conf, new bool);
    s.new_vars(20);
    VarReplacer& repl = *s.varReplacer;

    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("-1, 2"));

    s.add_clause_outer(str_to_cl("1, 2, 5"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 1);
    std::string exp = "2, 5";
    EXPECT_TRUE(check_irred_cls_eq(s, exp));
}

TEST(vrepl_test, remove_cl)
{
    SolverConf conf;
    conf.doCache = false;
    Solver s(&conf, new bool);
    s.new_vars(20);
    VarReplacer& repl = *s.varReplacer;

    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("-1, 2"));

    s.add_clause_outer(str_to_cl("1, -2, 5"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 1);
    std::string exp = "";
    EXPECT_TRUE(check_irred_cls_eq(s, exp));
}

TEST(vrepl_test, replace_twice)
{
    SolverConf conf;
    conf.doCache = false;
    Solver s(&conf, new bool);
    s.new_vars(20);
    VarReplacer& repl = *s.varReplacer;

    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("-1, 2"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 1);

    s.add_clause_outer(str_to_cl("3, -2"));
    s.add_clause_outer(str_to_cl("-3, 2"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 2);

    s.add_clause_outer(str_to_cl("1, -2, 3"));
    s.add_clause_outer(str_to_cl("1, 2, 3, 5"));
    std::string exp = "2, 5";
    EXPECT_TRUE(check_irred_cls_eq(s, exp));
}

TEST(vrepl_test, replace_thrice)
{
    SolverConf conf;
    conf.doCache = false;
    Solver s(&conf, new bool);
    s.new_vars(20);
    VarReplacer& repl = *s.varReplacer;

    s.add_clause_outer(str_to_cl("1, -2"));
    s.add_clause_outer(str_to_cl("-1, 2"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 1);

    s.add_clause_outer(str_to_cl("3, -2"));
    s.add_clause_outer(str_to_cl("-3, 2"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 2);

    s.add_clause_outer(str_to_cl("4, -2"));
    s.add_clause_outer(str_to_cl("-4, 2"));

    repl.replace_if_enough_is_found();
    EXPECT_EQ(repl.get_num_replaced_vars(), 3);

    s.add_clause_outer(str_to_cl("1, -2, 3"));
    s.add_clause_outer(str_to_cl("1, 2, 4, 5"));
    std::string exp = "2, 5";
    EXPECT_TRUE(check_irred_cls_eq(s, exp));
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

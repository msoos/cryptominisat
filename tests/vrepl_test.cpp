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

struct varreplace : public ::testing::Test {
    varreplace()
    {
        SolverConf conf;
        conf.doCache = false;
        s = new Solver(&conf, &must_inter);
        s->new_vars(20);
        repl = s->varReplacer;
    }
    ~varreplace()
    {
        delete s;
    }
    Solver* s = NULL;
    VarReplacer* repl = NULL;
    bool must_inter = false;
};

TEST_F(varreplace, find_one_1)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    s->add_clause_outer(str_to_cl("1, 3, 4, 5"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 1);
    EXPECT_EQ(s->get_num_long_irred_cls(), 2);
    std::string exp = "-2, 3, 4, 5;  2, 3, 4, 5";
    check_irred_cls_eq(s, exp);
}

TEST_F(varreplace, find_one_2)
{
    s->add_clause_outer(str_to_cl("1, -3"));
    s->add_clause_outer(str_to_cl("-1, 3"));

    s->add_clause_outer(str_to_cl("1, 4, 5"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 1);
    std::string exp = "3, 4, 5;  2, 3, 4, 5";
    check_irred_cls_eq(s, exp);
}

TEST_F(varreplace, remove_lit)
{
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("1, 2, 5"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 1);
    std::string exp = "2, 5";
    check_irred_cls_eq(s, exp);
}

TEST_F(varreplace, remove_cl)
{
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("1, -2, 5"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 1);
    std::string exp = "";
    check_irred_cls_eq(s, exp);
}

TEST_F(varreplace, replace_twice)
{
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 1);

    s->add_clause_outer(str_to_cl("3, -2"));
    s->add_clause_outer(str_to_cl("-3, 2"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 2);

    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 5"));
    std::string exp = "2, 5";
    check_irred_cls_eq(s, exp);
}

TEST_F(varreplace, replace_thrice)
{
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 1);

    s->add_clause_outer(str_to_cl("3, -2"));
    s->add_clause_outer(str_to_cl("-3, 2"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 2);

    s->add_clause_outer(str_to_cl("4, -2"));
    s->add_clause_outer(str_to_cl("-4, 2"));

    repl->replace_if_enough_is_found();
    EXPECT_EQ(repl->get_num_replaced_vars(), 3);

    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 4, 5"));
    std::string exp = "2, 5";
    check_irred_cls_eq(s, exp);
}

TEST_F(varreplace, replace_limit_check_below)
{
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("3, -2"));
    s->add_clause_outer(str_to_cl("-3, 2"));

    repl->replace_if_enough_is_found(3);
    EXPECT_EQ(repl->get_num_replaced_vars(), 0);
}

TEST_F(varreplace, replace_limit_check_above)
{
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("3, -2"));
    s->add_clause_outer(str_to_cl("-3, 2"));

    repl->replace_if_enough_is_found(2);
    EXPECT_EQ(repl->get_num_replaced_vars(), 2);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

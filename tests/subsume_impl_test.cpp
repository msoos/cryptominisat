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
#include "src/subsumeimplicit.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct sub_impl : public ::testing::Test {
    sub_impl()
    {
        must_inter = false;
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        sub = s->subsumeImplicit;
    }
    ~sub_impl()
    {
        delete s;
    }

    Solver* s;
    SubsumeImplicit* sub;
    bool must_inter;
};

//SUB 2-by-2
TEST_F(sub_impl, sub_2by2_1)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, -2"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "1, -2");
}

TEST_F(sub_impl, sub_2by2_2)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, 3"));
    s->add_clause_outer(str_to_cl("1, -4"));
    s->add_clause_outer(str_to_cl("1, -2"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "1, -2; 1, 3; 1, -4");
}

TEST_F(sub_impl, sub_2by2_3)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("2, 1"));
    s->add_clause_outer(str_to_cl("2, 4"));
    s->add_clause_outer(str_to_cl("1, 3"));
    s->add_clause_outer(str_to_cl("1, 3"));
    s->add_clause_outer(str_to_cl("-1, -3"));
    s->add_clause_outer(str_to_cl("1, 2"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "1, 2; 1, 3;2, 4;-1, -3");
}

TEST_F(sub_impl, sub_2by2_4)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 4"));
    s->add_clause_outer(str_to_cl("-1, 2, 4"));
    s->add_clause_outer(str_to_cl("1, 4"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "1, 4; 1, 2, 3, 4;-1, 2, 4");
}

//SUB 3-by-2
TEST_F(sub_impl, sub_3by2_1)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, -2"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "1, -2");
}

TEST_F(sub_impl, sub_3by2_2)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("3, 5, 7"));
    s->add_clause_outer(str_to_cl("3, 5"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "3, 5");
}

TEST_F(sub_impl, sub_3by2_3)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("3, 4, 7"));
    s->add_clause_outer(str_to_cl("3, 5, -7"));
    s->add_clause_outer(str_to_cl("3, 5, 6"));
    s->add_clause_outer(str_to_cl("3, 5"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "3, 5; 3, 4, 7");
}

TEST_F(sub_impl, sub_3by2_no)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("3, 4, 7"));
    s->add_clause_outer(str_to_cl("3, 1, -7"));
    s->add_clause_outer(str_to_cl("3, 5"));

    sub->subsume_implicit();
    check_irred_cls_eq(s, "3, 5; 3, 4, 7; 3, 1, -7");
}


//Stamp 3
TEST_F(sub_impl, sub_3bystamp_1)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    add_to_stamp_irred(s, "1, 3");

    sub->subsume_implicit();
    check_irred_cls_eq(s, "");
}

TEST_F(sub_impl, sub_3bystamp_2)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    add_to_stamp_irred(s, "1, -2");

    sub->subsume_implicit();
    check_irred_cls_eq(s, "");
}

TEST_F(sub_impl, sub_3bystamp_3)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("4, 2, 3, 1"));
    s->add_clause_outer(str_to_cl("4, 3, -2"));
    s->add_clause_outer(str_to_cl("4, -2, 5"));
    add_to_stamp_irred(s, "4, -2");

    sub->subsume_implicit();
    check_irred_cls_eq(s, "4, 2, 3, 1");
}

TEST_F(sub_impl, sub_3bystamp_no)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("4, 2"));
    add_to_stamp_irred(s, "4, -2");

    sub->subsume_implicit();
    check_irred_cls_eq(s, "4, 2");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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
#include "src/str_impl_w_impl_stamp.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct distill_impl_w_impl : public ::testing::Test {
    distill_impl_w_impl()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        str = s->dist_impl_with_impl;
    }
    ~distill_impl_w_impl()
    {
        delete s;
    }

    Solver* s;
    StrImplWImplStamp* str;
    std::atomic<bool> must_inter;
};

//STR 3-by-2

TEST_F(distill_impl_w_impl, str_3by2_1)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("-1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, -2"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "-2, 3; 1, -2");
}

TEST_F(distill_impl_w_impl, str_3by2_2)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "1, 3; 2, 3");
}

TEST_F(distill_impl_w_impl, str_3by2_3)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("4, -2, 3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "2, 3; 1, 2; 4, 3");
}

TEST_F(distill_impl_w_impl, str_3by2_no)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "2, 3; 1, 2, 3");
}

//STR 3-by-3
TEST_F(distill_impl_w_impl, str_3by3_1)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_contains(s, "1, 3");
}

TEST_F(distill_impl_w_impl, str_3by3_2)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, -3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_contains(s, "1, 2");
}

TEST_F(distill_impl_w_impl, str_3by3_3)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "1, -2, 3; 1, 3; 1, 3");
}

TEST_F(distill_impl_w_impl, str_3by3_no)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "1, 2, 3;1, 2, 3");
}

//STR 3-by-stamp
TEST_F(distill_impl_w_impl, str_3bystamp_1)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("1, -2, 3"));

    add_to_stamp_irred(s, "1, 2");
    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "1, 3");
}

TEST_F(distill_impl_w_impl, str_3bystamp_2)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("2, -3, 5"));

    add_to_stamp_irred(s, "3, 5");
    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "2, 5");
}

TEST_F(distill_impl_w_impl, str_3bystamp_3)
{
    s->new_vars(7);
    s->add_clause_outer(str_to_cl("2, -3, 5"));
    s->add_clause_outer(str_to_cl("-3, 4, 5"));

    add_to_stamp_irred(s, "3, 5");
    str->str_impl_w_impl_stamp();
    check_irred_cls_eq(s, "2, 5; 4, 5");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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
#include "src/distillerwithbin.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct distill_with_bintri_test : public ::testing::Test {
    distill_with_bintri_test()
    {
        must_inter = false;
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        distillwbin = s->distillerwithbin;
    }
    ~distill_with_bintri_test()
    {
        delete s;
    }

    Solver* s;
    DistillerWithBin* distillwbin;
    bool must_inter;
};

//Subsume long with bin

TEST_F(distill_with_bintri_test, subsume_w_bin)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "1, 2");
}

TEST_F(distill_with_bintri_test, subsume_w_bin2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, 4, -5"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "1, 2");
}

TEST_F(distill_with_bintri_test, subsume_w_bin3)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("2, 3, -4, 5"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "2, 3");
}


//Subsume long with tri

TEST_F(distill_with_bintri_test, subsume_w_tri)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, -2, -3"));
    s->add_clause_outer(str_to_cl("1, -2, -3, 4"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "1, -2, -3");
}

TEST_F(distill_with_bintri_test, subsume2_w_tri2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, -2, 4"));
    s->add_clause_outer(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, 4, -5"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "1, -2, 4");
}

//No subsumption

TEST_F(distill_with_bintri_test, no_subsume)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "1, 2; 2, 3, 4, 5");
}

TEST_F(distill_with_bintri_test, no_subsume_tri)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5"));

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "1, 2; 2, 3, 4, 5");
}

//Strengthening long with bin

TEST_F(distill_with_bintri_test, str_w_bin)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distillwbin->distill_with_bin(true);
    check_irred_cls_contains(s, "1, 3, 4");
}

TEST_F(distill_with_bintri_test, str_w_bin2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, -2"));
    s->add_clause_outer(str_to_cl("-1, 2, 3, 4"));

    distillwbin->distill_with_bin(true);
    check_irred_cls_contains(s, "-1, 3, 4");
}

TEST_F(distill_with_bintri_test, str_w_bin3)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-2, -3"));
    s->add_clause_outer(str_to_cl("-2, 3, 4, 5"));
    s->add_clause_outer(str_to_cl("-1, 2, -3, 4"));

    distillwbin->distill_with_bin(true);
    check_irred_cls_contains(s, "-2, 4, 5");
    check_irred_cls_contains(s, "-1, -3, 4");
}


//Strengthening long with tri

TEST_F(distill_with_bintri_test, str_w_tri)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, -2, 3"));
    s->add_clause_outer(str_to_cl("-1, 2, 3, 4"));

    distillwbin->distill_with_bin(true);
    check_irred_cls_contains(s, "-1, 3, 4");
}

TEST_F(distill_with_bintri_test, str_w_tri2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, -2, -5"));
    s->add_clause_outer(str_to_cl("-1, 2, 3, -5"));

    distillwbin->distill_with_bin(true);
    check_irred_cls_contains(s, "-1, 3, -5");
}

TEST_F(distill_with_bintri_test, str_w_tri3)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-2, -3, 5"));
    s->add_clause_outer(str_to_cl("-2, 3, 4, 5"));
    s->add_clause_outer(str_to_cl("-1, 2, -3, 4, 5"));

    distillwbin->distill_with_bin(true);
    check_irred_cls_contains(s, "-2, 4, 5");
    check_irred_cls_contains(s, "-1, -3, 4, 5");
}

//Subsume with cache

TEST_F(distill_with_bintri_test, sub_w_cache)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, 2, -3, 4"));
    add_to_cache_irred(s, "2, -3");

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "");
}

TEST_F(distill_with_bintri_test, sub_w_cache2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, 2, -3, 4"));
    add_to_cache_irred(s, "2, 4");

    distillwbin->distill_with_bin(false);
    check_irred_cls_eq(s, "");
}

//STR with cache

TEST_F(distill_with_bintri_test, str_w_cache)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, 2, -3, 4"));
    add_to_cache_irred(s, "-1, -2");

    distillwbin->distill_with_bin(true);
    check_irred_cls_eq(s, "-1, -3, 4");
}

TEST_F(distill_with_bintri_test, str_w_cache2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("2, -3, 4, 5"));
    add_to_cache_irred(s, "4, -5");

    distillwbin->distill_with_bin(true);
    check_irred_cls_eq(s, "2, -3, 4");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

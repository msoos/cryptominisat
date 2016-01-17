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
#include "src/clausecleaner.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct clause_clean_test : public ::testing::Test {
    clause_clean_test()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        conf.doCache = false;
        s = new Solver(&conf, &must_inter);
        s->new_vars(20);
        cc = s->clauseCleaner;
    }
    ~clause_clean_test()
    {
        delete s;
    }
    Solver* s = NULL;
    ClauseCleaner* cc = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(clause_clean_test, no_clean)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, 2"));

    cc->remove_and_clean_all();
    EXPECT_EQ(s->binTri.irredBins, 1U);
    EXPECT_EQ(s->binTri.irredTris, 1U);
    std::string exp = "1, 2;  1, 2, 3";
    check_irred_cls_eq(s, exp);
}

TEST_F(clause_clean_test, clean_bin_pos)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("1"));
    check_irred_cls_eq(s, "1, 2");

    cc->remove_and_clean_all();
    EXPECT_EQ(s->binTri.irredBins, 0U);
    EXPECT_EQ(s->binTri.irredTris, 0U);
}

TEST_F(clause_clean_test, clean_bin_neg)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1"));
    check_irred_cls_eq(s, "1, 2");

    cc->remove_and_clean_all();
    check_irred_cls_eq(s, "");
}

TEST_F(clause_clean_test, clean_tri_pos)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1"));
    check_irred_cls_eq(s, "1, 2, 3");

    cc->remove_and_clean_all();
    check_irred_cls_eq(s, "");
}

TEST_F(clause_clean_test, clean_tri_neg)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("-1"));
    check_irred_cls_eq(s, "1, 2, 3");

    cc->remove_and_clean_all();
    EXPECT_EQ(s->binTri.irredBins, 1U);
    EXPECT_EQ(s->binTri.irredTris, 0U);
    check_irred_cls_eq(s, "2, 3");
}

TEST_F(clause_clean_test, clean_long_pos)
{
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1"));
    check_irred_cls_eq(s, "1, 2, 3, 4");

    cc->remove_and_clean_all();
    check_irred_cls_eq(s, "");
}

TEST_F(clause_clean_test, clean_long_neg)
{
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("-1"));
    check_irred_cls_eq(s, "1, 2, 3, 4");

    cc->remove_and_clean_all();
    check_irred_cls_eq(s, "2, 3, 4");
}

TEST_F(clause_clean_test, clean_mix)
{
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, 9"));
    s->add_clause_outer(str_to_cl("-1"));
    check_irred_cls_eq(s, "1, 2, 3, 4; 1, 2, 3; 1, 9");

    cc->remove_and_clean_all();
    check_irred_cls_eq(s, "2, 3, 4; 2, 3");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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
#include "src/gaussian.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct gauss : public ::testing::Test {
    gauss()
    {
        must_inter = false;
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
    }
    ~gauss()
    {
        delete g;
        delete s;
    }

    Solver* s;
    Gaussian* g = NULL;
    std::vector<uint32_t> vars;
    bool must_inter;
    vector<Xor> xs;
};

//2 XORs inside

TEST_F(gauss, propagate_1)
{
    s->conf.verbosity = 20;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("1, 2, 3, 4"), 0));

    g = new Gaussian(s, xs, 0);
    bool ret = g->init_until_fixedpoint();

    EXPECT_EQ(ret, true);
    EXPECT_EQ(s->ok, true);
    check_zero_assigned_lits_eq(s, "-4");
}

TEST_F(gauss, propagate_2)
{
    //s->conf.verbosity = 20;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("1, 2, 3, 4"), 1));

    g = new Gaussian(s, xs, 0);
    bool ret = g->init_until_fixedpoint();

    EXPECT_EQ(ret, true);
    EXPECT_EQ(s->ok, true);
    check_zero_assigned_lits_eq(s, "4");
}

TEST_F(gauss, propagate_3)
{
    //s->conf.verbosity = 20;
    xs.push_back(Xor(str_to_vars("1, 3, 4, 5"), 0));
    xs.push_back(Xor(str_to_vars("1, 3, 5"), 1));

    g = new Gaussian(s, xs, 0);
    bool ret = g->init_until_fixedpoint();

    EXPECT_EQ(ret, true);
    EXPECT_EQ(s->ok, true);
    check_zero_assigned_lits_eq(s, "4");
}

TEST_F(gauss, propagate_4)
{
    //s->conf.verbosity = 20;
    xs.push_back(Xor(str_to_vars("1, 3, 4, 5"), 0));
    xs.push_back(Xor(str_to_vars("1, 3, 5"), 1));
    xs.push_back(Xor(str_to_vars("1, 2, 4, 7"), 1));

    g = new Gaussian(s, xs, 0);
    bool ret = g->init_until_fixedpoint();

    EXPECT_EQ(ret, true);
    EXPECT_EQ(s->ok, true);
    check_zero_assigned_lits_eq(s, "4");
}

//UNSAT

TEST_F(gauss, unsat_4)
{
    //s->conf.verbosity = 20;
    xs.push_back(Xor(str_to_vars("1, 3, 5"), 0));
    xs.push_back(Xor(str_to_vars("1, 3, 5"), 1));

    g = new Gaussian(s, xs, 0);
    bool ret = g->init_until_fixedpoint();

    EXPECT_EQ(ret, false);
    EXPECT_EQ(s->ok, false);
}

//double prop

TEST_F(gauss, propagate_unsat)
{
    //s->conf.verbosity = 20;
    xs.push_back(Xor(str_to_vars("1, 3, 4, 5"), 0));
    xs.push_back(Xor(str_to_vars("1, 3, 5"), 0));
    //-> 4 = 0
    xs.push_back(Xor(str_to_vars("5, 6, 7"), 1));
    xs.push_back(Xor(str_to_vars("4, 5, 6, 7"), 0));
    //-> unsat

    g = new Gaussian(s, xs, 0);
    bool ret = g->init_until_fixedpoint();

    EXPECT_EQ(ret, false);
    EXPECT_EQ(s->ok, false);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

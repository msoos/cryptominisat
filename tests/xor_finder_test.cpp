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
#include "src/xorfinder.h"
#include "src/solverconf.h"
#include "src/occsimplifier.h"
using namespace CMSat;
#include "test_helper.h"

struct xor_finder : public ::testing::Test {
    xor_finder()
    {
        SolverConf conf;
        conf.doCache = false;
        s = new Solver(&conf, &must_inter);
        s->new_vars(20);
        occsimp = s->occsimplifier;
    }
    ~xor_finder()
    {
        delete s;
    }
    Solver* s = NULL;
    OccSimplifier* occsimp = NULL;
    bool must_inter = false;
};

TEST_F(xor_finder, find_none)
{
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    EXPECT_EQ(finder.xors.size(), 0);
}

TEST_F(xor_finder, find_tri_1)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("-1, -2, 3"));
    s->add_clause_outer(str_to_cl("-1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, -2, -3"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3 = 1");
}

TEST_F(xor_finder, find_tri_2)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("-1, -2, -3"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3 = 0");
}

TEST_F(xor_finder, find_tri_3)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("-1, -2, -3"));

    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("-1, -2, 3"));
    s->add_clause_outer(str_to_cl("-1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, -2, -3"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3 = 0; 1, 2, 3 = 1");
}

TEST_F(xor_finder, find_tri_4)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3 = 0");
}

TEST_F(xor_finder, find_tri_5)
{
    s->add_clause_outer(str_to_cl("-1, 2"));
    s->add_clause_outer(str_to_cl("1, -2, 3"));
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("-1, -2"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3 = 0");
}


TEST_F(xor_finder, find_4_1)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, 3, -4"));

    s->add_clause_outer(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3, 4 = 0;");
}

TEST_F(xor_finder, find_4_2)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    s->add_clause_outer(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3, 4 = 0;");
}

TEST_F(xor_finder, find_4_3)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    s->add_clause_outer(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outer(str_to_cl("-1, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3, 4 = 0;");
}

TEST_F(xor_finder, find_4_4)
{
    s->add_clause_outer(str_to_cl("-1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, -3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, 2,  -3, 4"));
    s->add_clause_outer(str_to_cl("-1, 2,  3, -4"));
    s->add_clause_outer(str_to_cl("1, -2,  3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, -3, -4"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3, 4 = 1");
}

TEST_F(xor_finder, find_4_5)
{
    s->add_clause_outer(str_to_cl("-1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, -3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, 2,  -3, 4"));
    s->add_clause_outer(str_to_cl("-1, 2,  3, -4"));
    s->add_clause_outer(str_to_cl("1, -2,  3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, -3, -4"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    s->add_clause_outer(str_to_cl("-1, 2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, -2, 3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, -3, 4"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    s->add_clause_outer(str_to_cl("1, -2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, 2, -3, -4"));
    s->add_clause_outer(str_to_cl("-1, -2, 3, -4"));
    s->add_clause_outer(str_to_cl("-1, -3, 4"));

    occsimp->setup();
    XorFinder finder(occsimp, s);
    finder.find_xors();
    check_xors_eq(finder.xors, "1, 2, 3, 4 = 1; 1, 2, 3, 4 = 0");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

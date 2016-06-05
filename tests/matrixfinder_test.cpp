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
#include "src/matrixfinder.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct gauss : public ::testing::Test {
    gauss()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        s->new_vars(40);
        s->conf.gaussconf.min_gauss_xor_clauses = 0;
        mf = new MatrixFinder(s);
    }
    ~gauss()
    {
        delete s;
    }

    Solver* s;
    MatrixFinder* mf = NULL;
    std::vector<uint32_t> vars;
    std::atomic<bool> must_inter;
    vector<Xor> xs;
};

TEST_F(gauss, min_rows)
{
    //s->conf.verbosity = 20;
    s->conf.gaussconf.min_matrix_rows = 2;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("1, 2, 3, 4"), 0));
    s->xorclauses = xs;

    mf->findMatrixes();

    EXPECT_EQ(s->gauss_matrixes.size(), 1);
}

TEST_F(gauss, min_rows_2)
{
    //s->conf.verbosity = 20;
    s->conf.gaussconf.min_matrix_rows = 3;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("1, 2, 3, 4"), 0));
    s->xorclauses = xs;

    mf->findMatrixes();

    EXPECT_EQ(s->gauss_matrixes.size(), 0);
}

TEST_F(gauss, separate_1)
{
    //s->conf.verbosity = 20;
    s->conf.gaussconf.min_matrix_rows = 1;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("5, 6, 7, 8"), 0));
    s->xorclauses = xs;

    mf->findMatrixes();

    EXPECT_EQ(s->gauss_matrixes.size(), 2);
}

TEST_F(gauss, separate_2)
{
    //s->conf.verbosity = 20;
    s->conf.gaussconf.min_matrix_rows = 1;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("4, 5, 6"), 0));
    xs.push_back(Xor(str_to_vars("3, 4, 10"), 0));

    xs.push_back(Xor(str_to_vars("15, 16, 17, 18"), 0));
    xs.push_back(Xor(str_to_vars("11, 15, 19"), 0));
    xs.push_back(Xor(str_to_vars("19, 20, 12"), 0));
    s->xorclauses = xs;

    mf->findMatrixes();

    EXPECT_EQ(s->gauss_matrixes.size(), 2);
}

TEST_F(gauss, separate_3)
{
    //s->conf.verbosity = 20;
    s->conf.gaussconf.min_matrix_rows = 1;
    xs.push_back(Xor(str_to_vars("1, 2, 3"), 0));
    xs.push_back(Xor(str_to_vars("4, 5, 6"), 0));
    xs.push_back(Xor(str_to_vars("3, 4, 10"), 0));

    xs.push_back(Xor(str_to_vars("15, 16, 17, 18"), 0));
    xs.push_back(Xor(str_to_vars("11, 15, 19"), 0));
    xs.push_back(Xor(str_to_vars("19, 20, 12"), 0));

    xs.push_back(Xor(str_to_vars("21, 22, 23, 29"), 0));
    xs.push_back(Xor(str_to_vars("21, 28, 29"), 0));
    xs.push_back(Xor(str_to_vars("25, 21, 27"), 0));
    s->xorclauses = xs;

    mf->findMatrixes();

    EXPECT_EQ(s->gauss_matrixes.size(), 3);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

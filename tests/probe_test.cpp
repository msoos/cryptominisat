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
#include "src/prober.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct probe_test : public ::testing::Test {
    probe_test()
    {
        must_inter = false;
        SolverConf conf;
        conf.doCache = false;
        s = new Solver(&conf, &must_inter);
        probe = s->prober;
    }
    ~probe_test()
    {
        delete s;
    }

    Solver* s;
    Prober* probe;
    bool must_inter;
};

TEST_F(probe_test, probe_one)
{
    s->new_vars(2);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    EXPECT_EQ(s->get_num_free_vars(), 2);
    probe->probe();
    EXPECT_EQ(s->get_num_free_vars(), 1);
}

TEST_F(probe_test, probe_two)
{
    s->new_vars(4);
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("3, -4"));
    s->add_clause_outer(str_to_cl("3, 4"));

    EXPECT_EQ(s->get_num_free_vars(), 4);
    probe->probe();
    EXPECT_EQ(s->get_num_free_vars(), 2);
}

TEST_F(probe_test, probe_fail)
{
    s->new_vars(4);
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-1, 2"));

    s->add_clause_outer(str_to_cl("3, -2"));
    s->add_clause_outer(str_to_cl("-3, -2"));

    EXPECT_EQ(s->okay(), true);
    probe->probe();
    EXPECT_EQ(s->okay(), false);
}

TEST_F(probe_test, probe_both)
{
    s->new_vars(6);
    s->add_clause_outer(str_to_cl("1, 2"));
    s->add_clause_outer(str_to_cl("-2, 3"));

    s->add_clause_outer(str_to_cl("-1, 6"));
    s->add_clause_outer(str_to_cl("-6, 3"));

    EXPECT_EQ(s->get_num_free_vars(), 6);
    probe->probe();
    EXPECT_EQ(s->get_num_free_vars(), 5);
    check_set_lits(s, "3");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

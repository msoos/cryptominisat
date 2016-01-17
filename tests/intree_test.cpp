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
#include "src/intree.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct intree : public ::testing::Test {
    intree()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
        inp = s->intree;
    }
    ~intree()
    {
        delete s;
    }

    Solver* s;
    InTree* inp;
    std::atomic<bool> must_inter;
};


//Fails

TEST_F(intree, fail_1)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2, -3"));

    inp->intree_probe();
    check_zero_assigned_lits_contains(s, "-2");
}


TEST_F(intree, fail_2)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2,  4"));
    s->add_clause_outer(str_to_cl("-2,  5"));
    s->add_clause_outer(str_to_cl("-3, -4, -5, 1"));

    inp->intree_probe();
    check_zero_assigned_lits_contains(s, "1");
}

TEST_F(intree, fail_3)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2,  4"));
    s->add_clause_outer(str_to_cl("-2,  5"));
    s->add_clause_outer(str_to_cl("-3, -4, -5, 6"));
    s->add_clause_outer(str_to_cl("-4, -5, -6"));

    inp->intree_probe();
    check_zero_assigned_lits_contains(s, "1");
}

//Hyper-binary resolution

TEST_F(intree, hyper_bin_1)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2,  4"));
    s->add_clause_outer(str_to_cl("-3, -4, 5"));

    inp->intree_probe();
    check_red_cls_contains(s, "-2, 5");
}

TEST_F(intree, hyper_bin_2)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2,  4"));
    s->add_clause_outer(str_to_cl("-3, -4, 5"));
    s->add_clause_outer(str_to_cl("-5,  6"));
    s->add_clause_outer(str_to_cl("-5,  7"));
    s->add_clause_outer(str_to_cl("-6, -7, 8"));

    inp->intree_probe();
    check_red_cls_contains(s, "-2, 5");
    check_red_cls_contains(s, "-5, 8");
}

//Transitive reduction

TEST_F(intree, trans_red_1)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2,  4"));
    s->add_clause_outer(str_to_cl("-3, -4, 5"));
    s->add_clause_outer(str_to_cl("1, 5"), false);

    inp->intree_probe();
    check_red_cls_doesnt_contain(s, "1, 5");
}

TEST_F(intree, trans_red_2)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1, 2"));
    s->add_clause_outer(str_to_cl("-2, 3"));
    s->add_clause_outer(str_to_cl("-3, 4"));
    s->add_clause_outer(str_to_cl("-4, 5"));
    s->add_clause_outer(str_to_cl("1, 5"));

    inp->intree_probe();
    check_irred_cls_doesnt_contain(s, "1, 5");
}

//Transitive reduction and hyper-binary resolution
TEST_F(intree, trans_red_and_hyper_bin_1)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1, 2"));
    s->add_clause_outer(str_to_cl("-2, 3"));
    s->add_clause_outer(str_to_cl("-3, 4"));
    s->add_clause_outer(str_to_cl("-4, 5"));
    s->add_clause_outer(str_to_cl("-3, -4, -5, 6"));
    s->add_clause_outer(str_to_cl("1, 6"), false);

    inp->intree_probe();
    check_red_cls_doesnt_contain(s, "1, 6");
    check_red_cls_contains(s, "-3, 6");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

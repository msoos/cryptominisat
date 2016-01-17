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
#include "src/distillerallwithall.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct distill_test : public ::testing::Test {
    distill_test()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        distill_all_with_all = s->distill_all_with_all;
    }
    ~distill_test()
    {
        delete s;
    }

    Solver* s;
    DistillerAllWithAll* distill_all_with_all;
    std::atomic<bool> must_inter;
};

//BY-BY 1

TEST_F(distill_test, long_by1)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(1);
    check_irred_cls_contains(s, "1, 3, 4");
}

TEST_F(distill_test, long_by1_transitive)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, -5"));
    s->add_clause_outer(str_to_cl("5, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(1);
    check_irred_cls_contains(s, "1, 3, 4");
}

TEST_F(distill_test, long_by1_2)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(1);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, long_by1_3)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(1);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, long_by1_nodistill
)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(1);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

TEST_F(distill_test, long_by1_nodistill2
)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(1);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

//BY-BY 2

TEST_F(distill_test, long_by2)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(2);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, long_by2_transitive)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2, -5"));
    s->add_clause_outer(str_to_cl("5, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(2);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, long_by2_2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("2, 3, 4, -5"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4, 5"));

    distill_all_with_all->distill(2);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

TEST_F(distill_test, long_by2_nodistill
)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distill_all_with_all->distill(2);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

//Tri -- always 2-by-2

TEST_F(distill_test, tri)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    distill_all_with_all->distill();
    check_irred_cls_contains(s, "1, 2");
}

TEST_F(distill_test, tri_transitive)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2, 4"));
    s->add_clause_outer(str_to_cl("-4, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    distill_all_with_all->distill();
    check_irred_cls_contains(s, "1, 2");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

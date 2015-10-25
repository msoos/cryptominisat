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
#include "src/distiller.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct distill_test : public ::testing::Test {
    distill_test()
    {
        must_inter = false;
        SolverConf conf;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        distiller = s->distiller;
    }
    ~distill_test()
    {
        delete s;
    }

    Solver* s;
    Distiller* distiller;
    bool must_inter;
};

//BY-BY 1

TEST_F(distill_test, distill_long_by1)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distiller->distill(1);
    check_irred_cls_contains(s, "1, 3, 4");
}

TEST_F(distill_test, distill_long_by1_2)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distiller->distill(1);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, distill_long_by1_3)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distiller->distill(1);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, distill_long_by1_nodistill
)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("-1, 3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distiller->distill(1);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

//BY-BY 2

TEST_F(distill_test, distill_long_by2)
{
    s->new_vars(4);
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distiller->distill(2);
    check_irred_cls_contains(s, "1, 2, 4");
}

TEST_F(distill_test, distill_long_by2_2)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("2, 3, 4, -5"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4, 5"));

    distiller->distill(2);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

TEST_F(distill_test, distill_long_by2_nodistill
)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, 2, 3, 4"));

    distiller->distill(2);
    check_irred_cls_contains(s, "1, 2, 3, 4");
}

//BY-BY 1, tri

TEST_F(distill_test, distill_tri)
{
    s->new_vars(5);
    s->add_clause_outer(str_to_cl("1, 2, -3"));
    s->add_clause_outer(str_to_cl("1, 2, 3"));

    distiller->distill();
    check_irred_cls_contains(s, "1, 2");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

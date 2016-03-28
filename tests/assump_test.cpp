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

#include "cryptominisat4/cryptominisat.h"
#include "src/solverconf.h"
#include <vector>
using std::vector;
using namespace CMSat;

struct assump_interf : public ::testing::Test {
    assump_interf()
    {
        SolverConf conf;
        s = new SATSolver(&conf);
    }
    ~assump_interf()
    {
        delete s;
    }
    SATSolver* s = NULL;
    vector<Lit> assumps;
};

TEST_F(assump_interf, empty)
{
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s->okay(), true);
}

TEST_F(assump_interf, single_true)
{
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false)});
    assumps.push_back(Lit(0, false));
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s->okay(), true);
}

TEST_F(assump_interf, single_false)
{
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false)});
    assumps.push_back(Lit(0, true));
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s->get_conflict().size(), 1);
    EXPECT_EQ( s->get_conflict()[0], Lit(0, false));
}


TEST_F(assump_interf, single_false_then_true)
{
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false)});
    assumps.push_back(Lit(0, true));
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s->okay(), true);

    ret = s->solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s->okay(), true);

}

TEST_F(assump_interf, binclause_true)
{
    s->new_var();
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    assumps.push_back(Lit(0, true));
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_True );
    EXPECT_EQ( s->get_model()[0], l_False );
    EXPECT_EQ( s->get_model()[1], l_True );
}

TEST_F(assump_interf, binclause_false)
{
    s->new_var();
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s->get_conflict().size(), 2);

    vector<Lit> tmp = s->get_conflict();
    std::sort(tmp.begin(), tmp.end());
    EXPECT_EQ( tmp[0], Lit(0, false) );
    EXPECT_EQ( tmp[1], Lit(1, false) );
}

TEST_F(assump_interf, replace_true)
{
    s->new_var();
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, true)});
    s->add_clause(vector<Lit>{Lit(0, true), Lit(1, false)});
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s->get_model()[0], l_False );
    EXPECT_EQ( s->get_model()[1], l_False );
}

TEST_F(assump_interf, replace_false)
{
    s->new_var();
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, true)}); //a V -b
    s->add_clause(vector<Lit>{Lit(0, true), Lit(1, false)}); //-a V b
    //a == b

    assumps.push_back(Lit(0, false));
    assumps.push_back(Lit(1, true));
    //a = 1, b = 0

    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s->okay(), true);

    EXPECT_EQ( s->get_conflict().size(), 2);

    vector<Lit> tmp = s->get_conflict();
    std::sort(tmp.begin(), tmp.end());
    EXPECT_EQ( tmp[0], Lit(0, true) );
    EXPECT_EQ( tmp[1], Lit(1, false) );
}

TEST_F(assump_interf, set_var_by_prop)
{
    s->new_var();
    s->new_var();
    s->add_clause(vector<Lit>{Lit(0, false)}); //a = 1
    s->add_clause(vector<Lit>{Lit(0, true), Lit(1, false)}); //-a V b
    //-> b = 1

    assumps.push_back(Lit(1, true));
    //b = 0

    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s->okay(), true);

    EXPECT_EQ( s->get_conflict().size(), 1);

    vector<Lit> tmp = s->get_conflict();
    EXPECT_EQ( tmp[0], Lit(1, false) );
}

TEST_F(assump_interf, only_assump)
{
    s->new_var();
    s->new_var();

    assumps.push_back(Lit(1, true));
    assumps.push_back(Lit(1, false));

    lbool ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s->okay(), true);
    EXPECT_EQ( s->get_conflict().size(), 2);

    vector<Lit> tmp = s->get_conflict();
    std::sort(tmp.begin(), tmp.end());
    EXPECT_EQ( tmp[0] , Lit(1, false) );
    EXPECT_EQ( tmp[1], Lit(1, true) );

    ret = s->solve(NULL);
    EXPECT_EQ( ret, l_True );

    ret = s->solve(&assumps);
    EXPECT_EQ( ret, l_False);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

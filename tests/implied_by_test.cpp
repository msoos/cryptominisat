/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "gtest/gtest.h"

#include <fstream>
#include <algorithm>

#include "cryptominisat5/cryptominisat.h"
#include "src/solverconf.h"
#include "test_helper.h"
using namespace CMSat;
#include <vector>
using std::vector;

struct impliedby : public ::testing::Test {
    impliedby()
    {

        SolverConf conf;
        //conf.verbosity = 20;
        s = new SATSolver(&conf);
        s->set_bva(0);
        s->set_no_bve();
        s->new_vars(30);
    }
    ~impliedby()
    {
        delete s;
    }

    SATSolver* s;
    vector<Lit> lits;
    vector<Lit> out_implied_by;
};


TEST_F(impliedby, noprop)
{
    s->add_clause(str_to_cl("1"));
    lits = str_to_cl("1");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(0, out_implied_by.size());
}

TEST_F(impliedby, prop_1)
{
    s->add_clause(str_to_cl("1, 2"));
    lits = str_to_cl("-1");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(str_to_cl("-1, 2"), out_implied_by);
}

TEST_F(impliedby, replace_prop_1)
{
    s->add_clause(str_to_cl("1, 2"));
    s->add_clause(str_to_cl("-1, -2"));
    s->simplify();
    lits = str_to_cl("-1");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    std::sort(out_implied_by.begin(), out_implied_by.end());
    EXPECT_EQ(str_to_cl("-1, 2"), out_implied_by);
}

TEST_F(impliedby, replace_prop_2)
{
    s->add_clause(str_to_cl("1, 2"));
    s->add_clause(str_to_cl("-1, -2"));
    s->simplify();
    lits = str_to_cl("-2");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    std::sort(out_implied_by.begin(), out_implied_by.end());
    EXPECT_EQ(str_to_cl("1, -2"), out_implied_by);
}

TEST_F(impliedby, ret_false_1)
{
    s->add_clause(str_to_cl("1, 2"));
    s->add_clause(str_to_cl("-1, -2"));
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("-1, 2"));
    s->simplify();
    lits = str_to_cl("3");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(false, ret);
}

TEST_F(impliedby, ret_false_2)
{
    s->add_clause(str_to_cl("1"));
    s->simplify();
    lits = str_to_cl("-1");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(false, ret);
}

TEST_F(impliedby, ret_false_3)
{
    s->add_clause(str_to_cl("1, -2"));
    s->simplify();
    lits = str_to_cl("-1, 2");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(false, ret);
}

TEST_F(impliedby, ret_false_4)
{
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("-1"));
    s->simplify();
    lits = str_to_cl("2");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(false, ret);
}

TEST_F(impliedby, no_imply)
{
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("-1"));
    s->simplify();
    lits = str_to_cl("-1");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(0, out_implied_by.size());
}

TEST_F(impliedby, no_imply_2)
{
    s->add_clause(str_to_cl("-1"));
    s->add_clause(str_to_cl("1, -2"));
    s->simplify();
    lits = str_to_cl("-2");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(0, out_implied_by.size());
}

TEST_F(impliedby, imply_3)
{
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("2, -3"));
    s->simplify();
    lits = str_to_cl("-1");
    bool ret = s->implied_by(lits, out_implied_by);
    EXPECT_EQ(true, ret);
    std::sort(out_implied_by.begin(), out_implied_by.end());
    EXPECT_EQ(str_to_cl("-1, -2, -3"), out_implied_by);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

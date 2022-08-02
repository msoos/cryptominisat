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

#include "cryptominisat5/cryptominisat.h"
#include "test_helper.h"

using namespace CMSat;

struct definability : public ::testing::Test {
    definability()
    {
        SolverConf conf;
        s = new SATSolver;
        s->new_vars(50);
        for(uint32_t i = 0; i < 50; i++) {
            vs.push_back(i);
        }
    }
    ~definability()
    {
        delete s;
    }
    SATSolver* s = NULL;
    vector<uint32_t> vs;
};

TEST_F(definability, none_found)
{
    s->add_clause(str_to_cl("1, 2, 3"));
    s->add_clause(str_to_cl("1, -2, 4"));

    auto defin = s->get_recovered_or_gates();
    EXPECT_EQ(0, defin.size());

}

TEST_F(definability, do_1)
{
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("1, -3"));
    s->add_clause(str_to_cl("2, 3, -1"));

    auto defin = s->get_recovered_or_gates();
    EXPECT_EQ(1, defin.size());
    EXPECT_EQ(0, defin[0].rhs.var());
}

TEST_F(definability, do_1_inv)
{
    s->add_clause(str_to_cl("-1, -2"));
    s->add_clause(str_to_cl("-1, -3"));
    s->add_clause(str_to_cl("2, 3, 1"));

    auto defin = s->get_recovered_or_gates();
    EXPECT_EQ(1, defin.size());
    EXPECT_EQ(0, defin[0].rhs.var());
}

TEST_F(definability, do_2)
{
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("1, -3"));
    s->add_clause(str_to_cl("2, 3, -1"));

    s->add_clause(str_to_cl("11, -12"));
    s->add_clause(str_to_cl("11, -13"));
    s->add_clause(str_to_cl("12, 13, -11"));

    auto defin = s->get_recovered_or_gates();
    EXPECT_EQ(2, defin.size());

    if (defin[0].rhs.var() > defin[1].rhs.var()) {
        std::swap(defin[0], defin[1]);
    }
    EXPECT_EQ(0, defin[0].rhs.var());
    EXPECT_EQ(10, defin[1].rhs.var());
}

TEST_F(definability, circular)
{
    //2 and 3 define 1
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("1, -3"));
    s->add_clause(str_to_cl("2, 3, -1"));

    s->add_clause(str_to_cl("3, -1"));
    s->add_clause(str_to_cl("3, -4"));
    s->add_clause(str_to_cl("4, 1, -3"));

    auto defin = s->get_recovered_or_gates();
    EXPECT_EQ(2, defin.size());
}

TEST_F(definability, circular_and_normal)
{
    //2 and 3 define 1
    s->add_clause(str_to_cl("1, -2"));
    s->add_clause(str_to_cl("1, -3"));
    s->add_clause(str_to_cl("2, 3, -1"));


    s->add_clause(str_to_cl("3, -1"));
    s->add_clause(str_to_cl("3, -4"));
    s->add_clause(str_to_cl("4, 1, -3"));

    //12 and 13 define 11
    s->add_clause(str_to_cl("11, -12"));
    s->add_clause(str_to_cl("11, -13"));
    s->add_clause(str_to_cl("12, 13, -11"));

    auto defin = s->get_recovered_or_gates();
    EXPECT_EQ(3, defin.size());

    //The non-circular one must be found
    bool found = false;
    for(const auto& v: defin) {
        if (v.rhs.var() == 10) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

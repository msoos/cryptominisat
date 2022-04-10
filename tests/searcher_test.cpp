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

#include <set>
using std::set;

#include "src/solver.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

namespace CMSat {
struct SearcherTest : public ::testing::Test {
    SearcherTest()
    {
        must_inter.store(false, std::memory_order_relaxed);
    }
    ~SearcherTest()
    {
        delete s;
    }

    void set_var_polar(uint32_t var, bool polarity)
    {
        s->new_decision_level();
        //it must be inverted to set
        s->enqueue<false>(Lit(var, !polarity));
        s->cancelUntil(0);
    }

    SolverConf conf;
    Solver* s = NULL;
    Searcher* ss = NULL;
    std::vector<uint32_t> vars;
    std::atomic<bool> must_inter;
};

//Regular, 1UIP fails

TEST_F(SearcherTest, pickpolar_rnd)
{
    conf.polarity_mode = PolarityMode::polarmode_rnd;
    s = new Solver(&conf, &must_inter);
    s->new_vars(30);
    ss = (Searcher*)s;

    s->add_clause_outside(str_to_cl(" 1,  2"));

    uint32_t num = 0;
    for(uint32_t i = 0 ; i < 1000000; i++)
        num += (unsigned)ss->pick_polarity(0);

    //Not far off from avg
    ASSERT_GT(num, 400000U);
    ASSERT_LT(num, 600000U);
}

TEST_F(SearcherTest, pickpolar_pos)
{
    conf.polarity_mode = PolarityMode::polarmode_pos;
    s = new Solver(&conf, &must_inter);
    s->new_vars(30);
    ss = (Searcher*)s;
    s->add_clause_outside(str_to_cl(" 1,  2"));

    uint32_t num = 0;
    for(uint32_t i = 0 ; i < 100000; i++)
        num += (unsigned)ss->pick_polarity(0);

    ASSERT_EQ(num, 100000U);
}

TEST_F(SearcherTest, pickpolar_neg)
{
    conf.polarity_mode = PolarityMode::polarmode_neg;
    s = new Solver(&conf, &must_inter);
    s->new_vars(30);
    ss = (Searcher*)s;
    s->add_clause_outside(str_to_cl(" 1,  2"));

    uint32_t num = 0;
    for(uint32_t i = 0 ; i < 100000; i++)
        num += (unsigned)ss->pick_polarity(0);

    ASSERT_EQ(num, 0U);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

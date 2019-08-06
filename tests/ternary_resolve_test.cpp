/******************************************
Copyright (c) 2016, Mate Soos

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

#include "src/solver.h"
#include "src/xorfinder.h"
#include "src/solverconf.h"
#include "src/occsimplifier.h"
#include "test_helper.h"

using namespace CMSat;

struct ternary_resolv : public ::testing::Test {
    ternary_resolv()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        s = new Solver(&conf, &must_inter);
        s->new_vars(50);
        occsimp = s->occsimplifier;
    }
    ~ternary_resolv()
    {
        delete s;
    }
    Solver* s = NULL;
    OccSimplifier* occsimp = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(ternary_resolv, do_1)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 4"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    check_red_cls_contains(s, "1, 3, 4");
}

TEST_F(ternary_resolv, do_2)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 4"));
    s->add_clause_outer(str_to_cl("2, -3, 5"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    check_red_cls_contains(s, "1, 3, 4");
    check_red_cls_contains(s, "1, 2, 5");
}

TEST_F(ternary_resolv, do_2_v2)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 4"));


    s->add_clause_outer(str_to_cl("10, 20, 30"));
    s->add_clause_outer(str_to_cl("10, -20, 40"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    check_red_cls_contains(s, "1, 3, 4");
    check_red_cls_contains(s, "10, 30, 40");
}

TEST_F(ternary_resolv, do_1_v2)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3"));
    s->add_clause_outer(str_to_cl("-1, -2, 4"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    check_red_cls_contains(s, "-1, 3, 4");
}

TEST_F(ternary_resolv, do_1_v3)
{
    s->add_clause_outer(str_to_cl("-1, 2, 3"));
    s->add_clause_outer(str_to_cl("-1, -2, -4"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    check_red_cls_contains(s, "-1, 3, -4");
}

TEST_F(ternary_resolv, do_1_v4)
{
    s->add_clause_outer(str_to_cl("-1, 2, -3"));
    s->add_clause_outer(str_to_cl("-1, -2, -4"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    check_red_cls_contains(s, "-1, -3, -4");
}

TEST_F(ternary_resolv, only_one_v1)
{
    s->add_clause_outer(str_to_cl("1, 2, 3"));
    s->add_clause_outer(str_to_cl("1, -2, 4"));

    occsimp->setup();
    occsimp->ternary_res();
    occsimp->ternary_res();
    occsimp->ternary_res();
    occsimp->finishUp(0);
    EXPECT_EQ(get_num_red_cls_contains(s, "1, 3, 4"), 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

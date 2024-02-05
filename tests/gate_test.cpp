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

#include "src/solver.h"
#include "src/xorfinder.h"
#include "src/solverconf.h"
#include "src/occsimplifier.h"
#include "test_helper.h"

using namespace CMSat;

struct gate_test : public ::testing::Test {
    gate_test() {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        s = new Solver(&conf, &must_inter);
        s->new_vars(50);
        occsimp = s->occsimplifier;
    }
    ~gate_test() { delete s; }
    Solver* s = NULL;
    OccSimplifier* occsimp = NULL;
    std::atomic<bool> must_inter;
};

TEST_F(gate_test, do_1)
{
    s->add_clause_outside(str_to_cl("1, -2"));
    s->add_clause_outside(str_to_cl("1, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));
    s->add_clause_outside(str_to_cl("2, 3, 4, 5"));

    occsimp->setup();
    occsimp->lit_rem_with_or_gates();
    occsimp->finish_up(0);
    check_irred_cls_contains(s, "1, 4, 5");
}

TEST_F(gate_test, do_2)
{
    s->add_clause_outside(str_to_cl("1, -2"));
    s->add_clause_outside(str_to_cl("1, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));
    s->add_clause_outside(str_to_cl("2, 3, 4, 5"));
    s->add_clause_outside(str_to_cl("2, 3, 7, 6"));

    occsimp->setup();
    occsimp->lit_rem_with_or_gates();
    occsimp->finish_up(0);
    check_irred_cls_contains(s, "1, 4, 5");
    check_irred_cls_contains(s, "1, 7, 6");
}

TEST_F(gate_test, do_3)
{
    s->add_clause_outside(str_to_cl("1, -2"));
    s->add_clause_outside(str_to_cl("1, -3"));
    s->add_clause_outside(str_to_cl("-1, 2, 3"));
    s->add_clause_outside(str_to_cl("2, 3, 4, 5"));
    s->add_clause_outside(str_to_cl("2, 3, 1, 6"));

    occsimp->setup();
    occsimp->lit_rem_with_or_gates();
    occsimp->finish_up(0);
    check_irred_cls_contains(s, "1, 4, 5");
    check_irred_cls_contains(s, "1, 6");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

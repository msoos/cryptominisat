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

#include <set>
using std::set;

#include "src/solver.h"
#include "src/prober.h"
#include "src/solverconf.h"
using namespace CMSat;
#include "test_helper.h"

struct probe : public ::testing::Test {
    probe()
    {
        must_inter.store(false, std::memory_order_relaxed);
        SolverConf conf;
        conf.doProbe = true;
        conf.otfHyperbin = true;
        //conf.verbosity = 20;
        s = new Solver(&conf, &must_inter);
        s->new_vars(30);
        p = s->prober;
    }
    ~probe()
    {
        delete s;
    }

    Solver* s;
    Prober* p;
    std::vector<uint32_t> vars;
    std::atomic<bool> must_inter;
};

//Regular, 1UIP fails

TEST_F(probe, uip_fail_1)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl(" 1,  2"));
    s->add_clause_outer(str_to_cl("-2,  3"));
    s->add_clause_outer(str_to_cl("-2, -3"));

    s->conf.doBothProp = false;
    s->conf.otfHyperbin = false;
    vars = str_to_vars("1");
    p->probe(&vars);

    //1UIP is -2
    check_zero_assigned_lits_contains(s, "-2");
}

TEST_F(probe, uip_fail_2)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, -3"));
    s->add_clause_outer(str_to_cl("1, -4"));
    s->add_clause_outer(str_to_cl("1, -5"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5, 6"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5, -6"));

    s->conf.doBothProp = false;
    s->conf.otfHyperbin = false;
    vars = str_to_vars("1");
    p->probe(&vars);

    //First UIP is 1
    check_zero_assigned_lits_eq(s, "1");
}

//BFS, DFS fails

TEST_F(probe, fail_dfs)
{
    //s->conf.verbosity = 20;
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, -3"));
    s->add_clause_outer(str_to_cl("1, -4"));
    s->add_clause_outer(str_to_cl("1, -5"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5, 6"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5, -6"));

    s->conf.doBothProp = false;
    s->conf.otfHyperbin = true;
    vars = str_to_vars("1");
    p->probe(&vars);

    //deepest common ancestor
    check_zero_assigned_lits_eq(s, "1");
}

TEST_F(probe, fail_bfs)
{
    //s->conf.verbosity = 10;
    s->add_clause_outer(str_to_cl("1, -2"));
    s->add_clause_outer(str_to_cl("1, -3"));
    s->add_clause_outer(str_to_cl("1, -4"));
    s->add_clause_outer(str_to_cl("1, -5"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5, 6"));
    s->add_clause_outer(str_to_cl("2, 3, 4, 5, -6"));

    s->conf.doBothProp = false;
    s->conf.otfHyperbin = true;
    vars = str_to_vars("1");
    p->probe(&vars);

    //deepest common ancestor
    check_zero_assigned_lits_eq(s, "1");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

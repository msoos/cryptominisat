/******************************************
Copyright (c) 2018, Mate Soos <soos.mate@gmail.com>

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

#include "time_mem.h"
#include <limits>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "constants.h"
#include "yalsat.h"
#include "solver.h"
#include "sqlstats.h"
extern "C" {
#include "yals.h"
}
//#define SLOW_DEBUG

using namespace CMSat;

Yalsat::Yalsat(Solver* _solver) :
    solver(_solver)
{
    yals = yals_new();
    if (solver->conf.verbosity) {
        yals_setopt (yals, "verbose", 1);
    } else {
        yals_setopt (yals, "verbose", 0);
    }
    //yals_setprefix (yals, "c 00 ");
}

Yalsat::~Yalsat()
{
    yals_del(yals);
}

lbool Yalsat::main()
{
    //It might not work well with few number of variables
    //rnovelty could also die/exit(-1), etc.
    if (solver->nVars() < 50) {
        if (solver->conf.verbosity) {
            cout << "c [walksat] too few variables for walksat"
            << endl;
        }
        return l_Undef;
    }
    double startTime = cpuTime();

    if (!init_problem()) {
        //it's actually l_False under assumptions
        //but we'll set the real SAT solver deal with that
        if (solver->conf.verbosity) {
            cout << "c [walksat] problem UNSAT under assumptions, returning to main solver"
            << endl;
        }
        return l_Undef;
    }
    //yals_setflipslimit(yals, 5*1000*1000);
    uint64_t mils = solver->conf.yalsat_max_mems*solver->conf.global_timeout_multiplier;
    if (solver->conf.verbosity) {
        cout << "c [yalsat] mems limit M: " << mils << endl;
    }
    yals_setmemslimit(yals, mils*1000*1000);
    yals_srand(yals, solver->mtrand.randInt() % 1000);
    for(int i = 0; i < (int)solver->nVars(); i++) {
        int v = i+1;
        if (solver->value(i) != l_Undef) {
            if (solver->value(i) == l_False) {
                v *= -1;
            }
        } else {
            if (!solver->varData[i].polarity) {
                v *= -1;
            }
        }
        yals_setphase(yals, v);
    }
    //yals_srand(yals, 0);
    //yals_setopt (yals, "hitlim", 5*1000*1000); //every time the minimum (or lower) is hit
    //yals_setopt (yals, "hitlim", 50000000);

    int res = yals_sat(yals);
    lbool ret = deal_with_solution(res);

    double time_used = cpuTime()-startTime;
    if (solver->conf.verbosity) {
        cout << "c [yalsat] time: " << time_used << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "sls-yalsat"
            , time_used
        );
    }
    return ret;
}

template<class T>
Yalsat::add_cl_ret Yalsat::add_this_clause(const T& cl)
{
    uint32_t sz = 0;
    bool sat = false;
    yals_lits.clear();
    for(size_t i3 = 0; i3 < cl.size(); i3++) {
        Lit lit = cl[i3];
        assert(solver->varData[lit.var()].removed == Removed::none);
        lbool val = l_Undef;
        if (solver->value(lit) != l_Undef) {
            val = solver->value(lit);
        } else {
            val = solver->lit_inside_assumptions(lit);
        }

        if (val == l_True) {
            //clause is SAT, skip!
            sat = true;
            continue;
        } else if (val == l_False) {
            continue;
        }
        int l = lit.var()+1;
        l *= lit.sign() ? -1 : 1;
        yals_lits.push_back(l);
        sz++;
    }
    if (sat) {
        return add_cl_ret::skipped_cl;
    }
    if (sz == 0) {
        //it's unsat because of assumptions
        if (solver->conf.verbosity) {
            cout << "c [walksat] UNSAT because of assumptions in clause: " << cl << endl;
        }
        return add_cl_ret::unsat;
    }

    for(int i: yals_lits) {
        yals_add(yals, i);
    }
    yals_add(yals, 0);
    yals_lits.clear();

    return add_cl_ret::added_cl;
}

bool Yalsat::init_problem()
{
    if (solver->check_assumptions_contradict_foced_assignment())
    {
        return false;
    }
    #ifdef SLOWDEBUG
    solver->check_stats();
    #endif

    //where all clauses' literals are
    vector<Lit> this_clause;
    for(size_t i2 = 0; i2 < solver->nVars()*2; i2++) {
        Lit lit = Lit::toLit(i2);
        for(const Watched& w: solver->watches[lit]) {
            if (w.isBin() && !w.red() && lit < w.lit2()) {
                this_clause.clear();
                this_clause.push_back(lit);
                this_clause.push_back(w.lit2());

                if (add_this_clause(this_clause) == add_cl_ret::unsat) {
                    return false;
                }
            }
        }
    }
    for(ClOffset offs: solver->longIrredCls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->getRemoved());

        if (add_this_clause(*cl) == add_cl_ret::unsat) {
            return false;
        }
    }

    return true;
}

lbool Yalsat::deal_with_solution(int res)
{
    if (res == 20) {
        if (solver->conf.verbosity) {
            cout << "c [yalsat] says UNSAT -- strange" << endl;
        }
        return l_Undef;
    }

    if (solver->conf.sls_get_phase || res == 10) {
        if (solver->conf.verbosity) {
            cout << "c [yalsat] saving best assignment phase -- it had " << yals_minimum(yals) << " clauses unsatisfied" << endl;
        }

        for(size_t i = 0; i < solver->nVars(); i++) {
            solver->varData[i].polarity = (yals_deref(yals, i+1) >= 0);
        }
    }

    if (res != 10) {
        if (solver->conf.verbosity >= 2) {
            cout << "c [yalsat] ASSIGNMENT NOT FOUND" << endl;
        }
        return l_Undef;
    }

    if (solver->conf.verbosity) {
        cout << "c [yalsat] ASSIGNMENT FOUND" << endl;
    }

    return l_Undef;
}

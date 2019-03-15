/******************************************
Copyright (c) 2018, Henry Kautz <henry.kautz@gmail.com>
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
#include "walksat_yalsat.h"
#include "solver.h"
extern "C" {
#include "yals.h"
}
//#define SLOW_DEBUG

using namespace CMSat;

WalkSATyalsat::WalkSATyalsat(Solver* _solver) :
    solver(_solver)
{
    yals = yals_new();
}

WalkSATyalsat::~WalkSATyalsat()
{
    yals_del(yals);
}

uint64_t WalkSATyalsat::mem_needed()
{
    numvars = solver->nVars();
    numclauses = solver->longIrredCls.size() + solver->binTri.irredBins;
    numliterals = solver->litStats.irredLits;
    uint64_t needed = 0;

    //LIT storage (all clause data)
    needed += (solver->litStats.irredLits+solver->binTri.irredBins*2)*sizeof(Lit);

    //NOTE: this is underreporting here, but by VERY little
    //best -> longestclause = ??
    //needed += sizeof(uint32_t) * longestclause;

    //clause
    needed += sizeof(Lit *) * numclauses;
    //clsize
    needed += sizeof(uint32_t) * numclauses;

    //false_cls
    needed += sizeof(uint32_t) * numclauses;
    //map_cl_to_false_cls
    needed += sizeof(uint32_t) * numclauses;
    //numtruelit
    needed += sizeof(uint32_t) * numclauses;

    //occurrence
    needed += sizeof(uint32_t *) * (2 * numvars);
    //numoccurrence
    needed += sizeof(uint32_t) * (2 * numvars);
    //assigns
    needed += sizeof(lbool) * numvars;
    //breakcount
    needed += sizeof(uint32_t) * numvars;
    //makecount
    needed += sizeof(uint32_t) * numvars;

    //occur_list_alloc
    needed += sizeof(uint32_t) * numliterals;


    return needed;
}

lbool WalkSATyalsat::main()
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
    if (solver->conf.verbosity) {
        yals_setopt (yals, "verbose", 1);
    }
    //yals_setflipslimit(yals, 5*1000*1000);
    yals_setmemslimit(yals, 150*1000*1000);
    //yals_setopt (yals, "hitlim", 5*1000*1000); //every time the minimum (or lower) is hit
    //yals_setopt (yals, "hitlim", 50000000);

    int res = yals_sat(yals);
    lbool ret = deal_with_solution(res);

    yals_stats(yals);
    if (solver->conf.verbosity) {
        cout << "c [yals] time: " << (cpuTime()-startTime) << endl;
    }
    return ret;
}

template<class T>
WalkSATyalsat::add_cl_ret WalkSATyalsat::add_this_clause(const T& cl)
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

bool WalkSATyalsat::init_problem()
{
    if (solver->check_assumptions_contradict_foced_assignement())
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

lbool WalkSATyalsat::deal_with_solution(int res)
{
    if (res == 20) {
        if (solver->conf.verbosity) {
            cout << "c [yalsat] says it's un-sat-is-fiable -- strange" << endl;
        }
        return l_Undef;
    }

    if (res != 10) {
        if (solver->conf.verbosity) {
            cout << "c [yalsat] ASSIGNMENT NOT FOUND" << endl;
        }
        return l_Undef;
    }

    if (solver->conf.verbosity) {
        cout << "c [yalsat] ASSIGNMENT FOUND" << endl;
    }

    //int lit = (yals_deref (yals, i) > 0) ? i : -i;
    assert(solver->decisionLevel() == 0);
    for(size_t i = 0; i < solver->nVars(); i++) {
        //this will get set automatically anyway, skip
        if (solver->varData[i].removed != Removed::none) {
            continue;
        }
        if (solver->value(i) != l_Undef) {
            //this variable has been removed already
            //so whatever value it sets, it doesn't matter
            //the solution is still correct
            continue;
        }

        int pre_val = yals_deref (yals, i+1);
        lbool val = pre_val < 0 ? l_False : l_True;

        //fix these up, they may have been flipped
        if (solver->var_inside_assumptions(i) != l_Undef) {
            val = solver->var_inside_assumptions(i);
        }

        solver->new_decision_level();
        solver->enqueue(Lit(i, val == l_False));
    }
    #ifdef SLOW_DEBUG
    solver->check_assigns_for_assumptions();
    #endif

    return l_True;
}

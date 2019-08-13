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
#include "ccnr_cms.h"
#include "solver.h"
#include "ccnr.h"
//#define SLOW_DEBUG

using namespace CMSat;

CMS_ccnr::CMS_ccnr(Solver* _solver) :
    solver(_solver)
{
    ls_s = new CCNR::ls_solver;
}

CMS_ccnr::~CMS_ccnr()
{
    delete ls_s;
}

lbool CMS_ccnr::main()
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

    /*for(int i = 0; i < (int)solver->nVars(); i++) {
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
    }*/

    int res = ls_s->local_search();
    lbool ret = deal_with_solution(res);

    if (solver->conf.verbosity) {
        cout << "c [ccnr] time: " << (cpuTime()-startTime) << endl;
    }
    return ret;
}

template<class T>
CMS_ccnr::add_cl_ret CMS_ccnr::add_this_clause(const T& cl)
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

    for(auto& lit: yals_lits) {
        ls_s->_clauses[cl_num].literals.push_back(CCNR::lit(lit, cl_num));
    }
    cl_num++;

    return add_cl_ret::added_cl;
}

bool CMS_ccnr::init_problem()
{
    if (solver->check_assumptions_contradict_foced_assignement())
    {
        return false;
    }
    #ifdef SLOWDEBUG
    solver->check_stats();
    #endif

    ls_s->_num_vars = solver->nVars();
    ls_s->_num_clauses = solver->longIrredCls.size() + solver->binTri.irredBins;
    ls_s->make_space();

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

    assert(ls_s->_num_clauses == (int)cl_num);
    for (int c=0; c < ls_s->_num_clauses; c++) {
        for(CCNR::lit item: ls_s->_clauses[c].literals) {
            int v = item.var_num;
            ls_s->_vars[v].literals.push_back(item);
        }
    }
    ls_s->build_neighborhood();

    return true;
}

lbool CMS_ccnr::deal_with_solution(int res)
{
    if (!res) {
        if (solver->conf.verbosity) {
            cout << "c [ccnr] ASSIGNMENT NOT FOUND" << endl;
        }

        if (solver->conf.sls_get_phase) {
            if (solver->conf.verbosity) {
                cout
                << "c [ccnr] saving best assignement phase"
                << endl;
            }

            for(size_t i = 0; i < solver->nVars(); i++) {
                solver->varData[i].polarity = ls_s->_solution[i+1];
            }
        }

        return l_Undef;
    }

    if (solver->conf.verbosity) {
        cout << "c [ccnr] ASSIGNMENT FOUND" << endl;
    }

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

        lbool val = ls_s->_solution[i+1] ? l_True : l_False;

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

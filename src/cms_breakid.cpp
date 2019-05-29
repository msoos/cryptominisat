/******************************************
Copyright (c) 2019, Mate Soos

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

#include "cms_breakid.h"
#include "solver.h"
#include "clausecleaner.h"
#include "breakid/breakid.hpp"

using namespace CMSat;

BreakID::BreakID(Solver* _solver):
    solver(_solver)
{
}

template<class T>
BreakID::add_cl_ret BreakID::add_this_clause(const T& cl)
{
    uint32_t sz = 0;
    bool sat = false;
    brkid_lits.clear();
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
        brkid_lits.push_back(lit);
        sz++;
    }
    if (sat) {
        assert(false && "let's not do this, this would lie about the number of clauses...");
        return add_cl_ret::skipped_cl;
    }
    if (sz == 0) {
        //it's unsat because of assumptions
        if (solver->conf.verbosity) {
            cout << "c [walksat] UNSAT because of assumptions in clause: " << cl << endl;
        }
        return add_cl_ret::unsat;
    }

    breakid->add_clause((BID::BLit*)brkid_lits.data(), brkid_lits.size());
    brkid_lits.clear();

    return add_cl_ret::added_cl;
}

bool BreakID::doit()
{
    solver->clauseCleaner->remove_and_clean_all();
    double myTime = cpuTime();

    assert(breakid == NULL);
    breakid = new BID::BreakID;

    breakid->set_verbosity(0);
    // breakid->set_symBreakingFormLength(2);
    uint32_t ncls = solver->binTri.irredBins;
    ncls += solver->longIrredCls.size();
    breakid->start_dynamic_cnf(solver->nVars(), ncls);

    if (solver->check_assumptions_contradict_foced_assignement()) {
        delete breakid;
        breakid = NULL;
        return false;
    }

    //Add binary clauses
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

    //Add long clauses
    for(ClOffset offs: solver->longIrredCls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->getRemoved());

        if (add_this_clause(*cl) == add_cl_ret::unsat) {
            return false;
        }
    }
    breakid->end_dynamic_cnf();

    if (solver->conf.verbosity > 3) {
        breakid->print_graph();
    }

    if (solver->conf.verbosity) {
        cout << "c [breakid] Generators: " << breakid->get_num_generators() << endl;
        //breakid->print_generators();
    }

    if (solver->conf.verbosity > 1) {
        cout << "c [breakid] Detecting subgroups..." << endl;
    }
    breakid->detect_subgroups();

    if (solver->conf.verbosity > 2) {
        breakid->print_subgroups();
    }

    breakid->clean_theory();
    breakid->break_symm();

    if (breakid->get_num_break_cls() != 0) {
        break_symms();
    }
    delete breakid;
    breakid = NULL;

    double time_used = cpuTime() - myTime;
    bool time_out = false;
    if (solver->conf.verbosity) {
        cout << "c [breakid] finished "
        << solver->conf.print_times(time_used, time_out)
        << endl;
    }

    return true;
}

void BreakID::break_symms()
{
    if (solver->conf.verbosity) {
        cout << "c [breakid] Breaking cls: "<< breakid->get_num_break_cls() << endl;
        cout << "c [breakid] Aux vars: "<< breakid->get_num_aux_vars() << endl;
    }
    for(uint32_t i = 0; i < breakid->get_num_aux_vars(); i++) {
        solver->new_var(true);
    }
    if (symm_var == var_Undef) {
        solver->new_var(true);
        symm_var = solver->nVars()-1;

        vector<Lit> ass;
        ass.push_back(Lit(symm_var, true));
        solver->set_assumptions(ass);
        assert(solver->varData[symm_var].removed == Removed::none);
    }

    auto brk = breakid->get_brk_cls();
    for (auto cl: brk) {
        vector<Lit>* cl2 = (vector<Lit>*)&cl;
        cl2->push_back(Lit(symm_var, false));
        Clause* newcl = solver->add_clause_int(*cl2
            , false //redundant
            , ClauseStats() //stats
            , true //attach
            , NULL //return simplified
            , false //DRAT... oops does not work right now
            , lit_Undef
        );
        if (newcl != NULL) {
            ClOffset offset = solver->cl_alloc.get_offset(newcl);
            solver->longIrredCls.push_back(offset);
        }
    }

    assert(solver->varData[symm_var].removed == Removed::none);
}

void BreakID::finished_solving()
{
    //Nothing actually
}

void BreakID::start_new_solving()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->okay());
    if (symm_var == var_Undef) {
        return;
    }

    assert(solver->varData[symm_var].removed == Removed::none);
    assert(solver->value(symm_var) == l_Undef);
    solver->enqueue(Lit(symm_var, false));
    PropBy ret = solver->propagate<false>();
    assert(ret == PropBy() && "Must not fail on resetting symmetry var");
    symm_var = var_Undef;
}

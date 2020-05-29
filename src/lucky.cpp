/******************************************
Copyright (c) 2020, Mate Soos
Originally from CaDiCaL's "lucky.cpp" by Armin Biere, 2019

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

#include "lucky.h"
#include "solver.h"
#include "sqlstats.h"
#include "time_mem.h"

using namespace CMSat;


Lucky::Lucky(Solver* _solver) :
    solver(_solver)
{
}

bool CMSat::Lucky::doit()
{
    assert(solver->okay());
    assert(solver->decisionLevel() == 0);

    bool ret = false;
    double myTime = cpuTime();

    if (check_all(true)) {
        ret = true;
        goto end;
    }

    if (check_all(false)) {
        ret = true;
        goto end;
    }

    if (search_fwd_sat(true)) {
        ret = true;
        goto end;
    }

    if (search_fwd_sat(false)) {
        ret = true;
        goto end;
    }

    if (search_backw_sat(true)) {
        ret = true;
        goto end;
    }
    if (search_backw_sat(false)) {
        ret = true;
        goto end;
    }

    if (horn_sat(true)) {
        ret = true;
        goto end;
    }

    if (horn_sat(false)) {
        ret = true;
        goto end;
    }

    end:
    double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout << "c [lucky] finished "
        << solver->conf.print_times(time_used)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "lucky"
            , time_used
        );
    }
    assert(solver->decisionLevel() == 0);
    return ret;
}

bool CMSat::Lucky::check_all(bool polar)
{
    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        Lit lit = Lit::toLit(i);

        if (solver->value(lit) == l_True) {
            continue;
        }
        if (!lit.sign() == polar) {
            continue;
        }
        for(const auto& w: solver->watches[lit]) {
            if (w.isBin() && solver->value(w.lit2()) == l_True)
                continue;
            if (w.isBin() && solver->value(w.lit2()) == l_False)
                return false;
            if (w.isBin() && !w.lit2().sign() != polar)
                return false;
        }
    }

    for(const auto off: solver->longIrredCls) {
        Clause* cl = solver->cl_alloc.ptr(off);
        bool ok = false;
        for(const Lit l: *cl) {
            if (solver->value(l) == l_True) {
                ok = true;
                break;
            }
            if (!l.sign() == polar) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            return false;
        }
    }

    if (solver->conf.verbosity) {
        cout << "c [lucky] all " << (int)polar << " worked. Saving phases." << endl;
    }
    for(auto& x: solver->varData) {
        x.polarity = polar;
        x.best_polarity = polar;
    }
    solver->longest_trail_ever = solver->nVarsOuter();
    return true;
}


void Lucky::set_polarities_to_enq_val()
{
    for(uint32_t i = 0; i < solver->nVars(); i++) {
        solver->varData[i].polarity = solver->value(i) == l_True;
        solver->varData[i].best_polarity = solver->varData[i].polarity;
    }
    solver->longest_trail_ever = solver->nVarsOuter();
}

bool CMSat::Lucky::search_fwd_sat(bool polar)
{
    for(uint32_t i = 0; i < solver->nVars(); i++) {
        if (solver->varData[i].removed != Removed::none) {
            continue;
        }

        if (solver->value(i) != l_Undef) {
            continue;
        }
        solver->new_decision_level();

        Lit lit = Lit(i, !polar);
        solver->enqueue<true>(lit);
        auto p = solver->propagate<true>();
        if (!p.isNULL()) {
            solver->cancelUntil<false, true>(0);
            return false;
        }
    }

    if (solver->conf.verbosity) {
        cout << "c [lucky] Forward polar " << (int)polar  << " worked. Saving phases." << endl;
    }

    set_polarities_to_enq_val();
    solver->cancelUntil<false, true>(0);

    return true;
}

bool CMSat::Lucky::enqueue_and_prop_assumptions()
{
    assert(solver->decisionLevel() == 0);
    while (solver->decisionLevel() < solver->assumptions.size()) {
        const Lit p = solver->map_outer_to_inter(
            solver->assumptions[solver->decisionLevel()].lit_outer);

        if (solver->value(p) == l_True) {
            // Dummy decision level:
            solver->new_decision_level();
            continue;
        } else if (solver->value(p) == l_False) {
            solver->cancelUntil<false, true>(0);
            return false;
        } else {
            assert(p.var() < solver->nVars());
            solver->new_decision_level();
            solver->enqueue<true>(p);
            auto prop = solver->propagate<true>();
            if (!prop.isNULL()) {
                solver->cancelUntil<false, true>(0);
                return false;
            }
        }
    }
    return true;
}

bool CMSat::Lucky::search_backw_sat(bool polar)
{
    if (!enqueue_and_prop_assumptions()) {
        return false;
    }

    for(int i = (int)solver->nVars() - 1; i >= 0; i--) {
        if (solver->varData[i].removed != Removed::none) {
            continue;
        }

        if (solver->value(i) != l_Undef) {
            continue;
        }
        solver->new_decision_level();

        Lit lit = Lit(i, !polar);
        solver->enqueue<true>(lit);
        auto p = solver->propagate<true>();
        if (!p.isNULL()) {
            solver->cancelUntil<false, true>(0);
            return false;
        }
    }

    if (solver->conf.verbosity) {
        cout << "c [lucky] Backward polar " << (int)polar  << " worked. Saving phases." << endl;
    }

    set_polarities_to_enq_val();
    solver->cancelUntil<false, true>(0);
    return true;
}

bool CMSat::Lucky::horn_sat(bool polar)
{
    if (!enqueue_and_prop_assumptions()) {
        return false;
    }

    for(const auto off: solver->longIrredCls) {
        Clause* cl = solver->cl_alloc.ptr(off);
        bool satisfied = false;
        Lit to_set = lit_Undef;
        for(const Lit l: *cl) {
            if (!l.sign() == polar && solver->value(l) == l_Undef) {
                to_set = l;
            }
            if (solver->value(l) == l_True) {
                satisfied = true;
                break;
            }
        }
        if (satisfied) {
            continue;
        }

        if (to_set == lit_Undef) {
            //no unassigned literal of correct polarity
            solver->cancelUntil<false, true>(0);
            return false;
        }
        solver->new_decision_level();
        solver->enqueue<true>(to_set);
        auto p = solver->propagate<true>();
        if (!p.isNULL()) {
            solver->cancelUntil<false, true>(0);
            return false;
        }
    }

    //NOTE: propagating WHILE going through a watchlist will SEGFAULT
    vector<Lit> toset;
    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        Lit lit = Lit::toLit(i);
        if (solver->value(lit) == l_True) {
            continue;
        }
        if (!lit.sign() == polar) {
            bool must_set = false;
            for(const auto& w: solver->watches[lit]) {
                if (w.isBin() &&
                    solver->value(w.lit2()) != l_True)
                {
                    must_set = true;
                    break;
                }
            }
            if (must_set) {
                solver->new_decision_level();
                solver->enqueue<true>(lit);
                auto p = solver->propagate<true>();
                if (!p.isNULL()) {
                    solver->cancelUntil<false, true>(0);
                    return false;
                }
            }
        } else {
            toset.clear();
            bool ok = true;
            for(const auto& w: solver->watches[lit]) {
                if (w.isBin() &&
                    solver->value(w.lit2()) != l_True)
                {
                    if (w.lit2().sign() != polar) {
                        ok = false;
                        break;
                    } else {
                        toset.push_back(w.lit2());
                    }
                }
            }
            if (!ok) {
                solver->cancelUntil<false, true>(0);
                return false;
            }
            for(const auto& x: toset) {
                if (solver->value(x) == l_False) {
                    solver->cancelUntil<false, true>(0);
                    return false;
                }
                if (solver->value(x) == l_True) {
                    continue;
                }
                solver->new_decision_level();
                solver->enqueue<true>(x);
                auto p = solver->propagate<true>();
                if (!p.isNULL()) {
                    solver->cancelUntil<false, true>(0);
                    return false;
                }
            }
        }
    }

    if (solver->conf.verbosity) {
        cout << "c [lucky] Horn polar " << (int)polar  << " worked. Saving phases." << endl;
    }

    set_polarities_to_enq_val();
    solver->cancelUntil<false, true>(0);
    return true;
}

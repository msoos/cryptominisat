/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
Originally from CaDiCaL's "lucky.cpp", Copyright by Armin Biere, 2019

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
#include "gaussian.h"
#include "sqlstats.h"
#include "time_mem.h"

using namespace CMSat;

/*
Even in the competition there are formulas that are easy to satisfy by setting
all variables to the same truth value, or by assigning them that value and
propagating -- in the order of the variables (forward or backward) or in the
order of the clauses (the horn checks). CaDiCaL tries these in a pre-solving
step, and so do we, in the same order.

Unlike in CaDiCaL, XOR constraints have to be handled separately: the clauses
they were recovered from are gone, so the all-same checks below take them into
account, and a candidate model is only accepted once every XOR holds.
*/

Lucky::Lucky(Solver* _solver) : solver(_solver) {}

bool Lucky::terminated() const { return solver->must_interrupt_asap(); }

//Common clean up: get back to level 0 after a conflict or a failed check
int Lucky::unlucky(const int res)
{
    if (solver->decisionLevel() > 0) solver->cancelUntil<true, true>(0);
    return res;
}

bool Lucky::assume(const Lit lit)
{
    if (terminated()) { aborted = true; unlucky(0); return false; }

    solver->new_decision_level();
    solver->enqueue<true>(lit);
    if (!solver->propagate<true>().isnullptr()) { unlucky(0); return false; }
    return true;
}

//Assign whatever is left to 'polar', as the tail of CaDiCaL's checks does
bool Lucky::assume_rest(const bool polar)
{
    for(uint32_t v = 0; v < solver->nVars(); v++) {
        if (solver->varData[v].removed != Removed::none) continue;
        if (solver->value(v) != l_Undef) continue;
        if (!assume(Lit(v, !polar))) return false;
    }
    return true;
}

//Does every XOR come out right with all its free variables set to 'polar'?
bool Lucky::all_same_sat_xors(const bool polar) const
{
    auto ok = [&](const Xor& x) {
        bool parity = false;
        for(const uint32_t v: x) {
            const lbool val = solver->value(v);
            parity ^= val == l_Undef ? polar : val == l_True;
        }
        return parity == x.rhs;
    };

    for(const Xor& x: solver->xorclauses) if (!ok(x)) return false;
    for(const auto& g: solver->gmatrices) {
        if (!g) continue;
        for(const Xor& x: g->xorclauses) if (!ok(x)) return false;
    }
    return true;
}

//Propagation only reaches the XORs that are attached, so check them all before
//believing we have a model
bool Lucky::xors_satisfied() const
{
    auto ok = [&](const Xor& x) {
        bool parity = false;
        for(const uint32_t v: x) {
            if (solver->value(v) == l_Undef) return false;
            parity ^= solver->value(v) == l_True;
        }
        return parity == x.rhs;
    };

    for(const Xor& x: solver->xorclauses) if (!ok(x)) return false;
    for(const auto& g: solver->gmatrices) {
        if (!g) continue;
        for(const Xor& x: g->xorclauses) if (!ok(x)) return false;
    }
    return true;
}

//CaDiCaL's 'trivially_true_satisfiable'/'trivially_false_satisfiable': every
//clause holds with all variables set to 'polar', so assign them and propagate
int Lucky::trivially_satisfiable(const bool polar)
{
    assert(solver->decisionLevel() == 0);

    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        if (terminated()) return -1;
        const Lit lit = Lit::toLit(i);
        if (solver->value(lit) == l_True) continue;
        if (lit.sign() != polar) continue; //satisfies its binaries on its own

        for(const Watched& w: solver->watches[lit]) {
            if (!w.isBin() || w.red()) continue;
            if (solver->value(w.lit2()) == l_True) continue;
            if (w.lit2().sign() == polar) return 0;
        }
    }

    for(const ClOffset off: solver->longIrredCls) {
        if (terminated()) return -1;
        const Clause* cl = solver->cl_alloc.ptr(off);
        bool sat = false;
        for(const Lit l: *cl)
            if (solver->value(l) == l_True || l.sign() != polar) { sat = true; break; }
        if (!sat) return 0;
    }

    if (!all_same_sat_xors(polar)) return 0;

    if (!assume_rest(polar)) return aborted ? -1 : 0;
    if (!xors_satisfied()) return unlucky(0);
    verb_print(1, "[lucky] satisfied by: all " << (int)polar);
    return 10;
}

//CaDiCaL's 'forward_true_satisfiable'/'forward_false_satisfiable'
int Lucky::forward_satisfiable(const bool polar)
{
    assert(solver->decisionLevel() == 0);
    if (!assume_rest(polar)) return aborted ? -1 : 0;
    if (!xors_satisfied()) return unlucky(0);
    verb_print(1, "[lucky] satisfied by: forward " << (int)polar);
    return 10;
}

//CaDiCaL's 'backward_true_satisfiable'/'backward_false_satisfiable'
int Lucky::backward_satisfiable(const bool polar)
{
    assert(solver->decisionLevel() == 0);
    for(int i = (int)solver->nVars()-1; i >= 0; i--) {
        const uint32_t v = i;
        if (solver->varData[v].removed != Removed::none) continue;
        if (solver->value(v) != l_Undef) continue;
        if (!assume(Lit(v, !polar))) return aborted ? -1 : 0;
    }
    if (!xors_satisfied()) return unlucky(0);
    verb_print(1, "[lucky] satisfied by: backward " << (int)polar);
    return 10;
}

//CaDiCaL's 'positive_horn_satisfiable'/'negative_horn_satisfiable': satisfy each
//clause with its first literal of sign 'polar', then set the rest the other way
int Lucky::horn_satisfiable(const bool polar)
{
    assert(solver->decisionLevel() == 0);

    for(const ClOffset off: solver->longIrredCls) {
        if (terminated()) return unlucky(-1);
        const Clause* cl = solver->cl_alloc.ptr(off);
        Lit pick = lit_Undef;
        bool sat = false;
        for(const Lit l: *cl) {
            if (solver->value(l) == l_True) { sat = true; break; }
            if (solver->value(l) == l_False) continue;
            if (l.sign() != polar) { pick = l; break; }
        }
        if (sat) continue;
        if (pick == lit_Undef) return unlucky(0); //no free literal of the right sign
        if (!assume(pick)) return aborted ? -1 : 0;
    }

    //Same for the binaries. Propagating while walking a watchlist would
    //invalidate it, so collect what to set first.
    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        if (terminated()) return unlucky(-1);
        const Lit lit = Lit::toLit(i);
        if (solver->value(lit) != l_Undef) continue;

        to_set.clear();
        for(const Watched& w: solver->watches[lit]) {
            if (!w.isBin() || w.red()) continue;
            if (solver->value(w.lit2()) == l_True) continue;
            if (lit.sign() != polar) { to_set.push_back(lit); break; }
            if (w.lit2().sign() == polar) return unlucky(0);
            to_set.push_back(w.lit2());
        }

        for(const Lit l: to_set) {
            if (solver->value(l) == l_True) continue;
            if (solver->value(l) == l_False) return unlucky(0);
            if (!assume(l)) return aborted ? -1 : 0;
        }
    }

    if (!assume_rest(!polar)) return aborted ? -1 : 0;
    if (!xors_satisfied()) return unlucky(0);
    verb_print(1, "[lucky] satisfied by: horn " << (int)polar);
    return 10;
}

lbool Lucky::doit()
{
    assert(solver->okay());
    assert(solver->decisionLevel() == 0);
    assert(solver->assumptions.empty());
    const double my_time = cpu_time();

    int res = trivially_satisfiable(false);
    if (!res) res = trivially_satisfiable(true);
    if (!res) res = forward_satisfiable(true);
    if (!res) res = forward_satisfiable(false);
    if (!res) res = backward_satisfiable(false);
    if (!res) res = backward_satisfiable(true);
    if (!res) res = horn_satisfiable(true);
    if (!res) res = horn_satisfiable(false);
    if (res < 0) res = 0; //terminated

    if (res == 10) {
        solver->model = solver->assigns;
        solver->cancelUntil<true, true>(0);
        const PropBy confl = solver->propagate<true>();
        assert(confl.isnullptr()); (void)confl;
    }
    assert(solver->decisionLevel() == 0);

    const double time_used = cpu_time() - my_time;
    verb_print(1, "[lucky] " << (res == 10 ? "found a model" : "no luck")
        << solver->conf.print_times(time_used));
    if (solver->sqlStats) solver->sqlStats->time_passed_min(solver, "lucky", time_used);

    return res == 10 ? l_True : l_Undef;
}

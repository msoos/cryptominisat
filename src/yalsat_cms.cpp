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

#include "yalsat_cms.h"
#include "solver.h"
#include "gaussian.h"
#include <limits>

extern "C" {
#include "yals/yals.h"
}

using namespace CMSat;

static int yals_terminate(void* solver) {
    return ((Solver*)solver)->must_interrupt_asap();
}

CMS_yalsat::CMS_yalsat(Solver* _solver) : solver(_solver) {
    yals = yals_new();
    yals_setprefix(yals, solver->conf.prefix.c_str());
    yals_setopt(yals, "verbose", solver->conf.verbosity >= 3 ? 1 : 0);
    yals_setopt(yals, "xorweight", solver->conf.walkxorweight);
    yals_seterm(yals, yals_terminate, solver);
    occurred.resize(solver->nVars(), 0);
}

CMS_yalsat::~CMS_yalsat() { yals_del(yals); }

template<class T>
CMS_yalsat::add_cl_ret CMS_yalsat::add_this_clause(const T& cl) {
    lits.clear();
    for(const Lit lit: cl) {
        assert(solver->varData[lit.var()].removed == Removed::none);
        lbool val = solver->value(lit);
        if (val == l_Undef) val = solver->lit_inside_assumptions(lit);

        //clause is SAT, skip
        if (val == l_True) return add_cl_ret::skipped_cl;
        if (val == l_False) continue;
        lits.push_back(lit.sign() ? -(int)(lit.var()+1) : (int)(lit.var()+1));
    }

    if (lits.empty()) {
        verb_print(1, "[sls] UNSAT because of assumptions in clause: " << cl);
        return add_cl_ret::unsat;
    }

    for(const int l: lits) {
        occurred[abs(l)-1] = 1;
        yals_add(yals, l);
    }
    yals_add(yals, 0);
    cl_num++;
    return add_cl_ret::added_cl;
}

// yalsat's XOR clauses are satisfied when the parity of the added literals'
// signs XOR-ed with the variables' values is 1, so 'rhs' is expressed by
// negating one literal.
CMS_yalsat::add_cl_ret CMS_yalsat::add_this_xor(const Xor& x) {
    lits.clear();
    bool rhs = x.rhs;
    for(const uint32_t v: x) {
        //XORs held by a matrix may mention variables the CNF no longer has
        if (solver->varData[v].removed != Removed::none) return add_cl_ret::skipped_cl;
        lbool val = solver->value(v);
        if (val == l_Undef) val = solver->lit_inside_assumptions(Lit(v, false));

        if (val == l_True) rhs ^= true;
        else if (val == l_False) { /* no-op */ }
        else lits.push_back(v+1);
    }

    if (lits.empty()) {
        if (!rhs) return add_cl_ret::skipped_cl;
        verb_print(1, "[sls] UNSAT because of assumptions in XOR: " << x);
        return add_cl_ret::unsat;
    }

    if (!rhs) lits[0] = -lits[0];
    yals_begin_xor_clause(yals);
    for(const int l: lits) {
        occurred[abs(l)-1] = 1;
        yals_add(yals, l);
    }
    yals_add(yals, 0);
    cl_num++;
    return add_cl_ret::added_cl;
}

bool CMS_yalsat::init_problem() {
    if (solver->check_assumptions_contradict_foced_assignment()) return false;
    SLOW_DEBUG_DO(solver->check_stats());

    vector<Lit> bin(2);
    for(size_t i = 0; i < solver->nVars()*2; i++) {
        const Lit lit = Lit::toLit(i);
        for(const Watched& w: solver->watches[lit]) {
            if (!w.isBin() || w.red() || w.lit2() < lit) continue;
            bin[0] = lit;
            bin[1] = w.lit2();
            if (add_this_clause(bin) == add_cl_ret::unsat) return false;
        }
    }

    for(const ClOffset offs: solver->longIrredCls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->get_removed());
        if (add_this_clause(*cl) == add_cl_ret::unsat) return false;
    }

    for(const Xor& x: solver->xorclauses)
        if (add_this_xor(x) == add_cl_ret::unsat) return false;

    // XORs handed to Gauss-Jordan are no longer in 'xorclauses'
    for(const auto& g: solver->gmatrices) {
        if (!g) continue;
        for(const Xor& x: g->xorclauses)
            if (add_this_xor(x) == add_cl_ret::unsat) return false;
    }

    return true;
}

void CMS_yalsat::set_phase(const uint32_t var, const bool val) {
    yals_setphase(yals, val ? (int)(var+1) : -(int)(var+1));
}

int64_t CMS_yalsat::run(const int64_t mems, const uint64_t seed) {
    yals_srand(yals, seed);
    yals_setmemslimit(yals, mems);
    if (yals_sat(yals) == 20) return -1;

    //left untouched if yalsat's own unit propagation emptied the formula
    const int64_t minimum = yals_minimum(yals);
    return minimum == std::numeric_limits<int>::max() ? -1 : minimum;
}

bool CMS_yalsat::value(const uint32_t var) const {
    assert(occurred[var]);
    return yals_deref(yals, (int)(var+1)) > 0;
}

int64_t CMS_yalsat::count_unsat(int64_t& cnf, int64_t& xr) const {
    cnf = 0;
    xr = 0;

    auto free_val = [&](const Lit lit) {
        lbool val = solver->value(lit);
        if (val == l_Undef) val = solver->lit_inside_assumptions(lit);
        return val;
    };
    auto cl_sat = [&](const auto& cl) {
        uint32_t n_free = 0;
        for(const Lit lit: cl) {
            const lbool val = free_val(lit);
            if (val == l_True) return true; //not handed over
            if (val == l_Undef) n_free++;
        }
        if (n_free == 0) return true;
        for(const Lit lit: cl)
            if (free_val(lit) == l_Undef && value(lit.var()) != lit.sign()) return true;
        return false;
    };

    vector<Lit> bin(2);
    for(size_t i = 0; i < solver->nVars()*2; i++) {
        const Lit lit = Lit::toLit(i);
        for(const Watched& w: solver->watches[lit]) {
            if (!w.isBin() || w.red() || w.lit2() < lit) continue;
            bin[0] = lit; bin[1] = w.lit2();
            if (!cl_sat(bin)) cnf++;
        }
    }
    for(const ClOffset offs: solver->longIrredCls)
        if (!cl_sat(*solver->cl_alloc.ptr(offs))) cnf++;

    auto xor_sat = [&](const Xor& x) {
        bool rhs = x.rhs;
        bool any = false;
        for(const uint32_t v: x) {
            if (solver->varData[v].removed != Removed::none) return true;
            lbool val = solver->value(v);
            if (val == l_Undef) val = solver->lit_inside_assumptions(Lit(v, false));
            if (val == l_True) rhs ^= true;
            else if (val == l_False) continue;
            else { any = true; rhs ^= value(v); }
        }
        if (!any) return true; //not handed over
        return !rhs;
    };
    for(const Xor& x: solver->xorclauses) if (!xor_sat(x)) xr++;
    for(const auto& g: solver->gmatrices) {
        if (!g) continue;
        for(const Xor& x: g->xorclauses) if (!xor_sat(x)) xr++;
    }

    return cnf + xr;
}

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

#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "cryptominisat.h"
#include "user_prop.h"

namespace CMSat {

// Fuzzing harness for the external propagator interface: --userprop <seed>.
// Sits between the parser and the solver and keeps a random share of the
// clauses back, handing them over only through the whole propagator interface.
// The answer must not change. Contract violations abort the run.
class UserPropFuzzer final : public ExternalPropagator
{
public:
    UserPropFuzzer(SATSolver* _s, uint64_t seed) : s(_s), rnd(seed) {
        for(int i = 0; i < 4; i++) next();
        hold_pct     = 10 + next() % 81;
        defer_pct    = chance(50) ? next() % 30 : 0;
        extra_pct    = next() % 20;
        phase_pct    = chance(50) ? next() % 10 : 0;
        prop_pct     = 20 + next() % 81;
        volunteer_pct = next() % 20;
        forget_pct   = next() % 80;
        decide_pct   = chance(50) ? next() % 20 : 0;
        backtracks_left = chance(50) ? next() % 50 : 0;
        flip_pct     = next() % 100;
        advises_polarity = chance(50);
        is_lazy = chance(10);
        s->set_lazy_external_reasons(chance(70));
        // A replaced variable cannot be observed any more, and a deferred one
        // is only observed once a model turns up.
        if (defer_pct > 0) s->set_no_equivalent_lit_replacement();
        s->connect_external_propagator(this);
    }

    //////////////////////////////
    // What DimacsParser asks of a solver
    //////////////////////////////
    uint32_t nVars() const { return s->nVars(); }
    void new_var() { s->new_var(); }
    void new_vars(const size_t n) { s->new_vars(n); }
    template<typename T> void set_lit_weight(const Lit l, const T& w) { s->set_lit_weight(l, w); }
    template<class T> void set_multiplier_weight(const T& w) { s->set_multiplier_weight(w); }
    void set_weighted(const bool b) { s->set_weighted(b); }
    void set_projected(const bool b) { s->set_projected(b); }
    void set_sampl_vars(const std::vector<uint32_t>& v) { s->set_sampl_vars(v); }
    void set_opt_sampl_vars(const std::vector<uint32_t>& v) { s->set_opt_sampl_vars(v); }
    bool add_red_clause(const std::vector<Lit>& cl) { return s->add_red_clause(cl); }
    bool add_xor_clause(const std::vector<Lit>& l, bool rhs = true) { return s->add_xor_clause(l, rhs); }
    bool add_bnn_clause(const std::vector<Lit>& l, signed cutoff, Lit out = lit_Undef) {
        return s->add_bnn_clause(l, cutoff, out);
    }
    lbool solve(const std::vector<Lit>* assumps = nullptr) {
        solved = true;
        return s->solve(assumps);
    }
    lbool simplify(const std::vector<Lit>* assumps = nullptr) {
        solved = true;
        return s->simplify(assumps);
    }
    const std::vector<lbool>& get_model() const { return s->get_model(); }
    const std::vector<Lit>& get_conflict() const { return s->get_conflict(); }

    // Once the solver has run, a variable not observed yet may have been
    // replaced, so only clauses over observed variables are kept back.
    bool add_clause(const std::vector<Lit>& cl) {
        if (cl.empty() || !chance(hold_pct) || (solved && !usable(cl))) {
            if (!solved) for(const Lit l: cl) if (chance(extra_pct)) observe(l.var());
            return s->add_clause(cl);
        }
        held.push_back(cl);
        if (chance(defer_pct)) deferred.push_back(held.size()-1);
        else for(const Lit l: cl) observe(l.var());
        return true;
    }

    //////////////////////////////
    // The propagator
    //////////////////////////////
    void notify_assignment(const std::vector<Lit>& lits) override {
        check(!lits.empty(), "empty notification");
        for(const Lit l: lits) {
            check(observed_var(l.var()), "notified about an unobserved variable");
            check(val[l.var()] == l_Undef, "variable notified twice without a backtrack");
            val[l.var()] = l.sign() ? l_False : l_True;
            trail.back().push_back(l);
        }
    }

    void notify_new_decision_level() override { trail.push_back({}); }

    void notify_backtrack(size_t new_level) override {
        check(new_level + 1 < trail.size(), "backtrack to a level not below the current one");
        for(size_t i = new_level+1; i < trail.size(); i++)
            for(const Lit l: trail[i]) val[l.var()] = l_Undef;
        trail.resize(new_level+1);
    }

    bool cb_check_found_model(const std::vector<Lit>& model) override {
        check(model.size() == order.size(), "model is not one literal per observed variable");
        for(size_t i = 0; i < model.size(); i++) {
            check(model[i].var() == order[i], "model is not in the order variables were observed");
            if (!is_lazy) check(val[order[i]] == (model[i].sign() ? l_False : l_True),
                "model disagrees with the notifications");
        }

        // Late observation backtracks, and the solver must notice that the
        // assignment is not complete any more.
        bool observed_new = false;
        for(const size_t i: deferred) for(const Lit l: held[i]) observed_new |= observe(l.var());
        deferred.clear();
        if (observed_new) return false;
        std::vector<lbool> mval(s->nVars(), l_Undef);
        for(const Lit l: model) mval[l.var()] = l.sign() ? l_False : l_True;
        for(size_t i = 0; i < held.size(); i++) {
            bool sat = false;
            for(const Lit l: held[i]) if (mval[l.var()] == (l.sign() ? l_False : l_True)) sat = true;
            if (!sat) { queued.push_back(i); return false; }
        }
        return true;
    }

    Lit cb_decide() override {
        if (backtracks_left > 0 && trail.size() > 2 && chance(5)) {
            backtracks_left--;
            s->force_backtrack(next() % (trail.size()-1));
        }
        if (order.empty() || !chance(decide_pct)) return lit_Undef;
        const uint32_t v = order[next() % order.size()];
        return val[v] == l_Undef ? Lit(v, chance(50)) : lit_Undef;
    }

    Lit cb_decide_polarity(Lit lit) override {
        check(advises_polarity && !is_lazy, "polarity advice asked for unrequested");
        check(observed_var(lit.var()), "polarity advice asked for an unobserved variable");
        check(val[lit.var()] == l_Undef, "polarity advice asked for an assigned variable");
        return chance(flip_pct) ? ~lit : lit;
    }

    // Unit (or falsified) under the notified trail: propagate, and remember the
    // clause, which is the reason whenever it is asked for.
    Lit cb_propagate() override {
        if (held.empty() || !chance(prop_pct)) return lit_Undef;
        const size_t start = next() % held.size();
        for(size_t k = 0; k < held.size(); k++) {
            const size_t i = (start + k) % held.size();
            if (!usable(held[i])) continue;
            Lit free = lit_Undef;
            uint32_t num_free = 0;
            bool sat = false;
            for(const Lit l: held[i]) {
                const lbool v = value(l);
                if (v == l_True) { sat = true; break; }
                if (v == l_Undef) { num_free++; free = l; }
            }
            if (sat || num_free > 1) continue;
            const Lit p = num_free == 1 ? free : held[i][0];
            reason[p.toInt()] = i+1;
            return p;
        }
        return lit_Undef;
    }

    Lit cb_add_reason_clause_lit(Lit p) override {
        if (reason_at == 0) {
            check(p.toInt() < reason.size() && reason[p.toInt()] != 0,
                "reason asked for a literal that was not propagated");
            reason_cl = reason[p.toInt()]-1;
        }
        if (reason_at == held[reason_cl].size()) { reason_at = 0; return lit_Undef; }
        return held[reason_cl][reason_at++];
    }

    bool cb_has_external_clause(bool& forgettable) override {
        forgettable = chance(forget_pct);
        if (!queued.empty()) return true;
        if (held.empty() || !chance(volunteer_pct)) return false;
        const size_t i = next() % held.size();
        if (!usable(held[i])) return false;
        queued.push_back(i);
        return true;
    }

    Lit cb_add_external_clause_lit() override {
        const std::vector<Lit>& cl = held[queued.front()];
        if (clause_at == cl.size()) {
            clause_at = 0;
            queued.erase(queued.begin());
            return lit_Undef;
        }
        return cl[clause_at++];
    }

private:
    SATSolver* s;
    uint64_t rnd;
    uint32_t hold_pct, defer_pct, extra_pct, phase_pct, prop_pct;
    uint32_t volunteer_pct, forget_pct, decide_pct, backtracks_left, flip_pct;
    bool solved = false;

    std::vector<std::vector<Lit>> held;  // CNF clauses the solver is not given
    std::vector<size_t> deferred;        // held, but not observed until a model
    std::vector<size_t> queued;          // to hand over
    std::vector<size_t> reason;          // by Lit::toInt(): held index + 1
    std::vector<uint32_t> order;         // observed variables, in order
    std::vector<char> is_observed;       // by var
    std::vector<lbool> val;              // by var, from notifications alone
    std::vector<std::vector<Lit>> trail = std::vector<std::vector<Lit>>(1);
    size_t reason_cl = 0, reason_at = 0, clause_at = 0;

    uint32_t next() {
        rnd = rnd * 6364136223846793005ULL + 1442695040888963407ULL;
        return (uint32_t)(rnd >> 33);
    }
    bool chance(uint32_t pct) { return next() % 100 < pct; }

    static void check(bool ok, const char* what) {
        if (ok) return;
        std::cerr << "ERROR: --userprop: " << what << std::endl;
        std::exit(-1);
    }

    bool observed_var(uint32_t v) const { return v < is_observed.size() && is_observed[v]; }
    lbool value(const Lit l) const {
        const lbool v = val[l.var()];
        return (v == l_Undef || !l.sign()) ? v : (v == l_True ? l_False : l_True);
    }
    bool usable(const std::vector<Lit>& cl) const {
        for(const Lit l: cl) if (!observed_var(l.var())) return false;
        return true;
    }

    bool observe(uint32_t v) {
        if (observed_var(v)) return false;
        if (v >= is_observed.size()) {
            is_observed.resize(v+1, 0);
            val.resize(v+1, l_Undef);
            reason.resize(2*(v+1), 0);
        }
        s->add_observed_var(v);
        is_observed[v] = 1;
        order.push_back(v);
        if (chance(phase_pct)) s->phase(Lit(v, chance(50)));
        return true;
    }
};

}

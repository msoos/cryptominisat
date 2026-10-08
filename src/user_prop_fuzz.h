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

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "cryptominisat.h"
#include "user_prop.h"

namespace CMSat {

// Fuzzing harness for the external propagator interface: --userprop <seed>.
// Sits between the parser and the solver and keeps a random share of the
// clauses back, handing them over only through the propagator interface. It
// takes every action the interface offers at random, from every callback that
// allows it and between solves, and also where the interface ignores it. The
// answer must not change: a model is only accepted once every held clause is
// observed and satisfied by it. Contract violations abort the run.
class UserPropFuzzer final : public ExternalPropagator
{
public:
    UserPropFuzzer(SATSolver* _s, uint64_t seed) : s(_s), rnd(seed) {
        for(int i = 0; i < 4; i++) next();
        hold_pct      = 10 + next() % 81;
        defer_pct     = chance(50) ? next() % 30 : 0;
        extra_pct     = next() % 20;
        phase_pct     = chance(50) ? next() % 10 : 0;
        prop_pct      = 20 + next() % 81;
        volunteer_pct = next() % 20;
        forget_pct    = next() % 80;
        decide_pct    = chance(50) ? next() % 20 : 0;
        backtracks_left = chance(50) ? next() % 50 : 0;
        flip_pct      = next() % 100;
        perturb_pct   = chance(40) ? 1 + next() % 5 : 0;
        unobserve_pct = chance(50) ? next() % 50 : 0;
        reset_pct     = chance(20) ? 1 + next() % 3 : 0;
        new_var_pct   = chance(20) ? 1 + next() % 10 : 0;
        new_vars_left = next() % 30;
        stale_pct     = chance(30) ? 1 + next() % 5 : 0;
        model_bt_left = chance(30) ? next() % 20 : 0;
        ignored_pct   = chance(20) ? 1 + next() % 5 : 0;
        shape_pct     = chance(30) ? next() % 50 : 0;
        between_pct   = chance(50) ? 10 + next() % 50 : 0;
        budget_pct    = chance(30) ? 20 + next() % 60 : 0;
        interrupt_pct = chance(30) ? 1 + next() % 3 : 0;
        // Interactions need a particular sequence of actions, which uniform
        // choices rarely make. So a seed may come back to a few variables
        // again and again, act several times in a row, take up again what it
        // dropped, and aim at what it has just done.
        focus_pct     = chance(40) ? 30 + next() % 61 : 0;
        focus_size    = 1 + next() % 8;
        burst_pct     = chance(40) ? 10 + next() % 50 : 0;
        recall_pct    = chance(50) ? 20 + next() % 60 : 0;
        target_pct    = chance(50) ? 20 + next() % 60 : 0;
        roll_flags();
        s->set_lazy_external_reasons(chance(70));
        // A replaced variable cannot be observed any more: keep every
        // variable observable for a seed that may observe one late
        if (defer_pct || perturb_pct || between_pct) s->set_no_equivalent_lit_replacement();
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
    // Sometimes cut the search short, with a conflict budget or from a
    // callback, and resume it
    lbool solve(const std::vector<Lit>* assumps = nullptr) {
        between_solves();
        solved = true;
        if (!chance(budget_pct)) return s->solve(assumps);
        lbool ret = l_Undef;
        interruptible = true;
        for(uint64_t confl = 1 + next() % 100; ret == l_Undef && confl < 100000; confl *= 4) {
            s->set_max_confl(confl);
            ret = s->solve(assumps);
            if (ret == l_Undef) between_solves();
        }
        interruptible = false;
        if (ret == l_Undef) ret = s->solve(assumps);
        return ret;
    }
    lbool simplify(const std::vector<Lit>* assumps = nullptr) {
        between_solves();
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
        if (!chance(defer_pct)) for(const Lit l: cl) observe(l.var());
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
        ignored_backtrack();
        perturb();
    }

    void notify_new_decision_level() override {
        trail.push_back({});
        ignored_backtrack();
        perturb();
    }

    void notify_backtrack(size_t new_level) override {
        check(new_level + 1 < trail.size(), "backtrack to a level not below the current one");
        for(size_t i = new_level+1; i < trail.size(); i++)
            for(const Lit l: trail[i]) val[l.var()] = l_Undef;
        trail.resize(new_level+1);
        ignored_backtrack();
        perturb(nullptr, true);
    }

    bool cb_check_found_model(const std::vector<Lit>& model) override {
        check(model.size() == order.size(), "model is not one literal per observed variable");
        for(size_t i = 0; i < model.size(); i++) {
            check(model[i].var() == order[i], "model is not in the order variables were observed");
            if (!is_lazy) check(val[order[i]] == (model[i].sign() ? l_False : l_True),
                "model disagrees with the notifications");
        }
        check_queries();
        // Whatever happens here may leave the assignment incomplete, which is
        // no rejection: the solver must notice by itself
        perturb();

        // Every held clause must be expressible before it can be checked.
        // Late observation backtracks, and the solver must notice.
        bool observed_new = false;
        for(const auto& cl: held) for(const Lit l: cl) observed_new |= observe(l.var());
        if (observed_new) return false;

        std::vector<lbool> mval(s->nVars(), l_Undef);
        for(const Lit l: model) mval[l.var()] = l.sign() ? l_False : l_True;
        for(size_t i = 0; i < held.size(); i++) {
            bool sat = false;
            for(const Lit l: held[i]) if (mval[l.var()] == (l.sign() ? l_False : l_True)) sat = true;
            if (sat) continue;
            // Reject by backtracking, a limited number of times, with the
            // clause, or both. A backtrack must go below the current level to
            // count, and a lazy propagator does not know what that is.
            bool backtracked = false;
            if (model_bt_left > 0 && trail.size() > 1 && chance(50)) {
                model_bt_left--;
                s->force_backtrack(next() % (trail.size()-1));
                backtracked = true;
            }
            if (!backtracked || chance(50)) queued.push_back(i);
            return false;
        }
        return true;
    }

    Lit cb_decide() override {
        check_queries();
        if (interruptible && chance(interrupt_pct)) s->interrupt_asap();
        if (backtracks_left > 0 && trail.size() > 2 && chance(5)) {
            backtracks_left--;
            const uint32_t level = next() % (trail.size()-1);
            s->force_backtrack(level);
            // Decide what the backtrack unassigns: the decision is made on the
            // trail after it
            if (chance(target_pct)) {
                const auto& lev = trail[level + 1 + next() % (trail.size()-1-level)];
                if (!lev.empty()) return chance(50) ? lev[next() % lev.size()] : ~lev[next() % lev.size()];
            }
        }
        perturb();
        if (order.empty() || !chance(decide_pct)) return lit_Undef;
        uint32_t v = order[next() % order.size()];
        if (!focus.empty() && chance(focus_pct)) {
            const uint32_t f = focus[next() % focus.size()];
            if (observed_var(f)) v = f;
        }
        // An assigned literal is ignored
        if (val[v] != l_Undef && !chance(stale_pct)) return lit_Undef;
        return Lit(v, chance(50));
    }

    Lit cb_decide_polarity(Lit lit) override {
        check(advises_polarity && !is_lazy, "polarity advice asked for unrequested");
        check(observed_var(lit.var()), "polarity advice asked for an unobserved variable");
        check(val[lit.var()] == l_Undef, "polarity advice asked for an assigned variable");
        ignored_backtrack();
        return chance(flip_pct) ? ~lit : lit;
    }

    // Unit (or falsified) under the notified trail: propagate, and remember the
    // clause, which is the reason whenever it is asked for.
    Lit cb_propagate() override {
        check_queries();
        if (interruptible && chance(interrupt_pct)) s->interrupt_asap();
        ignored_backtrack();
        perturb();
        // A literal that is already true is ignored
        if (chance(stale_pct)) {
            for(const auto& lev: trail) if (!lev.empty()) return lev[next() % lev.size()];
        }
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
            reason_extra[p.toInt()] = false_above_root();
            return p;
        }
        return lit_Undef;
    }

    Lit cb_add_reason_clause_lit(Lit p) override {
        ignored_backtrack();
        if (reason_at == 0) {
            check(p.toInt() < reason.size() && reason[p.toInt()] != 0,
                "reason asked for a literal that was not propagated");
            shape(held[reason[p.toInt()]-1], reason_out, false);
            const Lit extra = reason_extra[p.toInt()];
            if (extra != lit_Undef) reason_out.insert(reason_out.begin() + next() % reason_out.size(), extra);
        }
        if (reason_at == reason_out.size()) { reason_at = 0; return lit_Undef; }
        return reason_out[reason_at++];
    }

    bool cb_has_external_clause(bool& forgettable) override {
        ignored_backtrack();
        perturb();
        forgettable = chance(forget_pct);
        if (queued.empty()) {
            if (held.empty() || !chance(volunteer_pct)) return false;
            const size_t i = next() % held.size();
            if (!usable(held[i])) return false;
            queued.push_back(i);
        }
        // It may only mention observed variables
        for(const Lit l: held[queued.front()]) observe(l.var());
        shape(held[queued.front()], clause_out, true);
        return true;
    }

    // What is observed may change while the clause is read, but not its own
    // variables
    Lit cb_add_external_clause_lit() override {
        ignored_backtrack();
        if (clause_at == clause_out.size()) {
            clause_at = 0;
            queued.erase(queued.begin());
            return lit_Undef;
        }
        if (clause_at > 0) perturb(&clause_out);
        return clause_out[clause_at++];
    }

private:
    SATSolver* s;
    uint64_t rnd;
    uint32_t hold_pct, defer_pct, extra_pct, phase_pct, prop_pct;
    uint32_t volunteer_pct, forget_pct, decide_pct, backtracks_left, flip_pct;
    uint32_t perturb_pct, unobserve_pct, reset_pct, new_var_pct, new_vars_left, stale_pct;
    uint32_t model_bt_left, ignored_pct, shape_pct, between_pct, budget_pct;
    uint32_t interrupt_pct, focus_pct, focus_size, burst_pct, recall_pct, target_pct;
    bool solved = false;
    bool interruptible = false;
    bool perturbing = false;

    std::vector<std::vector<Lit>> held;  // CNF clauses the solver is not given
    std::vector<size_t> queued;          // to hand over
    std::vector<size_t> reason;          // by Lit::toInt(): held index + 1
    std::vector<Lit> reason_extra;       // by Lit::toInt(): weakens that reason
    std::vector<uint32_t> order;         // observed variables, in order
    std::vector<char> is_observed;       // by var
    std::vector<lbool> val;              // by var, from notifications alone
    std::vector<std::vector<Lit>> trail = std::vector<std::vector<Lit>>(1);
    std::vector<Lit> clause_out, reason_out;  // as they are handed over
    std::vector<uint32_t> focus;         // the few variables a seed comes back to
    std::vector<uint32_t> dropped;       // no longer observed, to take up again
    size_t reason_at = 0, clause_at = 0;

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

    void roll_flags() {
        advises_polarity = chance(50);
        are_reasons_forgettable = chance(50);
        is_lazy = chance(10);
    }

    // The clause as handed over: sometimes shuffled and with a literal
    // repeated, and an external one weakened by an observed literal, or by a
    // literal and its negation, which makes it a tautology
    void shape(const std::vector<Lit>& cl, std::vector<Lit>& out, const bool external) {
        out = cl;
        if (!chance(shape_pct)) return;
        for(size_t i = out.size(); i > 1; i--) std::swap(out[i-1], out[next() % i]);
        if (chance(50)) out.push_back(out[next() % out.size()]);
        if (external && !order.empty() && chance(30)) {
            const Lit l = Lit(order[next() % order.size()], chance(50));
            out.push_back(l);
            if (chance(20)) out.push_back(~l);
        }
    }

    // Sometimes a literal that is false above the root, to weaken a reason
    // with: it stays false for as long as the propagation it explains holds
    Lit false_above_root() {
        if (trail.size() < 2 || !chance(shape_pct)) return lit_Undef;
        const auto& lev = trail[1 + next() % (trail.size()-1)];
        return lev.empty() ? lit_Undef : ~lev[next() % lev.size()];
    }

    void grow(uint32_t v) {
        if (v < is_observed.size()) return;
        is_observed.resize(v+1, 0);
        val.resize(v+1, l_Undef);
        reason.resize(2*(v+1), 0);
        reason_extra.resize(2*(v+1), lit_Undef);
    }

    bool observe(uint32_t v) {
        if (observed_var(v)) return false;
        grow(v);
        s->add_observed_var(v);
        is_observed[v] = 1;
        order.push_back(v);
        if (chance(phase_pct)) s->phase(Lit(v, chance(50)));
        return true;
    }

    // Forgetting a variable keeps no view of it: one fixed at the root stays
    // fixed, and is notified again if it is observed again.
    void unobserve(uint32_t v) {
        if (!observed_var(v)) return;
        s->remove_observed_var(v);
        is_observed[v] = 0;
        val[v] = l_Undef;
        order.erase(std::find(order.begin(), order.end(), v));
        drop_unobserved();
        remember_dropped(v);
    }

    void reset() {
        s->reset_observed_vars();
        for(const uint32_t v: order) { is_observed[v] = 0; val[v] = l_Undef; remember_dropped(v); }
        order.clear();
        drop_unobserved();
    }

    // The solver backtracks over what is no longer observed, except at the
    // root, where the assignment stays but is no longer part of the view
    void drop_unobserved() {
        for(auto& lev: trail) {
            lev.erase(std::remove_if(lev.begin(), lev.end(),
                [&](const Lit l) { return !observed_var(l.var()); }), lev.end());
        }
    }

    void remember_dropped(uint32_t v) {
        if (dropped.size() < 64) dropped.push_back(v);
        else dropped[next() % dropped.size()] = v;
    }

    // Often one of the seed's few variables, picked from the clauses it holds
    uint32_t pick_var() {
        if (focus.empty() && focus_pct > 0 && !held.empty()) {
            for(uint32_t i = 0; i < focus_size; i++) {
                const auto& cl = held[next() % held.size()];
                focus.push_back(cl[next() % cl.size()].var());
            }
        }
        if (!focus.empty() && chance(focus_pct)) return focus[next() % focus.size()];
        return next() % s->nVars();
    }

    // Often one dropped before, which may be fixed at the root by now and is
    // then owed a notification again
    void observe_some() {
        if (s->nVars() == 0) return;
        uint32_t v = pick_var();
        if (!dropped.empty() && chance(recall_pct)) {
            const size_t i = next() % dropped.size();
            v = dropped[i];
            dropped[i] = dropped.back();
            dropped.pop_back();
        }
        if (!observed_var(v)) observe(v);
    }

    // Sometimes the variable of a literal it propagated, while that holds:
    // the solver can no longer explain it, so must backtrack over it
    void unobserve_some(const std::vector<Lit>* keep = nullptr) {
        if (order.empty()) return;
        uint32_t v = order[next() % order.size()];
        if (chance(target_pct)) {
            const Lit p = own_propagation();
            if (p != lit_Undef) v = p.var();
        } else if (!focus.empty() && chance(focus_pct)) {
            const uint32_t f = focus[next() % focus.size()];
            if (observed_var(f)) v = f;
        }
        if (keep != nullptr) for(const Lit l: *keep) if (l.var() == v) return;
        unobserve(v);
    }

    // A literal true above the root that this propagated at some point
    Lit own_propagation() {
        if (trail.size() < 2) return lit_Undef;
        const auto& lev = trail[1 + next() % (trail.size()-1)];
        for(const Lit l: lev) if (reason[l.toInt()] != 0) return l;
        return lit_Undef;
    }

    // What a callback may do, except those that must not change the solver:
    // change what is observed, which may backtrack, add a variable, or pin a
    // phase, which notify_backtrack() may do too. Sometimes several in a row,
    // but not again from a callback this one caused.
    void perturb(const std::vector<Lit>* keep = nullptr, const bool phase_only = false) {
        if (perturbing || !chance(perturb_pct)) return;
        perturbing = true;
        const uint32_t actions = chance(burst_pct) ? 2 + next() % 4 : 1;
        for(uint32_t a = 0; a < actions; a++) perturb_once(keep, phase_only);
        perturbing = false;
    }

    void perturb_once(const std::vector<Lit>* keep, const bool phase_only) {
        if (phase_only) {
            if (s->nVars() > 0) s->phase(Lit(next() % s->nVars(), chance(50)));
        } else if (keep == nullptr && chance(reset_pct)) reset();
        else if (chance(unobserve_pct)) unobserve_some(keep);
        // A few: each must be assigned before a model is accepted, so adding
        // them without end would grow the problem faster than it is solved
        else if (new_vars_left > 0 && chance(new_var_pct)) {
            new_vars_left--;
            s->new_var();
            if (chance(50)) observe(s->nVars()-1);
        } else if (chance(phase_pct) && s->nVars() > 0) {
            const uint32_t v = pick_var();
            if (chance(50)) s->phase(Lit(v, chance(50)));
            else s->unphase(v);
        } else observe_some();
    }

    // Only cb_decide() and cb_check_found_model() may backtrack
    void ignored_backtrack() {
        if (chance(ignored_pct)) s->force_backtrack(next() % trail.size());
    }

    // What the solver says about the trail must agree with what it notified:
    // a decision is the first assignment of its level.
    void check_queries() {
        if (s->nVars() == 0) return;
        const uint32_t v = next() % s->nVars();
        check(s->is_observed_var(v) == observed_var(v), "is_observed_var() disagrees");
        if (!observed_var(v) || is_lazy) return;
        const Lit l = Lit(v, val[v] == l_False);
        if (val[v] == l_Undef) {
            check(!s->is_decision(l) && !s->is_decision(~l), "an unassigned literal is a decision");
            return;
        }
        if (!s->is_decision(l)) return;
        bool first = false;
        for(size_t lev = 1; lev < trail.size(); lev++)
            if (!trail[lev].empty() && trail[lev][0] == l) first = true;
        check(first, "a decision is not the first assignment of its level");
    }

    // Reconnect, which drops every observation and may change how the
    // propagator works, or change what is observed or how reasons are given
    void between_solves() {
        if (!chance(between_pct)) return;
        switch(next() % 6) {
            case 0:
                s->disconnect_external_propagator();
                for(const uint32_t v: order) remember_dropped(v);
                order.clear();
                std::fill(is_observed.begin(), is_observed.end(), 0);
                std::fill(val.begin(), val.end(), l_Undef);
                trail.assign(1, {});
                roll_flags();
                s->connect_external_propagator(this);
                break;
            case 1: reset(); break;
            case 2: unobserve_some(); break;
            case 3: s->set_lazy_external_reasons(chance(50)); break;
            case 4: {
                const uint32_t pct = perturb_pct;
                perturb_pct = 100;
                perturb();
                perturb_pct = pct;
                break;
            }
            default: for(const auto& cl: held) for(const Lit l: cl) observe(l.var());
        }
    }
};

}

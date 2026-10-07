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

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <vector>

#include "cryptominisat5/cryptominisat.h"
#include "cryptominisat5/user_prop.h"
#include "src/solver.h"
#include "src/solverconf.h"
#include "test_helper.h"

using namespace CMSat;
using std::vector;

namespace {

// Does nothing; the solver must behave as if no propagator were connected.
class NoopPropagator : public ExternalPropagator
{
public:
    uint32_t num_assignment_notifications = 0;
    uint32_t num_new_level_notifications = 0;
    uint32_t num_backtrack_notifications = 0;
    uint32_t num_model_checks = 0;

    void notify_assignment(const vector<Lit>&) override {
        num_assignment_notifications++;
    }
    void notify_new_decision_level() override { num_new_level_notifications++; }
    void notify_backtrack(size_t) override { num_backtrack_notifications++; }
    bool cb_check_found_model(const vector<Lit>&) override {
        num_model_checks++;
        return true;
    }
    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return false;
    }
    Lit cb_add_external_clause_lit() override { return lit_Undef; }
};

// Rebuilds the trail from notifications alone and checks it against the
// solver's wherever the two must agree.
class MirrorPropagator : public ExternalPropagator
{
public:
    Solver* s = nullptr;
    vector<vector<Lit>> stack;   // observed literals, per level
    vector<char> assigned;       // indexed by outer var
    vector<lbool> value_of;      // indexed by outer var
    uint32_t num_comparisons = 0;
    uint32_t num_backtracks = 0;
    uint32_t max_level_seen = 0;

    void start(Solver* _s, uint32_t nvars) {
        s = _s;
        stack.assign(1, {});
        assigned.assign(nvars, 0);
        value_of.assign(nvars, l_Undef);
    }

    /// The truth value of an outer literal, as the propagator sees it.
    lbool val(const Lit l) const {
        const lbool v = value_of[l.var()];
        if (v == l_Undef) return l_Undef;
        return l.sign() ? (v == l_True ? l_False : l_True) : v;
    }

    void notify_assignment(const vector<Lit>& lits) override {
        EXPECT_FALSE(lits.empty());
        for(const Lit l: lits) {
            EXPECT_TRUE(s->is_observed_var(l.var()))
                << "notified about unobserved var " << l.var()+1;
            EXPECT_FALSE(assigned[l.var()])
                << "var " << l.var()+1 << " assigned twice without a backtrack";
            assigned[l.var()] = 1;
            value_of[l.var()] = l.sign() ? l_False : l_True;
            stack.back().push_back(l);
        }
    }

    void notify_new_decision_level() override {
        stack.push_back({});
        max_level_seen = std::max<uint32_t>(max_level_seen, stack.size()-1);
        compare();
    }

    void notify_backtrack(size_t new_level) override {
        // must always pop at least one level
        ASSERT_LT(new_level + 1, stack.size());
        for(size_t i = new_level+1; i < stack.size(); i++) {
            for(const Lit l: stack[i]) {
                assigned[l.var()] = 0;
                value_of[l.var()] = l_Undef;
            }
        }
        stack.resize(new_level + 1);
        num_backtracks++;
        compare();
    }

    bool cb_check_found_model(const vector<Lit>&) override { compare(); return true; }
    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return false;
    }
    Lit cb_add_external_clause_lit() override { return lit_Undef; }

    void compare() {
        vector<vector<Lit>> expected;
        // false: notifications still owed, the two may differ
        if (!s->ext_get_observed_trail(expected)) return;
        num_comparisons++;

        vector<vector<Lit>> mine = stack;
        // order within the root level is meaningless
        std::sort(mine[0].begin(), mine[0].end());
        ASSERT_EQ(mine, expected);
    }
};

// Hands clauses to the solver one at a time during the search instead of up
// front, keeping the mirror's checks.
class OraclePropagator : public MirrorPropagator
{
public:
    vector<vector<Lit>> to_hand_over;   // OUTER numbering
    bool forgettable = false;
    size_t next_clause = 0;
    size_t next_lit = 0;
    size_t num_handed_over = 0;

    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = forgettable;
        return next_clause < to_hand_over.size();
    }

    Lit cb_add_external_clause_lit() override {
        assert(next_clause < to_hand_over.size());
        const vector<Lit>& cl = to_hand_over[next_clause];
        if (next_lit == cl.size()) {
            next_lit = 0;
            next_clause++;
            num_handed_over++;
            return lit_Undef;
        }
        return cl[next_lit++];
    }
};

// Unit-propagates over clauses the solver never sees, using only the mirrored
// trail, and explains each propagation on request.
class UnitPropagator : public MirrorPropagator
{
public:
    vector<vector<Lit>> theory;      // OUTER numbering
    // Recorded at propagation time: lazy reasons are asked for much later.
    vector<size_t> reason_for_lit;   // indexed by Lit::toInt()
    size_t cur_clause = 0;
    size_t cur_lit = 0;
    size_t num_propagations = 0;
    size_t num_explanations = 0;

    void start_theory(Solver* _s, uint32_t nvars) {
        start(_s, nvars);
        reason_for_lit.assign(2*nvars, 0);
    }

    Lit cb_propagate() override {
        for(size_t i = 0; i < theory.size(); i++) {
            Lit implied = lit_Undef;
            bool satisfied = false;
            uint32_t num_free = 0;
            for(const Lit l: theory[i]) {
                const lbool v = val(l);
                if (v == l_True) { satisfied = true; break; }
                if (v == l_Undef) { num_free++; implied = l; }
            }
            if (satisfied) continue;
            // unit, or falsified: any literal; the solver sees the conflict
            if (num_free == 1 || num_free == 0) {
                const Lit ret = (num_free == 1) ? implied : theory[i][0];
                reason_for_lit[ret.toInt()] = i;
                num_propagations++;
                return ret;
            }
        }
        return lit_Undef;
    }

    Lit cb_add_reason_clause_lit(Lit propagated_lit) override {
        if (cur_lit == 0) {
            cur_clause = reason_for_lit[propagated_lit.toInt()];
            num_explanations++;
        }
        const vector<Lit>& cl = theory[cur_clause];
        if (cur_lit == cl.size()) { cur_lit = 0; return lit_Undef; }
        return cl[cur_lit++];
    }
};

// Always decides the lowest unassigned observed variable, with a fixed sign.
class DecidingPropagator : public MirrorPropagator
{
public:
    uint32_t nvars = 0;
    bool sign = false;
    uint32_t num_decisions = 0;
    bool check_is_decision = false;

    Lit cb_decide() override {
        for(uint32_t v = 0; v < nvars; v++) {
            if (val(Lit(v, false)) != l_Undef) continue;
            num_decisions++;
            return Lit(v, sign);
        }
        return lit_Undef;
    }

    bool cb_check_found_model(const vector<Lit>& model) override {
        compare();
        if (check_is_decision) {
            // only our own decisions may be reported as decisions
            for(const Lit l: model) {
                if (s->ext_is_decision(l)) { EXPECT_EQ(l.sign(), sign); }
            }
        }
        return true;
    }
};

// Enumerates models: rejects each one and blocks it with its negation.
class EnumeratingPropagator : public MirrorPropagator
{
public:
    vector<vector<Lit>> models;      // one literal per observed var
    vector<vector<Lit>> blocking;
    size_t next_clause = 0;
    size_t next_lit = 0;

    bool cb_check_found_model(const vector<Lit>& model) override {
        compare();
        models.push_back(model);
        vector<Lit> block;
        block.reserve(model.size());
        for(const Lit l: model) block.push_back(~l);
        blocking.push_back(block);
        return false;
    }

    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return next_clause < blocking.size();
    }

    Lit cb_add_external_clause_lit() override {
        const vector<Lit>& cl = blocking[next_clause];
        if (next_lit == cl.size()) { next_lit = 0; next_clause++; return lit_Undef; }
        return cl[next_lit++];
    }
};

// Pseudo-randomly observes late, propagates, hands over clauses, forces
// backtracks, and rejects models that violate its theory.
class AdversarialPropagator : public MirrorPropagator
{
public:
    Solver* raw = nullptr;
    vector<vector<Lit>> theory;      // sound clauses the solver never sees
    vector<size_t> reason_for_lit;   // indexed by Lit::toInt()
    vector<size_t> queued;           // theory clauses waiting to be handed over
    size_t cur_clause = 0;
    size_t cur_lit = 0;
    uint32_t backtrack_budget = 0;
    uint64_t rnd_state = 1;
    size_t num_propagations = 0;
    size_t num_clauses_given = 0;
    size_t num_forced_backtracks = 0;
    size_t num_late_observes = 0;
    size_t num_rejected_models = 0;

    uint32_t next() {
        rnd_state = rnd_state * 6364136223846793005ULL + 1442695040888963407ULL;
        return (uint32_t)(rnd_state >> 33);
    }
    bool chance(uint32_t percent) { return (next() % 100) < percent; }

    void start_adversary(Solver* _s, uint32_t nvars, uint32_t seed) {
        raw = _s;
        start(_s, nvars);
        reason_for_lit.assign(2*nvars, 0);
        rnd_state = seed * 2862933555777941757ULL + 3037000493ULL;
    }

    bool all_observed(const vector<Lit>& cl) const {
        for(const Lit l: cl) if (!raw->is_observed_var(l.var())) return false;
        return true;
    }

    bool is_satisfied(const vector<Lit>& cl) const {
        for(const Lit l: cl) if (val(l) == l_True) return true;
        return false;
    }

    Lit cb_propagate() override {
        if (!chance(60)) return lit_Undef;
        for(size_t i = 0; i < theory.size(); i++) {
            if (!all_observed(theory[i])) continue;
            Lit implied = lit_Undef;
            bool satisfied = false;
            uint32_t num_free = 0;
            for(const Lit l: theory[i]) {
                const lbool v = val(l);
                if (v == l_True) { satisfied = true; break; }
                if (v == l_Undef) { num_free++; implied = l; }
            }
            if (satisfied || num_free > 1) continue;
            const Lit ret = (num_free == 1) ? implied : theory[i][0];
            reason_for_lit[ret.toInt()] = i;
            num_propagations++;
            return ret;
        }
        return lit_Undef;
    }

    Lit cb_add_reason_clause_lit(Lit propagated_lit) override {
        if (cur_lit == 0) cur_clause = reason_for_lit[propagated_lit.toInt()];
        const vector<Lit>& cl = theory[cur_clause];
        if (cur_lit == cl.size()) { cur_lit = 0; return lit_Undef; }
        return cl[cur_lit++];
    }

    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = chance(50);
        if (!queued.empty()) return true;
        // occasionally volunteer a clause nobody asked for
        if (!chance(10)) return false;
        for(uint32_t tries = 0; tries < 4; tries++) {
            const size_t i = next() % theory.size();
            if (all_observed(theory[i])) { queued.push_back(i); return true; }
        }
        return false;
    }

    Lit cb_add_external_clause_lit() override {
        const vector<Lit>& cl = theory[queued.front()];
        if (cur_lit == cl.size()) {
            cur_lit = 0;
            queued.erase(queued.begin());
            num_clauses_given++;
            return lit_Undef;
        }
        return cl[cur_lit++];
    }

    Lit cb_decide() override {
        if (backtrack_budget > 0 && stack.size() > 3 && chance(20)) {
            backtrack_budget--;
            num_forced_backtracks++;
            raw->ext_force_backtrack((uint32_t)(next() % (stack.size()-1)));
        }
        return lit_Undef;
    }

    bool cb_check_found_model(const vector<Lit>& model) override {
        compare();
        (void)model;

        // Observe the theory's remaining vars first; that backtracks.
        bool observed_something = false;
        for(const auto& cl: theory) {
            for(const Lit l: cl) {
                if (!raw->is_observed_var(l.var())) {
                    raw->add_observed_var(l.var());
                    num_late_observes++;
                    observed_something = true;
                }
            }
        }
        if (observed_something) return false;

        for(size_t i = 0; i < theory.size(); i++) {
            if (is_satisfied(theory[i])) continue;
            queued.push_back(i);
            num_rejected_models++;
            return false;
        }
        return true;
    }
};

// Forces a backtrack whenever the trail gets deep, 'budget' times.
class BacktrackingPropagator : public MirrorPropagator
{
public:
    SATSolver* api = nullptr;
    Solver* raw = nullptr;
    uint32_t budget = 0;
    uint32_t num_forced = 0;

    Lit cb_decide() override {
        if (budget > 0 && stack.size() > 4) {
            budget--;
            num_forced++;
            raw->ext_force_backtrack(1);
        }
        return lit_Undef;
    }
};

}

TEST(user_prop_connect, connect_and_disconnect)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(4);
    s.add_clause(str_to_cl("1, 2"));
    s.add_clause(str_to_cl("-1, 3"));

    s.connect_external_propagator(&p);
    EXPECT_EQ(s.solve(), l_True);
    s.disconnect_external_propagator();

    EXPECT_EQ(s.solve(), l_True);
}

TEST(user_prop_connect, multi_threading_is_refused)
{
    // Connecting before any variable exists used to slip past the check.
    {
        SATSolver s;
        NoopPropagator p;
        s.connect_external_propagator(&p);
        EXPECT_THROW(s.set_num_threads(4), std::runtime_error);
    }

    // ...and the other way round. phase()/unphase() need no propagator, so
    // they can reach a multi-threaded solver too.
    {
        SATSolver s;
        NoopPropagator p;
        s.set_num_threads(4);
        EXPECT_THROW(s.connect_external_propagator(&p), std::runtime_error);
        s.new_vars(4);
        EXPECT_THROW(s.phase(Lit(0, false)), std::runtime_error);
        EXPECT_THROW(s.unphase(0), std::runtime_error);

        // vars still pending, so the solver is unharmed
        s.add_clause(str_to_cl("1, 2"));
        EXPECT_EQ(s.solve(), l_True);
    }
}

TEST(user_prop_connect, model_changing_simplification_is_refused)
{
    // Adding blocked clauses drops models without consulting the propagator.
    SATSolver s;
    NoopPropagator p;
    s.new_vars(4);
    s.add_clause(str_to_cl("1, 2"));
    s.connect_external_propagator(&p);
    s.add_observed_var(0);
    EXPECT_THROW(s.reverse_bce(), std::runtime_error);

    // fine again once the propagator is gone
    s.disconnect_external_propagator();
    EXPECT_EQ(s.solve(), l_True);
}

TEST(user_prop_connect, no_propagator_no_change)
{
    for(int with_prop = 0; with_prop < 2; with_prop++) {
        SATSolver s;
        NoopPropagator p;
        s.new_vars(5);
        s.add_clause(str_to_cl("1, 2, 3"));
        s.add_clause(str_to_cl("-1, -2"));
        s.add_clause(str_to_cl("-2, -3"));
        s.add_clause(str_to_cl("-1, -3"));
        if (with_prop) s.connect_external_propagator(&p);
        EXPECT_EQ(s.solve(), l_True);
    }
}

TEST(user_prop_observe, observe_and_unobserve)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(6);
    s.connect_external_propagator(&p);

    EXPECT_FALSE(s.is_observed_var(0));
    s.add_observed_var(0);
    s.add_observed_var(3);
    EXPECT_TRUE(s.is_observed_var(0));
    EXPECT_TRUE(s.is_observed_var(3));
    EXPECT_FALSE(s.is_observed_var(1));

    // observing twice is idempotent
    s.add_observed_var(0);
    EXPECT_TRUE(s.is_observed_var(0));

    s.remove_observed_var(0);
    EXPECT_FALSE(s.is_observed_var(0));
    EXPECT_TRUE(s.is_observed_var(3));

    s.reset_observed_vars();
    EXPECT_FALSE(s.is_observed_var(3));
}

TEST(user_prop_observe, unobserved_root_unit_is_not_notified_later)
{
    SATSolver s;
    NoopPropagator p;
    s.new_var();
    ASSERT_TRUE(s.add_clause(vector<Lit>{Lit(0, false)}));
    s.connect_external_propagator(&p);

    // A pre-existing root unit goes to a separate queue when observed;
    // unobserve it before anything drains that queue.
    s.add_observed_var(0);
    s.remove_observed_var(0);
    ASSERT_EQ(s.solve(), l_True);

    EXPECT_FALSE(s.is_observed_var(0));
    EXPECT_EQ(p.num_assignment_notifications, 0U);
}

TEST(user_prop_observe, disconnect_resets_observed)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(6);
    s.connect_external_propagator(&p);
    s.add_observed_var(2);
    EXPECT_TRUE(s.is_observed_var(2));

    s.disconnect_external_propagator();
    EXPECT_FALSE(s.is_observed_var(2));
}

TEST(user_prop_observe, observe_var_added_after_connect)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(2);
    s.connect_external_propagator(&p);
    s.new_vars(3);
    s.add_observed_var(4);
    EXPECT_TRUE(s.is_observed_var(4));
    EXPECT_EQ(s.solve(), l_True);
}

TEST(user_prop_phase, forced_phase_shows_in_model)
{
    // No constraints: every var is decided, so the phase fixes the model.
    for(int polarity = 0; polarity < 2; polarity++) {
        SATSolver s;
        NoopPropagator p;
        s.new_vars(8);
        s.connect_external_propagator(&p);
        for(uint32_t i = 0; i < 8; i++) s.phase(Lit(i, polarity == 0));

        EXPECT_EQ(s.solve(), l_True);
        const auto& model = s.get_model();
        for(uint32_t i = 0; i < 8; i++) {
            EXPECT_EQ(model[i], polarity == 0 ? l_False : l_True);
        }
    }
}

TEST(user_prop_phase, unphase_gives_control_back)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(4);
    s.connect_external_propagator(&p);
    s.phase(Lit(0, false));
    s.unphase(0);
    EXPECT_EQ(s.solve(), l_True);
}

// Observes v while the level for z is opened, backtracking over x -> v. The
// pending z decision must be dropped, not installed at the root, where it used
// to turn the external unit -z into a root conflict (UNSAT).
class ObservesDuringNewLevelPropagator : public NoopPropagator
{
public:
    SATSolver* raw = nullptr;
    uint32_t levels_opened = 0;
    size_t next_reason_lit = 0;
    bool chose_x = false;
    bool observed_v = false;
    bool propagated_not_z = false;

    void notify_new_decision_level() override {
        levels_opened++;
        if (levels_opened == 2) {
            raw->add_observed_var(1);
            observed_v = true;
        }
    }

    Lit cb_decide() override {
        if (chose_x) return lit_Undef;
        chose_x = true;
        return Lit(0, false);
    }

    Lit cb_propagate() override {
        if (!observed_v || propagated_not_z) return lit_Undef;
        propagated_not_z = true;
        return Lit(2, true);
    }

    Lit cb_add_reason_clause_lit(Lit propagated_lit) override {
        EXPECT_EQ(propagated_lit, Lit(2, true));
        if (next_reason_lit++ == 0) return propagated_lit;
        next_reason_lit = 0;
        return lit_Undef;
    }
};

TEST(user_prop_observe, observing_during_new_level_discards_pending_decision)
{
    SATSolver s;
    ObservesDuringNewLevelPropagator p;
    p.raw = &s;
    s.set_no_simplify();
    s.new_vars(3); // x, v, z
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, false)}); // x -> v
    s.connect_external_propagator(&p);
    s.add_observed_var(0);
    s.add_observed_var(2);
    s.phase(Lit(2, false)); // make the discarded internal decision z, not -z

    ASSERT_EQ(s.solve(), l_True);
    EXPECT_TRUE(p.observed_v);
    EXPECT_TRUE(p.propagated_not_z);
    EXPECT_EQ(s.get_model()[2], l_False);
}

// Adds and observes a fresh var from the model callback, leaving the model
// incomplete: the search must resume before returning it.
class AddsVarAtModelPropagator : public NoopPropagator
{
public:
    SATSolver* raw = nullptr;
    uint32_t model_checks = 0;
    bool saw_complete_grown_model = false;

    bool cb_check_found_model(const vector<Lit>& model) override {
        model_checks++;
        if (model_checks == 1) {
            EXPECT_EQ(model.size(), 1U);
            raw->new_var();
            raw->add_observed_var(1);
        } else if (model.size() == 2) {
            saw_complete_grown_model = true;
        }
        return true;
    }
};

TEST(user_prop_observe, model_callback_cannot_return_an_incomplete_grown_model)
{
    SATSolver s;
    AddsVarAtModelPropagator p;
    p.raw = &s;
    s.set_no_simplify();
    s.new_var();
    s.connect_external_propagator(&p);
    s.add_observed_var(0);

    ASSERT_EQ(s.solve(), l_True);
    EXPECT_GE(p.model_checks, 2U);
    EXPECT_TRUE(p.saw_complete_grown_model);
    ASSERT_EQ(s.get_model().size(), 2U);
    EXPECT_NE(s.get_model()[1], l_Undef);
}

TEST(user_prop_is_decision, unassigned_is_not_a_decision)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(3);
    s.connect_external_propagator(&p);
    s.add_observed_var(0);
    EXPECT_FALSE(s.is_decision(Lit(0, false)));
}

TEST(user_prop_is_decision, root_level_unit_is_not_a_decision)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(3);
    s.connect_external_propagator(&p);
    s.add_observed_var(0);
    s.add_clause(str_to_cl("1"));
    EXPECT_EQ(s.solve(), l_True);
    EXPECT_FALSE(s.is_decision(Lit(0, false)));
}

// ---- Observed variables are frozen ----

TEST(user_prop_freeze, observed_var_survives_bve)
{
    // var 3 occurs only in the first two clauses, so BVE would eliminate it
    for(int observe = 0; observe < 2; observe++) {
        SATSolver s;
        NoopPropagator p;
        s.new_vars(10);
        s.connect_external_propagator(&p);
        if (observe) s.add_observed_var(2);

        s.add_clause(str_to_cl("1, 3"));
        s.add_clause(str_to_cl("2, -3"));
        s.add_clause(str_to_cl("4, 5"));
        s.add_clause(str_to_cl("-4, 6"));
        std::string strategy = "occ-bve";
        EXPECT_EQ(s.simplify(nullptr, &strategy), l_Undef);

        if (observe) { EXPECT_FALSE(s.removed_var(2)); }
        else { EXPECT_TRUE(s.removed_var(2)); }
    }
}

TEST(user_prop_freeze, observed_var_not_replaced)
{
    // 1 <-> 2, so SCC replacement would merge them
    for(int observe = 0; observe < 2; observe++) {
        SATSolver s;
        NoopPropagator p;
        s.new_vars(6);
        s.connect_external_propagator(&p);
        if (observe) {
            s.add_observed_var(0);
            s.add_observed_var(1);
        }

        s.add_clause(str_to_cl("1, -2"));
        s.add_clause(str_to_cl("-1, 2"));
        s.add_clause(str_to_cl("1, 3, 4"));
        s.add_clause(str_to_cl("-3, 5"));
        std::string strategy = "must-scc-vrepl";
        EXPECT_EQ(s.simplify(nullptr, &strategy), l_Undef);

        const bool merged = s.removed_var(0) || s.removed_var(1);
        EXPECT_EQ(merged, observe == 0);
    }
}

TEST(user_prop_freeze, eliminated_var_is_uneliminated_when_observed)
{
    SATSolver s;
    NoopPropagator p;
    s.new_vars(10);
    s.add_clause(str_to_cl("1, 3"));
    s.add_clause(str_to_cl("2, -3"));
    s.add_clause(str_to_cl("4, 5"));
    std::string strategy = "occ-bve";
    EXPECT_EQ(s.simplify(nullptr, &strategy), l_Undef);
    ASSERT_TRUE(s.removed_var(2));

    s.connect_external_propagator(&p);
    s.add_observed_var(2);
    EXPECT_FALSE(s.removed_var(2));
    EXPECT_TRUE(s.is_observed_var(2));
    EXPECT_EQ(s.solve(), l_True);
}

// Deterministic random 3-SAT.
static void add_random_3sat(Solver* s, uint32_t nvars, uint32_t ncls, uint32_t seed)
{
    uint64_t st = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    auto next = [&st]() {
        st = st * 6364136223846793005ULL + 1442695040888963407ULL;
        return (uint32_t)(st >> 33);
    };
    s->new_vars(nvars);
    for(uint32_t i = 0; i < ncls; i++) {
        vector<Lit> cl;
        while(cl.size() < 3) {
            const uint32_t v = next() % nvars;
            bool dup = false;
            for(const Lit l: cl) if (l.var() == v) dup = true;
            if (!dup) cl.push_back(Lit(v, next() & 1));
        }
        s->add_clause_outside(cl);
        if (!s->okay()) return;
    }
}

namespace CMSat {

struct UserPropFreezeTest : public ::testing::Test {
    UserPropFreezeTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropFreezeTest() { delete s; }

    SolverConf conf;
    Solver* s = nullptr;
    std::atomic<bool> must_inter;
};

TEST_F(UserPropFreezeTest, no_chrono_backtracking_with_propagator)
{
    // UNSAT, so conflicts are guaranteed. With diff_declev_for_chrono = 0 and
    // no propagator, every multi-level backjump is chronological.
    conf.diff_declev_for_chrono = 0;

    Solver control(&conf, &must_inter);
    add_random_3sat(&control, 120, 600, 42);
    EXPECT_EQ(control.solve_with_assumptions(), l_False);
    ASSERT_GT(control.chrono_backtrack, 0U);
    // a finished solve raises the shared interrupt flag
    must_inter.store(false, std::memory_order_relaxed);

    NoopPropagator p;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    add_random_3sat(s, 120, 600, 42);
    EXPECT_EQ(s->solve_with_assumptions(), l_False);
    EXPECT_EQ(s->chrono_backtrack, 0U);
    EXPECT_GT(s->non_chrono_backtrack, 0U);
}

TEST_F(UserPropFreezeTest, no_gauss_jordan_matrices_with_propagator)
{
    NoopPropagator p;
    s = new Solver(&conf, &must_inter);
    s->conf.do_find_xors = true;
    s->connect_external_propagator(&p);
    s->new_vars(30);
    // overlapping XORs: normally a Gauss-Jordan matrix
    for(uint32_t i = 0; i + 3 < 24; i += 2) {
        vector<uint32_t> vars = {i, i+1, i+2, i+3};
        s->add_xor_clause_outside(vars, (i/2) % 2 == 0);
    }
    s->solve_with_assumptions();
    EXPECT_TRUE(s->gmatrices.empty());
}

TEST_F(UserPropFreezeTest, matrices_come_back_after_disconnecting)
{
    NoopPropagator p;
    s = new Solver(&conf, &must_inter);
    s->conf.do_find_xors = true;
    // otherwise lucky phases solve it before any matrix is built
    s->conf.lucky = 0;
    s->connect_external_propagator(&p);
    s->new_vars(30);
    for(uint32_t i = 0; i + 3 < 24; i += 2) {
        vector<uint32_t> vars = {i, i+1, i+2, i+3};
        s->add_xor_clause_outside(vars, (i/2) % 2 == 0);
    }
    must_inter.store(false, std::memory_order_relaxed);
    s->solve_with_assumptions();
    ASSERT_TRUE(s->gmatrices.empty());

    s->disconnect_external_propagator();
    must_inter.store(false, std::memory_order_relaxed);
    s->solve_with_assumptions();
    EXPECT_FALSE(s->gmatrices.empty()) << "Gauss-Jordan never came back";
}

}

// ---- Trail notifications ----

namespace CMSat {

struct UserPropNotifyTest : public ::testing::Test {
    UserPropNotifyTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropNotifyTest() { delete s; }

    void run_mirror(uint32_t nvars, uint32_t ncls, uint32_t seed, uint32_t observe_every)
    {
        s = new Solver(&conf, &must_inter);
        s->connect_external_propagator(&p);
        add_random_3sat(s, nvars, ncls, seed);
        for(uint32_t v = 0; v < nvars; v += observe_every) s->add_observed_var(v);
        p.start(s, nvars);

        must_inter.store(false, std::memory_order_relaxed);
        const lbool ret = s->solve_with_assumptions();
        EXPECT_NE(ret, l_Undef);
        EXPECT_GT(p.num_comparisons, 0U) << "the mirror never got to compare anything";
        delete s;
        s = nullptr;
    }

    SolverConf conf;
    Solver* s = nullptr;
    MirrorPropagator p;
    std::atomic<bool> must_inter;
};

TEST_F(UserPropNotifyTest, mirror_small_sat)
{
    run_mirror(40, 130, 7, 3);
    EXPECT_GT(p.max_level_seen, 0U);
}

TEST_F(UserPropNotifyTest, mirror_larger_with_inprocessing)
{
    // big enough for restarts, cleaning, simplification and renumbering
    run_mirror(200, 860, 11, 2);
    EXPECT_GT(p.num_backtracks, 0U);
}

TEST_F(UserPropNotifyTest, mirror_unsat)
{
    run_mirror(30, 220, 23, 1);
}

TEST_F(UserPropNotifyTest, mirror_all_observed)
{
    run_mirror(120, 500, 5, 1);
}

TEST_F(UserPropNotifyTest, mirror_seeds)
{
    for(uint32_t seed = 1; seed <= 12; seed++) {
        run_mirror(60, 240, seed, 2);
    }
}

TEST_F(UserPropNotifyTest, mirror_survives_incremental_solving)
{
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    add_random_3sat(s, 60, 150, 3);
    for(uint32_t v = 0; v < 60; v += 2) s->add_observed_var(v);
    p.start(s, 60);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    const uint32_t after_first = p.num_comparisons;
    EXPECT_GT(after_first, 0U);

    // the propagator keeps its level-0 view across calls
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_GT(p.num_comparisons, after_first);
}

TEST_F(UserPropNotifyTest, observing_an_already_fixed_variable)
{
    // units assigned before being observed must be notified exactly once
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(8);
    s->add_clause_outside(str_to_cl("1"));
    s->add_clause_outside(str_to_cl("-2"));
    s->add_clause_outside(str_to_cl("3, 4"));
    for(uint32_t v = 0; v < 8; v++) s->add_observed_var(v);
    p.start(s, 8);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_GT(p.num_comparisons, 0U);
}

TEST_F(UserPropNotifyTest, observing_a_fixed_variable_after_a_solve)
{
    // After a solve, renumbering has wiped the level-0 trail, so this unit has
    // to be handed over separately.
    s = new Solver(&conf, &must_inter);
    s->conf.simplify_at_startup = true;
    s->conf.full_simplify_at_startup = true;
    s->connect_external_propagator(&p);
    add_random_3sat(s, 60, 150, 3);
    s->add_clause_outside(str_to_cl("1"));
    p.start(s, 60);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);

    s->add_observed_var(0);
    for(uint32_t v = 1; v < 60; v += 3) s->add_observed_var(v);
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_GT(p.num_comparisons, 0U);
}

// Observes then_observe from inside the notification about on_seeing.
class ObservesFromNotificationPropagator : public MirrorPropagator
{
public:
    uint32_t on_seeing = 0;
    uint32_t then_observe = 1;
    bool done = false;

    void notify_assignment(const vector<Lit>& lits) override {
        MirrorPropagator::notify_assignment(lits);
        if (done) return;
        for(const Lit l: lits) {
            if (l.var() != on_seeing) continue;
            done = true;
            s->add_observed_var(then_observe);
        }
    }
};

TEST_F(UserPropNotifyTest, observing_a_fixed_variable_from_inside_a_notification)
{
    // Vars 1 and 2 are fixed and behind the cursor when observed; observing 2
    // from inside the notification about 1 must not lose it.
    ObservesFromNotificationPropagator op;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&op);
    s->new_vars(6);
    s->add_clause_outside(str_to_cl("1"));
    s->add_clause_outside(str_to_cl("-2"));
    s->add_clause_outside(str_to_cl("3, 4"));
    s->add_clause_outside(str_to_cl("5, 6"));
    for(uint32_t v = 2; v < 6; v++) s->add_observed_var(v);
    op.start(s, 6);
    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    ASSERT_FALSE(op.done);

    s->add_observed_var(0);
    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_TRUE(op.done);
    EXPECT_TRUE(s->is_observed_var(1));
    EXPECT_EQ(op.value_of[0], l_True);
    EXPECT_EQ(op.value_of[1], l_False);
    EXPECT_GT(op.num_comparisons, 0U);
}

TEST_F(UserPropNotifyTest, opening_a_level_hands_over_what_is_still_owed)
{
    // Owed assignments must be notified before a level opens, or they land on
    // the wrong level of the propagator's stack.
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(4);
    for(uint32_t v = 0; v < 4; v++) s->add_observed_var(v);
    p.start(s, 4);
    // an owed root assignment
    s->add_clause_outside(str_to_cl("1"));
    ASSERT_TRUE(p.stack[0].empty());

    s->new_decision_level();
    ASSERT_EQ(p.stack.size(), 2U);
    EXPECT_EQ(p.stack[0], vector<Lit>{Lit(0, false)});
    EXPECT_EQ(p.num_comparisons, 1U);
    s->cancel_until(0);
}

TEST_F(UserPropNotifyTest, no_notifications_from_inprocessing)
{
    // any assignment leaking from probing or distillation breaks the mirror
    s = new Solver(&conf, &must_inter);
    s->conf.simplify_at_startup = true;
    s->conf.full_simplify_at_startup = true;
    s->connect_external_propagator(&p);
    add_random_3sat(s, 150, 620, 31);
    for(uint32_t v = 0; v < 150; v++) s->add_observed_var(v);
    p.start(s, 150);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_NE(s->solve_with_assumptions(), l_Undef);
    EXPECT_GT(p.num_comparisons, 0U);
}

}

// ---- External clauses during the search ----

namespace CMSat {

// Deterministic random 3-SAT as a clause list, OUTER numbering.
static vector<vector<Lit>> gen_3sat(uint32_t nvars, uint32_t ncls, uint32_t seed)
{
    uint64_t st = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    auto next = [&st]() {
        st = st * 6364136223846793005ULL + 1442695040888963407ULL;
        return (uint32_t)(st >> 33);
    };
    vector<vector<Lit>> out;
    for(uint32_t i = 0; i < ncls; i++) {
        vector<Lit> cl;
        while(cl.size() < 3) {
            const uint32_t v = next() % nvars;
            bool dup = false;
            for(const Lit l: cl) if (l.var() == v) dup = true;
            if (!dup) cl.push_back(Lit(v, next() & 1));
        }
        out.push_back(cl);
    }
    return out;
}

static bool model_satisfies(const vector<lbool>& model, const vector<vector<Lit>>& cls)
{
    for(const auto& cl: cls) {
        bool sat = false;
        for(const Lit l: cl) {
            if (l.var() >= model.size()) return false;
            if (model[l.var()] == (l.sign() ? l_False : l_True)) { sat = true; break; }
        }
        if (!sat) return false;
    }
    return true;
}

struct UserPropClauseTest : public ::testing::Test {
    UserPropClauseTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropClauseTest() { delete s; delete ref; }

    lbool solve_plain(const vector<vector<Lit>>& cls, uint32_t nvars) {
        ref = new Solver(&conf, &must_inter);
        ref->new_vars(nvars);
        for(const auto& cl: cls) {
            vector<Lit> tmp = cl;
            ref->add_clause_outside(tmp);
        }
        must_inter.store(false, std::memory_order_relaxed);
        return ref->solve_with_assumptions();
    }

    // the first 'split' clauses go in up front, the rest via the propagator
    lbool solve_split(const vector<vector<Lit>>& cls, uint32_t nvars, size_t split) {
        s = new Solver(&conf, &must_inter);
        s->connect_external_propagator(&p);
        s->new_vars(nvars);
        for(size_t i = 0; i < split; i++) {
            vector<Lit> tmp = cls[i];
            s->add_clause_outside(tmp);
        }
        for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
        for(size_t i = split; i < cls.size(); i++) p.to_hand_over.push_back(cls[i]);
        p.start(s, nvars);

        must_inter.store(false, std::memory_order_relaxed);
        return s->solve_with_assumptions();
    }

    SolverConf conf;
    Solver* s = nullptr;
    Solver* ref = nullptr;
    OraclePropagator p;
    std::atomic<bool> must_inter;
};

TEST_F(UserPropClauseTest, forgettable_clauses_are_actually_forgotten)
{
    // Forgettable clauses must be filed where clause-database reduction looks.
    const uint32_t nvars = 150;
    auto cls = gen_3sat(nvars, 750, 3);
    const size_t split = cls.size();
    std::set<vector<Lit>> extra;
    uint64_t st = 99;
    while(extra.size() < 200) {
        vector<Lit> cl;
        while(cl.size() < 5) {
            st = st * 6364136223846793005ULL + 1442695040888963407ULL;
            const Lit l = Lit((st >> 33) % nvars, (st >> 20) & 1);
            bool dup = false;
            for(const Lit q: cl) if (q.var() == l.var()) dup = true;
            if (!dup) cl.push_back(l);
        }
        std::sort(cl.begin(), cl.end());
        if (extra.insert(cl).second) cls.push_back(cl);
    }

    // no simplification, so a clause can only disappear through reduction
    conf.do_simplify_problem = 0;
    conf.do_distill_clauses = 0;
    conf.reduceint = 10;
    p.forgettable = true;
    solve_split(cls, nvars, split);
    ASSERT_EQ(p.num_handed_over, extra.size());
    ASSERT_GT(s->sum_conflicts, 1000U);

    size_t kept = 0;
    for(const auto& tier: s->long_red_cls) for(const ClOffset offs: tier) {
        const Clause& c = *s->cl_alloc.ptr(offs);
        vector<Lit> outer;
        for(const Lit l: c) outer.push_back(s->map_inter_to_outer(l));
        std::sort(outer.begin(), outer.end());
        kept += extra.count(outer);
    }
    EXPECT_LT(kept, extra.size() / 2);
}

TEST_F(UserPropClauseTest, many_seeds)
{
    for(uint32_t seed = 1; seed <= 15; seed++) {
        const uint32_t nvars = 30;
        auto cls = gen_3sat(nvars, 125, seed);
        const lbool expected = solve_plain(cls, nvars);
        ASSERT_EQ(solve_split(cls, nvars, cls.size()/2), expected) << "seed " << seed;
        if (expected == l_True) {
            EXPECT_TRUE(model_satisfies(s->get_model(), cls)) << "seed " << seed;
        }
        delete s; s = nullptr;
        delete ref; ref = nullptr;
        p = OraclePropagator();
    }
}

TEST_F(UserPropClauseTest, unit_and_empty_clauses_from_the_propagator)
{
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(8);
    s->add_clause_outside(str_to_cl("1, 2"));
    s->add_clause_outside(str_to_cl("-1, 3"));
    for(uint32_t v = 0; v < 8; v++) s->add_observed_var(v);

    p.to_hand_over.push_back(str_to_cl("1"));
    p.to_hand_over.push_back(str_to_cl("-1"));
    p.start(s, 8);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_False);
    EXPECT_FALSE(s->okay());
}

TEST_F(UserPropClauseTest, frat_proof_is_written_without_tripping_anything)
{
    // Verifying the proof needs the toolchain in README_VERIFIER.md; this only
    // checks the FRAT path runs, writes output and gets the right answer.
    const uint32_t nvars = 24;
    auto cls = gen_3sat(nvars, 180, 5);
    const lbool expected = solve_plain(cls, nvars);
    ASSERT_EQ(expected, l_False);

    const char* fname = "user_prop_test.frat";
    FILE* f = fopen(fname, "wb");
    ASSERT_NE(f, nullptr);

    s = new Solver(&conf, &must_inter);
    s->add_frat(f);
    s->connect_external_propagator(&p);
    s->new_vars(nvars);
    for(size_t i = 0; i < 90; i++) {
        vector<Lit> tmp = cls[i];
        s->add_clause_outside(tmp);
    }
    for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
    for(size_t i = 90; i < cls.size(); i++) p.to_hand_over.push_back(cls[i]);
    p.start(s, nvars);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_False);
    delete s; s = nullptr;
    fclose(f);

    FILE* check = fopen(fname, "rb");
    ASSERT_NE(check, nullptr);
    fseek(check, 0, SEEK_END);
    EXPECT_GT(ftell(check), 0);
    fclose(check);
    std::remove(fname);
}

TEST_F(UserPropClauseTest, forgettable_is_ignored_under_frat)
{
    // FRAT cannot express a redundant input clause, so the clauses are kept.
    const uint32_t nvars = 30;
    auto cls = gen_3sat(nvars, 125, 4);
    const lbool expected = solve_plain(cls, nvars);

    const char* fname = "user_prop_test_forget.frat";
    FILE* f = fopen(fname, "wb");
    ASSERT_NE(f, nullptr);

    s = new Solver(&conf, &must_inter);
    s->add_frat(f);
    s->connect_external_propagator(&p);
    s->new_vars(nvars);
    for(size_t i = 0; i < 60; i++) {
        vector<Lit> tmp = cls[i];
        s->add_clause_outside(tmp);
    }
    for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
    p.forgettable = true;
    for(size_t i = 60; i < cls.size(); i++) p.to_hand_over.push_back(cls[i]);
    p.start(s, nvars);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), expected);
    delete s; s = nullptr;
    fclose(f);
    std::remove(fname);
}

TEST_F(UserPropClauseTest, propagator_forces_a_specific_model)
{
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(12);
    s->add_clause_outside(str_to_cl("1, 2, 3"));
    for(uint32_t v = 0; v < 12; v++) {
        s->add_observed_var(v);
        p.to_hand_over.push_back(vector<Lit>{Lit(v, v % 2 == 0)});
    }
    p.start(s, 12);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    for(uint32_t v = 0; v < 12; v++) {
        EXPECT_EQ(s->get_model()[v], v % 2 == 0 ? l_False : l_True) << "var " << v+1;
    }
}

// Decides a fixed chain, hands over one clause at a chosen depth, and records
// what the trail did in response.
class DeepHandoverPropagator : public MirrorPropagator
{
public:
    uint32_t chain = 0;              // decide vars 0..chain-1 true
    uint32_t hand_over_at = 0;       // ...and give the clause at this level
    vector<Lit> clause;              // OUTER numbering
    size_t next_lit = 0;
    bool handed = false;
    uint32_t backtracks_at_handover = 0;
    uint32_t level_at_handover = 0;
    uint32_t backtracks_after = 0;
    uint32_t level_after = 0;
    bool saw_after = false;

    Lit cb_decide() override {
        // the first cb_decide() after the hand-over sees what it did
        if (handed && !saw_after) {
            saw_after = true;
            backtracks_after = num_backtracks;
            level_after = stack.size()-1;
        }
        for(uint32_t v = 0; v < chain; v++) {
            if (val(Lit(v, false)) == l_Undef) return Lit(v, false);
        }
        return lit_Undef;
    }

    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        if (handed || stack.size()-1 < hand_over_at) return false;
        backtracks_at_handover = num_backtracks;
        level_at_handover = stack.size()-1;
        return true;
    }

    Lit cb_add_external_clause_lit() override {
        if (next_lit == clause.size()) { next_lit = 0; handed = true; return lit_Undef; }
        return clause[next_lit++];
    }
};

// Every 'o' (original clause) line of a text FRAT proof, literals only.
static vector<vector<Lit>> read_frat_original_clauses(const char* fname)
{
    vector<vector<Lit>> ret;
    FILE* f = fopen(fname, "rb");
    if (f == nullptr) return ret;
    char line[8192];
    while (fgets(line, sizeof(line), f) != nullptr) {
        if (line[0] != 'o' || line[1] != ' ') continue;
        vector<Lit> cl;
        const char* at = line+2;
        bool first = true; // the ID
        while (true) {
            char* end = nullptr;
            const long v = strtol(at, &end, 10);
            if (end == at) break;
            at = end;
            if (first) { first = false; continue; }
            if (v == 0) break;
            cl.push_back(Lit((uint32_t)std::labs(v)-1, v < 0));
        }
        std::sort(cl.begin(), cl.end());
        ret.push_back(cl);
    }
    fclose(f);
    return ret;
}

TEST_F(UserPropClauseTest, external_clauses_are_original_clauses_in_the_proof)
{
    // External clauses enter the proof as input clauses, over the user's
    // variables: their literals are already outer and must not be renumbered.
    const char* fname = "user_prop_test_orig.frat";
    FILE* f = fopen(fname, "wb");
    ASSERT_NE(f, nullptr);

    DeepHandoverPropagator dp;
    dp.chain = 6;
    dp.hand_over_at = 6;
    dp.clause = str_to_cl("7, -8, 9");

    s = new Solver(&conf, &must_inter);
    s->add_frat(f);
    s->connect_external_propagator(&dp);
    s->new_vars(12);
    for(uint32_t v = 0; v < 12; v++) s->add_observed_var(v);
    dp.start(s, 12);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    ASSERT_TRUE(dp.handed);
    delete s; s = nullptr;
    fclose(f);

    vector<Lit> want = dp.clause;
    std::sort(want.begin(), want.end());
    const vector<vector<Lit>> got = read_frat_original_clauses(fname);
    EXPECT_NE(std::find(got.begin(), got.end(), want), got.end())
        << "the external clause is not in the proof as an original clause";
    std::remove(fname);
}

TEST_F(UserPropClauseTest, a_satisfied_clause_does_not_move_the_trail)
{
    // Var v is decided true on level v+1. The clause is satisfied on level 1,
    // below its falsified literals, so there is nothing to repair.
    DeepHandoverPropagator dp;
    dp.chain = 6;
    dp.hand_over_at = 6;
    dp.clause = str_to_cl("1, -3, -2");

    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&dp);
    s->new_vars(12);
    for(uint32_t v = 0; v < 12; v++) s->add_observed_var(v);
    dp.start(s, 12);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    ASSERT_TRUE(dp.handed);
    ASSERT_TRUE(dp.saw_after);
    EXPECT_EQ(dp.level_at_handover, 6U);
    EXPECT_EQ(dp.level_after, dp.level_at_handover);
    EXPECT_EQ(dp.backtracks_after, dp.backtracks_at_handover);
}

TEST_F(UserPropClauseTest, a_clause_that_should_have_propagated_lower_backtracks)
{
    // Satisfied only on level 6, above both falsified literals: backtrack to
    // level 3, where the clause propagates.
    DeepHandoverPropagator dp;
    dp.chain = 6;
    dp.hand_over_at = 6;
    dp.clause = str_to_cl("6, -3, -2");

    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&dp);
    s->new_vars(12);
    for(uint32_t v = 0; v < 12; v++) s->add_observed_var(v);
    dp.start(s, 12);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    ASSERT_TRUE(dp.handed);
    ASSERT_TRUE(dp.saw_after);
    EXPECT_EQ(dp.level_at_handover, 6U);
    EXPECT_EQ(dp.level_after, 3U);
    EXPECT_EQ(dp.backtracks_after, dp.backtracks_at_handover + 1);
}

}

// ---- External propagation, eager reasons ----

namespace CMSat {

struct UserPropPropagateTest : public ::testing::Test {
    UserPropPropagateTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropPropagateTest() { delete s; delete ref; }

    lbool solve_plain(const vector<vector<Lit>>& cls, uint32_t nvars) {
        delete ref;
        ref = new Solver(&conf, &must_inter);
        ref->new_vars(nvars);
        for(const auto& cl: cls) {
            vector<Lit> tmp = cl;
            ref->add_clause_outside(tmp);
        }
        must_inter.store(false, std::memory_order_relaxed);
        return ref->solve_with_assumptions();
    }

    // first 'split' clauses go to the solver, the rest only to the propagator
    lbool solve_with_theory(const vector<vector<Lit>>& cls, uint32_t nvars, size_t split,
                            bool lazy_reasons = false) {
        delete s;
        conf.ext_lazy_reasons = lazy_reasons;
        s = new Solver(&conf, &must_inter);
        s->connect_external_propagator(&p);
        s->new_vars(nvars);
        for(size_t i = 0; i < split; i++) {
            vector<Lit> tmp = cls[i];
            s->add_clause_outside(tmp);
        }
        for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
        p.theory.clear();
        for(size_t i = split; i < cls.size(); i++) p.theory.push_back(cls[i]);
        p.start_theory(s, nvars);

        must_inter.store(false, std::memory_order_relaxed);
        return s->solve_with_assumptions();
    }

    SolverConf conf;
    Solver* s = nullptr;
    Solver* ref = nullptr;
    UnitPropagator p;
    std::atomic<bool> must_inter;
};

TEST_F(UserPropPropagateTest, many_seeds)
{
    for(uint32_t seed = 1; seed <= 15; seed++) {
        const uint32_t nvars = 30;
        auto cls = gen_3sat(nvars, 125, seed);
        const lbool expected = solve_plain(cls, nvars);
        ASSERT_EQ(solve_with_theory(cls, nvars, cls.size()/2), expected)
            << "seed " << seed;
        if (expected == l_True) {
            EXPECT_TRUE(model_satisfies(s->get_model(), cls)) << "seed " << seed;
        }
        p = UnitPropagator();
    }
}

TEST_F(UserPropPropagateTest, forgettable_reason_clauses)
{
    const uint32_t nvars = 60;
    auto cls = gen_3sat(nvars, 200, 17);
    p.are_reasons_forgettable = true;
    ASSERT_EQ(solve_with_theory(cls, nvars, 140), l_True);
    EXPECT_TRUE(model_satisfies(s->get_model(), cls));
}

TEST_F(UserPropPropagateTest, lazy_propagator_is_never_asked)
{
    const uint32_t nvars = 40;
    auto cls = gen_3sat(nvars, 120, 3);
    p.is_lazy = true;
    solve_with_theory(cls, nvars, cls.size());   // nothing left for the theory
    EXPECT_EQ(p.num_propagations, 0U);

    // ...nor notified of anything
    EXPECT_EQ(p.num_backtracks, 0U);
    EXPECT_EQ(p.max_level_seen, 0U);
    EXPECT_EQ(p.num_comparisons, 0U);
    ASSERT_EQ(p.stack.size(), 1U);
    EXPECT_TRUE(p.stack[0].empty());
}

}

// ---- Decisions, forced backtracking, model checks ----

namespace CMSat {

// Brute-force model count over 'nvars' variables.
static uint32_t count_models(const vector<vector<Lit>>& cls, uint32_t nvars)
{
    uint32_t count = 0;
    for(uint32_t mask = 0; mask < (1U << nvars); mask++) {
        bool all_sat = true;
        for(const auto& cl: cls) {
            bool sat = false;
            for(const Lit l: cl) {
                const bool v = (mask >> l.var()) & 1;
                if (v != l.sign()) { sat = true; break; }
            }
            if (!sat) { all_sat = false; break; }
        }
        if (all_sat) count++;
    }
    return count;
}

struct UserPropDecideTest : public ::testing::Test {
    UserPropDecideTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropDecideTest() { delete s; }

    SolverConf conf;
    Solver* s = nullptr;
    std::atomic<bool> must_inter;
};

TEST_F(UserPropDecideTest, cb_decide_drives_the_search)
{
    for(int sign = 0; sign < 2; sign++) {
        DecidingPropagator p;
        s = new Solver(&conf, &must_inter);
        s->connect_external_propagator(&p);
        s->new_vars(8);
        for(uint32_t v = 0; v < 8; v++) s->add_observed_var(v);
        p.nvars = 8;
        p.sign = (sign == 1);
        p.check_is_decision = true;
        p.start(s, 8);

        must_inter.store(false, std::memory_order_relaxed);
        ASSERT_EQ(s->solve_with_assumptions(), l_True);
        EXPECT_EQ(p.num_decisions, 8U);
        for(uint32_t v = 0; v < 8; v++) {
            EXPECT_EQ(s->get_model()[v], sign == 1 ? l_False : l_True) << "var " << v+1;
        }
        delete s; s = nullptr;
    }
}

TEST_F(UserPropDecideTest, cb_decide_is_ignored_for_assigned_literals)
{
    // keeps asking for var 1, which a unit has already fixed
    class Stubborn : public DecidingPropagator {
    public:
        Lit cb_decide() override { num_decisions++; return Lit(0, false); }
    } p;

    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(6);
    s->add_clause_outside(str_to_cl("1"));
    for(uint32_t v = 0; v < 6; v++) s->add_observed_var(v);
    p.nvars = 6;
    p.start(s, 6);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_GT(p.num_decisions, 0U);
    EXPECT_EQ(s->get_model()[0], l_True);
}

TEST_F(UserPropDecideTest, model_enumeration_through_cb_check_found_model)
{
    const uint32_t nvars = 8;
    auto cls = gen_3sat(nvars, 10, 21);
    const uint32_t expected = count_models(cls, nvars);
    ASSERT_GT(expected, 0U);

    EnumeratingPropagator p;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(nvars);
    for(const auto& cl: cls) { vector<Lit> tmp = cl; s->add_clause_outside(tmp); }
    for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
    p.start(s, nvars);

    must_inter.store(false, std::memory_order_relaxed);
    // every model is rejected and blocked
    EXPECT_EQ(s->solve_with_assumptions(), l_False);
    EXPECT_EQ(p.models.size(), expected);

    vector<vector<Lit>> seen = p.models;
    std::sort(seen.begin(), seen.end());
    EXPECT_EQ(std::unique(seen.begin(), seen.end()), seen.end());
}

TEST_F(UserPropDecideTest, a_lazy_propagator_is_still_asked_for_clauses)
{
    // A lazy propagator can only reject models, which is useless unless it can
    // then hand over the blocking clause.
    const uint32_t nvars = 8;
    auto cls = gen_3sat(nvars, 10, 21);
    const uint32_t expected = count_models(cls, nvars);
    ASSERT_GT(expected, 0U);

    EnumeratingPropagator p;
    p.is_lazy = true;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(nvars);
    for(const auto& cl: cls) { vector<Lit> tmp = cl; s->add_clause_outside(tmp); }
    for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
    p.start(s, nvars);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_False);
    EXPECT_EQ(p.models.size(), expected);
    EXPECT_EQ(p.max_level_seen, 0U);
    EXPECT_EQ(p.num_backtracks, 0U);
}

TEST_F(UserPropDecideTest, rejecting_without_a_clause_is_taken_as_acceptance)
{
    class AlwaysNo : public MirrorPropagator {
    public:
        uint32_t num_rejections = 0;
        bool cb_check_found_model(const vector<Lit>&) override {
            num_rejections++;
            return false;
        }
    } p;

    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(5);
    s->add_clause_outside(str_to_cl("1, 2"));
    for(uint32_t v = 0; v < 5; v++) s->add_observed_var(v);
    p.start(s, 5);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_EQ(p.num_rejections, 1U);
}

TEST_F(UserPropDecideTest, force_backtrack_from_cb_decide)
{
    BacktrackingPropagator p;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(40);
    auto cls = gen_3sat(40, 120, 13);
    for(const auto& cl: cls) { vector<Lit> tmp = cl; s->add_clause_outside(tmp); }
    for(uint32_t v = 0; v < 40; v++) s->add_observed_var(v);
    p.raw = s;
    p.budget = 25;
    p.start(s, 40);

    must_inter.store(false, std::memory_order_relaxed);
    const lbool ret = s->solve_with_assumptions();
    EXPECT_NE(ret, l_Undef);
    EXPECT_GT(p.num_forced, 0U);
    EXPECT_GT(p.num_backtracks, 0U);
}

// Forces a backtrack and returns a decision with it: the negation of a literal
// from above the target level, so it can only be ours.
class BacktrackAndDecidePropagator : public MirrorPropagator
{
public:
    Solver* raw = nullptr;
    uint32_t budget = 0;
    Lit expected = lit_Undef;
    bool awaiting = false;
    uint32_t num_honoured = 0;
    uint32_t num_asked_again = 0;

    Lit cb_decide() override {
        if (awaiting) { num_asked_again++; return lit_Undef; }
        if (budget == 0 || stack.size() <= 4 || stack[3].empty()) return lit_Undef;
        budget--;
        const Lit above = stack[3][0]; // undone by the backtrack
        raw->ext_force_backtrack(1);
        expected = ~above;
        awaiting = true;
        return expected;
    }

    void notify_assignment(const vector<Lit>& lits) override {
        MirrorPropagator::notify_assignment(lits);
        if (!awaiting) return;
        // the next assignment is our decision, on level 2
        awaiting = false;
        EXPECT_EQ(stack.size(), 3U);
        EXPECT_EQ(lits[0], expected);
        EXPECT_TRUE(raw->ext_is_decision(expected));
        if (lits[0] == expected) num_honoured++;
    }
};

TEST_F(UserPropDecideTest, a_decision_handed_over_with_a_forced_backtrack_is_made)
{
    BacktrackAndDecidePropagator p;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(40);
    auto cls = gen_3sat(40, 120, 13);
    for(const auto& cl: cls) { vector<Lit> tmp = cl; s->add_clause_outside(tmp); }
    for(uint32_t v = 0; v < 40; v++) s->add_observed_var(v);
    p.raw = s;
    p.budget = 10;
    p.start(s, 40);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_NE(s->solve_with_assumptions(), l_Undef);
    EXPECT_GT(p.num_honoured, 0U);
    EXPECT_EQ(p.num_honoured, s->ext_stats.forced_backtracks);
    EXPECT_EQ(p.num_asked_again, 0U);
}

TEST_F(UserPropDecideTest, force_backtrack_is_ignored_outside_the_callbacks)
{
    NoopPropagator p;
    SATSolver api;
    api.new_vars(5);
    api.add_clause(str_to_cl("1, 2"));
    api.connect_external_propagator(&p);
    api.add_observed_var(0);
    api.force_backtrack(0);
    EXPECT_EQ(api.solve(), l_True);
}

}

// ---- Lazy reason clauses ----

namespace CMSat {

struct UserPropLazyTest : public UserPropPropagateTest {};

TEST_F(UserPropLazyTest, lazy_and_eager_agree_over_many_seeds)
{
    size_t eager_explanations = 0;
    size_t eager_propagations = 0;
    size_t lazy_explanations = 0;
    size_t lazy_propagations = 0;

    for(uint32_t seed = 1; seed <= 15; seed++) {
        const uint32_t nvars = 30;
        auto cls = gen_3sat(nvars, 125, seed);
        const lbool expected = solve_plain(cls, nvars);

        for(int lazy = 0; lazy < 2; lazy++) {
            p = UnitPropagator();
            ASSERT_EQ(solve_with_theory(cls, nvars, cls.size()/2, lazy == 1), expected)
                << "seed " << seed << " lazy " << lazy;
            if (expected == l_True) {
                EXPECT_TRUE(model_satisfies(s->get_model(), cls))
                    << "seed " << seed << " lazy " << lazy;
            }
            // an explanation is only ever asked for once per propagation
            EXPECT_LE(p.num_explanations, p.num_propagations);
            if (lazy) {
                lazy_explanations += p.num_explanations;
                lazy_propagations += p.num_propagations;
            } else {
                EXPECT_EQ(p.num_explanations, p.num_propagations);
                eager_explanations += p.num_explanations;
                eager_propagations += p.num_propagations;
            }
        }
    }

    EXPECT_GT(eager_explanations, 0U);
    EXPECT_GT(lazy_propagations, 0U);
    EXPECT_EQ(eager_explanations, eager_propagations);
    // most propagations are never explained
    EXPECT_LT(lazy_explanations, lazy_propagations);
    EXPECT_LT((double)lazy_explanations/(double)lazy_propagations, 0.9);
}

TEST_F(UserPropLazyTest, lazy_reasons_are_actually_used_in_conflict_analysis)
{
    const uint32_t nvars = 25;
    auto cls = gen_3sat(nvars, 100, 9);
    const lbool expected = solve_plain(cls, nvars);
    ASSERT_EQ(solve_with_theory(cls, nvars, 0, true), expected);
    // the whole problem is in the propagator, so reasons must be asked for
    EXPECT_GT(p.num_explanations, 0U);
    EXPECT_LT(p.num_explanations, p.num_propagations);
}

TEST_F(UserPropLazyTest, explanations_are_lazy_by_default)
{
    // as in CaDiCaL; only proof logging switches it off
    EXPECT_TRUE(SolverConf().ext_lazy_reasons);

    const uint32_t nvars = 25;
    auto cls = gen_3sat(nvars, 100, 9);
    const lbool expected = solve_plain(cls, nvars);

    delete s;
    conf = SolverConf();
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(nvars);
    for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
    p.theory = cls;
    p.start_theory(s, nvars);
    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(), expected);

    EXPECT_GT(s->ext_stats.props, 0U);
    EXPECT_GT(s->ext_stats.props_lazy, 0U);
    EXPECT_EQ(s->ext_stats.explanations, p.num_explanations);
    EXPECT_LT(p.num_explanations, p.num_propagations);
}

// Resets observation right after lazily propagating x -> y. Without a
// backtrack, y would keep a lazy reason that can no longer be asked for.
class ResetsAfterLazyPropagation : public ExternalPropagator
{
public:
    Solver* raw = nullptr;
    vector<vector<Lit>> stack = vector<vector<Lit>>(1);
    size_t next_reason_lit = 0;
    uint32_t explanations = 0;
    bool x_is_true = false;
    bool propagated_y = false;
    bool reset_done = false;

    void notify_assignment(const vector<Lit>& lits) override {
        for(const Lit l: lits) {
            stack.back().push_back(l);
            if (l == Lit(0, false)) x_is_true = true;
        }
    }
    void notify_new_decision_level() override { stack.push_back({}); }
    void notify_backtrack(size_t new_level) override {
        for(size_t level = new_level + 1; level < stack.size(); level++) {
            for(const Lit l: stack[level]) {
                if (l.var() == 0) x_is_true = false;
            }
        }
        stack.resize(new_level + 1);
    }
    bool cb_check_found_model(const vector<Lit>&) override { return true; }
    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return false;
    }
    Lit cb_add_external_clause_lit() override { return lit_Undef; }

    Lit cb_propagate() override {
        if (!propagated_y && x_is_true) {
            propagated_y = true;
            return Lit(1, false);
        }
        if (propagated_y && !reset_done) {
            reset_done = true;
            raw->reset_observed_vars();
        }
        return lit_Undef;
    }
    Lit cb_add_reason_clause_lit(Lit propagated_lit) override {
        explanations++;
        EXPECT_EQ(propagated_lit, Lit(1, false));
        const Lit reason[] = {Lit(1, false), Lit(0, true)};
        if (next_reason_lit == 2) {
            next_reason_lit = 0;
            return lit_Undef;
        }
        return reason[next_reason_lit++];
    }
};

TEST_F(UserPropLazyTest, resetting_observation_retires_live_lazy_reasons)
{
    conf.ext_lazy_reasons = true;
    ResetsAfterLazyPropagation rp;
    delete s;
    s = new Solver(&conf, &must_inter);
    rp.raw = s;
    s->connect_external_propagator(&rp);
    s->new_vars(4); // x, y, a, b
    s->add_clause_outside(str_to_cl("-2, -3, 4"));
    s->add_clause_outside(str_to_cl("-2, -3, -4"));
    for(uint32_t v = 0; v < 4; v++) s->add_observed_var(v);

    vector<Lit> assumptions = {Lit(0, false), Lit(2, false)};
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(&assumptions), l_True);
    EXPECT_TRUE(rp.propagated_y);
    EXPECT_TRUE(rp.reset_done);
    EXPECT_EQ(rp.explanations, 0U);
    for(uint32_t v = 0; v < 4; v++) EXPECT_FALSE(s->is_observed_var(v));
}

// Lazily propagates x -> y and x -> z, repeating the propagated literal in
// each reason; a duplicate pivot left in place breaks first-UIP analysis.
class DuplicatePivotReasonPropagator : public ExternalPropagator
{
public:
    vector<vector<Lit>> stack = vector<vector<Lit>>(1);
    size_t next_reason_lit = 0;
    uint32_t explanations = 0;
    bool x_is_true = false;
    bool propagated_y = false;
    bool propagated_z = false;

    void notify_assignment(const vector<Lit>& lits) override {
        for(const Lit l: lits) {
            stack.back().push_back(l);
            if (l.var() == 0) x_is_true = !l.sign();
        }
    }
    void notify_new_decision_level() override { stack.push_back({}); }
    void notify_backtrack(size_t new_level) override {
        for(size_t level = new_level + 1; level < stack.size(); level++) {
            for(const Lit l: stack[level]) {
                if (l.var() == 0) x_is_true = false;
            }
        }
        stack.resize(new_level + 1);
    }
    bool cb_check_found_model(const vector<Lit>&) override { return true; }
    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return false;
    }
    Lit cb_add_external_clause_lit() override { return lit_Undef; }

    Lit cb_decide() override {
        if (!propagated_y) return Lit(0, false);
        return lit_Undef;
    }
    Lit cb_propagate() override {
        if (!x_is_true) return lit_Undef;
        if (!propagated_y) {
            propagated_y = true;
            return Lit(1, false);
        }
        if (!propagated_z) {
            propagated_z = true;
            return Lit(2, false);
        }
        return lit_Undef;
    }
    Lit cb_add_reason_clause_lit(Lit propagated_lit) override {
        if (next_reason_lit == 0) explanations++;
        if (next_reason_lit < 2) {
            next_reason_lit++;
            return propagated_lit;
        }
        if (next_reason_lit++ == 2) return Lit(0, true);
        next_reason_lit = 0;
        return lit_Undef;
    }
};

TEST_F(UserPropLazyTest, duplicate_pivot_is_removed_from_lazy_reason)
{
    conf.ext_lazy_reasons = true;
    DuplicatePivotReasonPropagator dp;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&dp);
    s->new_vars(4); // x, y, z, a
    s->add_clause_outside(str_to_cl("-2, -3, 4"));
    s->add_clause_outside(str_to_cl("-2, -3, -4"));
    for(uint32_t v = 0; v < 4; v++) s->add_observed_var(v);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_TRUE(dp.propagated_y);
    EXPECT_TRUE(dp.propagated_z);
    EXPECT_GT(dp.explanations, 0U);
    EXPECT_EQ(s->get_model()[0], l_False);
}

TEST_F(UserPropLazyTest, lazy_is_ignored_under_frat)
{
    // Reason clauses have to be in the proof, so they are asked for eagerly.
    const uint32_t nvars = 24;
    auto cls = gen_3sat(nvars, 180, 5);
    const lbool expected = solve_plain(cls, nvars);
    ASSERT_EQ(expected, l_False);

    const char* fname = "user_prop_test_lazy.frat";
    FILE* f = fopen(fname, "wb");
    ASSERT_NE(f, nullptr);

    conf.ext_lazy_reasons = true;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->add_frat(f);
    s->connect_external_propagator(&p);
    s->new_vars(nvars);
    for(size_t i = 0; i < 90; i++) {
        vector<Lit> tmp = cls[i];
        s->add_clause_outside(tmp);
    }
    for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
    for(size_t i = 90; i < cls.size(); i++) p.theory.push_back(cls[i]);
    p.start_theory(s, nvars);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_False);
    EXPECT_EQ(p.num_explanations, p.num_propagations);
    delete s; s = nullptr;
    fclose(f);
    std::remove(fname);
}

TEST_F(UserPropLazyTest, lazy_reason_in_a_failed_assumption_core)
{
    // Assume 1 and -2 with 1 -> 2 in the propagator: -2 is contradicted by a
    // lazy propagation whose reason the final-conflict analysis must fetch.
    conf.ext_lazy_reasons = true;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&p);
    s->new_vars(6);
    s->add_clause_outside(str_to_cl("3, 4"));
    for(uint32_t v = 0; v < 6; v++) s->add_observed_var(v);
    p.theory.push_back(str_to_cl("-1, 2"));
    p.start_theory(s, 6);

    vector<Lit> assumps = {Lit(0, false), Lit(1, true)};
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(&assumps), l_False);
    EXPECT_GT(p.num_propagations, 0U);
    EXPECT_FALSE(s->conflict.empty());
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
}

TEST_F(UserPropLazyTest, lazy_with_non_recursive_clause_minimisation)
{
    // normalClMinim() reads reasons too
    conf.do_recursive_minim = false;
    for(uint32_t seed = 1; seed <= 8; seed++) {
        const uint32_t nvars = 30;
        auto cls = gen_3sat(nvars, 125, seed);
        const lbool expected = solve_plain(cls, nvars);
        p = UnitPropagator();
        conf.do_recursive_minim = false;
        ASSERT_EQ(solve_with_theory(cls, nvars, cls.size()/2, true), expected)
            << "seed " << seed;
        if (expected == l_True) {
            EXPECT_TRUE(model_satisfies(s->get_model(), cls)) << "seed " << seed;
        }
    }
}

TEST_F(UserPropLazyTest, lazy_with_the_whole_problem_in_the_propagator)
{
    for(uint32_t seed = 1; seed <= 8; seed++) {
        const uint32_t nvars = 22;
        auto cls = gen_3sat(nvars, 92, seed);
        const lbool expected = solve_plain(cls, nvars);
        p = UnitPropagator();
        ASSERT_EQ(solve_with_theory(cls, nvars, 0, true), expected) << "seed " << seed;
        if (expected == l_True) {
            EXPECT_TRUE(model_satisfies(s->get_model(), cls)) << "seed " << seed;
        }
    }
}

// Propagates one literal lazily, then unobserves a root-fixed var named in its
// reason. Not a MirrorPropagator: the dropped var would upset the mirror.
class DropsAFixedVarPropagator : public ExternalPropagator
{
public:
    Solver* raw = nullptr;
    vector<lbool> value_of;      // indexed by outer var
    vector<vector<Lit>> stack;   // so that backtracking undoes assignments
    vector<Lit> reason;          // OUTER, reason[0] is the literal it explains
    uint32_t drop_var = 0;
    size_t next_lit = 0;
    bool propagated = false;
    uint32_t num_explanations = 0;

    void start(Solver* _raw, uint32_t nvars) {
        raw = _raw;
        value_of.assign(nvars, l_Undef);
        stack.assign(1, {});
    }
    lbool val(const Lit l) const {
        const lbool v = value_of[l.var()];
        if (v == l_Undef) return l_Undef;
        return l.sign() ? (v == l_True ? l_False : l_True) : v;
    }

    void notify_assignment(const vector<Lit>& lits) override {
        for(const Lit l: lits) {
            value_of[l.var()] = l.sign() ? l_False : l_True;
            stack.back().push_back(l);
        }
    }
    void notify_new_decision_level() override { stack.push_back({}); }
    void notify_backtrack(size_t new_level) override {
        for(size_t i = new_level+1; i < stack.size(); i++) {
            for(const Lit l: stack[i]) value_of[l.var()] = l_Undef;
        }
        stack.resize(new_level+1);
    }
    bool cb_check_found_model(const vector<Lit>&) override { return true; }
    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return false;
    }
    Lit cb_add_external_clause_lit() override { return lit_Undef; }

    Lit cb_propagate() override {
        if (propagated || val(reason[0]) != l_Undef) return lit_Undef;
        for(size_t i = 1; i < reason.size(); i++) {
            if (val(reason[i]) != l_False) return lit_Undef;
        }
        propagated = true;
        raw->remove_observed_var(drop_var);
        return reason[0];
    }
    Lit cb_add_reason_clause_lit(Lit) override {
        if (next_lit == 0) num_explanations++;
        if (next_lit == reason.size()) { next_lit = 0; return lit_Undef; }
        return reason[next_lit++];
    }
};

TEST_F(UserPropLazyTest, a_lazy_reason_may_name_a_dropped_root_fixed_variable)
{
    // 1 is a root unit; 1 & 2 -> 3 is propagated lazily as 1 is dropped.
    // Assuming -3 then needs that reason, which is still valid.
    conf.ext_lazy_reasons = true;
    DropsAFixedVarPropagator dp;
    dp.reason = str_to_cl("3, -1, -2", false); // keep reason[0] first
    dp.drop_var = 0;

    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&dp);
    s->new_vars(6);
    s->add_clause_outside(str_to_cl("1"));
    s->add_clause_outside(str_to_cl("4, 5"));
    for(uint32_t v = 0; v < 6; v++) s->add_observed_var(v);
    dp.start(s, 6);

    vector<Lit> assumps = {Lit(1, false), Lit(2, true)};
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(&assumps), l_False);
    EXPECT_TRUE(dp.propagated);
    EXPECT_GT(dp.num_explanations, 0U);
    EXPECT_FALSE(s->is_observed_var(0));
    EXPECT_FALSE(s->conflict.empty());
    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
}

// Propagates reason[0] on a fixed level without checking the reason.
class SloppyReasonPropagator : public ExternalPropagator
{
public:
    vector<Lit> reason;      // OUTER, reason[0] is the literal it explains
    uint32_t propagate_at = 1;
    uint32_t level = 0;
    size_t next_lit = 0;
    bool propagated = false;

    void notify_assignment(const vector<Lit>&) override {}
    void notify_new_decision_level() override { level++; }
    void notify_backtrack(size_t new_level) override { level = (uint32_t)new_level; }
    bool cb_check_found_model(const vector<Lit>&) override { return true; }
    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return false;
    }
    Lit cb_add_external_clause_lit() override { return lit_Undef; }

    Lit cb_propagate() override {
        if (propagated || level != propagate_at) return lit_Undef;
        propagated = true;
        return reason[0];
    }
    Lit cb_add_reason_clause_lit(Lit) override {
        if (next_lit == reason.size()) { next_lit = 0; return lit_Undef; }
        return reason[next_lit++];
    }
};

// Propagates 3 with reason 2 & 4 -> 3 before 4 is assigned: when asked for,
// the reason is falsified but names a later assignment.
static void solve_with_a_sloppy_reason(SolverConf& conf)
{
    SloppyReasonPropagator sp;
    sp.reason = str_to_cl("3, -2, -4", false);
    std::atomic<bool> inter;
    inter.store(false, std::memory_order_relaxed);
    Solver solver(&conf, &inter);
    solver.connect_external_propagator(&sp);
    solver.new_vars(6);
    solver.add_clause_outside(str_to_cl("5, 6"));
    for(uint32_t v = 0; v < 6; v++) solver.add_observed_var(v);
    vector<Lit> as = {Lit(1, false), Lit(3, false), Lit(2, true)};
    solver.solve_with_assumptions(&as);
}

TEST_F(UserPropLazyTest, a_reason_naming_a_later_assignment_is_caught)
{
    conf.ext_lazy_reasons = true;
    EXPECT_DEATH(solve_with_a_sloppy_reason(conf),
                 "assigned after the one it explains");
}

// Observes a var from inside cb_add_reason_clause_lit(), which is forbidden
// whether asked lazily (mid conflict analysis) or eagerly (may renumber).
class ObservesWhileExplainingPropagator : public UnitPropagator
{
public:
    uint32_t to_observe = 0;
    bool only_when_asked_lazily = false;

    Lit cb_add_reason_clause_lit(Lit propagated_lit) override {
        if (cur_lit == 0) {
            // lazily: the literal is already true with a placeholder reason
            const Lit il = s->map_outer_to_inter(propagated_lit);
            const bool asked_lazily =
                s->value(il) == l_True && s->var_data[il.var()].reason.is_ext();
            if (!only_when_asked_lazily || asked_lazily) s->add_observed_var(to_observe);
        }
        return UnitPropagator::cb_add_reason_clause_lit(propagated_lit);
    }
};

static void observe_while_explaining(SolverConf& conf, bool only_when_asked_lazily)
{
    const uint32_t nvars = 25;
    auto cls = gen_3sat(nvars, 100, 9);
    ObservesWhileExplainingPropagator op;
    op.to_observe = nvars; // exists, but the theory never mentions it
    op.only_when_asked_lazily = only_when_asked_lazily;
    std::atomic<bool> inter;
    inter.store(false, std::memory_order_relaxed);
    Solver solver(&conf, &inter);
    solver.connect_external_propagator(&op);
    solver.new_vars(nvars + 1);
    for(uint32_t v = 0; v < nvars; v++) solver.add_observed_var(v);
    op.theory = cls;
    op.start_theory(&solver, nvars + 1);
    solver.solve_with_assumptions();
}

TEST_F(UserPropLazyTest, observing_while_a_reason_is_being_asked_for_is_caught)
{
    // only when asked lazily, from inside conflict analysis
    conf.ext_lazy_reasons = true;
    EXPECT_DEATH(observe_while_explaining(conf, true),
                 "while a reason clause is being asked for");
}

TEST_F(UserPropLazyTest, observing_while_a_reason_is_asked_for_eagerly_is_caught_too)
{
    conf.ext_lazy_reasons = false;
    EXPECT_DEATH(observe_while_explaining(conf, false),
                 "while a reason clause is being asked for");
}

}

// ---- Everything at once ----

namespace CMSat {

struct UserPropAdversaryTest : public ::testing::Test {
    UserPropAdversaryTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropAdversaryTest() { delete s; delete ref; }

    lbool solve_plain(const vector<vector<Lit>>& cls, uint32_t nvars) {
        delete ref;
        ref = new Solver(&conf, &must_inter);
        ref->new_vars(nvars);
        for(const auto& cl: cls) { vector<Lit> tmp = cl; ref->add_clause_outside(tmp); }
        must_inter.store(false, std::memory_order_relaxed);
        return ref->solve_with_assumptions();
    }

    lbool solve_adversarially(const vector<vector<Lit>>& cls, uint32_t nvars,
                              size_t split, uint32_t seed, bool lazy)
    {
        delete s;
        conf.ext_lazy_reasons = lazy;
        s = new Solver(&conf, &must_inter);
        s->connect_external_propagator(&p);
        s->new_vars(nvars);
        for(size_t i = 0; i < split; i++) {
            vector<Lit> tmp = cls[i];
            s->add_clause_outside(tmp);
        }
        // the propagator observes the rest when checking a model
        for(uint32_t v = 0; v < nvars; v++) if ((v + seed) % 3 != 0) s->add_observed_var(v);

        p.theory.clear();
        for(size_t i = split; i < cls.size(); i++) p.theory.push_back(cls[i]);
        p.start_adversary(s, nvars, seed);
        p.backtrack_budget = 40;

        must_inter.store(false, std::memory_order_relaxed);
        return s->solve_with_assumptions();
    }

    SolverConf conf;
    Solver* s = nullptr;
    Solver* ref = nullptr;
    AdversarialPropagator p;
    std::atomic<bool> must_inter;
};

TEST_F(UserPropAdversaryTest, everything_at_once_under_frat)
{
    for(uint32_t seed = 200; seed <= 204; seed++) {
        const uint32_t nvars = 26;
        auto cls = gen_3sat(nvars, 108, seed);
        const lbool expected = solve_plain(cls, nvars);

        const char* fname = "user_prop_adversary.frat";
        FILE* f = fopen(fname, "wb");
        ASSERT_NE(f, nullptr);

        delete s;
        s = new Solver(&conf, &must_inter);
        s->add_frat(f);
        s->connect_external_propagator(&p);
        s->new_vars(nvars);
        const size_t split = cls.size()*2/3;
        for(size_t i = 0; i < split; i++) { vector<Lit> tmp = cls[i]; s->add_clause_outside(tmp); }
        for(uint32_t v = 0; v < nvars; v++) if ((v + seed) % 3 != 0) s->add_observed_var(v);
        p = AdversarialPropagator();
        for(size_t i = split; i < cls.size(); i++) p.theory.push_back(cls[i]);
        p.start_adversary(s, nvars, seed);
        p.backtrack_budget = 40;

        must_inter.store(false, std::memory_order_relaxed);
        EXPECT_EQ(s->solve_with_assumptions(), expected) << "seed " << seed;
        delete s; s = nullptr;
        fclose(f);
        std::remove(fname);
    }
}

}

// ---- API calls that touch the trail outside the search ----

namespace CMSat {

struct UserPropOtherApiTest : public ::testing::Test {
    UserPropOtherApiTest() { must_inter.store(false, std::memory_order_relaxed); }
    ~UserPropOtherApiTest() { delete s; }

    void setup(uint32_t nvars, uint32_t ncls, uint32_t seed) {
        s = new Solver(&conf, &must_inter);
        s->connect_external_propagator(&p);
        add_random_3sat(s, nvars, ncls, seed);
        for(uint32_t v = 0; v < nvars; v++) s->add_observed_var(v);
        p.start(s, nvars);
        must_inter.store(false, std::memory_order_relaxed);
        ASSERT_EQ(s->solve_with_assumptions(), l_True);
        // back at the root: the propagator's stack must be level 0 only
        ASSERT_EQ(p.stack.size(), 1U);
    }

    SolverConf conf;
    Solver* s = nullptr;
    MirrorPropagator p;
    std::atomic<bool> must_inter;
};

// Unobserves assigned variables from cb_decide(), 'budget' times.
class UnobservingPropagator : public MirrorPropagator
{
public:
    Solver* raw = nullptr;
    uint32_t budget = 0;
    uint32_t next_var = 0;
    uint32_t num_removed = 0;

    Lit cb_decide() override {
        if (budget > 0 && stack.size() > 2) {
            for(uint32_t tries = 0; tries < 32; tries++) {
                const uint32_t v = next_var++ % assigned.size();
                if (!raw->is_observed_var(v) || !assigned[v]) continue;
                budget--;
                num_removed++;
                raw->remove_observed_var(v);
                break;
            }
        }
        return lit_Undef;
    }
};

// Observes an eliminated variable from the model callback.
class ObservesEliminatedAtModelPropagator : public NoopPropagator
{
public:
    SATSolver* raw = nullptr;
    uint32_t var = 2;
    bool observed_it = false;
    bool saw_it_in_a_model = false;

    bool cb_check_found_model(const vector<Lit>& model) override {
        NoopPropagator::cb_check_found_model(model);
        if (!observed_it) {
            EXPECT_TRUE(raw->removed_var(var));
            raw->add_observed_var(var);
            observed_it = true;
            EXPECT_FALSE(raw->removed_var(var));
            return false; // the trail moved anyway
        }
        for(const Lit l: model) if (l.var() == var) saw_it_in_a_model = true;
        return true;
    }
};

TEST_F(UserPropOtherApiTest, eliminated_var_is_uneliminated_when_observed_during_solving)
{
    // BVE eliminates var 3. No renumbering, so it stays in the search's range
    // and observing it must backtrack and uneliminate by itself.
    SATSolver api;
    ObservesEliminatedAtModelPropagator ep;
    ep.raw = &api;
    api.set_renumber(false);
    const uint32_t nvars = 30;
    api.new_vars(nvars);
    vector<vector<Lit>> cls = {str_to_cl("1, 3"), str_to_cl("2, -3")};
    for(const auto& cl: gen_3sat(nvars, 110, 21)) {
        bool touches = false;
        for(const Lit l: cl) if (l.var() == 2) touches = true;
        if (!touches) cls.push_back(cl);
    }
    for(const auto& cl: cls) api.add_clause(cl);
    std::string strategy = "occ-bve";
    ASSERT_EQ(api.simplify(nullptr, &strategy), l_Undef);
    ASSERT_TRUE(api.removed_var(2));

    api.connect_external_propagator(&ep);
    for(uint32_t v = 0; v < nvars; v++) if (v != 2) api.add_observed_var(v);

    ASSERT_EQ(api.solve(), l_True);
    EXPECT_TRUE(ep.observed_it);
    EXPECT_TRUE(ep.saw_it_in_a_model);
    EXPECT_TRUE(api.is_observed_var(2));
    EXPECT_FALSE(api.removed_var(2));
    EXPECT_TRUE(model_satisfies(api.get_model(), cls));
}

TEST_F(UserPropOtherApiTest, remove_observed_var_during_solving)
{
    UnobservingPropagator up;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&up);
    add_random_3sat(s, 60, 240, 5);
    for(uint32_t v = 0; v < 60; v++) s->add_observed_var(v);
    up.raw = s;
    up.budget = 25;
    up.start(s, 60);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_NE(s->solve_with_assumptions(), l_Undef);
    EXPECT_GT(up.num_removed, 0U);
    EXPECT_GT(up.num_comparisons, 0U);
}

// Observes or unobserves one variable, once, from cb_decide(). That can
// backtrack, so the solver must not then decide on what is left.
class LateObservingPropagator : public MirrorPropagator
{
public:
    Solver* raw = nullptr;
    uint32_t var = 0;
    uint32_t at_level = 0;      // act once the search is this deep
    bool observe = true;        // observe, or un-observe
    bool done = false;
    uint32_t level_when_done = 0;

    Lit cb_decide() override {
        if (done || raw->decision_level() < at_level) return lit_Undef;
        done = true;
        level_when_done = raw->decision_level();
        if (observe) raw->add_observed_var(var);
        else raw->remove_observed_var(var);
        return lit_Undef;
    }
};

TEST_F(UserPropOtherApiTest, observing_a_fixed_variable_from_cb_decide)
{
    // a root-fixed var must be notified on level 0, before any level opens
    LateObservingPropagator lp;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&lp);
    add_random_3sat(s, 40, 120, 17);
    s->add_clause_outside(str_to_cl("1"));
    for(uint32_t v = 1; v < 40; v++) s->add_observed_var(v);
    lp.raw = s;
    lp.var = 0;
    lp.start(s, 40);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_TRUE(lp.done);
    EXPECT_GT(lp.num_comparisons, 0U);
    ASSERT_EQ(lp.stack.size(), 1U);
    EXPECT_NE(std::find(lp.stack[0].begin(), lp.stack[0].end(), Lit(0, false)),
              lp.stack[0].end()) << "the root assignment was notified at the wrong level";
}

// Observes one variable, once, from inside cb_check_found_model().
class ModelTimeObservingPropagator : public MirrorPropagator
{
public:
    Solver* raw = nullptr;
    uint32_t var = 0;
    bool done = false;
    uint32_t num_models = 0;
    bool seen_in_a_later_model = false;

    bool cb_check_found_model(const vector<Lit>& model) override {
        num_models++;
        compare();
        if (!done) {
            done = true;
            raw->add_observed_var(var);
            return true;
        }
        if (std::find(model.begin(), model.end(), Lit(var, false)) != model.end()) {
            seen_in_a_later_model = true;
        }
        return true;
    }
};

TEST_F(UserPropOtherApiTest, observing_a_fixed_variable_from_cb_check_found_model)
{
    // All vars are root units, so observing one moves nothing; the model must
    // still not be final while its notification is owed.
    ModelTimeObservingPropagator mp;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&mp);
    s->new_vars(8);
    for(uint32_t v = 0; v < 8; v++) {
        vector<Lit> unit = {Lit(v, false)};
        s->add_clause_outside(unit);
    }
    for(uint32_t v = 1; v < 8; v++) s->add_observed_var(v);
    mp.raw = s;
    mp.var = 0;
    mp.start(s, 8);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_TRUE(mp.done);
    EXPECT_GE(mp.num_models, 2U) << "the model was taken as final with a notification owed";
    EXPECT_TRUE(mp.seen_in_a_later_model);
    ASSERT_EQ(mp.stack.size(), 1U);
    EXPECT_NE(std::find(mp.stack[0].begin(), mp.stack[0].end(), Lit(0, false)),
              mp.stack[0].end()) << "the root assignment was never notified";
}

// Observes a variable halfway through handing over an external clause, which
// can change the outer-to-inter mapping mid-clause.
class ObservesWhileReadingPropagator : public MirrorPropagator
{
public:
    Solver* raw = nullptr;
    vector<Lit> clause;      // OUTER numbering
    uint32_t observe_var = 0;
    size_t next_lit = 0;
    bool handed = false;
    bool observed_mid_clause = false;

    bool cb_has_external_clause(bool& is_forgettable) override {
        is_forgettable = false;
        return !handed && stack.size() > 2;
    }
    Lit cb_add_external_clause_lit() override {
        if (next_lit == clause.size()) { next_lit = 0; handed = true; return lit_Undef; }
        if (next_lit == 1 && !raw->is_observed_var(observe_var)) {
            raw->add_observed_var(observe_var);
            observed_mid_clause = true;
        }
        return clause[next_lit++];
    }
};

TEST_F(UserPropOtherApiTest, observing_a_variable_while_reading_an_external_clause)
{
    ObservesWhileReadingPropagator op;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&op);
    add_random_3sat(s, 40, 130, 23);
    for(uint32_t v = 0; v < 39; v++) s->add_observed_var(v);
    op.raw = s;
    op.clause = str_to_cl("1, -2, 3");
    op.observe_var = 39;
    op.start(s, 40);

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
    EXPECT_TRUE(op.handed);
    EXPECT_TRUE(op.observed_mid_clause);
    EXPECT_TRUE(s->is_observed_var(39));
    const vector<lbool>& m = s->get_model();
    EXPECT_TRUE(m[0] == l_True || m[1] == l_False || m[2] == l_True);
}

// (Un)observing an assumption var backtracks into the assumption prefix;
// deciding anything else there would skip an assumption for good.
static void check_assumptions_survive_cb_decide(SolverConf& conf,
    std::atomic<bool>& must_inter, Solver*& s, bool observe)
{
    const uint32_t nvars = 20;
    const uint32_t num_assumps = 5;

    LateObservingPropagator lp;
    delete s;
    // negative polarity, so a skipped assumption ends up falsified
    conf.polarity_mode = PolarityMode::polarmode_neg;
    conf.simplify_at_startup = false;
    conf.do_var_elim = false;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&lp);
    s->new_vars(nvars);

    // assumption vars occur in no clause, so each is decided on its own level
    vector<Lit> assumps;
    for(uint32_t v = 0; v < num_assumps; v++) assumps.push_back(Lit(v, false));
    for(uint32_t i = num_assumps; i + 2 < nvars; i++) {
        s->add_clause_outside(vector<Lit>{Lit(i, false), Lit(i+1, false), Lit(i+2, true)});
        s->add_clause_outside(vector<Lit>{Lit(i, true), Lit(i+1, true), Lit(i+2, false)});
    }

    const uint32_t target = 2;           // the middle assumption
    for(uint32_t v = 0; v < nvars; v++) {
        if (observe && v == target) continue;
        s->add_observed_var(v);
    }
    lp.raw = s;
    lp.var = target;
    lp.at_level = num_assumps + 1;       // past the prefix, so it is complete
    lp.observe = observe;
    lp.start(s, nvars);

    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(&assumps), l_True);
    ASSERT_TRUE(lp.done) << "cb_decide() never got deep enough to act";
    for(const Lit a: assumps) {
        EXPECT_EQ(s->get_model()[a.var()], a.sign() ? l_False : l_True)
            << "assumption " << a << " is not satisfied by the model";
    }
}

TEST_F(UserPropOtherApiTest, observing_an_assumption_from_cb_decide)
{
    check_assumptions_survive_cb_decide(conf, must_inter, s, true);
}

TEST_F(UserPropOtherApiTest, un_observing_an_assumption_from_cb_decide)
{
    check_assumptions_survive_cb_decide(conf, must_inter, s, false);
}

TEST_F(UserPropOtherApiTest, statistics_are_collected)
{
    OraclePropagator op;
    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&op);
    auto cls = gen_3sat(30, 125, 4);
    s->new_vars(30);
    for(size_t i = 0; i < 60; i++) { vector<Lit> tmp = cls[i]; s->add_clause_outside(tmp); }
    for(uint32_t v = 0; v < 30; v++) s->add_observed_var(v);
    for(size_t i = 60; i < cls.size(); i++) op.to_hand_over.push_back(cls[i]);
    op.start(s, 30);

    EXPECT_TRUE(s->ext_stats.empty());
    must_inter.store(false, std::memory_order_relaxed);
    s->solve_with_assumptions();

    EXPECT_FALSE(s->ext_stats.empty());
    EXPECT_GT(s->ext_stats.cb_calls, 0U);
    EXPECT_GT(s->ext_stats.clause_calls, 0U);
    EXPECT_EQ(s->ext_stats.clauses, cls.size() - 60);
    EXPECT_GT(s->ext_stats.model_checks, 0U);
    // the report must run (and survive a zero denominator)
    testing::internal::CaptureStdout();
    s->print_ext_prop_stats();
    const std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("user-prop callbacks"), std::string::npos);
    EXPECT_NE(out.find("user-prop clauses"), std::string::npos);

    // reconnecting resets the counts
    s->disconnect_external_propagator();
    s->connect_external_propagator(&op);
    EXPECT_TRUE(s->ext_stats.empty());
    testing::internal::CaptureStdout();
    s->print_ext_prop_stats();
    EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

TEST_F(UserPropOtherApiTest, is_decision_agrees_with_the_trail_under_assumptions)
{
    // At each model, is_decision() must hold for exactly the decided literals,
    // assumptions included (as in CaDiCaL).
    class Checker : public MirrorPropagator {
    public:
        Solver* raw = nullptr;
        uint32_t num_decisions_seen = 0;
        uint32_t num_checked = 0;
        bool cb_check_found_model(const vector<Lit>& model) override {
            compare();
            num_checked++;
            vector<vector<Lit>> expected;
            if (!raw->ext_get_observed_trail(expected)) return true;
            // the first literal of each level above 0 is that level's decision
            for(size_t lev = 1; lev < expected.size(); lev++) {
                if (expected[lev].empty()) continue;
                EXPECT_TRUE(raw->ext_is_decision(expected[lev][0]))
                    << "level " << lev << " first literal not a decision";
                num_decisions_seen++;
            }
            for(const Lit l: model) {
                // a literal fixed at the root is never a decision
                if (raw->ext_is_decision(l)) {
                    EXPECT_GT(raw->var_data[raw->map_outer_to_inter(l.var())].level, 0U);
                }
            }
            return true;
        }
    } chk;

    delete s;
    s = new Solver(&conf, &must_inter);
    s->connect_external_propagator(&chk);
    add_random_3sat(s, 40, 120, 19);
    for(uint32_t v = 0; v < 40; v++) s->add_observed_var(v);
    chk.raw = s;
    chk.start(s, 40);

    vector<Lit> assumps = {Lit(0, false), Lit(3, true), Lit(7, false)};
    must_inter.store(false, std::memory_order_relaxed);
    ASSERT_EQ(s->solve_with_assumptions(&assumps), l_True);
    EXPECT_GT(chk.num_checked, 0U);
    EXPECT_GT(chk.num_decisions_seen, 0U);
    // back at the root after solve(): nothing is a decision any more
    for(const Lit l: assumps) EXPECT_FALSE(s->ext_is_decision(l)) << "assumption " << l;
}

TEST_F(UserPropOtherApiTest, implied_by_does_not_reach_the_propagator)
{
    setup(40, 100, 7);
    vector<Lit> out;
    s->implied_by(str_to_cl("1, 2"), out);
    EXPECT_EQ(p.stack.size(), 1U) << "phantom decision level leaked from implied_by()";

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
}

TEST_F(UserPropOtherApiTest, minimize_clause_does_not_reach_the_propagator)
{
    setup(40, 100, 11);
    vector<Lit> cl = str_to_cl("1, 2, 3");
    s->minimize_clause(cl);
    EXPECT_EQ(p.stack.size(), 1U) << "phantom decision level leaked from minimize_clause()";

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
}

TEST_F(UserPropOtherApiTest, probe_does_not_reach_the_propagator)
{
    setup(40, 100, 13);
    uint32_t min_props = 0;
    for(uint32_t v = 0; v < 10; v++) s->probe_outside(Lit(v, false), min_props);
    EXPECT_EQ(p.stack.size(), 1U) << "phantom decision level leaked from probe()";

    must_inter.store(false, std::memory_order_relaxed);
    EXPECT_EQ(s->solve_with_assumptions(), l_True);
}

}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

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

/**
IPASIR-UP: the solver side of the external propagator interface (user_prop.h).
See "Satisfiability Modulo User Propagators", Fazekas et al., JAIR 81 (2024).
Everything crossing the interface is in OUTER numbering, the rest is INTER.
*/

#include "solver.h"

#include <algorithm>

#include "constants.h"
#include "searcher.h"
#include "occsimplifier.h"
#include "varreplacer.h"

using namespace CMSat;

void Solver::connect_external_propagator(ExternalPropagator* p)
{
    release_assert(p != nullptr && "Use disconnect_external_propagator() to disconnect");
    release_assert(ext_prop == nullptr &&
        "At most one external propagator can be connected at a time");
    release_assert(decision_level() == 0 &&
        "An external propagator can only be connected outside of solving");

    release_assert(!fast_backw.fast_backw_on &&
        "An external propagator cannot be combined with fast backward subsumption");

    ext_prop = p;
    ext_theory_seen = true;
    //Nothing on the trail is observed yet. Variables observed later while
    //already fixed go through ext_pending_fixed.
    ext_notified = trail.size();
    ext_pending_fixed.clear();
    ext_stats = ExtPropStats();
    verb_print(1, "[user-prop] external propagator connected");
}

void Solver::disconnect_external_propagator()
{
    if (ext_prop == nullptr) return;
    release_assert(decision_level() == 0 &&
        "An external propagator can only be disconnected outside of solving");

    reset_observed_vars();
    ext_prop = nullptr;
    ext_prop_private_steps = false;
    ext_reasons.clear();
    ext_reasons_empty_slots.clear();
    verb_print(1, "[user-prop] external propagator disconnected");
}

void Solver::add_observed_var(const uint32_t outer_var)
{
    release_assert(ext_prop != nullptr &&
        "Cannot observe a variable without a connected external propagator");
    release_assert(outer_var < nVarsOuter() &&
        "Cannot observe a variable that does not exist yet -- call new_vars() first");
    //var_data is never shrunk by renumbering, so this is safe to ask first
    if (var_data[map_outer_to_inter(outer_var)].observed) return;
    release_assert(!ext_frozen() && "The set of observed variables cannot change from"
        " cb_add_reason_clause_lit(), cb_decide_polarity() or notify_backtrack()");

    //Once the clauses are UNSAT there is no more search, and a variable that
    //was renumbered or eliminated away cannot be put back: only record it
    if (!okay()) {
        var_data[map_outer_to_inter(outer_var)].observed = 1;
        ext_observed_vars.push_back(outer_var);
        return;
    }

    //Renumbering may have moved the variable past nVars(): put it back, as
    //add_clause_helper() does
    if (map_outer_to_inter(outer_var) >= nVars()) {
        release_assert(okay());
        cancel_until(0);
        //uneliminate() below inserts an eliminated var into the heap itself
        const bool insert_varorder =
            var_data[map_outer_to_inter(outer_var)].removed == Removed::none;
        new_var(false, outer_var, insert_varorder);
    }

    const uint32_t inter_var = map_outer_to_inter(outer_var);
    release_assert(inter_var < nVars());

    //An eliminated variable must be put back into the search. That needs the
    //root, so inside a callback it costs the search above it.
    if (var_data[inter_var].removed == Removed::elimed) {
        release_assert(okay());
        release_assert(occsimplifier != nullptr);
        cancel_until(0);
        occsimplifier->uneliminate(inter_var);
    }
    if (var_data[inter_var].removed == Removed::replaced) {
        cout << "ERROR: variable " << outer_var+1 << " has been replaced by "
            << var_replacer->get_lit_replaced_with_outer(Lit(outer_var, false))
            << " and can no longer be observed. Observe it before the first"
            " solve(), or call set_no_equivalent_lit_replacement()." << endl;
        release_assert(false);
    }
    release_assert(var_data[inter_var].removed == Removed::none);

    var_data[inter_var].observed = 1;
    ext_observed_vars.push_back(outer_var);

    //The propagator sees a stack, so undo an existing assignment to have it
    //redone and notified normally. Root ones are never undone: queue those.
    if (value(inter_var) != l_Undef) {
        const uint32_t assigned_at = var_data[inter_var].level;
        if (assigned_at > 0) {
            release_assert(assigned_at <= decision_level());
            cancel_until(assigned_at - 1);
        } else {
            cancel_until(0);
            //Queue it, unless its trail entry is still ahead of the cursor
            //(not yet passed over, nor wiped by renumbering)
            const uint32_t sub = var_data[inter_var].sublevel;
            const bool cursor_covers_it =
                sub >= ext_notified && sub < trail.size() &&
                trail[sub].lit.var() == inter_var;
            if (!cursor_covers_it) {
                ext_pending_fixed.push_back(Lit(outer_var, value(inter_var) == l_False));
            }
        }
    }
    verb_print(6, "[user-prop] observing outer var " << outer_var+1);
}

void Solver::remove_observed_var(const uint32_t outer_var)
{
    release_assert(outer_var < nVarsOuter());
    const uint32_t inter_var = map_outer_to_inter(outer_var);
    if (!var_data[inter_var].observed) return;
    release_assert(!ext_frozen() && "The set of observed variables cannot change from"
        " cb_add_reason_clause_lit(), cb_decide_polarity() or notify_backtrack()");

    //Unassign it first: an external propagation over it could no longer be
    //explained once it is un-observed
    if (value(inter_var) != l_Undef && var_data[inter_var].level > 0) {
        cancel_until(var_data[inter_var].level - 1);
    }

    //A queued root notification for it must not be delivered any more
    ext_pending_fixed.erase(
        std::remove_if(ext_pending_fixed.begin(), ext_pending_fixed.end(),
            [outer_var](const Lit l) { return l.var() == outer_var; }),
        ext_pending_fixed.end());

    var_data[inter_var].observed = 0;
    ext_observed_vars.erase(
        std::remove(ext_observed_vars.begin(), ext_observed_vars.end(), outer_var),
        ext_observed_vars.end());
    verb_print(6, "[user-prop] no longer observing outer var " << outer_var+1);
}

void Solver::reset_observed_vars()
{
    release_assert(!ext_frozen() && "The set of observed variables cannot change from"
        " cb_add_reason_clause_lit(), cb_decide_polarity() or notify_backtrack()");
    //Backtrack once, below the earliest non-root observed assignment, so every
    //lazy external propagation is undone while its reason can still be asked
    //for. Root external propagations are always explained eagerly.
    uint32_t backtrack_to = decision_level();
    for(const uint32_t outer_var: ext_observed_vars) {
        const uint32_t inter_var = map_outer_to_inter(outer_var);
        if (value(inter_var) != l_Undef && var_data[inter_var].level > 0) {
            backtrack_to = std::min(backtrack_to, var_data[inter_var].level - 1);
        }
    }
    if (backtrack_to < decision_level()) cancel_until(backtrack_to);

    for(const uint32_t outer_var: ext_observed_vars) {
        var_data[map_outer_to_inter(outer_var)].observed = 0;
    }
    ext_observed_vars.clear();
    ext_pending_fixed.clear();
}

bool Solver::is_observed_var(const uint32_t outer_var) const
{
    if (outer_var >= nVarsOuter()) return false;
    return var_data[map_outer_to_inter(outer_var)].observed;
}

bool Solver::ext_get_observed_trail(vector<vector<Lit>>& out) const
{
    out.clear();
    out.resize(decision_level() + 1);

    //Level 0 is rebuilt from the assignments, as renumbering keeps only the
    //trail's length. Sorted, for a canonical order.
    for(const uint32_t outer_var: ext_observed_vars) {
        const uint32_t v = map_outer_to_inter(outer_var);
        if (value(v) == l_Undef || var_data[v].level != 0) continue;
        out[0].push_back(Lit(outer_var, value(v) == l_False));
    }
    std::sort(out[0].begin(), out[0].end());

    for(const auto& t: trail) {
        if (t.lev == 0 || t.lit == lit_Undef) continue;
        if (!var_data[t.lit.var()].observed) continue;
        assert(t.lev < out.size());
        out[t.lev].push_back(map_inter_to_outer(t.lit));
    }
    //A lazy propagator is never notified, so it has no view to compare
    return ext_notify_active()
        && ext_notified == trail.size() && ext_pending_fixed.empty();
}

bool Solver::ext_is_decision(const Lit outer_lit) const
{
    release_assert(outer_lit.var() < nVarsOuter());
    const uint32_t v = map_outer_to_inter(outer_lit.var());
    if (value(v) == l_Undef) return false;
    return var_data[v].level > 0 && var_data[v].reason == PropBy();
}

void Solver::ext_force_backtrack(const uint32_t new_level)
{
    if (!ext_forced_backtrack_allowed) {
        verb_print(2, "[user-prop] force_backtrack() outside of cb_decide()/"
            "cb_check_found_model(), ignoring");
        return;
    }
    if (new_level >= decision_level()) {
        verb_print(2, "[user-prop] force_backtrack(" << new_level << ") is not below the "
            "current decision level " << decision_level() << ", ignoring");
        return;
    }
    ext_forced_backtrack_set = true;
    ext_forced_backtrack_level = new_level;
}

void Solver::ext_phase(const Lit outer_lit)
{
    release_assert(outer_lit.var() < nVarsOuter());
    auto& vd = var_data[map_outer_to_inter(outer_lit.var())];
    vd.forced_polarity_set = 1;
    vd.forced_polarity = !outer_lit.sign();
}

void Solver::ext_unphase(const uint32_t outer_var)
{
    release_assert(outer_var < nVarsOuter());
    var_data[map_outer_to_inter(outer_var)].forced_polarity_set = 0;
}

////////////////////////////
// Adding external clauses during the CDCL loop (Algorithm 3 of the paper)
//
// Rather than repairing assignment levels in place as CaDiCaL does, we
// backtrack to the level of the second watch: the clause is then satisfied,
// propagating or conflicting, with the watch invariant intact. Unlike CaDiCaL
// this includes a clause satisfied only above its falsified literals, which
// would otherwise silently become unit once that literal is backtracked over.
////////////////////////////

namespace {
//Watch quality, higher is better: satisfied, then free, then falsified (latest
//first). Among satisfied literals the earliest is best, as it survives
//backtracking longest; CaDiCaL's move_literals_to_watch() does the same.
inline uint64_t ext_watch_rank(const lbool val, const uint32_t level)
{
    if (val == l_True)  return (3ULL << 32) | (numeric_limits<uint32_t>::max() - level);
    if (val == l_Undef) return (2ULL << 32);
    return (1ULL << 32) | level;
}
}

PropBy Searcher::ext_attach_clause(const int32_t ID, const bool red)
{
    assert(ext_cl.size() >= 2);
    if (ext_cl.size() == 2) {
        solver->attach_bin_clause(ext_cl[0], ext_cl[1], red, ID, false);
        return PropBy(ext_cl[1], red, ID);
    }

    Clause* cl = cl_alloc.Clause_new(ext_cl, sum_conflicts, ID);
    cl->is_red = red;
    cl->stats.id = ID;
    cl->stats.glue = std::min<uint32_t>(ext_cl.size(), numeric_limits<uint32_t>::max());
    if (red) {
        //Forgettable: filed as a fresh learnt clause is (see handle_last_confl())
        cl->stats.which_red_array = 0;
        cl->stats.keep = 0;
        cl->stats.used = CL_MAX_USED;
        #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
        red_stats_extra.push_back(ClauseStatsExtra());
        cl->stats.extra_pos = red_stats_extra.size()-1;
        auto& stats_extra = red_stats_extra[cl->stats.extra_pos];
        stats_extra.introduced_at_conflict = sum_conflicts;
        stats_extra.orig_glue = cl->stats.glue;
        stats_extra.orig_size = cl->size();
        #endif
    }

    const ClOffset offset = cl_alloc.get_offset(cl);
    if (red) long_red_cls[cl->stats.which_red_array].push_back(offset);
    else long_irred_cls.push_back(offset);
    solver->attach_clause(*cl, false);
    return PropBy(offset);
}

PropBy Searcher::add_external_clause(const bool forgettable_in, const Lit reason_for)
{
    assert(ext_prop != nullptr);
    assert(okay());
    frat_func_start();

    //A redundant input clause cannot go into a FRAT proof (see
    //Solver::add_clause_outer); keeping it is always sound
    const bool forgettable = forgettable_in && !frat->enabled();

    //Read it, literal by literal
    ext_cl_outer.clear();
    ext_cl.clear();
    ext_stats.cb_calls++;
    if (reason_for != lit_Undef) ext_stats.explanations++;
    bool saw_reason_lit = false;
    //Observed vars are frozen while a reason is read (see CNF::ext_explaining)
    ext_explaining = (reason_for != lit_Undef);
    Lit l = (reason_for == lit_Undef)
        ? ext_prop->cb_add_external_clause_lit()
        : ext_prop->cb_add_reason_clause_lit(reason_for);
    while (l != lit_Undef) {
        release_assert(l.var() < nVarsOuter() &&
            "external clause over a variable that does not exist");
        if (l == reason_for) saw_reason_lit = true;
        ext_cl_outer.push_back(l);
        l = (reason_for == lit_Undef)
            ? ext_prop->cb_add_external_clause_lit()
            : ext_prop->cb_add_reason_clause_lit(reason_for);
    }
    ext_explaining = false;
    release_assert((reason_for == lit_Undef || saw_reason_lit) &&
        "the reason clause of an external propagation must contain the propagated literal");

    //Translate only once the whole clause is in: a callback may observe a
    //renumbered-out variable, and CNF::new_var() then swaps it with another
    //variable, changing that one's INTER number too
    for(const Lit outer: ext_cl_outer) {
        const Lit inter = map_outer_to_inter(outer);
        release_assert(var_data[inter.var()].observed &&
            "external clauses must only mention observed variables");
        ext_cl.push_back(inter);
    }

    //Clean it, using root assignments only: anything above can be undone
    std::sort(ext_cl.begin(), ext_cl.end());
    Lit prev = lit_Undef;
    bool root_satisfied = false;
    uint32_t j = 0;
    for(uint32_t i = 0; i < ext_cl.size(); i++) {
        const Lit q = ext_cl[i];
        if (q == prev) continue;                              // duplicate
        if (q == ~prev) { root_satisfied = true; break; }     // tautology
        prev = q;
        if (value(q) != l_Undef && var_data[q.var()].level == 0) {
            if (value(q) == l_True) { root_satisfied = true; break; }
            continue;                                         // root-falsified
        }
        ext_cl[j++] = q;
    }
    if (root_satisfied) {
        ext_stats.clause_ignored++;
        verb_print(6, "[user-prop] external clause is root-satisfied, ignoring");
        frat_func_end();
        return PropBy();
    }
    ext_cl.resize(j);

    //Log it as an input clause arriving mid-derivation (JAIR 81, section 3.6):
    //the proof is of the CNF plus the propagator's clauses
    int32_t ID = ++clause_id;
    if (frat->enabled()) {
        //The writer wants INTER numbering. OUTER agrees with it here, as with
        //add_clause_outer(): renumbering is off while a proof is written, and
        //observed variables are never eliminated or replaced.
        for([[maybe_unused]] const Lit o: ext_cl_outer) assert(map_outer_to_inter(o) == o);
        *frat << "external clause\n" << origcl << ID << ext_cl_outer << fin;
        const int32_t cleaned_ID = ++clause_id;
        *frat << add << cleaned_ID << ext_cl << fin;
        *frat << del << ID << ext_cl_outer << fin;
        ID = cleaned_ID;
    }

    //The empty clause: we are done
    ext_stats.clauses++;
    if (ext_cl.empty()) {
        verb_print(2, "[user-prop] external propagator gave the empty clause");
        set_unsat_cl_id(ID);
        ok = false;
        frat_func_end();
        return PropBy();
    }

    //A unit holds at the root, so go back there and enqueue it
    if (ext_cl.size() == 1) {
        ext_stats.clause_units++;
        cancel_until(0);
        //Cleaning dropped every root-assigned literal
        assert(value(ext_cl[0]) == l_Undef);
        enqueue<false>(ext_cl[0], 0, PropBy());
        if (frat->enabled()) *frat << del << ID << ext_cl[0] << fin;
        frat_func_end();
        return PropBy();
    }

    //Pick the two watches, then backtrack so that the clause is satisfied,
    //propagating or conflicting, and correctly watched
    std::sort(ext_cl.begin(), ext_cl.end(), [&](const Lit a, const Lit b) {
        return ext_watch_rank(value(a), var_data[a.var()].level) >
               ext_watch_rank(value(b), var_data[b.var()].level);
    });

    //Satisfied by the first watch no later than the second is falsified: no
    //backtrack can unassign the first while the second stays falsified
    const bool safely_satisfied =
        value(ext_cl[0]) == l_True
        && (value(ext_cl[1]) != l_False
            || var_data[ext_cl[0].var()].level <= var_data[ext_cl[1].var()].level);

    if (!safely_satisfied && value(ext_cl[1]) == l_False) {
        //All but the first are falsified, the latest on 'blevel': that is
        //where the clause should have propagated
        const uint32_t blevel = var_data[ext_cl[1].var()].level;
        if (blevel < decision_level()) cancel_until(blevel);
        assert(value(ext_cl[1]) == l_False);
    }

    const PropBy by = ext_attach_clause(ID, forgettable);

    if (value(ext_cl[1]) != l_False) {
        //Two free literals, or satisfied: nothing follows yet
        frat_func_end();
        return PropBy();
    }
    if (value(ext_cl[0]) == l_Undef) {
        enqueue<false>(ext_cl[0], decision_level(), by);
        frat_func_end();
        return PropBy();
    }
    if (value(ext_cl[0]) == l_False) {
        ext_stats.clause_confls++;
        //Both watches are falsified on the current level: a normal conflict.
        assert(var_data[ext_cl[0].var()].level == decision_level());
        if (ext_cl.size() == 2) fail_bin_lit = ext_cl[0];
        frat_func_end();
        return by;
    }
    //Satisfied by the first literal, assigned no later than the second
    assert(var_data[ext_cl[0].var()].level <= var_data[ext_cl[1].var()].level);
    frat_func_end();
    return PropBy();
}

/**
Algorithms 2 and 3 of the paper, run when unit propagation is at a fixed point:
ask for implied literals, then for clauses. Either may move the trail, so each
round ends with propagation and notification, and a clause earns another round.

Propagations are explained lazily by default (paper section 2.3, as CaDiCaL):
assigned with a placeholder reason, the clause asked for only if conflict
analysis needs it (PropEngine::get_ext_reason). The reason is asked for eagerly,
and added as an external clause, at the root (the assignment is permanent), with
FRAT (every step is logged), or if conf.ext_lazy_reasons is off.

Unlike CaDiCaL (exteagerrecalc), a lazily propagated literal keeps the current
level even if its reason justifies a lower one. That over-approximates, so the
learnt clause is still a valid resolvent, just weaker.
*/
PropBy Searcher::external_propagate()
{
    assert(ext_prop != nullptr);
    //A lazy propagator only ever looks at complete assignments.
    if (ext_prop->is_lazy || ext_prop_private_steps) return PropBy();

    PropBy confl;
    bool another_round = true;
    while (another_round && confl.isnullptr() && okay()) {
        another_round = false;

        //Algorithm 2: literals implied by the propagator
        notify_assignments();
        ext_stats.cb_calls++;
        ext_stats.prop_calls++;
        Lit elit = ext_prop->cb_propagate();
        while (elit != lit_Undef) {
            //Stay responsive to interrupt_asap(). Dropping the literal is fine:
            //cb_propagate() will offer it again on the next call.
            if (must_interrupt_asap()) return confl;
            release_assert(elit.var() < nVarsOuter() &&
                "external propagation of a variable that does not exist");
            const Lit ilit = map_outer_to_inter(elit);
            release_assert(var_data[ilit.var()].observed &&
                "external propagation is only allowed over observed variables");

            //An already satisfied literal is ignored
            if (value(ilit) != l_True) {
                //Lazy unless at the root or with FRAT, see above
                const bool lazy = conf.ext_lazy_reasons
                    && value(ilit) == l_Undef
                    && decision_level() > 0
                    && !frat->enabled();

                ext_stats.props++;
                if (lazy) {
                    ext_stats.props_lazy++;
                    enqueue<false>(ilit, decision_level(), PropBy(ExtPropTag()));
                } else {
                    //Eager: the reason clause itself propagates the literal,
                    //or conflicts if it is falsified
                    confl = add_external_clause(ext_prop->are_reasons_forgettable, elit);
                    if (!okay()) return PropBy();
                    if (!confl.isnullptr()) break;

                    release_assert(value(ilit) != l_Undef &&
                        "the reason clause of an external propagation must imply it"
                        " under the current trail");
                }

                if (qhead != trail.size()) {
                    confl = propagate<false>();
                    if (!confl.isnullptr()) break;
                }
                notify_assignments();
            }
            ext_stats.cb_calls++;
            ext_stats.prop_calls++;
            elit = ext_prop->cb_propagate();
        }
        if (!confl.isnullptr() || !okay()) break;

        //Algorithm 3: clauses the propagator wants to add
        notify_assignments();
        bool forgettable = false;
        ext_stats.cb_calls++;
        ext_stats.clause_calls++;
        while (ext_prop->cb_has_external_clause(forgettable)) {
            confl = add_external_clause(forgettable);
            forgettable = false;
            if (!okay()) return PropBy();
            if (!confl.isnullptr()) break;

            if (qhead != trail.size()) {
                confl = propagate<false>();
                if (!confl.isnullptr()) break;
            }
            notify_assignments();
            //The trail moved, so the propagator may have more to say now
            another_round = true;
            //Stay responsive to interrupt_asap()
            if (must_interrupt_asap()) return confl;
            ext_stats.cb_calls++;
            ext_stats.clause_calls++;
        }
    }
    return confl;
}

////////////////////////////
// Decisions and solution analysis
////////////////////////////

void Searcher::apply_ext_forced_backtrack()
{
    assert(ext_forced_backtrack_set);
    ext_forced_backtrack_set = false;
    ext_stats.forced_backtracks++;
    if (ext_forced_backtrack_level < decision_level()) {
        verb_print(6, "[user-prop] forced backtrack to level " << ext_forced_backtrack_level);
        cancel_until(ext_forced_backtrack_level);
    }
}

/**
Algorithm 4 of the paper. Only called once every assumption is satisfied.
*/
Lit Searcher::ext_decide()
{
    assert(ext_prop != nullptr);
    if (ext_prop->is_lazy || ext_prop_private_steps) return lit_Undef;

    notify_assignments();
    ext_forced_backtrack_allowed = true;
    ext_stats.cb_calls++;
    const Lit elit = ext_prop->cb_decide();
    ext_forced_backtrack_allowed = false;
    //Backtrack first, so the decision is judged against the new trail
    if (ext_forced_backtrack_set) apply_ext_forced_backtrack();
    if (elit == lit_Undef) return lit_Undef;

    release_assert(elit.var() < nVarsOuter() &&
        "external decision over a variable that does not exist");
    const Lit ilit = map_outer_to_inter(elit);
    release_assert(var_data[ilit.var()].observed &&
        "external decisions are only allowed over observed variables");

    //Already assigned: fall back on the solver's own heuristic
    if (value(ilit) != l_Undef) return lit_Undef;
    ext_stats.decisions++;
    return ilit;
}

//Not part of IPASIR-UP: sign-only advice on a decision the solver's heuristic
//made. Nothing was assigned since cb_decide(), so the view is up to date.
Lit Searcher::ext_advise_polarity(const Lit lit)
{
    if (!ext_prop_active() || ext_prop->is_lazy || !ext_prop->advises_polarity
        || !var_data[lit.var()].observed
    ) {
        return lit;
    }
    assert(ext_notified == trail.size() && ext_pending_fixed.empty());

    const Lit elit = map_inter_to_outer(lit);
    ext_stats.cb_calls++;
    ext_stats.polarity_asked++;
    ext_advising = true;
    const Lit advice = ext_prop->cb_decide_polarity(elit);
    ext_advising = false;
    release_assert((advice == elit || advice == ~elit) &&
        "cb_decide_polarity() must return the literal it was given or its negation");
    if (advice == elit) return lit;
    ext_stats.polarity_flips++;
    return ~lit;
}

/**
Solution analysis: the propagator approves or rejects a complete assignment.
Rejecting without adding a clause or forcing a backtrack leaves nothing to act
on, so it counts as acceptance, as in CaDiCaL.
*/
lbool Searcher::external_check_solution()
{
    assert(ext_prop != nullptr);
    if (ext_prop_private_steps) return l_True;

    notify_assignments();

    //One literal per observed variable, in the order they were observed
    ext_model.clear();
    for(const uint32_t outer_var: ext_observed_vars) {
        const uint32_t v = map_outer_to_inter(outer_var);
        release_assert(value(v) != l_Undef &&
            "an observed variable is unassigned in a complete assignment");
        ext_model.push_back(Lit(outer_var, value(v) == l_False));
    }

    //Observing a variable from the callback can backtrack too
    const size_t trail_before = trail.size();
    const uint32_t level_before = decision_level();

    ext_forced_backtrack_allowed = true;
    ext_stats.cb_calls++;
    ext_stats.model_checks++;
    const bool consistent = ext_prop->cb_check_found_model(ext_model);
    ext_forced_backtrack_allowed = false;

    if (ext_forced_backtrack_set) {
        apply_ext_forced_backtrack();
        return l_Undef;
    }
    if (trail.size() != trail_before
        || decision_level() != level_before
        //A newly observed root-fixed variable is owed a notification, and is
        //missing from the model just handed over
        || !ext_pending_fixed.empty()
    ) {
        //The assignment is not complete any more, whatever the answer was
        verb_print(6, "[user-prop] the trail moved during cb_check_found_model()");
        return l_Undef;
    }

    //The callback may have added a variable without moving the trail. All
    //non-removed vars are decision vars: a side-effect-free pickBranchLit().
    for(uint32_t v = 0; v < nVars(); v++) {
        if (var_data[v].removed == Removed::none && value(v) == l_Undef) {
            verb_print(6, "[user-prop] cb_check_found_model() made the assignment incomplete");
            return l_Undef;
        }
    }
    if (consistent) return l_True;
    ext_stats.models_rejected++;

    //Rejected: take the propagator's clauses
    bool any_clause = false;
    bool forgettable = false;
    ext_stats.cb_calls++;
    ext_stats.clause_calls++;
    while (ext_prop->cb_has_external_clause(forgettable)) {
        any_clause = true;
        ext_confl = add_external_clause(forgettable);
        forgettable = false;
        if (!okay()) return l_False;
        if (!ext_confl.isnullptr()) return l_Undef;

        if (qhead != trail.size()) {
            ext_confl = propagate<false>();
            if (!ext_confl.isnullptr()) return l_Undef;
        }
        notify_assignments();
        if (must_interrupt_asap()) return l_Undef;
        ext_stats.cb_calls++;
        ext_stats.clause_calls++;
    }

    if (!any_clause) {
        verb_print(2, "[user-prop] the model was rejected but nothing was added,"
            " treating it as accepted");
        return l_True;
    }
    //The assignment is likely no longer complete: back to the search
    return l_Undef;
}

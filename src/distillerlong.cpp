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

#include "distillerlong.h"
#include "clausecleaner.h"
#include "constants.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "reducedb.h"
#include "sqlstats.h"

#include <iomanip>
#include <random>
using namespace CMSat;
using std::cout;
using std::endl;

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_DEBUG

//#define VERBOSE_SUBSUME_NONEXIST

//CaDiCaL's vivify_more_noccs: literal order for the trie effect. Higher
//Jeroslow-Wang-style score first; ties broken as in CaDiCaL.
struct LitScoreDescSort
{
    LitScoreDescSort(const vector<uint64_t>& _lit_scores) :
        lit_scores(_lit_scores)
    {}

    bool operator()(const Lit& a, const Lit& b) const {
        const uint64_t n = lit_scores[a.toInt()];
        const uint64_t m = lit_scores[b.toInt()];
        if (n != m) return n > m;
        if (a.var() == b.var()) return !a.sign(); //positive first
        return a.var() < b.var(); //smaller index first
    }

    const vector<uint64_t>& lit_scores;
};

DistillerLong::DistillerLong(Solver* _solver) :
    solver(_solver)
{}

//Visit one literal of a reason clause: mark its var and queue its own
//reason, as CaDiCaL's vivify_analyze_redundant
void DistillerLong::analyze_visit(const Lit l, bool& only_bin)
{
    (void)only_bin;
    const auto& vd = solver->varData[l.var()];
    if (vd.level == 0) return;
    auto& s = solver->seen[l.var()*2];
    if (s) return;
    s = 1;
    analyzed_vars.push_back(l.var());
    if (!vd.reason.isnullptr()) reason_stack.push_back(vd.reason);
}

//DFS over the reasons reachable from 'start', marking every level>0 var.
//Decisions end the recursion. Returns false on a BNN reason, where this
//analysis is unsupported
bool DistillerLong::analyze_seen_reasons(const PropBy start, bool& only_bin)
{
    //visits done before this call may have queued reasons already: keep them
    reason_stack.push_back(start);
    while (!reason_stack.empty()) {
        const PropBy r = reason_stack.back();
        reason_stack.pop_back();
        switch (r.getType()) {
            case binary_t:
                analyze_visit(r.lit2(), only_bin);
                break;
            case clause_t: {
                const Clause& rcl = *solver->cl_alloc.ptr(r.get_offset());
                if (rcl.size() > 2) only_bin = false;
                for (const Lit l: rcl) analyze_visit(l, only_bin);
                break;
            }
            case xor_t: {
                only_bin = false;
                int32_t id;
                for (const Lit l: *solver->get_xor_reason(r, id))
                    analyze_visit(l, only_bin);
                break;
            }
            default: return false; //BNN reason: bail out
        }
    }
    return true;
}

//CaDiCaL's vivify_post_process_analysis: keep the subsume literal and the
//decisions the derivation used; flush the rest. Result in kept_lits.
//Returns false when nothing is gained ('@7': all literals were used)
bool DistillerLong::post_process_analysis(const Clause& cl, const Lit subsume_lit)
{
    bool all_dec = true;
    for (const Lit l: cl) {
        if (l == subsume_lit) continue;
        if (solver->value(l) != l_False) { all_dec = false; break; }
        const auto& vd = solver->varData[l.var()];
        if (vd.level == 0) continue;
        if (!vd.reason.isnullptr()) { all_dec = false; break; }
        if (!solver->seen[l.var()*2]) { all_dec = false; break; }
    }
    if (all_dec) return false;

    kept_lits.clear();
    for (const Lit l: cl) {
        if (l == subsume_lit) { kept_lits.push_back(l); continue; }
        if (solver->value(l) != l_False) continue;          //true/unassigned: flush
        const auto& vd = solver->varData[l.var()];
        if (vd.level == 0) continue;                        //fixed false: drop
        if (!vd.reason.isnullptr()) continue;               //implied false: flush
        if (solver->seen[l.var()*2]) kept_lits.push_back(l); //used decision: keep
    }
    return !kept_lits.empty();
}

//Strict hints for an analysis-strengthened clause: units, the seen
//propagations' reasons in trail order, then the closing clause
void DistillerLong::analysis_hints(const Lit subsume_lit, const PropBy confl)
{
    hints.clear();
    if (!solver->frat->enabled()) return;

    props_tmp.clear();
    for (const uint32_t v: analyzed_vars) {
        if (subsume_lit != lit_Undef && v == subsume_lit.var()) continue;
        if (!solver->varData[v].reason.isnullptr()) props_tmp.push_back(v);
    }
    std::sort(props_tmp.begin(), props_tmp.end(),
        [this](const uint32_t a, const uint32_t b) {
            return solver->varData[a].sublevel < solver->varData[b].sublevel;
        });
    vector<int32_t> rsns;
    for (const uint32_t v: props_tmp)
        rsns.push_back(solver->get_reason_id(solver->varData[v].reason, hint_units));
    int32_t final_id;
    if (subsume_lit != lit_Undef)
        final_id = solver->get_reason_id(
            solver->varData[subsume_lit.var()].reason, hint_units);
    else
        final_id = solver->get_confl_id(confl, hint_units);
    hints = hint_units;
    hints.insert(hints.end(), rsns.begin(), rsns.end());
    hints.push_back(final_id);

}

void DistillerLong::clear_seen()
{
    for (const uint32_t v: analyzed_vars) solver->seen[v*2] = 0;
    analyzed_vars.clear();
    reason_stack.clear(); //non-empty after a BNN bail-out
}

bool DistillerLong::distill(const bool red, bool only_rem_cl)
{
    frat_func_start();
    assert(solver->ok);
    numCalls_red += (unsigned)red;
    numCalls_irred += (unsigned)!red;
    runStats.clear();

    if (!red) {
        if (!distill_long_cls_all(
            solver->longIrredCls,
            solver->conf.distill_irred_alsoremove_ratio,
            true, //also remove
            only_rem_cl,
            red))
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();

        if (!only_rem_cl) {
            if (!distill_long_cls_all(
                solver->longIrredCls,
                solver->conf.distill_irred_noremove_ratio,
                false, //also remove
                only_rem_cl,
                red))
            {
                goto end;
            }
        }
        globalStats += runStats;
        runStats.clear();
    } else {
        //Redundant
        if (!distill_long_cls_all(
            solver->longRedCls[0],
            1.0,
            false, //dont' remove (it's always redundant)
            only_rem_cl,
            red,
            0)) //red lev (only to print)
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();

        #ifdef FINAL_PREDICTOR //only predictor builds populate longRedCls[1]
        if (!distill_long_cls_all(
            solver->longRedCls[1],
            1.0,
            false, //dont' remove (it's always redundant)
            only_rem_cl,
            red,
            1))  // //red lev (only to print)
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();
        #endif
    }

end:
    lit_counts.clear();
    lit_counts.shrink_to_fit();
    frat_func_end();

    return solver->okay();
}

bool DistillerLong::distill_long_cls_all(
    vector<ClOffset>& offs
    , double time_mult
    , bool also_remove
    , bool only_remove
    , bool red
    , uint32_t red_lev
) {
    assert(solver->ok);
    if (time_mult == 0.0) return solver->okay();
    verb_print(6, "c Doing distillation branch for long clauses");
    frat_func_start();

    double my_time = cpu_time();
    const size_t origTrailSize = solver->trail_size();

    //Time-limiting
    if (red) {
        //CaDiCaL's vivify budget for redundant rounds: relative to the
        //propagations done since the last round, clamped, then the
        //'vivifyredeff' 75-per-mille share
        int64_t lim = (int64_t)(solver->propStats.propagations - last_red_props);
        last_red_props = solver->propStats.propagations;
        lim *= 1e-3 * (double)solver->conf.distill_red_releff;
        lim = std::max<int64_t>(lim, 60LL*1000LL);
        lim = std::min<int64_t>(lim, 60LL*1000LL*1000LL);
        maxNumProps = lim * 75LL / 1000LL;
    } else {
        maxNumProps =
            solver->conf.distill_long_cls_time_limitM*1000LL*1000ULL
            *solver->conf.global_timeout_multiplier;

        if (solver->litStats.irredLits + solver->litStats.redLits <
                (500ULL*1000ULL*solver->conf.var_and_mem_out_mult)
        ) {
            maxNumProps *=2;
        }
        maxNumProps *= time_mult;
    }
    orig_maxNumProps = maxNumProps;

    //stats setup
    oldBogoProps = solver->propStats.bogoProps;
    runStats.numCalled += 1;

    //Select candidates. prio 0: not checked since their bit was cleared,
    //as CaDiCaL's 'vivify' bit; prio 1: the rest (irred only)
    vector<ClOffset> todo;
    vector<uint32_t> todo_prio;
    todo.reserve(offs.size());
    for(uint32_t prio = 0; prio < (red ? 1: 2); prio ++) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < offs.size(); i ++) {
            Clause* cl = solver->cl_alloc.ptr(offs[i]);
            VERBOSE_PRINT("Clause at " << i << " is:  " << *cl);
            bool ok = false;
            if (!cl->stats.is_ternary_resolvent
                && (!red || solver->reduceDB->likely_to_be_kept(*cl))
                && !solver->satisfied(*cl)
            ) {
                if (also_remove) {
                    if (cl->tried_to_remove == prio) {
                        ok = true;
                    }
                } else {
                    if (cl->distilled == prio) {
                        ok = true;
                    }
                }
            }

            if (ok) {
                todo.push_back(offs[i]);
                todo_prio.push_back(prio);
                VERBOSE_PRINT("Adding this one to TODO");
            } else {
                offs[j++] = offs[i];
                continue;
            }
        }
        offs.resize(j);
    }

    //CaDiCaL's noccs: capped Jeroslow-Wang score of each literal over the
    //candidates. Kept for the whole round; try_distill re-sorts each clause
    //by it so consecutive clauses share decision prefixes (the trie effect)
    lit_counts.clear();
    lit_counts.resize(solver->nVars()*2, 0);
    for(const ClOffset off: todo) {
        const Clause& cl = *solver->cl_alloc.ptr(off);
        const uint64_t score = cl.size() >= 12 ? 1ULL : 1ULL << (12-cl.size());
        for(const Lit l: cl) lit_counts[l.toInt()] += score;
    }

    //Sort the schedule as CaDiCaL's vivify_clause_later: unchecked first,
    //small glue (red), short, then lexicographically by the literal order
    //so clauses with shared prefixes are tried back-to-back
    {
        const LitScoreDescSort lit_sort(lit_counts);
        vector<vector<Lit>> slits(todo.size());
        for(uint32_t i = 0; i < todo.size(); i++) {
            const Clause& cl = *solver->cl_alloc.ptr(todo[i]);
            slits[i].assign(cl.begin(), cl.end());
            std::sort(slits[i].begin(), slits[i].end(), lit_sort);
        }

        //CaDiCaL's flush_vivification_schedule: a candidate whose sorted
        //literals extend another candidate's full literal set is subsumed
        //by it (duplicates included). Delete such candidates outright.
        {
            vector<uint32_t> ford(todo.size());
            for(uint32_t i = 0; i < todo.size(); i++) ford[i] = i;
            std::stable_sort(ford.begin(), ford.end(),
                [&](const uint32_t a, const uint32_t b) {
                    const auto& x = slits[a];
                    const auto& y = slits[b];
                    const uint32_t n = std::min(x.size(), y.size());
                    for(uint32_t i = 0; i < n; i++)
                        if (x[i] != y[i]) return x[i] < y[i];
                    return x.size() < y.size();
                });
            vector<char> removed(todo.size(), 0);
            uint32_t num_subsumed = 0;
            uint32_t prev = numeric_limits<uint32_t>::max();
            for(const uint32_t at: ford) {
                if (prev == numeric_limits<uint32_t>::max()
                    || slits[at].size() < slits[prev].size()
                ) {
                    prev = at;
                    continue;
                }
                bool is_prefix = true;
                for(uint32_t i = 0; i < slits[prev].size(); i++) {
                    if (slits[prev][i] != slits[at][i]) { is_prefix = false; break; }
                }
                if (is_prefix) {
                    Clause* cl = solver->cl_alloc.ptr(todo[at]);
                    solver->detachClause(*cl);
                    solver->free_cl(todo[at]);
                    removed[at] = 1;
                    num_subsumed++;
                } else {
                    prev = at;
                }
            }
            if (num_subsumed) {
                runStats.clRemoved += num_subsumed;
                uint32_t j = 0;
                for(uint32_t i = 0; i < todo.size(); i++) {
                    if (removed[i]) continue;
                    if (i != j) {
                        todo[j] = todo[i];
                        todo_prio[j] = todo_prio[i];
                        slits[j] = std::move(slits[i]);
                    }
                    j++;
                }
                todo.resize(j);
                todo_prio.resize(j);
                slits.resize(j);
            }
        }

        vector<uint32_t> order(todo.size());
        for(uint32_t i = 0; i < todo.size(); i++) order[i] = i;
        std::stable_sort(order.begin(), order.end(),
            [&](const uint32_t a, const uint32_t b) {
                if (todo_prio[a] != todo_prio[b]) return todo_prio[a] < todo_prio[b];
                const Clause& c = *solver->cl_alloc.ptr(todo[a]);
                const Clause& d = *solver->cl_alloc.ptr(todo[b]);
                if (red && c.stats.glue != d.stats.glue)
                    return c.stats.glue < d.stats.glue;
                if (c.size() != d.size()) return c.size() < d.size();
                for(uint32_t i = 0; i < c.size(); i++) {
                    if (slits[a][i] != slits[b][i])
                        return lit_sort(slits[a][i], slits[b][i]);
                }
                return false;
            });
        vector<ClOffset> todo2(todo.size());
        for(uint32_t i = 0; i < todo.size(); i++) todo2[i] = todo[order[i]];
        todo = std::move(todo2);
    }

    const uint32_t orig_todo_size = todo.size();
    runStats.potentialClauses += orig_todo_size;

    assert(runStats.checkedClauses == 0);
    bool time_out = go_through_clauses(todo, also_remove, only_remove);

    //Add back the prioritized clauses
    for(const auto off: todo) offs.push_back(off);

    const double time_used = cpu_time() - my_time;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 1) {
        cout << solver->conf.prefix << "[distill-long";
        if (red) {
            cout << "-red" << red_lev << "]";
        } else {
            cout << "-irred]";
        }
        cout
        << " cls"
        << " tried: " << runStats.checkedClauses << "/" << orig_todo_size
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "distill long cls"
            , time_used
            , time_out
            , time_remain
        );
    }

    //Update stats
    runStats.time_used += time_used;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    frat_func_end();
    return solver->okay();
}

bool DistillerLong::go_through_clauses(vector<ClOffset>& cls, bool also_remove, bool only_remove) {
    frat_func_start();
    bool time_out = false;
    size_t kept = 0;
    for (size_t at = 0; at < cls.size(); at++) {
        const ClOffset offset = cls[at];
        VERBOSE_PRINT("At offset: " << offset);

        //Check if we are in state where we only copy offsets around
        if (time_out || !solver->ok) {
            cls[kept++] = offset;
            continue;
        }

        Clause& cl = *solver->cl_alloc.ptr(offset);

        //if done enough, stop doing it
        if ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps >= maxNumProps
            || solver->must_interrupt_asap()
        ) {
            if (solver->conf.verbosity >= 3) {
                cout
                << "c Need to finish distillation -- ran out of prop (=allocated time)"
                << endl;
            }
            runStats.timeOut++;
            time_out = true;
        }

        //Time to dereference
        maxNumProps -= 5;

        if (also_remove) cl.tried_to_remove = 1;
        else cl.distilled = 1;
        runStats.checkedClauses++;
        assert(cl.size() > 2);

        //Try to distill clause
        const ClOffset offset2 = try_distill_clause_and_return_new(
            offset, &cl.stats, also_remove, only_remove);

        if (offset2 != CL_OFFSET_MAX) cls[kept++] = offset2;
    }
    cls.resize(kept);

    //decisions may have been left in place for reuse between candidates
    solver->cancelUntil<false, true>(0);

    frat_func_end();
    return time_out;
}

ClOffset DistillerLong::try_distill_clause_and_return_new(
    ClOffset offset, const ClauseStats* const stats,
    const bool also_remove, const bool only_remove
) {
    frat_func_start();
    assert(solver->prop_at_head());
    bool True_confl = false;
    PropBy confl;

    //Disable this clause
    Clause& cl = *solver->cl_alloc.ptr(offset);
    const Lit cl_lit1 = cl[0];
    const Lit cl_lit2 = cl[1];
    const uint32_t orig_size = cl.size();
    cl.disabled = true;
    *solver->frat << deldelay << cl << fin;
    const bool red = cl.red();
    if (red) assert(!also_remove);
    VERBOSE_PRINT("Trying to distill clause:" << cl);

    const auto remove_cl = [&]() {
        solver->cancelUntil<false, true>(0);
        solver->detach_modified_clause(cl_lit1, cl_lit2, orig_size, &cl);
        *solver->frat << findelay;
        solver->free_cl(offset);
        runStats.clRemoved++;
        frat_func_end();
        return CL_OFFSET_MAX;
    };

    //Copy the non-fixed literals to 'sorted', as CaDiCaL's vivify_clause.
    //Fixed-true: clause is satisfied; fixed-false: drop with a unit hint
    hint_units.clear();
    sorted.clear();
    for (const Lit l: cl) {
        const lbool val = solver->value(l);
        if (val != l_Undef && solver->varData[l.var()].level == 0) {
            if (val == l_True) return remove_cl();
            if (solver->frat->enabled())
                hint_units.push_back(solver->unit_cl_IDs[l.var()]);
        } else {
            sorted.push_back(l);
        }
    }
    assert(sorted.size() > 1); //fixed-false lits must have been propagated

    //Sort by the global literal order so the decision prefix lines up
    //with the previous candidate's
    std::sort(sorted.begin(), sorted.end(), LitScoreDescSort(lit_counts));

    //If this clause forced one of its literals on the reused trail, it may
    //not be used to prove itself redundant: backtrack below that level
    if (solver->decisionLevel() > 0 && solver->clause_locked(cl, offset)) {
        assert(solver->varData[cl[0].var()].level > 0);
        solver->cancelUntil<false, true>(solver->varData[cl[0].var()].level - 1);
    }

    //Reuse the decisions of the previous candidate as long as they match
    //our sorted literals, as CaDiCaL does (the trie effect)
    if (solver->decisionLevel() > 0) {
        uint32_t match = 0;
        for (const Lit l: sorted) {
            if (match >= solver->decisionLevel()) break;
            const Lit dec = solver->trail_at(solver->trail_begin_of_level(match));
            if (dec == ~l) match++;
            else break;
        }
        if (match < solver->decisionLevel()) solver->cancelUntil<false, true>(match);
    }

    const uint32_t seg_start =
        solver->decisionLevel() > 0 ? solver->trail_begin_of_level(0) : solver->getTrailSize();
    uint32_t num_dropped = orig_size - sorted.size(); //fixed-false lits
    kept_lits.clear();
    for (const Lit lit: sorted) {
        const lbool val = solver->value(lit);
        if (val == l_Undef) {
            solver->new_decision_level();
            solver->enqueue<true>(~lit);
            kept_lits.push_back(lit);

            maxNumProps -= 5;
            if (!red && also_remove) {
                //ONLY propagate on irred
                confl = solver->propagate<true, false, true>();
            } else {
                //Normal propagation, on all clauses
                confl = solver->propagate<true, true, true>();
            }
            if (!confl.isnullptr()) break;
        } else if (val == l_False) {
            if (solver->varData[lit.var()].reason.isnullptr()) {
                //one of our own (possibly reused) decisions
                kept_lits.push_back(lit);
            } else if (only_remove) {
                // if we don't want to shorten, then don't remove literals
                kept_lits.push_back(lit);
            } else {
                num_dropped++;
            }
        } else {
            assert(val == l_True);
            kept_lits.push_back(lit);
            True_confl = true;
            confl = solver->varData[lit.var()].reason;
            break;
        }
    }
    assert(solver->ok);

    VERBOSE_PRINT("also_remove: " << also_remove
        << "red: " << red
        << "True_confl: " << True_confl
        << "confl.isnullptr(): " << confl.isnullptr());

    //Subsumed via propagation: a conflict ('@6') or a literal positively
    //implied ('@5'), as in CaDiCaL's vivify_clause
    const bool subsumed = !confl.isnullptr();
    bool have_analysis = false;

    //Propagation was over irred only, so the clause is an asymmetric
    //tautology of the irred formula: drop it
    if (subsumed && !red && also_remove) {
        const int lev = solver->conf.distill_rem_level;
        if (lev >= 2 || (lev == 1 && !True_confl)) {
            VERBOSE_PRINT("CL Removed.");
            return remove_cl();
        }
        //Keeping it: the trail is at a conflict, so it cannot be reused
        solver->cancelUntil<false, true>(0);
        if (num_dropped == 0 && kept_lits.size() == orig_size) {
            cl.disabled = false;
            solver->frat->forget_delay();
            frat_func_end();
            return offset;
        }
        //Only the level-0-false literals go: units, then the original clause
        hints.clear();
        if (solver->frat->enabled()) {
            hints = hint_units;
            hints.push_back(stats->id);
        }
        have_analysis = true;
    }

    //Redundant mode: strengthen instead of subsuming, by resolving the
    //involved reasons, as CaDiCaL's vivify_analyze_redundant
    if (subsumed && red) {
        const Lit subsume_lit = True_confl ? kept_lits.back() : lit_Undef;
        bool only_bin = true;
        bool analyzed_ok;
        assert(analyzed_vars.empty());
        if (True_confl) {
            solver->seen[subsume_lit.var()*2] = 1;
            analyzed_vars.push_back(subsume_lit.var());
            analyzed_ok = analyze_seen_reasons(
                solver->varData[subsume_lit.var()].reason, only_bin);
        } else {
            if (confl.getType() == binary_t)
                analyze_visit(solver->get_fail_bin_lit(), only_bin);
            analyzed_ok = analyze_seen_reasons(confl, only_bin);
        }
        if (analyzed_ok) {
            if (only_bin) {
                //hidden tautology: derived through binaries only, drop it
                clear_seen();
                return remove_cl();
            }
            if (!post_process_analysis(cl, subsume_lit)) {
                //'@7': the derivation used every literal, nothing to gain
                clear_seen();
                cl.disabled = false;
                solver->frat->forget_delay();
                solver->cancelUntil<false, true>(solver->decisionLevel()-1);
                frat_func_end();
                return offset;
            }
            analysis_hints(subsume_lit, confl);
            clear_seen();
            have_analysis = true; //kept_lits & hints are set
        } else {
            clear_seen(); //BNN reason: plain prefix shortening below
        }
    }

    //CaDiCaL's vivify instantiation: no conflict and the last literal is a
    //plain decision. Try assigning it true: a conflict proves the clause
    //with the last literal flipped, and resolving that with this clause
    //removes the literal.
    if (!subsumed && !only_remove && solver->conf.distill_instantiate) {
        const Lit last = sorted.back();
        if (solver->value(last) == l_False
            && solver->varData[last.var()].reason.isnullptr()
            && solver->varData[last.var()].level == solver->decisionLevel()
        ) {
            solver->cancelUntil<false, true>(solver->decisionLevel()-1);
            solver->new_decision_level();
            solver->enqueue<true>(last);
            maxNumProps -= 5;
            PropBy confl2;
            if (!red && also_remove) confl2 = solver->propagate<true, false, true>();
            else confl2 = solver->propagate<true, true, true>();
            if (!confl2.isnullptr()) {
                //chain: prefix reasons, this clause (it forces 'last' under
                //the negated shortened clause), the instantiate-level
                //reasons, and the conflict
                hints.clear();
                if (solver->frat->enabled()) {
                    vector<int32_t> rsns;
                    const uint32_t inst_start =
                        solver->trail_begin_of_level(solver->decisionLevel()-1);
                    solver->collect_trail_seg_hints(
                        seg_start, hint_units, rsns, var_Undef, inst_start);
                    rsns.push_back(stats->id);
                    solver->collect_trail_seg_hints(inst_start, hint_units, rsns);
                    rsns.push_back(solver->get_confl_id(confl2, hint_units));
                    hints = hint_units;
                    hints.insert(hints.end(), rsns.begin(), rsns.end());
                }
                const auto it = std::find(kept_lits.begin(), kept_lits.end(), last);
                assert(it != kept_lits.end());
                kept_lits.erase(it);
                have_analysis = true; //kept_lits & hints are set
            } else {
                solver->cancelUntil<false, true>(solver->decisionLevel()-1);
            }
        }
    }

    //Couldn't simplify the clause. Keep the trail for the next candidate
    if (!subsumed && !have_analysis
        && num_dropped == 0 && kept_lits.size() == orig_size
    ) {
        VERBOSE_PRINT("CL Cannot be simplified.");
        cl.disabled = false;
        solver->frat->forget_delay();
        frat_func_end();
        return offset;
    }

    //strict hints while the trail is still intact: units, propagation
    //reasons in trail order, closing clause (conflict/true-lit reason/orig)
    if (!have_analysis) {
        hints.clear();
        if (solver->frat->enabled()) {
            vector<int32_t> rsns;
            solver->collect_trail_seg_hints(seg_start, hint_units, rsns);
            int32_t final_id;
            if (True_confl) final_id = solver->get_reason_id(confl, hint_units);
            else if (!confl.isnullptr()) final_id = solver->get_confl_id(confl, hint_units);
            else final_id = stats->id;
            hints = hint_units;
            hints.insert(hints.end(), rsns.begin(), rsns.end());
            hints.push_back(final_id);
        }
    }

    solver->cancelUntil<false, true>(0);
    solver->detach_modified_clause(cl_lit1, cl_lit2, orig_size, &cl);
    runStats.numLitsRem += orig_size - kept_lits.size();
    runStats.numClShorten++;

    //Make new clause
    lits.resize(kept_lits.size());
    std::copy(kept_lits.begin(), kept_lits.end(), lits.begin());

    // we have to copy because the re-alloc can invalidate the data
    ClauseStats backup_stats(*stats);
    // new clause will inherit this clause's ID
    // so let's set this to 0, this way, when we free() it, it won't be
    // deleted as per cl_last_in_solver
#ifdef DEBUG_FRAT
    {
        std::stringstream ss2;
        ss2 << lits;
        *solver->frat << " new smaller cl: " << ss2.str().c_str() << "\n";
    }
#endif
    solver->free_cl(offset, false);
    Clause *cl2 = solver->add_clause_int(lits, red, &backup_stats,
        true, nullptr, true, lit_Undef, false, false, &hints);
    *solver->frat << findelay;

    if (cl2 != nullptr) {
        //This new, distilled clause has been distilled now.
        if (also_remove) cl2->tried_to_remove = 1;
        else cl2->distilled = 1;
        frat_func_end();
        return solver->cl_alloc.get_offset(cl2);
    } else {
        STATS_DO(solver->stats_del_cl(offset));
        //it became a bin/unit/zero
        frat_func_end();
        return CL_OFFSET_MAX;
    }
}

DistillerLong::Stats& DistillerLong::Stats::operator+=(const Stats& other)
{
    time_used += other.time_used;
    timeOut += other.timeOut;
    zeroDepthAssigns += other.zeroDepthAssigns;
    numClShorten += other.numClShorten;
    numLitsRem += other.numLitsRem;
    checkedClauses += other.checkedClauses;
    potentialClauses += other.potentialClauses;
    numCalled += other.numCalled;
    clRemoved += other.clRemoved;

    return *this;
}

void DistillerLong::Stats::print(const size_t nVars) const
{
    cout << "c -------- DISTILL-LONG STATS --------" << endl;
    print_stats_line("c time"
        , time_used
        , ratio_for_stat(time_used, numCalled)
        , "per call"
    );

    print_stats_line("c timed out"
        , timeOut
        , stats_line_percent(timeOut, numCalled)
        , "% of calls"
    );

    print_stats_line("c distill/checked/potential"
        , numClShorten
        , checkedClauses
        , potentialClauses
    );

    print_stats_line("c lits-rem",
        numLitsRem
    );
    print_stats_line("c 0-depth-assigns",
        zeroDepthAssigns
        , stats_line_percent(zeroDepthAssigns, nVars)
        , "% of vars"
    );
    cout << "c -------- DISTILL STATS END --------" << endl;
}

double DistillerLong::mem_used() const
{
    double mem_used = sizeof(DistillerLong);
    mem_used += lits.size()*sizeof(Lit);
    return mem_used;
}

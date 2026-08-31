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

#include "intree.h"
#include "constants.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "sqlstats.h"
#include "watchalgos.h"

#include <algorithm>
#include <cmath>
#include <cassert>

using namespace CMSat;

InTree::InTree(Solver* _solver) :
    solver(_solver)
    , seen(_solver->seen)
{}

bool InTree::replace_until_fixedpoint(bool& aborted)
{
    assert(solver->conf.doFindAndReplaceEqLits);
    uint64_t time_limit =
        solver->conf.intree_scc_varreplace_time_limitM*1000ULL*1000ULL
        *solver->conf.global_timeout_multiplier
        *0.5;
    time_limit = (double)time_limit * std::min(std::pow((double)(numCalls+1), 0.2), 3.0);
    frat_func_start();

    aborted = false;
    uint64_t bogoprops = 0;
    uint32_t last_replace = numeric_limits<uint32_t>::max();
    uint32_t this_replace = solver->varReplacer->get_num_replaced_vars();
    while(last_replace != this_replace && !aborted) {
        last_replace = this_replace;
        if (!solver->clauseCleaner->remove_and_clean_all()) return false;
        bool OK = solver->varReplacer->replace_if_enough_is_found(0, &bogoprops);
        if (!OK) return false;

        if (solver->varReplacer->get_scc_depth_warning_triggered()) {
            aborted = true;
            return solver->okay();
        }
        this_replace = solver->varReplacer->get_num_replaced_vars();

        if (bogoprops > time_limit) {
            aborted = true;
            return solver->okay();
        }
    }

    frat_func_end();
    return true;
}

bool InTree::watches_only_contains_nonbin(const Lit lit) const
{
    watch_subarray_const ws = solver->watches[lit];
    return std::none_of(ws.begin(), ws.end(),
                        [](const Watched& w) { return w.isBin(); });
}

bool InTree::check_timeout_due_to_hyperbin()
{
    assert(!(solver->timedOutPropagateFull && solver->frat->enabled()));

    // Can turn it off ONLY if frat is not enabled
    // otherwise proofs don't go through
    if (solver->timedOutPropagateFull && !solver->frat->enabled()) {
        verb_print(1, "[intree] intra-propagation timeout, turning off OTF hyper-bin&trans-red");
        solver->conf.do_hyperbin_and_transred = false;
        return true;
    }

    return false;
}

void InTree::fill_roots()
{
    //l is root if no clause of form (l, l2).

    roots.clear();
    for(uint32_t i = 0; i < solver->nVars()*2; i++)
    {
        Lit lit(i/2, i%2);
        if (solver->varData[lit.var()].removed != Removed::none
            || solver->value(lit) != l_Undef
        ) {
            continue;
        }

        if (watches_only_contains_nonbin(lit)) {
            roots.push_back(lit);
        }
    }
}

bool InTree::intree_probe() {
    assert(solver->okay());
    queue.clear();
    reset_reason_stack.clear();
    solver->use_depth_trick = false;
    solver->perform_transitive_reduction = true;
    hyperbin_added = 0;
    removedIrredBin = 0;
    removedRedBin = 0;
    numCalls++;
    frat_func_start();

    if (!solver->conf.doFindAndReplaceEqLits) {
      verb_print(1, "[intree] SCC is not allowed, intree cannot work this way, aborting");
      return solver->okay();
    }

    bool aborted = false;
    if (!replace_until_fixedpoint(aborted)) return solver->okay();
    if (aborted) {
        if (solver->conf.verbosity) {
            cout
            << "c [intree] too expensive or depth exceeded during SCC: aborting"
            << endl;
        }
        solver->use_depth_trick = true;
        solver->perform_transitive_reduction = true;
        return solver->okay();
    }

    double my_time = cpu_time();
    bogoprops_to_use = solver->conf.intree_time_limitM*1000ULL*1000ULL
        *solver->conf.global_timeout_multiplier;
    bogoprops_to_use = (double)bogoprops_to_use * std::pow((double)(numCalls+1), 0.3);
    start_bogoprops = solver->propStats.bogoProps;

    fill_roots();
    std::shuffle(roots.begin(), roots.end(), solver->mtrand);

    //Let's enqueue all ~root -s.
    for(Lit lit: roots) enqueue(~lit, lit_Undef, false, 0);

    //clear seen
    for(QueueElem elem: queue) {
        if (elem.propagated != lit_Undef) seen[elem.propagated.toInt()] = 0;
    }
    const size_t orig_num_free_vars = solver->get_num_free_vars();

    tree_look();
    unmark_all_bins();
    if (solver->frat->enabled()) solver->flush_ghost_hyper_bins();

    const double time_used = cpu_time() - my_time;
    const double time_remain = float_div(
        (int64_t)solver->propStats.bogoProps-start_bogoprops, bogoprops_to_use);
    const bool time_out = ((int64_t)solver->propStats.bogoProps > start_bogoprops + bogoprops_to_use);

    verb_print(1,
        "[intree] Set "
        << (orig_num_free_vars - solver->get_num_free_vars())
        << " vars"
        << " hyper-added: " << hyperbin_added
        << " trans-irred: " << removedIrredBin
        << " trans-red: " << removedRedBin
        << solver->conf.print_times(time_used,  time_out, time_remain));

    if (solver->sqlStats) {
        solver->sqlStats->time_passed( solver , "intree" , time_used , time_out , time_remain);
    }

    frat_func_end();
    solver->use_depth_trick = true;
    solver->perform_transitive_reduction = true;
    return solver->okay();
}

void InTree::unmark_all_bins()
{
    for(watch_subarray wsub: solver->watches) {
        for(Watched& w: wsub) {
            if (w.isBin()) {
                w.unmark_bin_cl();
            }
        }
    }
}

void InTree::tree_look()
{
    assert(failed.empty());
    depth_failed.clear();
    depth_failed.push_back(false);
    solver->propStats.clear();

    bool timeout = false;
    while(!queue.empty())
    {
        if (start_bogoprops + bogoprops_to_use <
            (int64_t)solver->propStats.bogoProps
            + (int64_t)solver->propStats.otfHyperTime
            || timeout
        ) {
            break;
        }

        const QueueElem elem = queue.front();
        queue.pop_front();

        if (solver->conf.verbosity >= 10) {
            cout << "Dequeued [[" << elem << "]] dec lev:"
            << solver->decisionLevel() << endl;
        }

        if (elem.propagated != lit_Undef) {
            timeout = handle_lit_popped_from_queue(
                elem.propagated, elem.other_lit, elem.red, elem.ID);
        } else {
            assert(solver->decisionLevel() > 0);
            solver->cancelUntil<false, true>(solver->decisionLevel()-1);

            depth_failed.pop_back();
            assert(!depth_failed.empty());

            if (reset_reason_stack.empty()) {
                assert(solver->decisionLevel() == 0);
            } else {
                assert(!reset_reason_stack.empty());
                ResetReason tmp = reset_reason_stack.back();
                reset_reason_stack.pop_back();
                if (tmp.var_reason_changed != var_Undef) {
                    solver->varData[tmp.var_reason_changed].reason = tmp.orig_propby;
                    if (solver->conf.verbosity >= 10) {
                        cout << "RESet reason for VAR " << tmp.var_reason_changed+1 << " to:  ????" << /*tmp.orig_propby.lit2() << */ " red: " << (int)tmp.orig_propby.isRedStep() << endl;
                    }
                }
            }
        }

        if (solver->decisionLevel() == 0) {
            if (!empty_failed_list()) {
                return;
            }
        }
    }

    solver->cancelUntil<false, true>(0);
    empty_failed_list();
}

bool InTree::handle_lit_popped_from_queue(
    const Lit lit, const Lit other_lit, const bool red, const int32_t ID)
{
    solver->new_decision_level();
    depth_failed.push_back(depth_failed.back());
    if (other_lit != lit_Undef) {
        reset_reason_stack.push_back(ResetReason(var_Undef, PropBy()));
    }

    bool timeout = false;

    if (solver->value(lit) == l_False
        || depth_failed.back() == 1
    ) {
        //l is failed.
        if (solver->frat->enabled())
            capture_failed_hints(lit, other_lit, ID, depth_failed.back() == 1, PropBy());
        failed.push_back(~lit);
        verb_print(10,"Failed :" << ~lit << " level: " << solver->decisionLevel());
        return false;
    }

    if (other_lit != lit_Undef) {
        //update 'other_lit' 's ancestor to 'lit'
        assert(solver->value(other_lit) == l_True);
        reset_reason_stack.back() = ResetReason(other_lit.var(), solver->varData[other_lit.var()].reason);
        solver->varData[other_lit.var()].reason = PropBy(~lit, red, false, false, ID);
        verb_print(10, "Set reason for VAR " << other_lit.var()+1
        << " to: " << ~lit << " red: " << (int)red);
    }

    if (solver->value(lit) == l_Undef) {
        solver->enqueue<true>(lit);

        //Should do HHBR here
        bool ok;
        if (solver->conf.do_hyperbin_and_transred) {
            uint64_t max_hyper_time = numeric_limits<uint64_t>::max();
            if (!solver->frat->enabled()) {
                max_hyper_time =
                solver->propStats.otfHyperTime
                + solver->propStats.bogoProps
                + 1600ULL*1000ULL*1000ULL;
            }

            Lit ret = solver->propagate_bfs(max_hyper_time);
            ok = (ret == lit_Undef);
            timeout = check_timeout_due_to_hyperbin();
        } else {
            ok = solver->propagate<true>().isnullptr();
        }

        if (!ok && !timeout) {
            depth_failed.back() = 1;
            if (solver->frat->enabled())
                capture_failed_hints(lit, other_lit, ID, false, solver->last_bfs_confl);
            failed.push_back(~lit);
            if (solver->conf.verbosity >= 10) {
                cout << "(timeout?) Failed :" << ~lit << " level: " << solver->decisionLevel() << endl;
            }
        } else {
            hyperbin_added += solver->hyper_bin_res_all(false);
            auto [a, b] = solver->remove_useless_bins(true);
            removedIrredBin += a;
            removedRedBin += b;
        }
        solver->uselessBin.clear();
        //FRAT: their adds were emitted at creation, delete before dropping
        for(const auto& b: solver->needToAddBinClause)
            *solver->frat << del << b.get_id() << b.getLit1() << b.getLit2() << fin;
        solver->needToAddBinClause.clear();
    }

    return timeout;
}

//Emit the failed unit's `add` right away, while the hint clauses (hyper
//bins etc.) still exist in the proof. Hints: units, the tree-edge bins
//(deepest first), then all trail reasons, then the conflict if any
void InTree::capture_failed_hints(
    const Lit lit, const Lit other_lit, const int32_t edge_id,
    const bool par_fail, const PropBy confl)
{
    failed_hints.emplace_back();
    FailedHints& h = failed_hints.back();
    h.unit_id = ++solver->clauseID;
    *solver->frat << add << h.unit_id << ~lit << fratchain;
    if (par_fail) {
        //parent's failed unit was emitted at its own capture
        const auto it = failed_ids.find(other_lit.var());
        assert(it != failed_ids.end());
        *solver->frat << it->second << edge_id << fin;
        failed_ids[lit.var()] = h.unit_id;
        return;
    }

    //the deepest decision's reason is a stale edge unless `lit` was enqueued
    //(which rewires it); the edge_id param covers that edge either way
    const uint32_t skip_var =
        (confl.isnullptr() && other_lit != lit_Undef) ? other_lit.var() : var_Undef;
    vector<int32_t> units, edges, cone;
    if (other_lit != lit_Undef) edges.push_back(edge_id);
    solver->collect_decision_reasons(edges, skip_var);
    //BFS trail order is not dependency order (hyper-bin rewiring), so the
    //cone is collected via ancestor-bin walks instead
    int32_t cid = 0;
    if (!confl.isnullptr()) {
        cid = solver->get_confl_id(confl, units);
        switch (confl.getType()) {
            case binary_t:
                ancestor_chain(~solver->get_fail_bin_lit(), cone);
                ancestor_chain(~confl.lit2(), cone);
                break;
            case clause_t: {
                const Clause& cl = *solver->cl_alloc.ptr(confl.get_offset());
                for(const Lit m: cl) ancestor_chain(~m, cone);
                break;
            }
            default: release_assert(false);
        }
    } else {
        //value(lit) was l_False: derive ~lit, conflicting with assumed lit
        ancestor_chain(~lit, cone);
    }
    for(const auto& id: units) *solver->frat << id;
    for(const auto& id: edges) *solver->frat << id;
    for(const auto& id: cone) *solver->frat << id;
    if (cid != 0) *solver->frat << cid;
    *solver->frat << fin;
    failed_ids[lit.var()] = h.unit_id;
}

//The bin chain deriving trail lit t from its level's decision, pushed
//decision-side first. During intree, all reasons are (ancestor) bins.
void InTree::ancestor_chain(const Lit t, vector<int32_t>& out)
{
    tmp_walk.clear();
    Lit x = t;
    uint32_t guard = 0;
    while (true) {
        if (solver->value(x) == l_Undef) break;
        if (solver->varData[x.var()].level == 0) break; //covered by units
        const PropBy r = solver->varData[x.var()].reason;
        if (r.isnullptr()) break; //decision, covered by the edges
        assert(r.getType() == binary_t);
        tmp_walk.push_back(r.get_id());
        x = r.getAncestor();
        release_assert(guard++ <= solver->nVars());
    }
    out.insert(out.end(), tmp_walk.rbegin(), tmp_walk.rend());
}

bool InTree::empty_failed_list()
{
    assert(solver->decisionLevel() == 0);
    const bool fr = solver->frat->enabled();
    for(size_t at = 0; at < failed.size(); at++) {
        const Lit lit = failed[at];
        if (!solver->ok) {
            return false;
        }

        if (solver->value(lit) == l_Undef) {
            if (fr) {
                solver->enqueue_registered_unit<true>(lit, failed_hints[at].unit_id);
            } else {
                solver->enqueue<true>(lit);
            }
            solver->ok = solver->propagate<true>().isnullptr();
            if (!solver->ok) {
                return false;
            }
        } else if (solver->value(lit) == l_True) {
            //became set meanwhile, our emitted unit is a redundant copy
            if (fr) *solver->frat << del << failed_hints[at].unit_id << lit << fin;
        } else {
            assert(solver->value(lit) == l_False);
            *solver->frat << add << ++solver->clauseID;
            if (fr) {
                assert(solver->unit_cl_IDs[lit.var()] != 0);
                *solver->frat << fratchain
                    << solver->unit_cl_IDs[lit.var()] << failed_hints[at].unit_id;
            }
            *solver->frat << fin;
            set_unsat_cl_id(solver->clauseID);
            solver->ok = false;
            return false;
        }
    }
    failed.clear();
    failed_hints.clear();
    failed_ids.clear();

    return true;
}


// (lit V otherlit) exists -> (~otherlit, lit) in queue
// Next: (~otherLit, lit2) exists -> (~lit2, ~otherLit) in queue
// --> original ~otherlit got enqueued by lit2 = False (--> PropBy(lit2) ).

void InTree::enqueue(const Lit lit, const Lit other_lit, const bool red_cl, const int32_t ID)
{
    queue.push_back(QueueElem(lit, other_lit, red_cl, ID));
    assert(!seen[lit.toInt()]);
    seen[lit.toInt()] = 1;
    assert(solver->value(lit) == l_Undef);

    watch_subarray ws = solver->watches[lit];
    for(Watched& w: ws) {
        if (w.isBin()
            && seen[(~w.lit2()).toInt()] == 0
            && solver->value(w.lit2()) == l_Undef
        ) {
            //Mark both
            w.mark_bin_cl();
            Watched& other_w = findWatchedOfBin(
                solver->watches, w.lit2(), lit, w.red(), w.get_id());
            other_w.mark_bin_cl();

            enqueue(~w.lit2(), lit, w.red(), w.get_id());
        }
    }
    queue.push_back(QueueElem(lit_Undef, lit_Undef, false, 0));
}


double InTree::mem_used() const
{
    double mem = 0;
    mem += sizeof(InTree);
    mem += roots.size()*sizeof(Lit);
    mem += failed.size()*sizeof(Lit);
    mem += reset_reason_stack.size()*sizeof(ResetReason);
    mem += queue.size()*sizeof(QueueElem);
    mem += depth_failed.size()*sizeof(char);
    return mem;
}

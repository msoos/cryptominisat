#include "intree.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "sqlstats.h"
#include "watchalgos.h"

using namespace CMSat;

InTree::InTree(Solver* _solver) :
    solver(_solver)
    , seen(_solver->seen)
{}

bool InTree::replace_until_fixedpoint()
{
    uint32_t last_replace = std::numeric_limits<uint32_t>::max();
    uint32_t this_replace = solver->varReplacer->get_num_replaced_vars();
    while(last_replace != this_replace) {
        last_replace = this_replace;
        solver->clauseCleaner->remove_and_clean_all();
        bool OK = solver->varReplacer->replace_if_enough_is_found();
        if (!OK) {
            return false;
        }
        this_replace = solver->varReplacer->get_num_replaced_vars();
    }

    return true;
}

bool InTree::watches_only_contains_nonbin(const Lit lit) const
{
    watch_subarray_const ws = solver->watches[lit.toInt()];
    for(const Watched w: ws) {
        if (w.isBinary()) {
            return false;
        }
    }

    return true;
}

void InTree::fill_roots()
{
    //l is root if no clause of form (l, l2).

    roots.clear();
    for(uint32_t i = 0; i < solver->nVars()*2; i++)
    {
        Lit lit(i/2, i%2);
        if (watches_only_contains_nonbin(lit))
        {
            roots.push_back(lit);
        }
    }
}

bool InTree::intree_probe()
{
    assert(solver->okay());
    queue.clear();
    reset_reason_stack.clear();
    solver->use_depth_trick = false;
    solver->perform_transitive_reduction = false;
    hyperbin_added = 0;
    removedIrredBin = 0;
    removedRedBin = 0;

    if (!replace_until_fixedpoint())
    {
        return false;
    }

    double myTime = cpuTime();
    bogoprops_to_use = solver->conf.intree_timeoutM*1000ULL*1000ULL;
    bogoprops_remain = bogoprops_to_use;

    fill_roots();
    randomize_roots();

    //Let's enqueue all ~root -s.
    for(Lit lit: roots) {
        enqueue(~lit, lit_Undef, false);
    }

    //clear seen
    for(QueueElem elem: queue) {
        if (elem.propagated != lit_Undef) {
            seen[elem.propagated.toInt()] = 0;
        }
    }
    const size_t orig_num_free_vars = solver->get_num_free_vars();

    tree_look();
    unmark_all_bins();

    const double time_used = cpuTime() - myTime;
    const double time_remain = (double)bogoprops_remain/(double)bogoprops_to_use;
    const bool time_out = (bogoprops_remain < 0);

    if (solver->conf.verbosity >= 2) {
        cout << "c [intree] Set "
        << (orig_num_free_vars - solver->get_num_free_vars())
        << " vars"
        << " hyper-added: " << hyperbin_added
        << " trans-irred::" << removedIrredBin
        << " trans-red::" << removedRedBin
        << solver->conf.print_times(time_used,  time_out, time_remain)
        << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "intree"
            , time_used
            , time_out
            , time_remain
        );
    }

    solver->use_depth_trick = true;
    solver->perform_transitive_reduction = true;
    return solver->okay();
}

void InTree::unmark_all_bins()
{
    for(watch_subarray wsub: solver->watches) {
        for(Watched& w: wsub) {
            if (w.isBinary()) {
                w.unmark_bin_cl();
            }
        }
    }
}

void InTree::randomize_roots()
{
    for (size_t i = 0
        ; i + 1< roots.size()
        ; i++
    ) {
        std::swap(
            roots[i]
            , roots[i+solver->mtrand.randInt(roots.size()-1-i)]
        );
    }
}

bool InTree::tree_look()
{
    assert(failed.empty());
    depth_failed.clear();
    depth_failed.push_back(false);
    int64_t orig_bogoprops = solver->propStats.bogoProps;

    while(!queue.empty())
    {
        if (((int64_t)solver->propStats.bogoProps - orig_bogoprops) > bogoprops_remain) {
            break;
        }

        const QueueElem elem = queue.front();
        queue.pop_front();

        if (solver->conf.verbosity >= 10) {
            cout << "Dequeued [[" << elem << "]] dec lev:"
            << solver->decisionLevel() << endl;
        }

        if (elem.propagated != lit_Undef) {
            handle_lit_popped_from_queue(elem.propagated, elem.other_lit, elem.red);
        } else {
            assert(solver->decisionLevel() > 0);
            solver->cancelUntil(solver->decisionLevel()-1);

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
                        cout << "RESet reason for VAR " << tmp.var_reason_changed+1 << " to: " << tmp.orig_propby.lit2() << " red: " << (int)tmp.orig_propby.isRedStep() << endl;
                    }
                }
            }
        }

        if (solver->decisionLevel() == 0) {
            if (!empty_failed_list()) {
                return false;
            }
        }
    }

    bogoprops_remain -= (int64_t)solver->propStats.bogoProps - orig_bogoprops;

    solver->cancelUntil(0);
    return empty_failed_list();
}

void InTree::handle_lit_popped_from_queue(const Lit lit, const Lit other_lit, const bool red)
{
    solver->new_decision_level();
    depth_failed.push_back(depth_failed.back());
    if (other_lit != lit_Undef) {
        reset_reason_stack.push_back(ResetReason(var_Undef, PropBy()));
    }

    if (solver->value(lit) == l_False
        || depth_failed.back() == 1
    ) {
        //l is failed.
        failed.push_back(~lit);
        if (solver->conf.verbosity >= 10) {
            cout << "Failed :" << ~lit << " level: " << solver->decisionLevel() << endl;
        }

        return;
    }

    if (other_lit != lit_Undef) {
        //update 'other_lit' 's ancestor to 'lit'
        assert(solver->value(other_lit) == l_True);
        reset_reason_stack.back() = ResetReason(other_lit.var(), solver->varData[other_lit.var()].reason);
        solver->varData[other_lit.var()].reason = PropBy(~lit, red, false, false);
        if (solver->conf.verbosity >= 10) {
            cout << "Set reason for VAR " << other_lit.var()+1 << " to: " << ~lit << " red: " << (int)red << endl;
        }
    }

    if (solver->value(lit) == l_Undef) {
        solver->enqueue(lit);

        //Should do HHBR here
        int64_t timeout = std::numeric_limits<int64_t>::max();
        Lit ret = solver->propagate_bfs(
            timeout //early-abort timeout
        );
        //const bool ok = solver->propagate().isNULL();
        if (ret != lit_Undef) {
            depth_failed.back() = 1;
            failed.push_back(~lit);
            if (solver->conf.verbosity >= 10) {
                cout << "Failed :" << ~lit << " level: " << solver->decisionLevel() << endl;
            }
        } else {
            hyperbin_added += solver->hyper_bin_res_all(false);
            std::pair<size_t, size_t> tmp = solver->remove_useless_bins(true);
            removedIrredBin += tmp.first;
            removedRedBin += tmp.second;
        }
    }
}

bool InTree::empty_failed_list()
{
    assert(solver->decisionLevel() == 0);
    for(const Lit lit: failed) {
        if (!solver->ok) {
            return false;
        }

        if (solver->value(lit) == l_Undef) {
            solver->enqueue(lit);
            solver->ok = solver->propagate().isNULL();
            if (!solver->ok) {
                return false;
            }
        } else if (solver->value(lit) == l_False) {
            solver->ok = false;
            return false;
        }
    }
    failed.clear();

    return true;
}


// (lit V otherlit) exists -> (~otherlit, lit) in queue
// Next: (~otherLit, lit2) exists -> (~lit2, ~otherLit) in queue
// --> original ~otherlit got enqueued by lit2 = False (--> PropBy(lit2) ).

void InTree::enqueue(const Lit lit, const Lit other_lit, bool red_cl)
{
    queue.push_back(QueueElem(lit, other_lit, red_cl));
    assert(!seen[lit.toInt()]);
    seen[lit.toInt()] = 1;

    watch_subarray ws = solver->watches[lit.toInt()];
    for(Watched& w: ws) {
        if (w.isBinary()
            && seen[(~w.lit2()).toInt()] == 0
        ) {
            //Mark both
            //w.mark_bin_cl();
            //Watched& other_w = findWatchedOfBin(solver->watches, w.lit2(), lit, w.red());
            //other_w.mark_bin_cl();

            enqueue(~w.lit2(), lit, w.red());
        }
    }
    queue.push_back(QueueElem(lit_Undef, lit_Undef, false));
}

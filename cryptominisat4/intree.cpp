#include "intree.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "sqlstats.h"

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
        enqueue(~lit);
    }

    //clear seen
    for(Lit lit: queue) {
        if (lit != lit_Undef) {
            seen[lit.toInt()] = 0;
        }
    }
    const size_t orig_num_free_vars = solver->get_num_free_vars();

    bool ret = tree_look();

    const double time_used = cpuTime() - myTime;
    const double time_remain = (double)bogoprops_remain/(double)bogoprops_to_use;
    const bool time_out = (bogoprops_remain < 0);

    cout << "c [intree] Set "
    << (orig_num_free_vars - solver->get_num_free_vars())
    << " vars"
    << solver->conf.print_times(time_used,  time_out, time_remain)
    << endl;
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "intree"
            , time_used
            , time_out
            , time_remain
        );
    }

    return ret;
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
        if (solver->conf.verbosity >= 10) {
            cout << "At queue point: " << queue.size() << endl;
        }

        if (((int64_t)solver->propStats.bogoProps - orig_bogoprops) > bogoprops_remain) {
            break;
        }

        const Lit l = queue.front();
        queue.pop_front();

        if (solver->conf.verbosity >= 10) {
            cout << "Dequeued: " << l << endl;
        }

        if (l != lit_Undef) {
            handle_lit_popped_from_queue(l);
        } else {
            assert(solver->decisionLevel() > 0);
            solver->cancelUntil(solver->decisionLevel()-1);
            depth_failed.pop_back();
            assert(!depth_failed.empty());
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

void InTree::handle_lit_popped_from_queue(const Lit lit)
{
    solver->new_decision_level();
    depth_failed.push_back(depth_failed.back());

    if (solver->value(lit) == l_False
        || depth_failed.back() == 1
    ) {
        //l is failed.
        failed.push_back(~lit);
        if (solver->conf.verbosity >= 10) {
            cout << "Failed :" << ~lit << " level: " << solver->decisionLevel() << endl;
        }

    } else if (solver->value(lit) == l_Undef) {
        solver->enqueue(lit);
        //Should do HHBR here
        const bool ok = solver->propagate().isNULL();
        if (!ok) {
            depth_failed.back() = 1;
            failed.push_back(~lit);
            if (solver->conf.verbosity >= 10) {
                cout << "Failed :" << ~lit << " level: " << solver->decisionLevel() << endl;
            }
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

void InTree::enqueue(const Lit lit)
{
    queue.push_back(lit);
    assert(!seen[lit.toInt()]);
    seen[lit.toInt()] = 1;

    watch_subarray_const ws = solver->watches[lit.toInt()];
    for(const Watched w: ws) {
        if (w.isBinary()
            && seen[(~w.lit2()).toInt()] == 0
        ) {
            enqueue(~w.lit2());
        }
    }
    queue.push_back(lit_Undef);
}

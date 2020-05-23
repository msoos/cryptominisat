/******************************************
Copyright (c) 2016, Mate Soos

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

#include "propengine.h"
#include <cmath>
#include <string.h>
#include <algorithm>
#include <limits.h>
#include <vector>
#include <iomanip>
#include <algorithm>

#include "solver.h"
#include "clauseallocator.h"
#include "clause.h"
#include "time_mem.h"
#include "varupdatehelper.h"
#include "watchalgos.h"
#include "sqlstats.h"

using namespace CMSat;
using std::cout;
using std::endl;

//#define DEBUG_ENQUEUE_LEVEL0
//#define VERBOSE_DEBUG_POLARITIES
//#define DEBUG_DYNAMIC_RESTART
//#define ALTERNATE_WATCH

/**
@brief Sets a sane default config and allocates handler classes
*/
PropEngine::PropEngine(
    const SolverConf* _conf
    , Solver* _solver
    , std::atomic<bool>* _must_interrupt_inter
) :
        CNF(_conf, _must_interrupt_inter)
        , order_heap_vsids(VarOrderLt(var_act_vsids))
        , order_heap_maple(VarOrderLt(var_act_maple))
        , qhead(0)
        , solver(_solver)
{
}

PropEngine::~PropEngine()
{
}

void PropEngine::new_var(const bool bva, uint32_t orig_outer)
{
    CNF::new_var(bva, orig_outer);

    var_act_vsids.insert(var_act_vsids.end(), 1, ActAndOffset());
    var_act_maple.insert(var_act_maple.end(), 1, ActAndOffset());
    #ifdef VMTF_NEEDED
    vmtf_btab.insert(vmtf_btab.end(), 1, 0);
    vmtf_links.insert(vmtf_links.end(), 1, Link());
    #endif

    //TODO
    //trail... update x->whatever
}

void PropEngine::new_vars(size_t n)
{
    CNF::new_vars(n);

    var_act_vsids.insert(var_act_vsids.end(), n, ActAndOffset());
    var_act_maple.insert(var_act_maple.end(), n, ActAndOffset());
    #ifdef VMTF_NEEDED
    vmtf_btab.insert(vmtf_btab.end(), n, 0);
    vmtf_links.insert(vmtf_links.end(), n, Link());
    #endif

    //TODO
    //trail... update x->whatever
}

void PropEngine::save_on_var_memory()
{
    CNF::save_on_var_memory();

    var_act_vsids.resize(nVars());
    var_act_maple.resize(nVars());

    var_act_vsids.shrink_to_fit();
    var_act_maple.shrink_to_fit();

}

/**
 @ *brief Attach normal a clause to the watchlists

 Handles 2, 3 and >3 clause sizes differently and specially
 */

void PropEngine::attachClause(
    const Clause& c
    , const bool checkAttach
) {
    const ClOffset offset = cl_alloc.get_offset(&c);

    assert(c.size() > 2);
    if (checkAttach) {
        assert(value(c[0]) == l_Undef);
        assert(value(c[1]) == l_Undef || value(c[1]) == l_False);
    }

    #ifdef DEBUG_ATTACH
    for (uint32_t i = 0; i < c.size(); i++) {
        assert(varData[c[i].var()].removed == Removed::none);
    }
    #endif //DEBUG_ATTACH

    const Lit blocked_lit = c[2];
    watches[c[0]].push(Watched(offset, blocked_lit));
    watches[c[1]].push(Watched(offset, blocked_lit));
}

/**
@brief Detaches a (potentially) modified clause

The first two literals might have chaned through modification, so they are
passed along as arguments -- they are needed to find the correct place where
the clause is
*/
void PropEngine::detach_modified_clause(
    const Lit lit1
    , const Lit lit2
    , const Clause* address
) {
    ClOffset offset = cl_alloc.get_offset(address);
    removeWCl(watches[lit1], offset);
    removeWCl(watches[lit2], offset);
}

/**
@brief Propagates a binary clause

Need to be somewhat tricky if the clause indicates that current assignment
is incorrect (i.e. both literals evaluate to FALSE). If conflict if found,
sets failBinLit
*/
template<bool update_bogoprops>
inline bool PropEngine::prop_bin_cl(
    const Watched* i
    , const Lit p
    , PropBy& confl
    , uint32_t currLevel
) {
    const lbool val = value(i->lit2());
    if (val == l_Undef) {
        #ifdef STATS_NEEDED
        if (i->red())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;
        #endif

        enqueue<update_bogoprops>(i->lit2(), currLevel, PropBy(~p, i->red()));
    } else if (val == l_False) {
        #ifdef STATS_NEEDED
        if (i->red())
            lastConflictCausedBy = ConflCausedBy::binred;
        else
            lastConflictCausedBy = ConflCausedBy::binirred;
        #endif

        confl = PropBy(~p, i->red());
        failBinLit = i->lit2();
        qhead = trail.size();
        return false;
    }

    return true;
}

template<bool update_bogoprops>
inline
bool PropEngine::prop_long_cl_any_order(
    Watched* i
    , Watched*& j
    , const Lit p
    , PropBy& confl
    , uint32_t currLevel
) {
    //Blocked literal is satisfied, so clause is satisfied
    if (value(i->getBlockedLit()) == l_True) {
        *j++ = *i;
        return true;
    }
    if (update_bogoprops) {
        propStats.bogoProps += 4;
    }
    const ClOffset offset = i->get_offset();
    Clause& c = *cl_alloc.ptr(offset);

    #ifdef SLOW_DEBUG
    assert(!c.getRemoved());
    assert(!c.freed());
    #endif

    if (prop_normal_helper(c, offset, j, p) == PROP_NOTHING) {
        return true;
    }

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        handle_normal_prop_fail(c, offset, confl);
        return false;
    } else {
        #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
        c.stats.propagations_made++;
        #endif
        #ifdef STATS_NEEDED
        c.stats.sum_propagations_made++;
        if (c.red())
            propStats.propsLongRed++;
        else
            propStats.propsLongIrred++;
        #endif

        if (currLevel == decisionLevel()) {
            enqueue<update_bogoprops>(c[0], currLevel, PropBy(offset));
        } else {
            uint32_t nMaxLevel = currLevel;
            uint32_t nMaxInd = 1;
            // pass over all the literals in the clause and find the one with the biggest level
            for (uint32_t nInd = 2; nInd < c.size(); ++nInd) {
                uint32_t nLevel = varData[c[nInd].var()].level;
                if (nLevel > nMaxLevel) {
                    nMaxLevel = nLevel;
                    nMaxInd = nInd;
                }
            }

            if (nMaxInd != 1) {
                std::swap(c[1], c[nMaxInd]);
                j--; // undo last watch
                watches[c[1]].push(*i);
            }

            enqueue<update_bogoprops>(c[0], nMaxLevel, PropBy(offset));
        }
    }

    return true;
}

PropBy PropEngine::propagate_any_order_fast()
{
    PropBy confl;

    #ifdef VERBOSE_DEBUG_PROP
    cout << "Fast Propagation started" << endl;
    #endif

    //so we don't re-calculate it all the time
    uint32_t declevel = decisionLevel();

    int64_t num_props = 0;
    while (qhead < trail.size()) {
        const Lit p = trail[qhead].lit;     // 'p' is enqueued fact to propagate.
        const uint32_t currLevel = trail[qhead].lev;
        qhead++;
        watch_subarray ws = watches[~p];
        Watched* i;
        Watched* j;
        Watched* end;
        num_props++;

        for (i = j = ws.begin(), end = ws.end(); unlikely(i != end);) {
            //Prop bin clause
            if (i->isBin()) {
                assert(j < end);
                *j++ = *i;
                const lbool val = value(i->lit2());
                if (val == l_Undef) {
                    #ifdef STATS_NEEDED
                    if (i->red())
                        propStats.propsBinRed++;
                    else
                        propStats.propsBinIrred++;
                    #endif
                    enqueue<false>(i->lit2(), currLevel, PropBy(~p, i->red()));
                    i++;
                } else if (val == l_False) {
                    confl = PropBy(~p, i->red());
                    failBinLit = i->lit2();
                    #ifdef STATS_NEEDED
                    if (i->red())
                        lastConflictCausedBy = ConflCausedBy::binred;
                    else
                        lastConflictCausedBy = ConflCausedBy::binirred;
                    #endif
                    i++;
                    while (i < end) {
                        *j++ = *i++;
                    }
                    qhead = trail.size();
                } else {
                    i++;
                }
                continue;
            }

            //propagate normal clause
            //assert(i->isClause());
            Lit blocked = i->getBlockedLit();
            if (likely(value(blocked) == l_True)) {
                *j++ = *i++;
                continue;
            }

            const ClOffset offset = i->get_offset();
            Clause& c = *cl_alloc.ptr(offset);
            Lit      false_lit = ~p;
            if (c[0] == false_lit) {
                c[0] = c[1], c[1] = false_lit;
            }
            assert(c[1] == false_lit);
            i++;

            Lit     first = c[0];
            Watched w     = Watched(offset, first);
            if (first != blocked && value(first) == l_True) {
                *j++ = w;
                continue;
            }

            // Look for new watch:
            for (uint32_t k = 2; k < c.size(); k++) {
                //Literal is either unset or satisfied, attach to other watchlist
                if (likely(value(c[k]) != l_False)) {
#ifndef ALTERNATE_WATCH
                    c[1] = c[k];
                    c[k] = false_lit;
#else
                    //code by Jo Devriendt
                    if (k >= 4) {
                        std::swap(c[1], c[k]);
                        std::swap(c[k/2+1], c[k]);
                    } else {
                        c[1] = c[k];
                        c[k] = false_lit;
                    }
#endif

                    watches[c[1]].push(w);
                    goto nextClause;
                }
            }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(c[0]) == l_False) {
                confl = PropBy(offset);
                #ifdef STATS_NEEDED
                if (c.red())
                    lastConflictCausedBy = ConflCausedBy::longred;
                else
                    lastConflictCausedBy = ConflCausedBy::longirred;
                #endif
                #ifdef VERBOSE_DEBUG
                cout << "Conflicting on clause:" << c << endl;
                #endif
                while (i < end) {
                    *j++ = *i++;
                }
                assert(j <= end);
                qhead = trail.size();
            } else {
                #if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
                c.stats.propagations_made++;
                #endif
                #ifdef STATS_NEEDED
                if (c.red())
                    propStats.propsLongRed++;
                else
                    propStats.propsLongIrred++;
                #endif
                if (currLevel == declevel) {
                    enqueue<false>(c[0], currLevel, PropBy(offset));
                } else {
                    uint32_t nMaxLevel = currLevel;
                    uint32_t nMaxInd = 1;
                    // pass over all the literals in the clause and find the one with the biggest level
                    for (uint32_t nInd = 2; nInd < c.size(); ++nInd) {
                        uint32_t nLevel = varData[c[nInd].var()].level;
                        if (nLevel > nMaxLevel) {
                            nMaxLevel = nLevel;
                            nMaxInd = nInd;
                        }
                    }

                    if (nMaxInd != 1) {
                        std::swap(c[1], c[nMaxInd]);
                        j--; // undo last watch
                        watches[c[1]].push(w);
                    }

                    enqueue<false>(c[0], nMaxLevel, PropBy(offset));
                }
            }

            nextClause:;
        }
        ws.shrink_(i-j);
    }
    qhead = trail.size();
    simpDB_props -= num_props;
    propStats.propagations += (uint64_t)num_props;

    #ifdef VERBOSE_DEBUG
    cout << "Propagation (propagate_any_order_fast) ended." << endl;
    #endif

    return confl;
}

template<bool update_bogoprops>
PropBy PropEngine::propagate_any_order()
{
    PropBy confl;

    #ifdef VERBOSE_DEBUG_PROP
    cout << "Fast Propagation started" << endl;
    #endif

    while (qhead < trail.size() && confl.isNULL()) {
        const Lit p = trail[qhead].lit;     // 'p' is enqueued fact to propagate.
        watch_subarray ws = watches[~p];
        uint32_t currLevel = trail[qhead].lev;

        Watched* i = ws.begin();
        Watched* j = i;
        Watched* end = ws.end();
        if (update_bogoprops) {
            propStats.bogoProps += ws.size()/4 + 1;
        }
        propStats.propagations++;
        for (; i != end; i++) {
            if (likely(i->isBin())) {
                *j++ = *i;
                if (!prop_bin_cl<update_bogoprops>(i, p, confl, currLevel)) {
                    i++;
                    break;
                }
                continue;
            }

            //propagate normal clause
            if (!prop_long_cl_any_order<update_bogoprops>(i, j, p, confl, currLevel)) {
                i++;
                break;
            }
            continue;
        }
        while (i != end) {
            *j++ = *i++;
        }
        ws.shrink_(end-j);
        qhead++;
    }

    #ifdef VERBOSE_DEBUG
    cout << "Propagation (propagate_any_order) ended." << endl;
    #endif

    return confl;
}
template PropBy PropEngine::propagate_any_order<true>();
template PropBy PropEngine::propagate_any_order<false>();


void PropEngine::printWatchList(const Lit lit) const
{
    watch_subarray_const ws = watches[lit];
    for (const Watched *it2 = ws.begin(), *end2 = ws.end()
        ; it2 != end2
        ; it2++
    ) {
        if (it2->isBin()) {
            cout << "bin: " << lit << " , " << it2->lit2() << " red : " <<  (it2->red()) << endl;
        } else if (it2->isClause()) {
            cout << "cla:" << it2->get_offset() << endl;
        } else {
            assert(false);
        }
    }
}

void PropEngine::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    //Trail is NOT correct, only its length is correct
    for(Trail& t: trail) {
        t.lit = lit_Undef;
    }
}

void PropEngine::print_trail()
{
    for(size_t i = trail_lim[0]; i < trail.size(); i++) {
        assert(varData[trail[i].lit.var()].level == trail[i].lev);
        cout
        << "trail " << i << ":" << trail[i].lit
        << " lev: " << trail[i].lev
        << " reason: " << varData[trail[i].lit.var()].reason
        << endl;
    }
}


bool PropEngine::propagate_occur()
{
    assert(ok);
    assert(decisionLevel() == 0);

    while (qhead < trail_size()) {
        const Lit p = trail[qhead].lit;
        qhead++;
        watch_subarray ws = watches[~p];

        //Go through each occur
        for (const Watched* it = ws.begin(), *end = ws.end()
            ; it != end
            ; ++it
        ) {
            if (it->isClause()) {
                if (!propagate_long_clause_occur(it->get_offset()))
                    return false;
            }

            if (it->isBin()) {
                if (!propagate_binary_clause_occur(*it))
                    return false;
            }
        }
    }

    return true;
}

bool PropEngine::propagate_binary_clause_occur(const Watched& ws)
{
    const lbool val = value(ws.lit2());
    if (val == l_False) {
        return false;
    }

    if (val == l_Undef) {
        enqueue(ws.lit2());
        #ifdef STATS_NEEDED
        if (ws.red())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;
        #endif
    }

    return true;
}

bool PropEngine::propagate_long_clause_occur(const ClOffset offset)
{
    const Clause& cl = *cl_alloc.ptr(offset);
    assert(!cl.freed() && "Cannot be already removed in occur");
    if (cl.getRemoved())
        return true;

    Lit lastUndef = lit_Undef;
    uint32_t numUndef = 0;
    bool satisfied = false;
    for (const Lit lit: cl) {
        const lbool val = value(lit);
        if (val == l_True) {
            satisfied = true;
            break;
        }
        if (val == l_Undef) {
            numUndef++;
            if (numUndef > 1) break;
            lastUndef = lit;
        }
    }
    if (satisfied)
        return true;

    //Problem is UNSAT
    if (numUndef == 0) {
        return false;
    }

    if (numUndef > 1)
        return true;

    enqueue(lastUndef);
    #ifdef STATS_NEEDED
    if (cl.red())
        propStats.propsLongRed++;
    else
        propStats.propsLongIrred++;
    #endif

    return true;
}

void PropEngine::save_state(SimpleOutFile& f) const
{
    f.put_vector(trail);
    f.put_uint32_t(qhead);

    CNF::save_state(f);
}

void PropEngine::load_state(SimpleInFile& f)
{
    f.get_vector(trail);
    qhead = f.get_uint32_t();

    CNF::load_state(f);
}

#ifdef STATS_NEEDED_BRANCH
void PropEngine::sql_dump_vardata_picktime(uint32_t v, PropBy from)
{
    if (!solver->sqlStats)
        return;

    bool dump = false;
    double rnd_num = solver->mtrand.randDblExc();
    if (rnd_num <= conf.dump_individual_cldata_ratio*0.1) {
        dump = true;
    }
    varData[v].dump = dump;
    if (!dump)
        return;

    solver->dump_restart_sql(rst_dat_type::var);

    uint64_t outer_var = map_inter_to_outer(v);

    varData[v].sumDecisions_at_picktime = sumDecisions;
    varData[v].sumConflicts_at_picktime = sumConflicts;
    varData[v].sumAntecedents_at_picktime = sumAntecedents;
    varData[v].sumAntecedentsLits_at_picktime = sumAntecedentsLits;
    varData[v].sumConflictClauseLits_at_picktime = sumConflictClauseLits;
    varData[v].sumPropagations_at_picktime = sumPropagations;
    varData[v].sumDecisionBasedCl_at_picktime = sumDecisionBasedCl;
    varData[v].sumClLBD_at_picktime = sumClLBD;
    varData[v].sumClSize_at_picktime = sumClSize;
    double rel_activity_at_picktime =
        std::log2(var_act_vsids[v]+10e-300)/std::log2(max_vsids_act+10e-300);

    varData[v].last_time_set_was_dec = (from == PropBy());

    //inside data
    varData[v].inside_conflict_clause_glue_at_picktime = varData[v].inside_conflict_clause_glue;
    varData[v].inside_conflict_clause_at_picktime = varData[v].inside_conflict_clause;
    varData[v].inside_conflict_clause_antecedents_at_picktime = varData[v].inside_conflict_clause_antecedents;

    solver->sqlStats->var_data_picktime(
        solver
        , outer_var
        , varData[v]
        , rel_activity_at_picktime
    );
}
#endif

#ifdef VMTF_NEEDED
// Update queue to point to last potentially still unassigned variable.
// All variables after 'queue.unassigned' in bump order are assumed to be
// assigned.  Then update the 'queue.vmtf_bumped' field and log it.  This is
// inlined here since it occurs in several inner loops.
//
void PropEngine::vmtf_update_queue_unassigned (uint32_t idx) {
    assert(idx != std::numeric_limits<uint32_t>::max());
    vmtf_queue.unassigned = idx;
    vmtf_queue.vmtf_bumped = vmtf_btab[idx];
}

void PropEngine::vmtf_init_enqueue (uint32_t var) {
    Link & l = vmtf_links[var];
    l.next = std::numeric_limits<uint32_t>::max();
    if (vmtf_queue.last != std::numeric_limits<uint32_t>::max()) {
        assert(vmtf_links[vmtf_queue.last].next == std::numeric_limits<uint32_t>::max());
        vmtf_links[vmtf_queue.last].next = var;
    } else {
        assert(vmtf_queue.first == std::numeric_limits<uint32_t>::max());
        vmtf_queue.first = var;
    }
    vmtf_btab[var] = ++vmtf_queue.vmtf_bumped;
    l.prev = vmtf_queue.last;
    vmtf_queue.last = var;
    vmtf_update_queue_unassigned(vmtf_queue.last);
}

// Move vmtf_bumped variables to the front of the (VMTF) decision queue.  The
// 'vmtf_bumped' time stamp is updated accordingly.  It is used to determine
// whether the 'queue.assigned' pointer has to be moved in 'unassign'.

void PropEngine::vmtf_bump_queue (uint32_t var) {
    if (vmtf_links[var].next == std::numeric_limits<uint32_t>::max()) {
        return;
    }
    //Remove from wherever it is, put to the top
    vmtf_queue.dequeue (vmtf_links, var);
    vmtf_queue.enqueue (vmtf_links, var);

    assert (vmtf_queue.vmtf_bumped != std::numeric_limits<uint32_t>::max());
    vmtf_btab[var] = ++vmtf_queue.vmtf_bumped;
    //LOG ("moved to front variable %d and vmtf_bumped to %" PRId64 "", idx, vmtf_btab[idx]);
    if (value(var) == l_Undef) {
        vmtf_update_queue_unassigned(var);
    }
}
#endif

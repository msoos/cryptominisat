/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

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

using namespace CMSat;
using std::cout;
using std::endl;

//#define DEBUG_ENQUEUE_LEVEL0
//#define VERBOSE_DEBUG_POLARITIES
//#define DEBUG_DYNAMIC_RESTART

/**
@brief Sets a sane default config and allocates handler classes
*/
PropEngine::PropEngine(
    const SolverConf& _conf, bool* _needToInterrupt
) :
        CNF(_conf, _needToInterrupt)
        // Stats
        , qhead(0)
        , agility(_conf.agilityG, _conf.agilityLimit)
{
}

PropEngine::~PropEngine()
{
}

void PropEngine::new_var(const bool bva, Var orig_outer)
{
    CNF::new_var(bva, orig_outer);
    //TODO
    //trail... update x->whatever
}

void PropEngine::new_vars(size_t n)
{
    CNF::new_vars(n);
    //TODO
    //trail... update x->whatever
}

void PropEngine::saveVarMem()
{
    CNF::saveVarMem();
}


void PropEngine::attachTriClause(
    Lit lit1
    , Lit lit2
    , Lit lit3
    , const bool red
) {
    #ifdef DEBUG_ATTACH
    assert(lit1.var() != lit2.var());
    assert(value(lit1.var()) == l_Undef);
    assert(value(lit2) == l_Undef || value(lit2) == l_False);

    assert(varData[lit1.var()].removed == Removed::none
            || varData[lit1.var()].removed == Removed::queued_replacer);
    assert(varData[lit2.var()].removed == Removed::none
            || varData[lit2.var()].removed == Removed::queued_replacer);
    #endif //DEBUG_ATTACH

    //Order them
    orderLits(lit1, lit2, lit3);

    //And now they are attached, ordered
    watches[lit1.toInt()].push(Watched(lit2, lit3, red));
    watches[lit2.toInt()].push(Watched(lit1, lit3, red));
    watches[lit3.toInt()].push(Watched(lit1, lit2, red));
}

void PropEngine::detachTriClause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
    , const bool allow_empty_watch
) {
    Lit lits[3];
    lits[0] = lit1;
    lits[1] = lit2;
    lits[2] = lit3;
    std::sort(lits, lits+3);
    if (!(allow_empty_watch && watches[lits[0].toInt()].empty())) {
        removeWTri(watches, lits[0], lits[1], lits[2], red);
    }
    if (!(allow_empty_watch && watches[lits[1].toInt()].empty())) {
        removeWTri(watches, lits[1], lits[0], lits[2], red);
    }
    if (!(allow_empty_watch && watches[lits[2].toInt()].empty())) {
        removeWTri(watches, lits[2], lits[0], lits[1], red);
    }
}

void PropEngine::detachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const bool allow_empty_watch
) {
    if (!(allow_empty_watch && watches[lit1.toInt()].empty())) {
        removeWBin(watches, lit1, lit2, red);
    }
    if (!(allow_empty_watch && watches[lit2.toInt()].empty())) {
        removeWBin(watches, lit2, lit1, red);
    }
}

void PropEngine::attachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const bool checkUnassignedFirst
) {
    #ifdef DEBUG_ATTACH
    assert(lit1.var() != lit2.var());
    if (checkUnassignedFirst) {
        assert(value(lit1.var()) == l_Undef);
        assert(value(lit2) == l_Undef || value(lit2) == l_False);
    }

    assert(varData[lit1.var()].removed == Removed::none
            || varData[lit1.var()].removed == Removed::queued_replacer);
    assert(varData[lit2.var()].removed == Removed::none
            || varData[lit2.var()].removed == Removed::queued_replacer);
    #endif //DEBUG_ATTACH

    watches[lit1.toInt()].push(Watched(lit2, red));
    watches[lit2.toInt()].push(Watched(lit1, red));
}

/**
 @ *brief Attach normal a clause to the watchlists

 Handles 2, 3 and >3 clause sizes differently and specially
 */

void PropEngine::attachClause(
    const Clause& c
    , const bool checkAttach
) {
    assert(c.size() > 3);
    if (checkAttach) {
        assert(value(c[0]) == l_Undef);
        assert(value(c[1]) == l_Undef || value(c[1]) == l_False);
    }

    #ifdef DEBUG_ATTACH
    for (uint32_t i = 0; i < c.size(); i++) {
        assert(varData[c[i].var()].removed == Removed::none
                || varData[c[i].var()].removed == Removed::queued_replacer);
    }
    #endif //DEBUG_ATTACH

    const ClOffset offset = clAllocator.getOffset(&c);

    //blocked literal is the lit in the middle (c.size()/2). For no reason.
    watches[c[0].toInt()].push(Watched(offset, c[c.size()/2]));
    watches[c[1].toInt()].push(Watched(offset, c[c.size()/2]));
}

/**
@brief Detaches a (potentially) modified clause

The first two literals might have chaned through modification, so they are
passed along as arguments -- they are needed to find the correct place where
the clause is
*/
void PropEngine::detachModifiedClause(
    const Lit lit1
    , const Lit lit2
    , const uint32_t origSize
    , const Clause* address
) {
    assert(origSize > 3);

    ClOffset offset = clAllocator.getOffset(address);
    removeWCl(watches[lit1.toInt()], offset);
    removeWCl(watches[lit2.toInt()], offset);
}

/**
@brief Propagates a binary clause

Need to be somewhat tricky if the clause indicates that current assignement
is incorrect (i.e. both literals evaluate to FALSE). If conflict if found,
sets failBinLit
*/
inline bool PropEngine::propBinaryClause(
    watch_subarray_const::const_iterator i
    , const Lit p
    , PropBy& confl
) {
    const lbool val = value(i->lit2());
    if (val == l_Undef) {
        #ifdef STATS_NEEDED
        if (i->red())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;
        #endif

        enqueue(i->lit2(), PropBy(~p));
    } else if (val == l_False) {
        //Update stats
        if (i->red())
            lastConflictCausedBy = ConflCausedBy::binred;
        else
            lastConflictCausedBy = ConflCausedBy::binirred;

        confl = PropBy(~p);
        failBinLit = i->lit2();
        qhead = trail.size();
        return false;
    }

    return true;
}


void PropEngine::lazy_hyper_bin_resolve(
    const Clause& c
    , ClOffset offset
) {
    //Do lazy hyper-binary resolution if possible
    const Lit other = varData[c[1].var()].reason.lit2();
    bool OK = true;
    for(uint32_t i = 2; i < c.size(); i++) {
        if (varData[c[i].var()].reason.getType() != binary_t
            || other != varData[c[i].var()].reason.lit2()
        ) {
            OK = false;
            break;
        }
    }

    //Is it possible?
    if (OK) {
        attachBinClause(other, c[0], true, false);
        #ifdef STATS_NEEDED
        propStats.longLHBR++;
        #endif

        *drup << other << c[0] << fin;
        enqueue(c[0], PropBy(other));
    } else {
        //no, not possible, just enqueue as normal
        enqueue(c[0], PropBy(offset));
    }
}

void PropEngine::update_glue(Clause& c)
{
    if (c.red()
        && c.stats.glue > 2
        && conf.updateGlues
    ) {
        uint32_t newGlue = calcGlue(c);
        c.stats.glue = std::min(c.stats.glue, newGlue);
    }
}

PropResult PropEngine::prop_normal_helper(
    Clause& c
    , ClOffset offset
    , watch_subarray::iterator &j
    , const Lit p
) {
    #ifdef STATS_NEEDED
    c.stats.clause_looked_at++;
    c.stats.visited_literals++;
    #endif

    // Make sure the false literal is data[1]:
    if (c[0] == ~p) {
        std::swap(c[0], c[1]);
    }

    assert(c[1] == ~p);

    // If 0th watch is true, then clause is already satisfied.
    if (value(c[0]) == l_True) {
        *j = Watched(offset, c[0]);
        j++;
        return PROP_NOTHING;
    }

    // Look for new watch:
    uint32_t numLitVisited = 0;
    for (Lit *k = c.begin() + 2, *end2 = c.end()
        ; k != end2
        ; k++, numLitVisited++
    ) {
        //Literal is either unset or satisfied, attach to other watchlist
        if (value(*k) != l_False) {
            c[1] = *k;
            #ifdef STATS_NEEDED
            //propStats.bogoProps += numLitVisited/10;
            c.stats.visited_literals+= numLitVisited;
            #endif
            *k = ~p;
            watches[c[1].toInt()].push(Watched(offset, c[0]));
            return PROP_NOTHING;
        }
    }
    #ifdef STATS_NEEDED
    //propStats.bogoProps += numLitVisited/10;
    c.stats.visited_literals+= numLitVisited;
    #endif

    return PROP_TODO;
}

PropResult PropEngine::handle_normal_prop_fail(
    Clause& c
    , ClOffset offset
    , PropBy& confl
) {
    confl = PropBy(offset);
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Conflict from ";
    for(size_t i = 0; i < c.size(); i++) {
        cout  << c[i] << " , ";
    }
    cout << endl;
    #endif //VERBOSE_DEBUG_FULLPROP

    //Update stats
    c.stats.conflicts_made++;
    c.stats.sum_of_branch_depth_conflict += decisionLevel() + 1;
    if (c.red())
        lastConflictCausedBy = ConflCausedBy::longred;
    else
        lastConflictCausedBy = ConflCausedBy::longirred;

    qhead = trail.size();
    return PROP_FAIL;
}

PropResult PropEngine::propNormalClause(
    watch_subarray_const::const_iterator i
    , watch_subarray::iterator &j
    , const Lit p
    , PropBy& confl
) {
    //Blocked literal is satisfied, so clause is satisfied
    if (value(i->getBlockedLit()) == l_True) {
        *j++ = *i;
        return PROP_NOTHING;
    }

    //Dereference pointer
    propStats.bogoProps += 4;
    const ClOffset offset = i->getOffset();
    Clause& c = *clAllocator.getPointer(offset);

    PropResult ret = prop_normal_helper(c, offset, j, p);
    if (ret != PROP_TODO)
        return ret;

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        return handle_normal_prop_fail(c, offset, confl);
    }

    //Update stats
    c.stats.propagations_made++;
    c.stats.sum_of_branch_depth_propagation += decisionLevel() + 1;
    #ifdef STATS_NEEDED
    if (c.red())
        propStats.propsLongRed++;
    else
        propStats.propsLongIrred++;
    #endif

    if (conf.doLHBR
        && varData[c[1].var()].reason.getType() == binary_t
    ) {
        lazy_hyper_bin_resolve(c, offset);
    } else {
        enqueue(c[0], PropBy(offset));
    }

    update_glue(c);

    return PROP_SOMETHING;
}


bool PropEngine::propNormalClauseAnyOrder(
    watch_subarray_const::const_iterator i
    , watch_subarray::iterator &j
    , const Lit p
    , PropBy& confl
) {
    //Blocked literal is satisfied, so clause is satisfied
    if (value(i->getBlockedLit()) == l_True) {
        *j++ = *i;
        return true;
    }
    propStats.bogoProps += 4;
    const ClOffset offset = i->getOffset();
    Clause& c = *clAllocator.getPointer(offset);
    #ifdef STATS_NEEDED
    c.stats.clause_looked_at++;
    c.stats.visited_literals++;
    #endif

    // Make sure the false literal is data[1]:
    if (c[0] == ~p) {
        std::swap(c[0], c[1]);
    }

    assert(c[1] == ~p);

    // If 0th watch is true, then clause is already satisfied.
    if (value(c[0]) == l_True) {
        *j = Watched(offset, c[0]);
        j++;
        return true;
    }

    // Look for new watch:
    #ifdef STATS_NEEDED
    uint numLitVisited = 2;
    #endif
    for (Lit *k = c.begin() + 2, *end2 = c.end()
        ; k != end2
        ; k++
        #ifdef STATS_NEEDED
        , numLitVisited++
        #endif
    ) {
        //Literal is either unset or satisfied, attach to other watchlist
        if (value(*k) != l_False) {
            c[1] = *k;
            //propStats.bogoProps += numLitVisited/10;
            #ifdef STATS_NEEDED
            c.stats.visited_literals+= numLitVisited;
            #endif
            *k = ~p;
            watches[c[1].toInt()].push(Watched(offset, c[0]));
            return true;
        }
    }
    #ifdef STATS_NEEDED
    //propStats.bogoProps += numLitVisited/10;
    c.stats.visited_literals+= numLitVisited;
    #endif

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {

        confl = PropBy(offset);
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Conflict from ";
        for(size_t i = 0; i < c.size(); i++) {
            cout  << c[i] << " , ";
        }
        cout << endl;
        #endif //VERBOSE_DEBUG_FULLPROP

        //Update stats
        c.stats.conflicts_made++;
        c.stats.sum_of_branch_depth_conflict += decisionLevel() + 1;
        if (c.red())
            lastConflictCausedBy = ConflCausedBy::longred;
        else
            lastConflictCausedBy = ConflCausedBy::longirred;

        qhead = trail.size();
        return false;
    } else {

        //Update stats
        c.stats.propagations_made++;
        c.stats.sum_of_branch_depth_propagation += decisionLevel() + 1;
        #ifdef STATS_NEEDED
        if (c.red())
            propStats.propsLongRed++;
        else
            propStats.propsLongIrred++;
        #endif
        enqueue(c[0], PropBy(offset));

        //Update glues?
        if (c.red()
            && c.stats.glue > 2
            && conf.updateGlues
        ) {
            uint32_t newGlue = calcGlue(c);
            c.stats.glue = std::min(c.stats.glue, newGlue);
        }
    }

    return true;
}

PropResult PropEngine::handle_prop_tri_fail(
    watch_subarray_const::const_iterator i
    , Lit lit1
    , PropBy& confl
) {
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Conflict from "
        << lit1 << " , "
        << i->lit2() << " , "
        << i->lit3() << endl;
    #endif //VERBOSE_DEBUG_FULLPROP
    confl = PropBy(~lit1, i->lit3());

    //Update stats
    if (i->red())
        lastConflictCausedBy = ConflCausedBy::trired;
    else
        lastConflictCausedBy = ConflCausedBy::triirred;

    failBinLit = i->lit2();
    qhead = trail.size();
    return PROP_FAIL;
}

PropResult PropEngine::propTriClause(
    watch_subarray_const::const_iterator i
    , const Lit lit1
    , PropBy& confl
) {
    const Lit lit2 = i->lit2();
    lbool val2 = value(lit2);

    //literal is already satisfied, nothing to do
    if (val2 == l_True)
        return PROP_NOTHING;

    const Lit lit3 = i->lit3();
    lbool val3 = value(lit3);

    //literal is already satisfied, nothing to do
    if (val3 == l_True)
        return PROP_NOTHING;

    if (val2 == l_False && val3 == l_False) {
        return handle_prop_tri_fail(i, lit1, confl);
    }

    if (val2 == l_Undef && val3 == l_False) {
        return propTriHelperSimple(lit1, lit2, lit3, i->red());
    }

    if (val3 == l_Undef && val2 == l_False) {
        return propTriHelperSimple(lit1, lit3, lit2, i->red());
    }

    return PROP_NOTHING;
}

inline bool PropEngine::propTriClauseAnyOrder(
    watch_subarray_const::const_iterator i
    , const Lit lit1
    , PropBy& confl
) {
    const Lit lit2 = i->lit2();
    lbool val2 = value(lit2);

    //literal is already satisfied, nothing to do
    if (val2 == l_True)
        return true;

    const Lit lit3 = i->lit3();
    lbool val3 = value(lit3);

    //literal is already satisfied, nothing to do
    if (val3 == l_True)
        return true;

    if (val2 == l_False && val3 == l_False) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Conflict from "
            << lit1 << " , "
            << i->lit2() << " , "
            << i->lit3() << endl;
        #endif //VERBOSE_DEBUG_FULLPROP
        confl = PropBy(~lit1, i->lit3());

        //Update stats
        if (i->red())
            lastConflictCausedBy = ConflCausedBy::trired;
        else
            lastConflictCausedBy = ConflCausedBy::triirred;

        failBinLit = i->lit2();
        qhead = trail.size();
        return false;
    }
    if (val2 == l_Undef && val3 == l_False) {
        propTriHelperAnyOrder(
            lit1
            , lit2
            , lit3
            #ifdef STATS_NEEDED
            , i->red()
            #endif
        );
        return true;
    }

    if (val3 == l_Undef && val2 == l_False) {
        propTriHelperAnyOrder(
            lit1
            , lit3
            , lit2
            #ifdef STATS_NEEDED
            , i->red()
            #endif
        );
        return true;
    }

    return true;
}

void PropEngine::lazy_hyper_bin_resolve(Lit lit1, Lit lit2)
{
    Lit lit= varData[lit1.var()].reason.lit2();

    attachBinClause(lit, lit2, true, false);
    enqueue(lit2, PropBy(lit));
    #ifdef STATS_NEEDED
    propStats.triLHBR++;
    #endif
    *drup << lit << lit2 << fin;
}

bool PropEngine::can_do_lazy_hyper_bin(Lit lit1, Lit lit2, Lit lit3)
{
    bool ret;
    ret = varData[lit1.var()].reason.getType() == binary_t
        && ((varData[lit3.var()].reason.getType() == binary_t
        && varData[lit3.var()].reason.lit2() == varData[lit1.var()].reason.lit2())
        || (varData[lit1.var()].reason.lit2().var() == lit3.var()));

    return ret;
}

PropResult PropEngine::propTriHelperSimple(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    #ifdef STATS_NEEDED
    if (red)
        propStats.propsTriRed++;
    else
        propStats.propsTriIrred++;
    #endif

    //Check if we could do lazy hyper-binary resoution
    if (conf.doLHBR
        && can_do_lazy_hyper_bin(lit1, lit2, lit3)
    ) {
        lazy_hyper_bin_resolve(lit1, lit2);
    } else {
        //Lazy hyper-bin is not possibe
        enqueue(lit2, PropBy(~lit1, lit3));
    }

    return PROP_SOMETHING;
}

inline void PropEngine::propTriHelperAnyOrder(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    #ifdef STATS_NEEDED
    , const bool red
    #endif
) {
    #ifdef STATS_NEEDED
    if (red)
        propStats.propsTriRed++;
    else
        propStats.propsTriIrred++;
    #endif

    //Lazy hyper-bin is not possibe
    enqueue(lit2, PropBy(~lit1, lit3));
}

PropBy PropEngine::propagateAnyOrder()
{
    PropBy confl;

    #ifdef VERBOSE_DEBUG_PROP
    cout << "Fast Propagation started" << endl;
    #endif

    while (qhead < trail.size() && confl.isNULL()) {
        const Lit p = trail[qhead];     // 'p' is enqueued fact to propagate.
        watch_subarray ws = watches[(~p).toInt()];
        watch_subarray::iterator i = ws.begin();
        watch_subarray::iterator j = ws.begin();
        watch_subarray_const::const_iterator end = ws.end();
        propStats.bogoProps += ws.size()/4 + 1;
        for (; i != end; i++) {
            if (i->isBinary()) {
                *j++ = *i;
                if (!propBinaryClause(i, p, confl)) {
                    i++;
                    break;
                }

                continue;
            }

            //Propagate tri clause
            if (i->isTri()) {
                *j++ = *i;
                if (!propTriClauseAnyOrder(i, p, confl)) {
                    i++;
                    break;
                }
                continue;
            }

            //propagate normal clause
            if (i->isClause()) {
                if (!propNormalClauseAnyOrder(i, j, p, confl)) {
                    i++;
                    break;
                }
                continue;
            }
        }
        while (i != end) {
            *j++ = *i++;
        }
        ws.shrink_(end-j);

        qhead++;
    }

    #ifdef VERBOSE_DEBUG
    cout << "Propagation (propagateAnyOrder) ended." << endl;
    #endif

    return confl;
}

void PropEngine::sortWatched()
{
    #ifdef VERBOSE_DEBUG
    cout << "Sorting watchlists:" << endl;
    #endif

    //double myTime = cpuTime();
    for (watch_array::iterator
        i = watches.begin(), end = watches.end()
        ; i != end
        ; ++i
    ) {
        watch_subarray ws = *i;
        if (ws.size() == 0)
            continue;

        #ifdef VERBOSE_DEBUG
        cout << "Before sorting:" << endl;
        for (uint32_t i2 = 0; i2 < ws.size(); i2++) {
            if (ws[i2].isBinary()) cout << "Binary,";
            if (ws[i2].isTri()) cout << "Tri,";
            if (ws[i2].isClause()) cout << "Normal,";
        }
        cout << endl;
        #endif //VERBOSE_DEBUG

        std::sort(ws.begin(), ws.end(), WatchedSorter());

        #ifdef VERBOSE_DEBUG
        cout << "After sorting:" << endl;
        for (uint32_t i2 = 0; i2 < ws.size(); i2++) {
            if (ws[i2].isBinary()) cout << "Binary,";
            if (ws[i2].isTri()) cout << "Tri,";
            if (ws[i2].isClause()) cout << "Normal,";
        }
        cout << endl;
        #endif //VERBOSE_DEBUG
    }

    /*if (conf.verbosity >= 3) {
        cout << "c watched "
        << "sorting time: " << cpuTime() - myTime
        << endl;
    }*/
}

void PropEngine::printWatchList(const Lit lit) const
{
    watch_subarray_const ws = watches[lit.toInt()];
    for (watch_subarray_const::const_iterator
        it2 = ws.begin(), end2 = ws.end()
        ; it2 != end2
        ; it2++
    ) {
        if (it2->isBinary()) {
            cout << "bin: " << lit << " , " << it2->lit2() << " red : " <<  (it2->red()) << endl;
        } else if (it2->isTri()) {
            cout << "tri: " << lit << " , " << it2->lit2() << " , " <<  (it2->lit3()) << endl;
        } else if (it2->isClause()) {
            cout << "cla:" << it2->getOffset() << endl;
        } else {
            assert(false);
        }
    }
}

vector<Lit> PropEngine::getUnitaries() const
{
    vector<Lit> unitaries;
    if (decisionLevel() > 0) {
        for (uint32_t i = 0; i != trail_lim[0]; i++) {
            unitaries.push_back(trail[i]);
        }
    }

    return unitaries;
}

void PropEngine::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
    , const vector<uint32_t>& interToOuter2
) {
    updateArray(varData, interToOuter);
    #ifdef STATS_NEEDED
    updateArray(varDataLT, interToOuter);
    #endif
    updateArray(assigns, interToOuter);
    assert(decisionLevel() == 0);

    //Trail is NOT correct, only its length is correct
    for(Lit& lit: trail) {
        lit = lit_Undef;
    }
    updateBySwap(watches, seen, interToOuter2);

    for(size_t i = 0; i < watches.size(); i++) {
        /*
        //TODO
        if (i+10 < watches.size())
            __builtin_prefetch(watches[i+10].begin());
        */

        if (!watches[i].empty())
            updateWatch(watches[i], outerToInter);
    }
}

inline void PropEngine::updateWatch(
    watch_subarray ws
    , const vector<uint32_t>& outerToInter
) {
    for(watch_subarray::iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isBinary()) {
            it->setLit2(
                getUpdatedLit(it->lit2(), outerToInter)
            );

            continue;
        }

        if (it->isTri()) {
            Lit lit1 = it->lit2();
            Lit lit2 = it->lit3();
            lit1 = getUpdatedLit(lit1, outerToInter);
            lit2 = getUpdatedLit(lit2, outerToInter);
            if (lit1 > lit2)
                std::swap(lit1, lit2);

            it->setLit2(lit1);
            it->setLit3(lit2);

            continue;
        }

        if (it->isClause()) {
            it->setBlockedLit(
                getUpdatedLit(it->getBlockedLit(), outerToInter)
            );
        }
    }
}

PropBy PropEngine::propagateBinFirst(
    #ifdef STATS_NEEDED
    AvgCalc<size_t>* watchListSizeTraversed
    #endif
) {
    PropBy confl;

    #ifdef VERBOSE_DEBUG_PROP
    cout << "Propagation started" << endl;
    #endif

    uint32_t qheadlong = qhead;

    startAgain:
    //Propagate binary clauses first
    while (qhead < trail.size() && confl.isNULL()) {
        const Lit p = trail[qhead++];     // 'p' is enqueued fact to propagate.
        watch_subarray_const ws = watches[(~p).toInt()];
        #ifdef STATS_NEEDED
        if (watchListSizeTraversed)
            watchListSizeTraversed->push(ws.size());
        #endif

        watch_subarray::const_iterator i = ws.begin();
        watch_subarray_const::const_iterator end = ws.end();
        propStats.bogoProps += ws.size()/10 + 1;
        for (; i != end; i++) {

            //Propagate binary clause
            if (i->isBinary()) {
                if (!propBinaryClause(i, p, confl)) {
                    break;
                }

                continue;
            }

            //Pre-fetch long clause
            if (i->isClause()) {
                if (value(i->getBlockedLit()) != l_True) {
                    const ClOffset offset = i->getOffset();
                    __builtin_prefetch(clAllocator.getPointer(offset));
                }

                continue;
            } //end CLAUSE
        }
    }

    PropResult ret = PROP_NOTHING;
    while (qheadlong < qhead && confl.isNULL()) {
        const Lit p = trail[qheadlong];     // 'p' is enqueued fact to propagate.
        watch_subarray ws = watches[(~p).toInt()];
        watch_subarray::iterator i = ws.begin();
        watch_subarray::iterator j = ws.begin();
        watch_subarray_const::const_iterator end = ws.end();
        propStats.bogoProps += ws.size()/4 + 1;
        for (; i != end; i++) {
            //Skip binary clauses
            if (i->isBinary()) {
                *j++ = *i;
                continue;
            }

            if (i->isTri()) {
                *j++ = *i;
                //Propagate tri clause
                ret = propTriClause(i, p, confl);
                 if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                    //Conflict or propagated something
                    i++;
                    break;
                } else {
                    //Didn't propagate anything, continue
                    assert(ret == PROP_NOTHING);
                    continue;
                }
            } //end TRICLAUSE

            if (i->isClause()) {
                ret = propNormalClause(i, j, p, confl);
                 if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                    //Conflict or propagated something
                    i++;
                    break;
                } else {
                    //Didn't propagate anything, continue
                    assert(ret == PROP_NOTHING);
                    continue;
                }
            } //end CLAUSE
        }
        while (i != end) {
            *j++ = *i++;
        }
        ws.shrink_(end-j);

        //If propagated something, goto start
        if (ret == PROP_SOMETHING) {
            goto startAgain;
        }

        qheadlong++;
    }

    #ifdef VERBOSE_DEBUG
    cout << "Propagation (propagateBinFirst) ended." << endl;
    #endif

    return confl;
}

PropBy PropEngine::propagateIrredBin()
{
    PropBy confl;
    while (qhead < trail.size()) {
        Lit p = trail[qhead++];
        watch_subarray ws = watches[(~p).toInt()];
        for(watch_subarray::iterator k = ws.begin(), end = ws.end(); k != end; k++) {

            //If not binary, or is redundant, skip
            if (!k->isBinary() || k->red())
                continue;

            //Propagate, if conflict, exit
            if (!propBinaryClause(k, p, confl))
                return confl;
        }
    }

    //No conflict, propagation done
    return PropBy();
}

void PropEngine::print_trail()
{
    for(size_t i = trail_lim[0]; i < trail.size(); i++) {
        cout
        << "trail " << i << ":" << trail[i]
        << " lev: " << varData[trail[i].var()].level
        << " reason: " << varData[trail[i].var()].reason
        << endl;
    }
}


bool PropEngine::propagate_occur()
{
    if (!ok)
        return false;

    while (qhead < trail_size()) {
        const Lit p = trail[qhead];
        qhead++;
        watch_subarray ws = watches[(~p).toInt()];

        //Go through each occur
        for (watch_subarray::const_iterator
            it = ws.begin(), end = ws.end()
            ; it != end
            ; it++
        ) {
            if (it->isClause()) {
                if (!propagate_long_clause_occur(it->getOffset()))
                    return false;
            }

            if (it->isTri()) {
                if (!propagate_tri_clause_occur(*it))
                    return false;
            }

            if (it->isBinary()) {
                if (!propagate_binary_clause_occur(*it))
                    return false;
            }
        }
    }

    return true;
}

bool PropEngine::propagate_tri_clause_occur(const Watched& ws)
{
    const lbool val2 = value(ws.lit2());
    const lbool val3 = value(ws.lit3());
    if (val2 == l_True
        || val3 == l_True
    ) {
        return true;
    }

    if (val2 == l_Undef
        && val3 == l_Undef
    ) {
        return true;
    }

    if (val2 == l_False
        && val3 == l_False
    ) {
        ok = false;
        return false;
    }

    #ifdef STATS_NEEDED
    if (ws.red())
        propStats.propsTriRed++;
    else
        propStats.propsTriIrred++;
    #endif

    if (val2 == l_Undef) {
        enqueue(ws.lit2());
    } else {
        enqueue(ws.lit3());
    }
    return true;
}

bool PropEngine::propagate_binary_clause_occur(const Watched& ws)
{
    const lbool val = value(ws.lit2());
    if (val == l_False) {
        ok = false;
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
    const Clause& cl = *clAllocator.getPointer(offset);
    assert(!cl.getFreed() && "Cannot be already removed in occur");

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
        ok = false;
        return false;
    }

    if (numUndef > 1)
        return true;

    enqueue(lastUndef);
    #ifdef STATS_NEEDED
    if (cl.size() == 3)
        if (cl.red())
            propStats.propsTriRed++;
        else
            propStats.propsTriIrred++;
    else {
        if (cl.red())
            propStats.propsLongRed++;
        else
            propStats.propsLongIrred++;
    }
    #endif

    return true;
}
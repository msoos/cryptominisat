/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#include "PropEngine.h"
#include <cmath>
#include <string.h>
#include <algorithm>
#include <limits.h>
#include <vector>
#include <iomanip>
#include <algorithm>

#include "Solver.h"
#include "ClauseAllocator.h"
#include "Clause.h"
#include "time_mem.h"
#include "VarUpdateHelper.h"

using std::cout;
using std::endl;

//#define DEBUG_ENQUEUE_LEVEL0
//#define VERBOSE_DEBUG_POLARITIES
//#define DEBUG_DYNAMIC_RESTART

/**
@brief Sets a sane default config and allocates handler classes
*/
PropEngine::PropEngine(
    ClauseAllocator *_clAllocator
    , const AgilityData& agilityData
    , const bool _updateGlues
    , const bool _doLHBR
) :
        // Stats
        updateGlues(_updateGlues)
        , doLHBR (_doLHBR)

        , clAllocator(_clAllocator)
        , ok(true)
        , qhead(0)
        , agility(agilityData)
{
}

/**
@brief Creates a new SAT variable
*/
Var PropEngine::newVar(const bool)
{
    const Var v = nVars();
    if (v >= 1<<30) {
        cout << "ERROR! Variable requested is far too large" << endl;
        exit(-1);
    }

    watches.resize(watches.size() + 2);  // (list for positive&negative literals)
    assigns.push_back(l_Undef);
    varData.push_back(VarData());

    //Temporaries
    seen      .push_back(0);
    seen      .push_back(0);
    seen2     .push_back(0);
    seen2     .push_back(0);

    return v;
}

void PropEngine::attachTriClause(
    Lit lit1
    , Lit lit2
    , Lit lit3
    , const bool learnt
) {
    #ifdef DEBUG_ATTACH
    assert(lit1.var() != lit2.var());
    assert(value(lit1.var()) == l_Undef);
    assert(value(lit2) == l_Undef || value(lit2) == l_False);

    assert(varData[lit1.var()].elimed == ELIMED_NONE
            || varData[lit1.var()].elimed == ELIMED_QUEUED_VARREPLACER);
    assert(varData[lit2.var()].elimed == ELIMED_NONE
            || varData[lit2.var()].elimed == ELIMED_QUEUED_VARREPLACER);
    #endif //DEBUG_ATTACH

    //Order them
    if (lit1 > lit3)
        std::swap(lit1, lit3);
    if (lit1 > lit2)
        std::swap(lit1, lit2);
    if (lit2 > lit3)
        std::swap(lit2, lit3);

    //They are now ordered
    assert(lit1 < lit2);
    assert(lit2 < lit3);

    //And now they are attached, ordered
    watches[lit1.toInt()].push(Watched(lit2, lit3, learnt));
    watches[lit2.toInt()].push(Watched(lit1, lit3, learnt));
    watches[lit3.toInt()].push(Watched(lit1, lit2, learnt));
}


void PropEngine::attachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool learnt
    , const bool checkUnassignedFirst
) {
    #ifdef DEBUG_ATTACH
    assert(lit1.var() != lit2.var());
    if (checkUnassignedFirst) {
        assert(value(lit1.var()) == l_Undef);
        assert(value(lit2) == l_Undef || value(lit2) == l_False);
    }

    assert(varData[lit1.var()].elimed == ELIMED_NONE
            || varData[lit1.var()].elimed == ELIMED_QUEUED_VARREPLACER);
    assert(varData[lit2.var()].elimed == ELIMED_NONE
            || varData[lit2.var()].elimed == ELIMED_QUEUED_VARREPLACER);
    #endif //DEBUG_ATTACH

    watches[lit1.toInt()].push(Watched(lit2, learnt));
    watches[lit2.toInt()].push(Watched(lit1, learnt));
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
        assert(value(c[0].var()) == l_Undef);
        assert(value(c[1]) == l_Undef || value(c[1]) == l_False);
    }

    #ifdef DEBUG_ATTACH
    for (uint32_t i = 0; i < c.size(); i++) {
        assert(varData[c[i].var()].elimed == ELIMED_NONE
                || varData[c[i].var()].elimed == ELIMED_QUEUED_VARREPLACER);
    }
    #endif //DEBUG_ATTACH

    const ClOffset offset = clAllocator->getOffset(&c);

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

    ClOffset offset = clAllocator->getOffset(address);
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
    const vec<Watched>::const_iterator i
    , const Lit p
    , PropBy& confl
) {
    const lbool val = value(i->lit1());
    if (val.isUndef()) {
        if (i->learnt())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;

        enqueue(i->lit1(), PropBy(~p));
    } else if (val == l_False) {
        //Update stats
        if (i->learnt())
            lastConflictCausedBy = CONFL_BY_BIN_RED_CLAUSE;
        else
            lastConflictCausedBy = CONFL_BY_BIN_IRRED_CLAUSE;

        confl = PropBy(~p);
        failBinLit = i->lit1();
        qhead = trail.size();
        return false;
    }

    return true;
}


/**
@brief Propagates a normal (n-long where n > 3) clause

We have blocked literals in this case in the watchlist. That must be checked
and updated.
*/
template<bool simple>
PropResult PropEngine::propNormalClause(
    const vec<Watched>::iterator i
    , vec<Watched>::iterator &j
    , const Lit p
    , PropBy& confl
    , Solver* solver
) {
    //Blocked literal is satisfied, so clause is satisfied
    if (value(i->getBlockedLit()).getBool()) {
        *j++ = *i;
        return PROP_NOTHING;
    }
    propStats.bogoProps += 4;
    const uint32_t offset = i->getOffset();
    Clause& c = *clAllocator->getPointer(offset);
    c.stats.numLookedAt++;
    c.stats.numLitVisited++;

    // Make sure the false literal is data[1]:
    if (c[0] == ~p) {
        std::swap(c[0], c[1]);
    }

    assert(c[1] == ~p);

    // If 0th watch is true, then clause is already satisfied.
    if (value(c[0]).getBool()) {
        *j = Watched(offset, c[0]);
        j++;
        return PROP_NOTHING;
    }

    // Look for new watch:
    uint16_t numLitVisited = 0;
    for (Lit *k = c.begin() + 2, *end2 = c.end()
        ; k != end2
        ; k++, numLitVisited++
    ) {
        //Literal is either unset or satisfied, attach to other watchlist
        if (value(*k) != l_False) {
            c[1] = *k;
            propStats.bogoProps += numLitVisited/10;
            c.stats.numLitVisited+= numLitVisited;
            *k = ~p;
            watches[c[1].toInt()].push(Watched(offset, c[0]));
            return PROP_NOTHING;
        }
    }
    propStats.bogoProps += numLitVisited/10;
    c.stats.numLitVisited+= numLitVisited;

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    c.stats.numPropAndConfl++;
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
        if (c.learnt())
            lastConflictCausedBy = CONFL_BY_LONG_RED_CLAUSE;
        else
            lastConflictCausedBy = CONFL_BY_LONG_IRRED_CLAUSE;

        qhead = trail.size();
        return PROP_FAIL;
    } else {

        //Update stats
        if (c.learnt())
            propStats.propsLongRed++;
        else
            propStats.propsLongIrred++;

        if (simple) {
            //Do lazy hyper-binary resolution if possible
            if (doLHBR
                && solver != NULL
                && varData[c[1].var()].reason.getType() == binary_t
            ) {
                Lit other = varData[c[1].var()].reason.lit1();
                bool OK = true;
                for(uint32_t i = 2; i < c.size(); i++) {
                    if (varData[c[i].var()].reason.getType() != binary_t
                        || other != varData[c[i].var()].reason.lit1()
                    ) {
                        OK = false;
                        break;
                    }
                }

                //Is it possible?
                if (OK) {
                    solver->attachBinClause(other, c[0], true, false);
                    propStats.longLHBR++;
                    enqueue(c[0], PropBy(other));
                } else {
                    //no, not possible, just enqueue as normal
                    goto norm;
                }
            } else {
                norm:
                enqueue(c[0], PropBy(offset));
            }


            //Update glues?
            if (c.learnt()
                && c.stats.glue > 2
                && updateGlues
            ) {
                uint16_t newGlue = calcGlue(c);
                c.stats.glue = std::min(c.stats.glue, newGlue);
            }
        } else {
            addHyperBin(c[0], c);
        }
    }

    return PROP_SOMETHING;
}

/**
@brief Propagates a tertiary (3-long) clause

Need to be somewhat tricky if the clause indicates that current assignement
is incorrect (i.e. all 3 literals evaluate to FALSE). If conflict is found,
sets failBinLit
*/
template<bool simple>
PropResult PropEngine::propTriClause(
    const vec<Watched>::const_iterator i
    , const Lit p
    , PropBy& confl
    , Solver* solver
) {
    Lit otherLit = i->lit1();
    lbool val = value(otherLit);

    //literal is already satisfied, nothing to do
    if (val == l_True)
        return PROP_NOTHING;

    Lit otherLit2 = i->lit2();
    lbool val2 = value(otherLit2);

    //literal is already satisfied, nothing to do
    if (val2 == l_True)
        return PROP_NOTHING;

    if (val == l_False && val2 == l_False) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Conflict from "
            << p << " , "
            << i->lit1() << " , "
            << i->lit2() << endl;
        #endif //VERBOSE_DEBUG_FULLPROP
        confl = PropBy(~p, i->lit2());

        //Update stats
        lastConflictCausedBy = CONFL_BY_TRI_CLAUSE;

        failBinLit = i->lit1();
        qhead = trail.size();
        return PROP_FAIL;
    }
    if (propTriHelper<simple>(val, val2, otherLit, otherLit2, p, solver))
        return PROP_SOMETHING;

    if (propTriHelper<simple>(val2, val, otherLit2, otherLit, p, solver))
        return PROP_SOMETHING;

    return PROP_NOTHING;
}

template<bool simple>
bool PropEngine::propTriHelper(
    const lbool val
    , const lbool val2
    , const Lit otherLit
    , const Lit otherLit2
    , const Lit p
    , Solver* solver
) {
    if (val.isUndef() && val2 == l_False) {
        propStats.propsTri++;
        if (simple) {
            //Check if we could do lazy hyper-binary resoution
            if (doLHBR
                && solver != NULL
                && varData[p.var()].reason.getType() == binary_t
                && ((varData[otherLit2.var()].reason.getType() == binary_t
                    && varData[otherLit2.var()].reason.lit1() == varData[p.var()].reason.lit1())
                    || (varData[p.var()].reason.lit1().var() == otherLit2.var())
                )
            ) {
                Lit lit= varData[p.var()].reason.lit1();

                solver->attachBinClause(lit, otherLit, true, false);
                enqueue(otherLit, PropBy(lit));
                propStats.triLHBR++;
            } else {
                //Lazy hyper-bin is not possibe
                enqueue(otherLit, PropBy(~p, otherLit2));
            }
        } else {
            addHyperBin(otherLit, ~p, otherLit2);
        }
        return true;
    }

    return false;
}

PropBy PropEngine::propagate(
    Solver* solver
    , bqueue<size_t>* watchListSizeTraversed
    , bqueue<bool>* litPropagatedSomething
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
        const vec<Watched>& ws = watches[(~p).toInt()];
        if (watchListSizeTraversed)
            watchListSizeTraversed->push(ws.size());

        vec<Watched>::const_iterator i = ws.begin();
        const vec<Watched>::const_iterator end = ws.end();
        propStats.bogoProps += ws.size()/10 + 1;
        size_t lastTrailSize = trail.size();
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
                    const uint32_t offset = i->getOffset();
                    __builtin_prefetch(clAllocator->getPointer(offset));
                }

                continue;
            } //end CLAUSE
        }
        if (litPropagatedSomething)
            litPropagatedSomething->push(trail.size() > lastTrailSize);
    }

    PropResult ret = PROP_NOTHING;
    while (qheadlong < qhead && confl.isNULL()) {
        const Lit p = trail[qheadlong];     // 'p' is enqueued fact to propagate.
        vec<Watched>& ws = watches[(~p).toInt()];
        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = ws.begin();
        const vec<Watched>::iterator end = ws.end();
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
                ret = propTriClause<true>(i, p, confl, solver);
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
                ret = propNormalClause<true>(i, j, p, confl, solver);
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
    cout << "Propagation ended." << endl;
    #endif

    return confl;
}

PropBy PropEngine::propagateNonLearntBin()
{
    PropBy confl;
    while (qhead < trail.size()) {
        Lit p = trail[qhead++];
        vec<Watched> & ws = watches[(~p).toInt()];
        for(vec<Watched>::iterator k = ws.begin(), end = ws.end(); k != end; k++) {
            if (!k->isBinary() || k->learnt()) continue;

            if (!propBinaryClause(k, p, confl)) return confl;
        }
    }

    return PropBy();
}

Lit PropEngine::propagateFull(
    bqueue<size_t>* watchListSizeTraversed
    , bqueue<bool>* litPropagatedSomething
) {
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Prop full started" << endl;
    #endif

    PropBy confl;

    //Assert startup: only 1 enqueued, uselessBin is empty
    assert(uselessBin.empty());
    assert(decisionLevel() == 1);

    //The toplevel decision has to be set specifically
    //If we came here as part of a backtrack to decision level 1, then
    //this is already set, and there is no need to set it
    if (trail.size() - trail_lim.back() == 1) {
        //Set up root node
        Lit root = trail[qhead];
        varData[root.var()].reason = PropBy(~lit_Undef, false, false, false);
    }

    uint32_t nlBinQHead = qhead;
    uint32_t lBinQHead = qhead;

    needToAddBinClause.clear();
    PropResult ret = PROP_NOTHING;
    start:

    //Propagate binary non-learnt
    while (nlBinQHead < trail.size()) {
        const Lit p = trail[nlBinQHead++];
        size_t lastTrailSize = trail.size();
        const vec<Watched>& ws = watches[(~p).toInt()];
        if (watchListSizeTraversed)
            watchListSizeTraversed->push(ws.size());
        propStats.bogoProps += 1;
        for(vec<Watched>::const_iterator k = ws.begin(), end = ws.end(); k != end; k++) {

            //If something other than non-learnt binary, skip
            if (!k->isBinary() || k->learnt())
                continue;

            ret = propBin(p, k, confl);
            if (ret == PROP_FAIL)
                return analyzeFail(confl);
        }
        if (litPropagatedSomething)
            litPropagatedSomething->push(trail.size() > lastTrailSize);
    }

    //Propagate binary learnt
    ret = PROP_NOTHING;
    while (lBinQHead < trail.size()) {
        const Lit p = trail[lBinQHead];
        const vec<Watched>& ws = watches[(~p).toInt()];
        propStats.bogoProps += 1;

        for(vec<Watched>::const_iterator k = ws.begin(), end = ws.end(); k != end; k++) {

            //If something other than learnt binary, skip
            if (!k->isBinary() || !k->learnt())
                continue;

            ret = propBin(p, k, confl);
            if (ret == PROP_FAIL) {
                return analyzeFail(confl);
            } else if (ret == PROP_SOMETHING) {
                goto start;
            } else {
                assert(ret == PROP_NOTHING);
            }
        }
        lBinQHead++;
    }

    ret = PROP_NOTHING;
    while (qhead < trail.size()) {
        const Lit p = trail[qhead];
        vec<Watched> & ws = watches[(~p).toInt()];
        propStats.bogoProps += 1;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = ws.begin();
        const vec<Watched>::iterator end = ws.end();
        for(; i != end; i++) {
            if (i->isBinary()) {
                *j++ = *i;
                continue;
            }

            if (i->isTri()) {
                *j++ = *i;
                ret = propTriClause<false>(i, p, confl, NULL);
                if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                    i++;
                    break;
                } else {
                    assert(ret == PROP_NOTHING);
                    continue;
                }
            }

            if (i->isClause()) {
                ret = propNormalClause<false>(i, j, p, confl, NULL);
                if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                    i++;
                    break;
                } else {
                    assert(ret == PROP_NOTHING);
                    continue;
                }
            }
        }
        while(i != end)
            *j++ = *i++;
        ws.shrink_(end-j);

        if (ret == PROP_FAIL) {
            return analyzeFail(confl);
        } else if (ret == PROP_SOMETHING) {
            goto start;
        }

        qhead++;
    }

    return lit_Undef;
}

PropResult PropEngine::propBin(
    const Lit p
    , vec<Watched>::const_iterator k
    , PropBy& confl
) {
    const Lit lit = k->lit1();
    const lbool val = value(lit);
    if (val.isUndef()) {
        if (k->learnt())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;

        //Never propagated before
        enqueueComplex(lit, p, k->learnt());
        return PROP_SOMETHING;

    } else if (val == l_False) {
        //Conflict
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Conflict from " << p << " , " << lit << endl;
        #endif //VERBOSE_DEBUG_FULLPROP

        //Update stats
        if (k->learnt())
            lastConflictCausedBy = CONFL_BY_BIN_RED_CLAUSE;
        else
            lastConflictCausedBy = CONFL_BY_BIN_IRRED_CLAUSE;

        failBinLit = lit;
        confl = PropBy(~p);
        return PROP_FAIL;

    } else if (varData[lit.var()].level != 0) {
        //Propaged already
        assert(val == l_True);

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Lit " << p << " also wants to propagate " << lit << endl;
        #endif
        Lit remove = removeWhich(lit, p, k->learnt());

        //Remove this one
        if (remove == p) {
            Lit origAnc = varData[lit.var()].reason.getAncestor();
            assert(origAnc != lit_Undef);

            //The binary clause we should remove
            const BinaryClause clauseToRemove(
                ~varData[lit.var()].reason.getAncestor()
                , lit
                , varData[lit.var()].reason.getLearntStep()
            );

            //We now remove the clause
            //If it's hyper-bin, then we remove the to-be-added hyper-binary clause
            //However, if the hyper-bin was never added because only 1 literal was unbound at level 0 (i.e. through
            //clause cleaning, the clause would have been 2-long), then we don't do anything.
            if (!varData[lit.var()].reason.getHyperbin()) {
                #ifdef VERBOSE_DEBUG_FULLPROP
                cout << "Normal removing clause " << clauseToRemove << endl;
                #endif
                uselessBin.insert(clauseToRemove);
            } else if (!varData[lit.var()].reason.getHyperbinNotAdded()) {
                #ifdef VERBOSE_DEBUG_FULLPROP
                cout << "Removing hyper-bin clause " << clauseToRemove << endl;
                #endif
                std::set<BinaryClause>::iterator it = needToAddBinClause.find(clauseToRemove);

                //In case this is called after a backtrack to decisionLevel 1
                //then in fact we might have already cleaned the
                //'needToAddBinClause'. When called from probing, the IF below
                //must ALWAYS be true
                if (it != needToAddBinClause.end()) {
                    needToAddBinClause.erase(it);
                }
                //This will subsume the clause later, so don't remove it
            }

            //Update data indicating what lead to lit
            varData[lit.var()].reason = PropBy(~p, k->learnt(), false, false);

            //for correctness, we would need this, but that would need re-writing of history :S
            //if (!onlyNonLearnt) return PropBy();

        } else if (remove != lit_Undef) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Removing this bin clause" << endl;
            #endif
            uselessBin.insert(BinaryClause(~p, lit, k->learnt()));
        }
    }

    return PROP_NOTHING;
}

void PropEngine::sortWatched()
{
    #ifdef VERBOSE_DEBUG
    cout << "Sorting watchlists:" << endl;
    #endif

    //double myTime = cpuTime();
    for (vector<vec<Watched> >::iterator
        i = watches.begin(), end = watches.end()
        ; i != end
        ; i++
    ) {
        if (i->size() == 0) continue;
        #ifdef VERBOSE_DEBUG
        vec<Watched>& ws = *i;
        cout << "Before sorting:" << endl;
        for (uint32_t i2 = 0; i2 < ws.size(); i2++) {
            if (ws[i2].isBinary()) cout << "Binary,";
            if (ws[i2].isTri()) cout << "Tri,";
            if (ws[i2].isClause()) cout << "Normal,";
        }
        cout << endl;
        #endif //VERBOSE_DEBUG

        std::sort(i->begin(), i->end(), WatchedSorter());

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
    const vec<Watched>& ws = watches[lit.toInt()];
    for (vec<Watched>::const_iterator
        it2 = ws.begin(), end2 = ws.end()
        ; it2 != end2
        ; it2++
    ) {
        if (it2->isBinary()) {
            cout << "bin: " << lit << " , " << it2->lit1() << " learnt : " <<  (it2->learnt()) << endl;
        } else if (it2->isTri()) {
            cout << "tri: " << lit << " , " << it2->lit1() << " , " <<  (it2->lit2()) << endl;
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

uint32_t PropEngine::countNumBinClauses(const bool alsoLearnt, const bool alsoNonLearnt) const
{
    uint32_t num = 0;

    size_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end(); it != end; it++, wsLit++) {
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                if (it2->learnt()) num += alsoLearnt;
                else num+= alsoNonLearnt;
            }
        }
    }

    assert(num % 2 == 0);
    return num/2;
}

void PropEngine::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
    , const vector<uint32_t>& interToOuter2
) {
    updateArray(varData, interToOuter);
    updateArray(assigns, interToOuter);
    updateLitsMap(trail, outerToInter);
    updateBySwap(watches, seen, interToOuter2);

    for(size_t i = 0; i < watches.size(); i++) {
        if (i+10 < watches.size())
            __builtin_prefetch(watches[i+10].begin());

        if (!watches[i].empty())
            updateWatch(watches[i], outerToInter);
    }
}

inline void PropEngine::updateWatch(
    vec<Watched>& ws
    , const vector<uint32_t>& outerToInter
) {
    for(vec<Watched>::iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isBinary() || it->isTri()) {
            it->setLit1(
                getUpdatedLit(it->lit1(), outerToInter)
            );
            if (it->isBinary())
                continue;
        }

        if (it->isTri()) {
            it->setLit2(
                getUpdatedLit(it->lit2(), outerToInter)
            );
            continue;
        }

        if (it->isClause()) {
            it->setBlockedLit(
                getUpdatedLit(it->getBlockedLit(), outerToInter)
            );
        }
    }
}

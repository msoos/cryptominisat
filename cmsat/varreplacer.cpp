/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
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

#include "varreplacer.h"
#include "varupdatehelper.h"
#include <iostream>
#include <iomanip>
#include <set>
#include <boost/type_traits/detail/is_mem_fun_pointer_impl.hpp>
using std::cout;
using std::endl;

#include "solver.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solutionextender.h"
#include "clauseallocator.h"

#ifdef VERBOSE_DEBUG
#define REPLACE_STATISTICS
#define VERBOSE_DEBUG_BIN_REPLACER
#endif

using namespace CMSat;

//#define VERBOSE_DEBUG
//#define REPLACE_STATISTICS
//#define DEBUG_BIN_REPLACER
//#define VERBOSE_DEBUG_BIN_REPLACER

VarReplacer::VarReplacer(Solver* _solver) :
    solver(_solver)
    , replacedVars(0)
    , lastReplacedVars(0)
{
}

VarReplacer::~VarReplacer()
{
}

/**
@brief Replaces variables, clears internal clauses, and reports stats

When replacing, it is imperative not to make variables decision variables
which have been removed by other methods:
\li variable removal at the xor-sphere
\li disconnected component finding and solving
\li variable elimination

NOTE: If any new such algoirhtms are added, this part MUST be updated such
that problems don't creep up
*/
bool VarReplacer::performReplace()
{
    assert(solver->ok);

    //Set up stats
    runStats.clear();
    runStats.numCalls = 1;
    const double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();

    #ifdef REPLACE_STATISTICS
    uint32_t numRedir = 0;
    for (uint32_t i = 0; i < table.size(); i++) {
        if (table[i].var() != i)
            numRedir++;
    }
    cout << "c Number of trees:" << reverseTable.size() << endl;
    cout << "c Number of redirected nodes:" << numRedir << endl;
    #endif //REPLACE_STATISTICS

    solver->clauseCleaner->removeAndCleanAll();
    solver->testAllClauseAttach();

    //Printing stats
    #ifdef VERBOSE_DEBUG
    {
        uint32_t i = 0;
        for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
            if (it->var() == i) continue;
            cout << "Replacing var " << i+1 << " with Lit " << *it << endl;
        }
    }
    #endif

    Var var = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, var++) {

        //Was queued for replacement, but it's the top of the tree, so, it's normal again
        if (it->var() == var
            && solver->varData[it->var()].removed == Removed::queued_replacer
        ) {
            solver->varData[it->var()].removed = Removed::none;
        }

        //Not replaced, or not replaceable, so skip
        if (it->var() == var
            || solver->varData[it->var()].removed == Removed::decomposed
            || solver->varData[it->var()].removed == Removed::elimed
        ) {
            continue;
        }

        //Has already been handled previously, just skip
        if (solver->varData[var].removed == Removed::replaced) {
            continue;
        }

        //Okay, so unset decision, and set the other one decision
        solver->varData[var].removed = Removed::replaced;
        assert(
            (solver->varData[it->var()].removed == Removed::none
                || solver->varData[it->var()].removed == Removed::queued_replacer)
            && "It MUST have been queued for varreplacement so top couldn't have been removed/decomposed/etc"
        );
        solver->unsetDecisionVar(var);
        solver->setDecisionVar(it->var());

        //Update activities. Top receives activities of the ones below
        uint32_t& activity1 = solver->activities[var];
        uint32_t& activity2 = solver->activities[it->var()];
        activity2 += activity1;
        activity1 = 0.0;
        solver->order_heap.update(var);
        solver->order_heap.update(it->var());
    }

    runStats.actuallyReplacedVars = replacedVars -lastReplacedVars;
    lastReplacedVars = replacedVars;

    solver->testAllClauseAttach();
    assert(solver->qhead == solver->trail.size());

#ifdef DEBUG_BIN_CLAUSE_NUM
    solver->countNumBinClauses(true, false);
    solver->countNumBinClauses(false, true);
#endif

    //Replace implicits
    if (!replaceImplicit()) {
        goto end;
    }

    //While replacing the implicit clauses
    //we cannot enqueue literals, so we do it now
    for(vector<Lit>::const_iterator
        it = delayedEnqueue.begin(), end = delayedEnqueue.end()
        ; it != end
        ; it++
    ) {
        if (solver->value(*it) == l_Undef) {
            solver->enqueue(*it);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
        } else if (solver->value(*it) == l_False) {
            solver->ok = false;
            break;
        }
    }
    delayedEnqueue.clear();
    if (!solver->ok)
        goto end;
    solver->ok = solver->propagate().isNULL();
    if (!solver->ok)
        goto end;

    //Replace longs
    if (!replace_set(solver->longIrredCls)) {
        goto end;
    }
    if (!replace_set(solver->longRedCls)) {
        goto end;
    }

    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    solver->checkStats();

end:
    assert(solver->qhead == solver->trail.size() || !solver->ok);

    //Update stamp dominators
    solver->stamp.updateDominators(this);

    //Update stats
    runStats.zeroDepthAssigns += solver->trail.size() - origTrailSize;
    runStats.cpu_time = cpuTime() - myTime;
    globalStats += runStats;
    if (solver->conf.verbosity  >= 1) {
        if (solver->conf.verbosity  >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }

    return solver->ok;
}

bool VarReplacer::replaceImplicit()
{
    size_t removedLearntBin = 0;
    size_t removedNonLearntBin = 0;
    size_t removedLearntTri = 0;
    size_t removedNonLearntTri = 0;

    vector<BinaryClause> delayedAttach;

    size_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        const Lit origLit1 = Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            //Don't bother clauses
            if (i->isClause()) {
                *j++ = *i;
                continue;
            }

            /*cout << "--------" << endl;
            cout << "ws size: " << ws.size() << endl;
            cout << "solver->value(origLit1): " << solver->value(origLit1) << endl;
            cout << "watch: " << *i << endl;*/

            assert(solver->value(origLit1) == l_Undef);
            Lit lit1 = origLit1;
            Lit lit2 = i->lit2();
            const Lit origLit2 = lit2;
            assert(solver->value(origLit2) == l_Undef);
            assert(origLit1.var() != origLit2.var());

            //Update lit2
            if (table[lit2.var()].var() != lit2.var()) {
                lit2 = table[lit2.var()] ^ lit2.sign();
                i->setLit1(lit2);
                runStats.replacedLits++;
            }

            //Update main lit
            if (table[lit1.var()].var() != lit1.var()) {
                lit1 = table[lit1.var()] ^ lit1.sign();
                runStats.replacedLits++;
            }

            if (i->isTri()) {
                Lit lit3 = i->lit3();
                Lit origLit3 = lit3;
                assert(origLit1.var() != origLit3.var());
                assert(origLit2.var() != origLit3.var());
                assert(origLit2 < origLit3);
                assert(solver->value(origLit3) == l_Undef);

                //Update lit3
                if (table[lit3.var()].var() != lit3.var()) {
                    lit3 = table[lit3.var()] ^ lit3.sign();
                    i->setLit2(lit3);
                    runStats.replacedLits++;
                }

                bool remove = false;

                //Tautology, remove
                if (lit1 == ~lit2
                    || lit1 == ~lit3
                    || lit2 == ~lit3
                ) {
                    remove = true;
                }

                //All 3 lits are the same
                if (!remove
                    && lit1 == lit2
                    && lit2 == lit3
                ) {
                    delayedEnqueue.push_back(lit1);
                    #ifdef DRUP
                    if (solver->drup) {
                        *(solver->drup)
                        << lit1
                        << " 0\n";
                    }
                    #endif
                    remove = true;
                }

                //1st and 2nd lits are the same
                if (!remove
                    && lit1 == lit2
                ) {
                    //Only attach once
                    if (origLit1 < origLit2
                        && origLit2 < origLit3
                    ){
                        delayedAttach.push_back(BinaryClause(lit1, lit3, i->learnt()));
                        #ifdef DRUP
                        if (solver->drup) {
                            *(solver->drup)
                            << lit1 << " " << lit3
                            << " 0\n";
                        }
                        #endif
                    }
                    remove = true;
                }

                //1st and 3rd lits  OR 2nd and 3rd lits are the same
                if (!remove
                    && (lit1 == lit3 || (lit2 == lit3))
                ) {
                    //1st&2nd OR 2nd&3rd the same

                    //Only attach once
                    if (origLit1 < origLit2
                        && origLit2 < origLit3
                    ){
                        delayedAttach.push_back(BinaryClause(lit1, lit2, i->learnt()));
                        #ifdef DRUP
                        if (solver->drup) {
                            *(solver->drup)
                            << lit1 << " " << lit2
                            << " 0\n";
                        }
                        #endif
                    }
                    remove = true;
                }

                if (remove) {
                    //Update function-internal stats
                    if (i->learnt()) {
                        removedLearntTri++;
                    } else {
                        removedNonLearntTri++;
                    }

                    #ifdef DRUP
                    if (solver->drup
                        //Only delete once
                        && origLit1 < origLit2
                        && origLit2 < origLit3
                    ) {
                        *(solver->drup)
                        << "d "
                        << origLit1 << " "
                        << origLit2 << " "
                        << origLit3
                        << " 0\n";
                    }
                    #endif

                    continue;
                }

                //Order literals
                orderLits(lit1, lit2, lit3);

                //Now make into the order this TRI was in
                if (origLit1 > origLit2
                    && origLit1 < origLit3
                ) {
                    std::swap(lit1, lit2);
                }
                if (origLit1 > origLit2
                    && origLit1 > origLit3
                ) {
                    std::swap(lit1, lit3);
                    std::swap(lit2, lit3);
                }
                i->setLit1(lit2);
                i->setLit2(lit3);

                #ifdef DRUP
                if (solver->drup
                    //Changed
                    && (lit1 != origLit1
                        || lit2 != origLit2
                        || lit3 != origLit3
                    )
                    //Remove&attach only once
                    && (origLit1 < origLit2
                        && origLit2 < origLit3
                    )
                ) {
                    *(solver->drup)
                    << lit1 << " "
                    << lit2 << " "
                    << lit3
                    << " 0\n"

                    //Delete old one
                    << "d "
                    << origLit1 << " "
                    << origLit2 << " "
                    << origLit3
                    << " 0\n";
                }
                #endif

                if (lit1 != origLit1) {
                    solver->watches[lit1.toInt()].push(*i);
                } else {
                    *j++ = *i;
                }

                continue;
            }

            //Only binary are here
            assert(i->isBinary());
            bool remove = false;

            //Two lits are the same in BIN
            if (lit1 == lit2) {
                delayedEnqueue.push_back(lit2);
                #ifdef DRUP
                if (solver->drup) {
                    *(solver->drup)
                    << lit2
                    << " 0\n";
                }
                #endif
                remove = true;
            }

            //Tautology
            if (lit1 == ~lit2)
                remove = true;

            if (remove) {
                //Update function-internal stats
                if (i->learnt()) {
                    removedLearntBin++;
                } else {
                    removedNonLearntBin++;
                }

                #ifdef DRUP
                if (solver->drup
                    //Delete only once
                     && origLit1 < origLit2
                ) {
                    *(solver->drup)
                    << "d "
                    << origLit1 << " "
                    << origLit2
                    << " 0\n";
                }
                #endif

                continue;
            }

            #ifdef DRUP
            if (solver->drup
                //Changed
                && (lit1 != origLit1
                    || lit2 != origLit2)
                //Delete&attach only once
                && (origLit1 < origLit2)
            ) {
                *(solver->drup)
                //Add replaced
                << lit1 << " " << lit2
                << " 0\n"

                //Delete old one
                << "d " << origLit1 << " " << origLit2
                << " 0\n";
            }

            #endif

            if (lit1 != origLit1) {
                solver->watches[lit1.toInt()].push(*i);
            } else {
                *j++ = *i;
            }
        }
        ws.shrink_(i-j);
    }

    for(vector<BinaryClause>::const_iterator
        it = delayedAttach.begin(), end = delayedAttach.end()
        ; it != end
        ; it++
    ) {
        solver->attachBinClause(it->getLit1(), it->getLit2(), it->getLearnt());
    }

    #ifdef VERBOSE_DEBUG_BIN_REPLACER
    cout << "c debug bin replacer start" << endl;
    cout << "c debug bin replacer end" << endl;
    #endif

    assert(removedLearntBin % 2 == 0);
    solver->binTri.redBins -= removedLearntBin/2;

    assert(removedNonLearntBin % 2 == 0);
    solver->binTri.irredBins -= removedNonLearntBin/2;

    assert(removedLearntTri % 3 == 0);
    solver->binTri.redTris -= removedLearntTri/3;

    assert(removedNonLearntTri % 3 == 0);
    solver->binTri.irredTris -= removedNonLearntTri/3;

    #ifdef DEBUG_IMPLICIT_STATS
    solver->checkImplicitStats();
    #endif

    //Global stats update
    runStats.removedBinClauses += removedLearntBin/2 + removedNonLearntBin/2;
    runStats.removedTriClauses += removedLearntTri/3 + removedNonLearntTri/3;

    return solver->ok;
}

/**
@brief Replaces variables in normal clauses
*/
bool VarReplacer::replace_set(vector<ClOffset>& cs)
{
    size_t at = 0;
    vector<ClOffset>::iterator i = cs.begin();
    vector<ClOffset>::iterator j = i;
    for (vector<ClOffset>::iterator end = i + cs.size(); i != end; i++, at++) {
        if (at + 1 < cs.size()) {
            Clause* cl = solver->clAllocator->getPointer(cs[at+1]);
            __builtin_prefetch(cl);
        }
        Clause& c = *solver->clAllocator->getPointer(*i);
        assert(c.size() > 3);

        bool changed = false;
        Lit origLit1 = c[0];
        Lit origLit2 = c[1];
        #ifdef DRUP
        vector<Lit> origCl(c.size());
        std::copy(c.begin(), c.end(), origCl.begin());
        #endif

        for (Lit *l = c.begin(), *end2 = l + c.size();  l != end2; l++) {
            if (table[l->var()].var() != l->var()) {
                changed = true;
                *l = table[l->var()] ^ l->sign();
                runStats.replacedLits++;
            }
        }

        if (changed && handleUpdatedClause(c, origLit1, origLit2)) {
            solver->clAllocator->clauseFree(*i);
            runStats.removedLongClauses++;
            if (!solver->ok) {
                return false;
            }
        } else {
            *j++ = *i;
        }

        #ifdef DRUP
        if (solver->drup && changed) {
            *(solver->drup)
            << "d "
            << origCl
            << " 0\n";
        }
        #endif
    }
    cs.resize(cs.size() - (i-j));

    return solver->ok;
}

/**
@brief Helper function for replace_set()
*/
bool VarReplacer::handleUpdatedClause(
    Clause& c
    , const Lit origLit1
    , const Lit origLit2
) {
    bool satisfied = false;
    std::sort(c.begin(), c.end());
    Lit p;
    uint32_t i, j;
    const uint32_t origSize = c.size();
    for (i = j = 0, p = lit_Undef; i != origSize; i++) {
        assert(solver->varData[c[i].var()].removed == Removed::none);
        if (solver->value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (solver->value(c[i]) != l_False && c[i] != p)
            c[j++] = p = c[i];
    }
    c.shrink(i - j);
    c.setChanged();

    solver->detachModifiedClause(origLit1, origLit2, origSize, &c);

    #ifdef VERBOSE_DEBUG
    cout << "clause after replacing: " << c << endl;
    #endif

    if (satisfied)
        return true;

    #ifdef DRUP
    if (solver->drup) {
        *(solver->drup)
        << c
        << " 0\n";
    }
    #endif

    switch(c.size()) {
    case 0:
        solver->ok = false;
        return true;
    case 1 :
        solver->enqueue(c[0]);
        #ifdef STATS_NEEDED
        solver->propStats.propsUnit++;
        #endif
        solver->ok = (solver->propagate().isNULL());
        runStats.removedLongLits += origSize;
        return true;
    case 2:
        solver->attachBinClause(c[0], c[1], c.learnt());
        runStats.removedLongLits += origSize;
        return true;

    case 3:
        solver->attachTriClause(c[0], c[1], c[2], c.learnt());
        runStats.removedLongLits += origSize;
        return true;

    default:
        solver->attachClause(c);
        runStats.removedLongLits += origSize - c.size();
        return false;
    }

    assert(false);
    return false;
}

/**
@brief Returns variables that have been replaced
*/
vector<Var> VarReplacer::getReplacingVars() const
{
    vector<Var> replacingVars;

    for(map<Var, vector<Var> >::const_iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        replacingVars.push_back(it->first);
    }

    return replacingVars;
}

/**
@brief Used when a variable was eliminated, but it replaced some other variables

This function will add to solver2 clauses that represent the relationship of
the variables to their replaced cousins. Then, calling solver2.solve() should
take care of everything
*/
void VarReplacer::extendModel(SolutionExtender* extender) const
{

    #ifdef VERBOSE_DEBUG
    cout << "c VarReplacer::extendModel() called" << endl;
    #endif //VERBOSE_DEBUG

    vector<Lit> tmpClause;
    uint32_t i = 0;
    for (vector<Lit>::const_iterator
        it = table.begin()
        ; it != table.end()
        ; it++, i++
    ) {
        //Not replaced, nothing to do
        if (it->var() == i)
            continue;

        tmpClause.clear();
        Lit lit1 = Lit(it->var(), true);
        Lit lit2 = Lit(i, it->sign());
        tmpClause.push_back(lit1);
        tmpClause.push_back(lit2);
        bool OK = extender->addClause(tmpClause);
        assert(OK);

        tmpClause.clear();
        lit1 ^= true;
        lit2 ^= true;
        tmpClause.push_back(lit1);
        tmpClause.push_back(lit2);
        OK = extender->addClause(tmpClause);
        assert(OK);
    }
}

/**
@brief Replaces two two vars in "ps" with one another. xorEqualFalse defines anti/equivalence

It can be tricky to do this. For example, if:

\li a replaces: b, c
\li f replaces: f, h
\li we just realised that c = h
This is the most difficult case, but there are other cases, e.g. if we already
know that c=h, in which case we don't do anything

@p ps must contain 2 variables(!), i.e literals with no sign
@p xorEqualFalse if True, the two variables are equivalent. Otherwise, they are antivalent
*/
bool VarReplacer::replace(
    Lit lit1
    , Lit lit2
    , const bool xorEqualFalse
    , bool addLaterAsTwoBins
)
{
    #ifdef VERBOSE_DEBUG
    cout << "replace() called with var " << lit1 << " and var " << lit2 << " with xorEqualFalse " << xorEqualFalse << endl;
    #endif

    assert(solver->ok);
    assert(solver->decisionLevel() == 0);
    assert(!lit1.sign());
    assert(!lit2.sign());
    assert(solver->value(lit1.var()) == l_Undef);
    assert(solver->value(lit2.var()) == l_Undef);

    assert(solver->varData[lit1.var()].removed == Removed::none
            || solver->varData[lit1.var()].removed == Removed::queued_replacer);
    assert(solver->varData[lit2.var()].removed == Removed::none
            || solver->varData[lit2.var()].removed == Removed::queued_replacer);

    #ifdef DRUP_DEBUG
    if (solver->drup) {
        *(solver->drup)
        << ~lit1 << " " << (lit2 ^!xorEqualFalse) << " 0\n"
        << lit1 << " " << (~lit2 ^!xorEqualFalse) << " 0\n"
        ;
    }
    #endif

    //Move forward circle
    lit1 = table[lit1.var()];
    lit2 = table[lit2.var()] ^ !xorEqualFalse;

    //Already inside?
    if (lit1.var() == lit2.var()) {
        if (lit1.sign() != lit2.sign()) {

            //Add new (that is inverse) and fail
            #ifdef DRUP
            if (solver->drup) {
                *(solver->drup)
                << ~lit1 << " " << lit2 << " 0\n"
                << lit1 << " " << ~lit2 << " 0\n"
                << lit1 << " 0\n"
                << ~lit1 << " 0\n"
                << "0\n"
                ;
            }
            #endif
            solver->ok = false;
            return false;
        }

        //Already inside in the correct way, return
        return true;
    }

    //Not already inside
    #ifdef DRUP
    if (solver->drup) {
        *(solver->drup)
        << ~lit1 << " " << lit2 << " 0\n"
        << lit1 << " " << ~lit2 << " 0\n"
        ;
    }
    #endif

    //Even the moved-forward version must be unremoved
    assert(solver->varData[lit1.var()].removed == Removed::none
            || solver->varData[lit1.var()].removed == Removed::queued_replacer);
    assert(solver->varData[lit2.var()].removed == Removed::none
            || solver->varData[lit2.var()].removed == Removed::queued_replacer);

    lbool val1 = solver->value(lit1);
    lbool val2 = solver->value(lit2);
    if (val1 != l_Undef && val2 != l_Undef) {
        if (val1 != val2) {
            #ifdef DRUP
            if (solver->drup) {
                *(solver->drup)
                << ~lit1 << " 0\n"
                << lit1 << " 0\n"
                << "0\n";
            }
            #endif
            solver->ok = false;
        }

        //Already set, return with correct code
        return solver->ok;
    }

    //exactly one l_Undef, exectly one l_True/l_False
    if ((val1 != l_Undef && val2 == l_Undef) || (val2 != l_Undef && val1 == l_Undef)) {
        if (solver->ok) {
            Lit toEnqueue;
            if (val1 != l_Undef) {
                toEnqueue = lit2 ^ (val1 == l_False);
            } else {
                toEnqueue = lit1 ^ (val2 == l_False);
            }
            solver->enqueue(toEnqueue);

            #ifdef DRUP
            if (solver->drup) {
                *(solver->drup)
                << toEnqueue
                << " 0\n";
            }
            #endif

            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif

            solver->ok = (solver->propagate().isNULL());
        }
        return solver->ok;
    }

    assert(val1 == l_Undef && val2 == l_Undef);

    if (addLaterAsTwoBins)
        laterAddBinXor.push_back(LaterAddBinXor(lit1, lit2^true));

    solver->varData[lit1.var()].removed = Removed::queued_replacer;
    solver->varData[lit2.var()].removed = Removed::queued_replacer;
    if (reverseTable.find(lit1.var()) == reverseTable.end()) {
        reverseTable[lit2.var()].push_back(lit1.var());
        table[lit1.var()] = lit2 ^ lit1.sign();
        replacedVars++;
        return true;
    }

    if (reverseTable.find(lit2.var()) == reverseTable.end()) {
        reverseTable[lit1.var()].push_back(lit2.var());
        table[lit2.var()] = lit1 ^ lit2.sign();
        replacedVars++;
        return true;
    }

    //both have children
    setAllThatPointsHereTo(lit1.var(), lit2 ^ lit1.sign()); //erases reverseTable[lit1.var()]
    replacedVars++;
    return true;
}

/**
@brief Returns if we already know that var = lit

Also checks if var = ~lit, in which it sets solver->ok = false
*/
bool VarReplacer::alreadyIn(const Var var, const Lit lit)
{
    Lit lit2 = table[var];
    if (lit2.var() == lit.var()) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver->ok = false;
        }
        return true;
    }

    lit2 = table[lit.var()];
    if (lit2.var() == var) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver->ok = false;
        }
        return true;
    }

    return false;
}

/**
@brief Changes internal graph to set everything that pointed to var to point to lit
*/
void VarReplacer::setAllThatPointsHereTo(const Var var, const Lit lit)
{
    map<Var, vector<Var> >::iterator it = reverseTable.find(var);
    if (it != reverseTable.end()) {
        for(vector<Var>::const_iterator it2 = it->second.begin(), end = it->second.end(); it2 != end; it2++) {
            assert(table[*it2].var() == var);
            if (lit.var() != *it2) {
                table[*it2] = lit ^ table[*it2].sign();
                reverseTable[lit.var()].push_back(*it2);
            }
        }
        reverseTable.erase(it);
    }
    table[var] = lit;
    reverseTable[lit.var()].push_back(var);
}

void VarReplacer::newVar()
{
    table.push_back(Lit(table.size(), false));
}

void VarReplacer::updateVars(
    const std::vector< uint32_t >& outerToInter
    , const std::vector< uint32_t >& interToOuter
) {
    assert(laterAddBinXor.empty());

    updateArray(table, interToOuter);
    updateLitsMap(table, outerToInter);
    map<Var, vector<Var> > newReverseTable;
    for(map<Var, vector<Var> >::iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        updateArrayMapCopy(it->second, outerToInter);
        newReverseTable[outerToInter.at(it->first)] = it->second;
    }
    reverseTable.swap(newReverseTable);
}


bool VarReplacer::addLaterAddBinXor()
{
    assert(solver->ok);

    vector<Lit> ps(2);
    for(vector<LaterAddBinXor>::const_iterator
        it = laterAddBinXor.begin(), end = laterAddBinXor.end()
        ; it != end
        ; it++
    ) {
        ps[0] = it->lit1;
        ps[1] = it->lit2;
        solver->addClauseInt(ps);
        if (!solver->ok)
            return false;

        ps[0] ^= true;
        ps[1] ^= true;
        solver->addClauseInt(ps);
        if (!solver->ok)
            return false;
    }
    laterAddBinXor.clear();

    return true;
}

uint64_t VarReplacer::bytesMemUsed() const
{
    uint64_t b = 0;
    b += delayedEnqueue.capacity()*sizeof(Lit);
    b += laterAddBinXor.capacity()*sizeof(LaterAddBinXor);
    b += table.capacity()*sizeof(Lit);
    for(map<Var, vector<Var> >::const_iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        b += it->second.capacity()*sizeof(Lit);
    }
    b += reverseTable.size()*(sizeof(Var) + sizeof(vector<Var>)); //TODO under-counting

    return b;
}

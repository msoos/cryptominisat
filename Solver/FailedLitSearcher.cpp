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

#include "FailedLitSearcher.h"

#include <iomanip>
#include <utility>
#include <set>
#include <utility>
using std::make_pair;
using std::set;

#include "ThreadControl.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"
#include "CompleteDetachReattacher.h"

//#define VERBOSE_DEUBUG

/**
@brief Sets up variables that are used between calls to search()
*/
FailedLitSearcher::FailedLitSearcher(ThreadControl* _control):
    control(_control)
    , tmpPs(2)
    , totalTime(0)
    , numPropsMultiplier(1.0)
    , lastTimeFoundTruths(0)
    , numCalls(0)
{
}

bool FailedLitSearcher::search()
{
    assert(control->decisionLevel() == 0);
    if (control->nVars() == 0) return control->ok;
    if (control->conf.doCache && numCalls > 0) {
        if (!control->implCache.tryBoth(control)) return false;
    }

    uint64_t numProps = 40L*1000L*1000L;

    control->testAllClauseAttach();
    double myTime = cpuTime();
    const uint32_t origNumUnsetVars = control->getNumUnsetVars();

    //General Stats
    numFailed = 0;
    goodBothSame = 0;
    numCalls++;
    visitedAlready.clear();
    visitedAlready.resize(control->nVars()*2);

    //If failed var searching is going good, do successively more and more of it
    if ((double)lastTimeFoundTruths > (double)control->getNumUnsetVars() * 0.10)
        numPropsMultiplier = std::max(numPropsMultiplier*1.3, 2.0);
    else
        numPropsMultiplier = 1.0;
    numProps = (uint64_t) ((double)numProps * numPropsMultiplier * control->conf.failedLitMultiplier);

    //For BothSame
    propagated.resize(control->nVars(), 0);
    propValue.resize(control->nVars(), 0);

    //For calculating how many variables have really been set
    origTrailSize = control->trail.size();

    //For HyperBin
    addedBin = 0;
    removedBins = 0;

    origBogoProps = control->bogoProps;
    uint32_t i;
    for (i = 0; i < control->nVars(); i++) {
        const Var var = (control->mtrand.randInt() + i) % control->nVars();
        if (control->value(var) != l_Undef || !control->decision_var[var])
            continue;
        if (control->bogoProps >= origBogoProps + numProps)
            break;
        if (!tryBoth(Lit(var, false)))
            goto end;
    }

end:
    if (control->conf.verbosity  >= 1)
        printResults(myTime);

    if (control->ok
        && (numFailed || goodBothSame)
    ) {
        double time = cpuTime();
        if ((int)origNumUnsetVars - (int)control->getNumUnsetVars() >  (int)origNumUnsetVars/15
            && control->getNumClauses() > 500000
        ) {
            CompleteDetachReatacher reattacher(control);
            reattacher.detachNonBinsNonTris(true);
            const bool ret = reattacher.reattachNonBins();
            release_assert(ret == true);
        } else {
            control->clauseCleaner->removeAndCleanAll();
        }
        if (control->conf.verbosity  >= 1 && numFailed + goodBothSame > 100) {
            std::cout << "c Cleaning up after failed var search: " << std::setw(8) << std::fixed << std::setprecision(2) << cpuTime() - time << " s "
            << std::endl;
        }
    }

    lastTimeFoundTruths = control->trail.size() - origTrailSize;
    totalTime += cpuTime() - myTime;

    control->testAllClauseAttach();
    return control->ok;
}

void FailedLitSearcher::printResults(const double myTime) const
{
    std::cout << "c Flit: "<< std::setw(5) << numFailed <<
    " Blit: " << std::setw(6) << goodBothSame <<
    " Bin:"   << std::setw(7) << addedBin <<
    " RemBin:" << std::setw(7) << removedBins <<
    " P: " << std::setw(4) << std::fixed << std::setprecision(1) << (double)(control->bogoProps - origBogoProps)/1000000.0  << "M"
    " T: " << std::setw(5) << std::fixed << std::setprecision(2) << cpuTime() - myTime
    << std::endl;
}

bool FailedLitSearcher::tryBoth(const Lit lit)
{
    //Start-up cleaning
    propagated.removeThese(propagatedBitSet);
    assert(propagated.isZero());
    propagatedBitSet.clear();
    bothSame.clear();
    assert(uselessBin.empty());

    if (!visitedAlready[lit.toInt()]) {
        //Test removal of non-learnt binary clauses
        #ifdef DEBUG_REMOVE_USELESS_BIN
        fillTestUselessBinRemoval(lit);
        #endif

        control->newDecisionLevel();
        control->enqueue(lit);
        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "Trying " << lit << std::endl;
        #endif
        failed = control->propagateFull(uselessBin);
        if (failed != lit_Undef) {
            control->cancelZeroLight();
            numFailed++;
            vector<Lit> lits;
            lits.push_back(~failed);
            control->addClauseInt(lits, true);
            removeUselessBins();
            return control->ok;
        }

        //Fill bothprop, cache
        assert(control->decisionLevel() > 0);
        litOTFCache.clear();
        litOTFCacheNL.clear();
        bool onlyNonLearntUntilNow = true;
        for (size_t c = control->trail_lim[0]; c < control->trail.size(); c++) {
            const Lit thisLit = control->trail[c];
            const Var x = thisLit.var();
            visitedAlready[thisLit.toInt()] = 1;

            propagated.setBit(x);
            propagatedBitSet.push_back(x);

            if (control->value(x).getBool())
                propValue.setBit(x);
            else
                propValue.clearBit(x);

            if (control->conf.doCache
                && c != control->trail_lim[0]
            ) {
                onlyNonLearntUntilNow &= !control->propData[x].learntStep;
                if (onlyNonLearntUntilNow)
                    litOTFCacheNL.push_back(control->trail[c]);
                else
                    litOTFCache.push_back(control->trail[c]);
            }
        }
        if (control->conf.doCache) {
            control->implCache[(~lit).toInt()].merge(litOTFCache, false, control->seen);
            control->implCache[(~lit).toInt()].merge(litOTFCacheNL, true, control->seen);
        }

        control->cancelZeroLight();
        hyperBinResAll();
        removeUselessBins();
        #ifdef DEBUG_REMOVE_USELESS_BIN
        testBinRemoval(lit);
        #endif
    }

    if (!visitedAlready[(~lit).toInt()]) {
        //Test removal of non-learnt binary clauses
        #ifdef DEBUG_REMOVE_USELESS_BIN
        fillTestUselessBinRemoval(~lit);
        #endif

        //Doing inverse
        control->newDecisionLevel();
        control->enqueue(~lit);
        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "Trying (opp) " << (~lit) << std::endl;
        #endif
        failed = control->propagateFull(uselessBin);
        if (failed != lit_Undef) {
            control->cancelZeroLight();
            numFailed++;
            vector<Lit> lits;
            lits.push_back(~failed);
            control->addClauseInt(lits, true);
            removeUselessBins();
            return control->ok;
        }

        //Fill cache, check bothprop
        assert(control->decisionLevel() > 0);
        litOTFCache.clear();
        litOTFCacheNL.clear();
        bool onlyNonLearntUntilNow = true;
        for (size_t c = control->trail_lim[0]; c < control->trail.size() ; c++) {
            const Lit thisLit = control->trail[c];
            const Var x = thisLit.var();
            visitedAlready[thisLit.toInt()] = 1;

            if (propagated[x]) {
                if (propValue[x] == control->value(x).getBool()) {
                    //they both imply the same
                    bothSame.push_back(Lit(x, !propValue[x]));
                }
            }

            if (control->conf.doCache
                && c != control->trail_lim[0]
            ) {
                onlyNonLearntUntilNow &= !control->propData[x].learntStep;
                if (onlyNonLearntUntilNow)
                    litOTFCacheNL.push_back(control->trail[c]);
                else
                    litOTFCache.push_back(control->trail[c]);
            }
        }
        if (control->conf.doCache) {
            control->implCache[lit.toInt()].merge(litOTFCache, false, control->seen);
            control->implCache[lit.toInt()].merge(litOTFCacheNL, true, control->seen);
        }

        control->cancelZeroLight();
        removeUselessBins();
        hyperBinResAll();
        #ifdef DEBUG_REMOVE_USELESS_BIN
        testBinRemoval(~lit);
        #endif
    }

    for(uint32_t i = 0; i != bothSame.size() && control->ok; i++) {
        vector<Lit> lits;
        lits.push_back(bothSame[i]);
        control->addClauseInt(lits, true);
    }
    goodBothSame += bothSame.size();
    return control->ok;
}

void FailedLitSearcher::hyperBinResAll()
{
    for(std::set<BinaryClause>::const_iterator it = control->needToAddBinClause.begin(), end = control->needToAddBinClause.end(); it != end; it++) {
        tmpPs[0] = it->getLit1();
        tmpPs[1] = it->getLit2();
        Clause* cl = control->addClauseInt(tmpPs, true);
        assert(cl == NULL);
        assert(control->ok);
        addedBin++;
    }
    return;
}

void FailedLitSearcher::removeUselessBins()
{
    if (control->conf.doRemUselessBins) {
        for(std::set<BinaryClause>::iterator
            it = uselessBin.begin()
            , end = uselessBin.end()
            ; it != end
            ; it++
        ) {
            //std::cout << "Removing binary clause: " << *it << std::endl;
            removeWBin(control->watches, it->getLit1(), it->getLit2(), it->getLearnt());
            removeWBin(control->watches, it->getLit2(), it->getLit1(), it->getLearnt());

            //Update stats
            control->numBins--;
            if (it->getLearnt())
                control->learntsLits -= 2;
            else
                control->clausesLits -= 2;
            removedBins++;

            #ifdef VERBOSE_DEBUG_FULLPROP
            std::cout << "Removed bin: "
            << it->getLit1() << " , " << it->getLit2()
            << " , learnt: " << it->getLearnt() << std::endl;
            #endif
        }
    }
    uselessBin.clear();
}

#ifdef DEBUG_REMOVE_USELESS_BIN
void FailedLitSearcher::fillTestUselessBinRemoval(const Lit lit)
{
    origNLBEnqueuedVars.clear();
    control->newDecisionLevel();
    control->enqueue(lit);
    failed = (!control->propagateNonLearntBin().isNULL());
    for (int c = control->trail.size()-1; c >= (int)control->trail_lim[0]; c--) {
        Var x = control->trail[c].var();
        origNLBEnqueuedVars.push_back(x);
    }
    control->cancelZeroLight();

    origEnqueuedVars.clear();
    control->newDecisionLevel();
    control->enqueue(lit);
    failed = (!control->propagate(false).isNULL());
    for (int c = control->trail.size()-1; c >= (int)control->trail_lim[0]; c--) {
        Var x = control->trail[c].var();
        origEnqueuedVars.push_back(x);
    }
    control->cancelZeroLight();
}

void FailedLitSearcher::testBinRemoval(const Lit origLit)
{
    control->newDecisionLevel();
    control->enqueue(origLit);
    bool ok = control->propagate().isNULL();
    assert(ok && "Prop failed after hyper-bin adding&bin removal. We never reach this point in that case.");
    bool wrong = false;
    for (vector<Var>::const_iterator it = origEnqueuedVars.begin(), end = origEnqueuedVars.end(); it != end; it++) {
        if (control->value(*it) == l_Undef) {
            std::cout << "Value of var " << Lit(*it, false) << " is unset, but was set before!" << std::endl;
            wrong = true;
        }
    }
    assert(!wrong && "Learnt/Non-learnt bin removal is incorrect");
    control->cancelZeroLight();

    control->newDecisionLevel();
    control->enqueue(origLit);
    ok = control->propagateNonLearntBin().isNULL();
    assert(ok && "Prop failed after hyper-bin adding&bin removal. We never reach this point in that case.");
    for (vector<Var>::const_iterator it = origNLBEnqueuedVars.begin(), end = origNLBEnqueuedVars.end(); it != end; it++) {
        if (control->value(*it) == l_Undef) {
            std::cout << "Value of var " << Lit(*it, false) << " is unset, but was set before when propagating non-learnt!" << std::endl;
            wrong = true;
        }
    }
    assert(!wrong && "Non-learnt bin removal is incorrect");
    control->cancelZeroLight();
}
#endif

// void FailedLitSearcher::fillToTry(vector<Var>& toTry)
// {
//     uint32_t max = std::min(control->negPosDist.size()-1, (size_t)300);
//     while(true) {
//         Var var = control->negPosDist[control->mtrand.randInt(max)].var;
//         if (control->value(var) != l_Undef
//             || (control->varData[var].elimed != ELIMED_NONE
//                 && control->varData[var].elimed != ELIMED_QUEUED_VARREPLACER)
//             ) continue;
//
//         bool OK = true;
//         for (uint32_t i = 0; i < toTry.size(); i++) {
//             if (toTry[i] == var) {
//                 OK = false;
//                 break;
//             }
//         }
//         if (OK) {
//             toTry.push_back(var);
//             return;
//         }
//     }
// }
//
// const bool FailedLitSearcher::tryMultiLevelAll()
// {
//     assert(control->ok);
//     uint32_t backupNumUnits = control->trail.size();
//     double myTime = cpuTime();
//     uint32_t numTries = 0;
//     uint32_t finished = 0;
//     uint64_t oldBogoProps = control->bogoProps;
//     uint32_t enqueued = 0;
//     uint32_t numFailed = 0;
//
//     if (control->negPosDist.size() < 30) return true;
//
//     propagated.resize(control->nVars(), 0);
//     propagated2.resize(control->nVars(), 0);
//     propValue.resize(control->nVars(), 0);
//     assert(propagated.isZero());
//     assert(propagated2.isZero());
//
//     vector<Var> toTry;
//     while(control->bogoProps < oldBogoProps + 300*1000*1000) {
//         toTry.clear();
//         for (uint32_t i = 0; i < 3; i++) {
//             fillToTry(toTry);
//         }
//         numTries++;
//         if (!tryMultiLevel(toTry, enqueued, finished, numFailed)) goto end;
//     }
//
//     end:
//     assert(propagated.isZero());
//     assert(propagated2.isZero());
//
//     std::cout
//     << "c multiLevelBoth tried " <<  numTries
//     << " finished: " << finished
//     << " units: " << (control->trail.size() - backupNumUnits)
//     << " enqueued: " << enqueued
//     << " numFailed: " << numFailed
//     << " time: " << (cpuTime() - myTime)
//     << std::endl;
//
//     return control->ok;
// }
//
// const bool FailedLitSearcher::tryMultiLevel(const vector<Var>& vars, uint32_t& enqueued, uint32_t& finished, uint32_t& numFailed)
// {
//     assert(control->ok);
//
//     vector<Lit> toEnqueue;
//     bool first = true;
//     bool last = false;
//     //std::cout << "//////////////////" << std::endl;
//     for (uint32_t comb = 0; comb < (1U << vars.size()); comb++) {
//         last = (comb == (1U << vars.size())-1);
//         control->newDecisionLevel();
//         for (uint32_t i = 0; i < vars.size(); i++) {
//             control->enqueue(Lit(vars[i], comb&(0x1 << i)));
//             //std::cout << "lit: " << Lit(vars[i], comb&(1U << i)) << std::endl;
//         }
//         //std::cout << "---" << std::endl;
//         bool failed = !(control->propagate().isNULL());
//         if (failed) {
//             control->cancelZeroLight();
//             if (!first) propagated.setZero();
//             numFailed++;
//             return true;
//         }
//
//         for (int sublevel = control->trail.size()-1; sublevel > (int)control->trail_lim[0]; sublevel--) {
//             Var x = control->trail[sublevel].var();
//             if (first) {
//                 propagated.setBit(x);
//                 if (control->assigns[x].getBool()) propValue.setBit(x);
//                 else propValue.clearBit(x);
//             } else if (last) {
//                 if (propagated[x] && control->assigns[x].getBool() == propValue[x])
//                     toEnqueue.push_back(Lit(x, !propValue[x]));
//             } else {
//                 if (control->assigns[x].getBool() == propValue[x]) {
//                     propagated2.setBit(x);
//                 }
//             }
//         }
//         control->cancelZeroLight();
//         if (!first && !last) propagated &= propagated2;
//         propagated2.setZero();
//         if (propagated.isZero()) return true;
//         first = false;
//     }
//     propagated.setZero();
//     finished++;
//
//     for (vector<Lit>::iterator l = toEnqueue.begin(), end = toEnqueue.end(); l != end; l++) {
//         enqueued++;
//         control->enqueue(*l);
//     }
//     control->ok = control->propagate().isNULL();
//     //exit(-1);
//
//     return control->ok;
// }

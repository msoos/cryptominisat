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

#include "Prober.h"

#include <iomanip>
#include <utility>
#include <set>
#include <utility>
#include <cmath>

using std::make_pair;
using std::set;
using std::cout;
using std::endl;

#include "Solver.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "ClauseCleaner.h"
#include "CompleteDetachReattacher.h"

//#define VERBOSE_DEUBUG

/**
@brief Sets up variables that are used between calls to probe()
*/
Prober::Prober(Solver* _solver):
    solver(_solver)
    , tmpPs(2)
    , numPropsMultiplier(1.0)
    , lastTimeZeroDepthAssings(0)
    , numCalls(0)
{
}

struct ActSorter
{
    ActSorter(const vector<uint32_t>& _activities) :
        activities(_activities)
    {};

    const vector<uint32_t>& activities;

    bool operator()(uint32_t var1, uint32_t var2) const
    {
        //Least active vars first
        return (activities[var1] < activities[var2]);
    }
};

void Prober::sortAndResetCandidates()
{
    candidates.clear();
    candidates.resize(solver->candidateForBothProp.size());
    for(size_t i = 0; i < solver->candidateForBothProp.size(); i++) {
        Lit lit = Lit(i, false);
        candidates[i].var = lit.var();

        //Calculate approx number of literals propagated for positive polarity
        size_t posPolar =
            std::max<size_t>(
                solver->watches[(~lit).toInt()].size()
                , solver->candidateForBothProp[i].posLit
            );
        posPolar = std::max<size_t>(posPolar
            , solver->implCache[(~lit).toInt()].lits.size());

        //Calculate approx number of literals propagated for negative polarity
        size_t negPolar =
            std::max<size_t>(
                solver->watches[lit.toInt()].size()
                , solver->candidateForBothProp[i].negLit
            );
        negPolar = std::max<size_t>(negPolar
            , solver->implCache[lit.toInt()].lits.size());

        //Minimim of the two polarities
        candidates[i].minOfPolarities = std::min(posPolar, negPolar);
        //cout << "candidate size: " << candidates[i].minOfPolarities << endl;
    }

    //Sort candidates from MAX to MIN of 'minOfPolarities'
    std::sort(candidates.begin(), candidates.end());

    //Reset candidates
    std::fill(solver->candidateForBothProp.begin()
        , solver->candidateForBothProp.end()
        , Solver::TwoSignAppearances()
    );
}


bool Prober::probe()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->nVars() > 0);

    uint64_t numPropsTodo = 40L*1000L*1000L;

    solver->testAllClauseAttach();
    const double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();
    solver->clauseCleaner->removeAndCleanAll();

    //Stats
    extraTime = 0;
    solver->propStats.clear();
    runStats.clear();
    runStats.origNumFreeVars = solver->getNumFreeVars();
    runStats.origNumBins = solver->numBinsLearnt + solver->numBinsNonLearnt;
    numCalls++;

    //State
    visitedAlready.clear();
    visitedAlready.resize(solver->nVars()*2, 0);
    cacheUpdated.clear();
    cacheUpdated.resize(solver->nVars()*2, 0);
    propagatedBitSet.clear();
    propagated.resize(solver->nVars(), 0);
    propValue.resize(solver->nVars(), 0);

    //If failed var searching is going good, do successively more and more of it
    if ((double)lastTimeZeroDepthAssings > (double)solver->getNumFreeVars() * 0.10)
        numPropsMultiplier = std::max(numPropsMultiplier*1.3, 1.6);
    else
        numPropsMultiplier = 1.0;
    numPropsTodo = (uint64_t) ((double)numPropsTodo * numPropsMultiplier * solver->conf.probeMultiplier);
    numPropsTodo = (double)numPropsTodo * std::pow(numCalls, 0.2);

    //Use candidates
    sortAndResetCandidates();
    size_t atCandidates = 0;

    uint64_t origBogoProps = solver->propStats.bogoProps;
    while (solver->propStats.bogoProps + extraTime < origBogoProps + numPropsTodo) {
        uint32_t litnum;

        if (atCandidates < candidates.size()
            && candidates[atCandidates].minOfPolarities > 100
        ) {
            litnum = Lit(candidates[atCandidates].var, 0).toInt();
            atCandidates++;
        } else {
            litnum = solver->mtrand.randInt() % (solver->nVars()*2);
        }
        Lit lit = Lit::toLit(litnum);
        extraTime += 20;

        //Check if var is set already
        if (solver->value(lit.var()) != l_Undef
            || !solver->decision_var[lit.var()]
            || visitedAlready[lit.toInt()]
        ) {
            continue;
        }

        //If this lit is reachable from somewhere else, then reach it from there
        if (solver->litReachable[lit.toInt()].lit != lit_Undef) {
            const Lit betterlit = solver->litReachable[lit.toInt()].lit;
            if (solver->value(betterlit.var()) == l_Undef
                && solver->decision_var[betterlit.var()]
            ) {
                lit = betterlit;
                assert(!visitedAlready[lit.toInt()]);
            }
        }

        //Try it
        if (!tryThis(lit, true))
            goto end;

        //If we are still unset, do the opposite, too
        //this lets us carry out BothProp
        if (solver->value(lit) == l_Undef
            && !tryThis(~lit, false)
        ) {
            goto end;
        }
    }

end:

    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;
    if (solver->ok && runStats.zeroDepthAssigns) {
        double time = cpuTime();
        bool advancedCleanup = false;
        //If more than 10% were set, detach&reattach. It's faster
        if ((double)runStats.origNumFreeVars - (double)solver->getNumFreeVars()
                >  (double)runStats.origNumFreeVars/10.0
            && solver->getNumLongClauses() > 200000
        ) {
            //Advanced cleanup
            if (solver->conf.verbosity >= 5)
                cout << "c Advanced cleanup after probing" << endl;
            advancedCleanup = true;
            CompleteDetachReatacher reattacher(solver);
            reattacher.detachNonBinsNonTris();
            const bool ret = reattacher.reattachLongs();
            release_assert(ret == true);
        } else {
            //Standard cleanup
            if (solver->conf.verbosity >= 5)
                cout << "c Standard cleanup after probing" << endl;
            solver->clauseCleaner->removeAndCleanAll();
        }

        //Tell me about the speed of cleanup
        if (solver->conf.verbosity  >= 1 &&
            (runStats.zeroDepthAssigns > 100 || advancedCleanup)
        ) {
            cout
            << "c Cleaning up after probing: "
            << std::setw(8) << std::fixed << std::setprecision(2)
            << cpuTime() - time << " s "
            << endl;
        }
    }

    //Update stats
    for(size_t i = 0; i < visitedAlready.size(); i++) {
        if (visitedAlready[i])
            runStats.numVisited++;
    }
    lastTimeZeroDepthAssings = runStats.zeroDepthAssigns;
    runStats.cpu_time = cpuTime() - myTime;
    runStats.propStats = solver->propStats;
    globalStats += runStats;

    //Print & update stats
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }

    solver->testAllClauseAttach();
    return solver->ok;
}

bool Prober::tryThis(const Lit lit, const bool first)
{
    if (first) {
        propagated.removeThese(propagatedBitSet);
        propagatedBitSet.clear();
        bothSame.clear();
    }

    //Start-up cleaning
    runStats.numProbed++;

    //Test removal of non-learnt binary clauses
    #ifdef DEBUG_REMOVE_USELESS_BIN
    fillTestUselessBinRemoval(lit);
    #endif

    solver->newDecisionLevel();
    solver->enqueue(lit);

    if (solver->conf.verbosity >= 6)
        cout << "c Probing lit " << lit << endl;

    const Lit failed = solver->propagateFull();
    if (failed != lit_Undef) {
        if (solver->conf.verbosity >= 6) {
            cout << "c Failed on lit " << lit << endl;
        }

        solver->cancelZeroLight();

        //Update conflict stats
        runStats.numFailed++;
        runStats.conflStats.update(solver->lastConflictCausedBy);
        runStats.conflStats.numConflicts++;
        runStats.addedBin += solver->hyperBinResAll();
        runStats.removedBin += solver->removeUselessBins();

        vector<Lit> lits;
        lits.push_back(~failed);
        solver->addClauseInt(lits, true);

        return solver->ok;
    }

    //Fill bothprop, cache
    assert(solver->decisionLevel() > 0);
    size_t numElemsSet = solver->trail.size() - solver->trail_lim[0];
    for (int64_t c = solver->trail.size()-1; c != (int64_t)solver->trail_lim[0] - 1; c--) {
        const Lit thisLit = solver->trail[c];
        const Var var = thisLit.var();

        //If this is the first, set what is propagated
        if (first) {
            //Visited this var, needs clear later on
            propagatedBitSet.push_back(var);

            //Set prop has been done
            propagated.setBit(var);

            //Set propValue
            if (solver->assigns[var].getBool())
                propValue.setBit(var);
            else
                propValue.clearBit(var);
        } else if (propagated[var]) {
            if (propValue[var] == solver->value(var).getBool()) {
                //they both imply the same
                bothSame.push_back(Lit(var, !propValue[var]));
            } /*else if (c != (int)solver->trail_lim[0]) {
                bool isEqualTrue;
                assert(litToSet.sign() == false);
                tmpPs[0] = Lit(~lit.var(), false);
                tmpPs[1] = Lit(var, false);
                isEqualTrue = !propValue[var];
                binXorToAdd.push_back(BinXorToAdd(tmpPs[0], tmpPs[1], isEqualTrue));
            }*/
        }

        visitedAlready[thisLit.toInt()] = 1;

        //Update cache, if the trail was within limits (cacheUpdateCutoff)
        const Lit ancestor = solver->varData[thisLit.var()].reason.getAncestor();
        if (solver->conf.doCache
            && thisLit != lit
            && numElemsSet <= solver->conf.cacheUpdateCutoff
            //&& cacheUpdated[(~ancestor).toInt()] == 0
        ) {
            //Update stats/markings
            cacheUpdated[(~ancestor).toInt()]++;
            extraTime += 1;
            extraTime += solver->implCache[(~ancestor).toInt()].lits.size()/100;
            extraTime += solver->implCache[(~thisLit).toInt()].lits.size()/100;

            const bool learntStep = solver->varData[thisLit.var()].reason.getLearntStep();

            //Update the cache now
            assert(ancestor != lit_Undef);
            solver->implCache[(~ancestor).toInt()].merge(
                solver->implCache[(~thisLit).toInt()].lits
                , thisLit
                , learntStep
                , ancestor
                , solver->seen
            );

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "The impl cache of " << (~ancestor) << " is now: ";
            cout << solver->implCache[(~ancestor).toInt()] << endl;
            #endif
        }
    }

    solver->cancelZeroLight();
    runStats.addedBin += solver->hyperBinResAll();
    runStats.removedBin += solver->removeUselessBins();
    #ifdef DEBUG_REMOVE_USELESS_BIN
    testBinRemoval(lit);
    #endif

    if (!first) {
        //Add bothsame
        for(size_t i = 0; i < bothSame.size(); i++) {
            extraTime += 3;
            solver->enqueue(bothSame[i]);
        }
        runStats.bothSameAdded += bothSame.size();
    }
    assert(solver->ok);
    solver->ok = solver->propagate().isNULL();

    return solver->ok;
}

#ifdef DEBUG_REMOVE_USELESS_BIN
void Prober::fillTestUselessBinRemoval(const Lit lit)
{
    origNLBEnqueuedVars.clear();
    solver->newDecisionLevel();
    solver->enqueue(lit);
    failed = (!solver->propagateNonLearntBin().isNULL());
    for (int c = solver->trail.size()-1; c >= (int)solver->trail_lim[0]; c--) {
        Var x = solver->trail[c].var();
        origNLBEnqueuedVars.push_back(x);
    }
    solver->cancelZeroLight();

    origEnqueuedVars.clear();
    solver->newDecisionLevel();
    solver->enqueue(lit);
    failed = (!solver->propagate(false).isNULL());
    for (int c = solver->trail.size()-1; c >= (int)solver->trail_lim[0]; c--) {
        Var x = solver->trail[c].var();
        origEnqueuedVars.push_back(x);
    }
    solver->cancelZeroLight();
}

void Prober::testBinRemoval(const Lit origLit)
{
    solver->newDecisionLevel();
    solver->enqueue(origLit);
    bool ok = solver->propagate().isNULL();
    assert(ok && "Prop failed after hyper-bin adding&bin removal. We never reach this point in that case.");
    bool wrong = false;
    for (vector<Var>::const_iterator it = origEnqueuedVars.begin(), end = origEnqueuedVars.end(); it != end; it++) {
        if (solver->value(*it) == l_Undef) {
            cout << "Value of var " << Lit(*it, false) << " is unset, but was set before!" << endl;
            wrong = true;
        }
    }
    assert(!wrong && "Learnt/Non-learnt bin removal is incorrect");
    solver->cancelZeroLight();

    solver->newDecisionLevel();
    solver->enqueue(origLit);
    ok = solver->propagateNonLearntBin().isNULL();
    assert(ok && "Prop failed after hyper-bin adding&bin removal. We never reach this point in that case.");
    for (vector<Var>::const_iterator it = origNLBEnqueuedVars.begin(), end = origNLBEnqueuedVars.end(); it != end; it++) {
        if (solver->value(*it) == l_Undef) {
            cout << "Value of var " << Lit(*it, false) << " is unset, but was set before when propagating non-learnt!" << endl;
            wrong = true;
        }
    }
    assert(!wrong && "Non-learnt bin removal is incorrect");
    solver->cancelZeroLight();
}
#endif

// void Prober::fillToTry(vector<Var>& toTry)
// {
//     uint32_t max = std::min(solver->negPosDist.size()-1, (size_t)300);
//     while(true) {
//         Var var = solver->negPosDist[solver->mtrand.randInt(max)].var;
//         if (solver->value(var) != l_Undef
//             || (solver->varData[var].elimed != ELIMED_NONE
//                 && solver->varData[var].elimed != ELIMED_QUEUED_VARREPLACER)
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
// const bool Prober::tryMultiLevelAll()
// {
//     assert(solver->ok);
//     uint32_t backupNumUnits = solver->trail.size();
//     double myTime = cpuTime();
//     uint32_t numTries = 0;
//     uint32_t finished = 0;
//     uint64_t oldBogoProps = solver->bogoProps;
//     uint32_t enqueued = 0;
//     uint32_t numFailed = 0;
//
//     if (solver->negPosDist.size() < 30) return true;
//
//     propagated.resize(solver->nVars(), 0);
//     propagated2.resize(solver->nVars(), 0);
//     propValue.resize(solver->nVars(), 0);
//     assert(propagated.isZero());
//     assert(propagated2.isZero());
//
//     vector<Var> toTry;
//     while(solver->bogoProps < oldBogoProps + 300*1000*1000) {
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
//     cout
//     << "c multiLevelBoth tried " <<  numTries
//     << " finished: " << finished
//     << " units: " << (solver->trail.size() - backupNumUnits)
//     << " enqueued: " << enqueued
//     << " numFailed: " << numFailed
//     << " time: " << (cpuTime() - myTime)
//     << endl;
//
//     return solver->ok;
// }
//
// const bool Prober::tryMultiLevel(const vector<Var>& vars, uint32_t& enqueued, uint32_t& finished, uint32_t& numFailed)
// {
//     assert(solver->ok);
//
//     vector<Lit> toEnqueue;
//     bool first = true;
//     bool last = false;
//     //cout << "//////////////////" << endl;
//     for (uint32_t comb = 0; comb < (1U << vars.size()); comb++) {
//         last = (comb == (1U << vars.size())-1);
//         solver->newDecisionLevel();
//         for (uint32_t i = 0; i < vars.size(); i++) {
//             solver->enqueue(Lit(vars[i], comb&(0x1 << i)));
//             //cout << "lit: " << Lit(vars[i], comb&(1U << i)) << endl;
//         }
//         //cout << "---" << endl;
//         bool failed = !(solver->propagate().isNULL());
//         if (failed) {
//             solver->cancelZeroLight();
//             if (!first) propagated.setZero();
//             numFailed++;
//             return true;
//         }
//
//         for (int sublevel = solver->trail.size()-1; sublevel > (int)solver->trail_lim[0]; sublevel--) {
//             Var x = solver->trail[sublevel].var();
//             if (first) {
//                 propagated.setBit(x);
//                 if (solver->assigns[x].getBool()) propValue.setBit(x);
//                 else propValue.clearBit(x);
//             } else if (last) {
//                 if (propagated[x] && solver->assigns[x].getBool() == propValue[x])
//                     toEnqueue.push_back(Lit(x, !propValue[x]));
//             } else {
//                 if (solver->assigns[x].getBool() == propValue[x]) {
//                     propagated2.setBit(x);
//                 }
//             }
//         }
//         solver->cancelZeroLight();
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
//         solver->enqueue(*l);
//     }
//     solver->ok = solver->propagate().isNULL();
//     //exit(-1);
//
//     return solver->ok;
// }

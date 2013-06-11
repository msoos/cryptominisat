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

#include "prober.h"

#include <iomanip>
#include <utility>
#include <set>
#include <utility>
#include <cmath>

#include "solver.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "clausecleaner.h"
#include "completedetachreattacher.h"

using namespace CMSat;
using std::make_pair;
using std::set;
using std::cout;
using std::endl;

//#define VERBOSE_DEUBUG

/**
@brief Sets up variables that are used between calls to probe()
*/
Prober::Prober(Solver* _solver):
    solver(_solver)
    , numPropsMultiplier(1.0)
    , lastTimeZeroDepthAssings(0)
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

void Prober::checkOTFRatio()
{
    double ratio = (double)solver->propStats.bogoProps
    /(double)(solver->propStats.otfHyperTime + solver->propStats.bogoProps);

    /*static int val = 0;
    if (val  % 10 == 0) {
        cout << "Ratio is " << std::setprecision(2) << ratio << endl;
    }
    val++;*/

    if (solver->conf.verbosity >= 2) {
        cout
        << "c [probe] Ratio of hyperbin/(bogo+hyperbin) is : "
        << std::setprecision(2) << ratio
        << " (this indicates how much time is spent doing hyperbin&trans-red)"
        << endl;
    }

    if (solver->propStats.bogoProps+solver->propStats.otfHyperTime
            > 0.8*800LL*1000LL*1000LL
        && ratio < 0.3
        && solver->conf.otfHyperbin
        #ifdef DRUP
        && !solver->drup
        #endif
    ) {
        solver->conf.otfHyperbin = false;
        if (solver->conf.verbosity >= 2) {
            cout << "c [probe] no longer doing OTF hyper-bin&trans-red" << endl;
        }
        solver->needToAddBinClause.clear();
        solver->uselessBin.clear();
    }
}

bool Prober::probe()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->nVars() > 0);

    uint64_t numPropsTodo = 1900LL*1000LL*1000LL;

    //Bogoprops for hyper-bin is MUCH more precise, so if no propagateFull???
    //then mush less bogoProps will lead to the same amount of time
    if (!solver->conf.otfHyperbin) {
        numPropsTodo /= 4;
    }

    //Account for cache being too small
    const size_t numActiveVars = solver->numActiveVars();
    if (numActiveVars < 50LL*1000LL) {
        numPropsTodo *= 1.2;
    }
    if (solver->binTri.redLits + solver->binTri.irredLits  < 2LL*1000LL*1000LL) {
        numPropsTodo *= 1.2;
    }
    if (numActiveVars > 600LL*1000LL) {
        numPropsTodo *= 0.8;
    }
    if (solver->binTri.redLits + solver->binTri.irredLits > 20LL*1000LL*1000LL) {
        numPropsTodo *= 0.8;
    }
    if (solver->conf.verbosity >= 2) {
    cout
        << "c [probe] lits : "
        << std::setprecision(2) << (double)(solver->binTri.redLits + solver->binTri.irredLits)/(1000.0*1000.0)
        << "M"
        << " act vars: "
        << std::setprecision(2) << (double)numActiveVars/1000.0 << "K"
        << " BP+HP todo: "
        << std::setprecision(2) << (double)numPropsTodo/(1000.0*1000.0) << "M"
        << endl;
    }

    solver->testAllClauseAttach();
    const double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();

    //Clean clauses
    if (solver->conf.verbosity >= 6) {
        cout << "c Cleaning clauses before probing." << endl;
    }
    solver->clauseCleaner->removeAndCleanAll();
    if (solver->conf.verbosity >= 6) {
        cout << "c Cleaning clauses before probing finished." << endl;
    }

    //Stats
    extraTime = 0;
    extraTimeCache = 0;
    solver->propStats.clear();
    runStats.clear();
    runStats.origNumFreeVars = numActiveVars;
    runStats.origNumBins = solver->binTri.redBins + solver->binTri.irredBins;

    //State
    visitedAlready.clear();
    visitedAlready.resize(solver->nVars()*2, 0);
    propagatedBitSet.clear();
    propagated.clear();
    propagated.resize(solver->nVars(), 0);
    propValue.resize(solver->nVars());

    //If failed var searching is going good, do successively more and more of it
    double percentEffectLast = (double)lastTimeZeroDepthAssings/(double)numActiveVars * 100.0;
    if (percentEffectLast > 20.0) {
        //It's doing VERY well
        numPropsMultiplier = std::min(numPropsMultiplier*2, 5.0);
    } else if (percentEffectLast >= 10.0) {
        //It's doing well
        numPropsMultiplier = std::min(numPropsMultiplier*1.6, 4.0);
    } else if (percentEffectLast <= 3) {
        //It's doing badly
        numPropsMultiplier = 0.5;
    } else {
        //It's doing OK
        numPropsMultiplier = 1.0;
    }

    //First start is special, there is no previous record
    if (globalStats.numCalls == 0) {
        numPropsMultiplier = 1.0;
    }

    numPropsTodo = (uint64_t) ((double)numPropsTodo * numPropsMultiplier * solver->conf.probeMultiplier);
    const size_t numPropsTodoAftPerf = numPropsTodo;
    numPropsTodo = (double)numPropsTodo * std::pow((double)(globalStats.numCalls+1), 0.2);

    if (solver->conf.verbosity >=2 ) {
        cout
        << "c [probe] NumProps after perf multi: "
        << std::setprecision(2) << (double)numPropsTodoAftPerf/(1000.0*1000.0)
        << "M"
        << " after numcall multi: "
        << std::setprecision(2) << (double)numPropsTodo/(1000.0*1000.0)
        << "M (<- final)"
        << endl;
    }

    //Use candidates
    //sortAndResetCandidates();
    //candidates.clear();

    //Calculate the set of possible variables for branching on randomly
    vector<Var> possCh;
    for(size_t i = 0; i < solver->nVars(); i++) {
        if (solver->value(i) == l_Undef
            && (solver->varData[i].removed == Removed::none
                || solver->varData[i].removed == Removed::queued_replacer)
        ) {
            possCh.push_back(i);
        }
    }

    //Random swap
    for (size_t i = 0
        ; i + 1< possCh.size()
        ; i++
    ) {
        std::swap(
            possCh[i]
            , possCh[i+solver->mtrand.randInt(possCh.size()-1-i)]
        );
    }

    //For fast black-listing, O(1)-time lookup
    vector<size_t> lookup(solver->nVars(), std::numeric_limits<size_t>::max());
    for (size_t i = 0; i < possCh.size(); i++) {
        lookup[possCh[i]] = i;
    }

    assert(solver->propStats.bogoProps == 0);
    assert(solver->propStats.otfHyperTime == 0);
    for(size_t i = 0
        ; i < possCh.size()
            && solver->propStats.bogoProps
                + solver->propStats.otfHyperTime
                + extraTime + extraTimeCache
                < numPropsTodo
        ; i++
    ) {
        extraTime += 20;
        runStats.numLoopIters++;
        const Var var = possCh[i];

        //Check if already blacklisted
        if (var == std::numeric_limits<Var>::max())
            continue;

        //Probe 'false' first --> this is not critical
        //but one has to be chosen.
        Lit lit = Lit(var, false);

        //Check if var is set already
        if (solver->value(lit.var()) != l_Undef
            || !solver->decisionVar[lit.var()]
            || visitedAlready[lit.toInt()]
        ) {
            continue;
        }

        if (solver->conf.doStamp) {
            //If this lit is reachable from somewhere else, then reach it from there
            if (solver->stamp.tstamp[lit.toInt()].dominator[STAMP_IRRED] != lit_Undef) {
                const Lit betterlit = solver->stamp.tstamp[lit.toInt()].dominator[STAMP_IRRED];
                if (solver->value(betterlit.var()) == l_Undef
                    && solver->decisionVar[betterlit.var()]
                ) {
                    //Update lit
                    lit = betterlit;

                    //Blacklist new lit
                    possCh[lookup[lit.var()]] = std::numeric_limits<Var>::max();

                    //Must not have visited it already, otherwise the stamp dominator would be incorrect
                    assert(!visitedAlready[lit.toInt()]);
                }
            }
        } else if (solver->conf.doCache) {
            if (solver->litReachable[lit.toInt()].lit != lit_Undef) {
                const Lit betterlit = solver->litReachable[lit.toInt()].lit;
                if (solver->value(betterlit.var()) == l_Undef
                    && solver->decisionVar[betterlit.var()]
                ) {
                    //Update lit
                    lit = betterlit;
                }
            }
        }

        //Update stats
        runStats.numVarProbed++;
        extraTime += 20;

        //Try it
        if (!tryThis(lit, true))
            goto end;

        //If we are still unset, do the opposite, too
        //this lets us carry out BothProp
        if (solver->value(lit) == l_Undef
            && !tryThis((~lit), false)
        ) {
            goto end;
        }
    }

end:

    //If time wasted on cache updating (extraTime) is large, stop cache
    //updation
    double timeOnCache = (double)extraTimeCache
            /(double)(solver->propStats.bogoProps
               + solver->propStats.otfHyperTime
               + extraTime + extraTimeCache
            ) * 100.0;

    //More than 50% of the time is spent updating the cache... that's a lot
    //Disable and free
    if (timeOnCache > 50.0 && solver->conf.doCache)  {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [probe] too much time spent on updating cache: "
            << std::fixed << std::setprecision(1) << timeOnCache
            << "% during probing --> disabling cache"
            << endl;
        }

        solver->conf.doCache = false;
        solver->implCache.free();
    } else {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [probe] time spent updating cache during probing: "
            << std::fixed << std::setprecision(1) << timeOnCache
            << "%"
            << endl;
        }
    }

    //Delete any remaining binaries to add or remove
    //next time, variables will be renumbered/etc. so it will be wrong
    //to add/remove them
    solver->needToAddBinClause.clear();
    solver->uselessBin.clear();

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
    runStats.timeAllocated += numPropsTodo;
    runStats.numCalls = 1;
    globalStats += runStats;

    //Check if we need to disable OTF hyper-bin&transitive reduction
    const double ratioUsedTime =
        (solver->propStats.bogoProps + solver->propStats.otfHyperTime + extraTime)
        /(double)numPropsTodo
    ;
    if (solver->conf.otfHyperbin
        //Visited less than half
        && (double)runStats.numVisited/(double)(runStats.origNumFreeVars*2) < 0.4
        //And we used up most of the time
        && ratioUsedTime > 0.8
    ) {
        checkOTFRatio();
    }

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
    //Clean state if this is the 1st of two
    if (first) {
        extraTime += propagatedBitSet.size();
        for(vector<uint32_t>::const_iterator
            it = propagatedBitSet.begin(), end = propagatedBitSet.end()
            ; it != end
            ; it++
        ) {
            propagated[*it] = false;
        }
        propagatedBitSet.clear();
    }
    toEnqueue.clear();

    //Start-up cleaning
    runStats.numProbed++;

    //Test removal of non-learnt binary clauses
    #ifdef DEBUG_REMOVE_USELESS_BIN
    fillTestUselessBinRemoval(lit);
    #endif

    solver->newDecisionLevel();
    solver->enqueue(lit);
    solver->varData[lit.var()].depth = 0;

    //Display what we are doing in case of high verbosity
    if (solver->conf.verbosity >= 6) {
        cout
        << "c Probing lit " << lit
        << endl;
    }

    Lit failed = lit_Undef;
    if (solver->conf.otfHyperbin) {
        //Set timeout for ONE enqueue. This used so that in case ONE enqueue
        //takes too long (usually because of hyper-bin), we exit early
        const uint64_t timeout =
            solver->propStats.otfHyperTime
            + solver->propStats.bogoProps
            + 1600ULL*1000ULL*1000ULL;

        //DFS is expensive, actually. So do BFS 50% of the time
        if (solver->conf.doStamp && solver->mtrand.randInt(1) == 0) {
            const StampType stampType = solver->mtrand.randInt(1) ? STAMP_IRRED : STAMP_RED;
            failed = solver->propagateFullDFS(
                stampType
                , timeout //early-abort timeout
            );
        } else {
            failed = solver->propagateFullBFS(
                timeout //early-abort timeout
            );
        }

        //If we timed out on ONE call, turn otf hyper-bin off
        //and return --> the "visitedAlready" will be wrong
        if (solver->timedOutPropagateFull
            #ifdef DRUP
            && !solver->drup
            #endif
        ) {
            if (solver->conf.verbosity >= 2) {
                cout
                << "c [probe] timeout during propagation,"
                << " turning off OTF hyper-bin&trans-red"
                << endl;
            }

            solver->conf.otfHyperbin = false;
            solver->cancelZeroLight();

            runStats.addedBin += solver->hyperBinResAll();
            std::pair<size_t, size_t> tmp = solver->removeUselessBins();
            runStats.removedIrredBin += tmp.first;
            runStats.removedRedBin += tmp.second;

            for(vector<uint32_t>::const_iterator
                it = propagatedBitSet.begin(), end = propagatedBitSet.end()
                ; it != end
                ; it++
            ) {
                propagated[*it] = false;
            }
            propagatedBitSet.clear();
            toEnqueue.clear();
            return solver->okay();
        }
    } else {
        //No hyper-bin so we use regular propagate and regular analyze

        PropBy confl = solver->propagate();
        if (!confl.isNULL()) {
            ResolutionTypes<uint16_t> resolutions;
            uint32_t  glue;
            uint32_t  backtrack_level;
            solver->analyze(
                confl
                , backtrack_level  //return backtrack level here
                , glue             //return glue here
                , resolutions   //return number of resolutions made here
                , true
            );
            if (solver->learnt_clause.empty()) {
                solver->ok = false;
                return false;
            }
            assert(solver->learnt_clause.size() == 1);
            failed = ~(solver->learnt_clause[0]);
        }
    }

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
        std::pair<size_t, size_t> tmp = solver->removeUselessBins();
        runStats.removedIrredBin += tmp.first;
        runStats.removedRedBin += tmp.second;

        vector<Lit> lits;
        lits.push_back(~failed);
        solver->addClauseInt(lits, true);

        return solver->ok;
    }

    //Fill bothprop, cache
    assert(solver->decisionLevel() > 0);
    size_t numElemsSet = solver->trail.size() - solver->trail_lim[0];
    for (int64_t c = solver->trail.size()-1
        ; c != (int64_t)solver->trail_lim[0] - 1
        ; c--
    ) {
        extraTime += 2;
        const Lit thisLit = solver->trail[c];
        const Var var = thisLit.var();

        //If this is the first, set what is propagated
        if (first) {
            //Visited this var, needs clear later on
            propagatedBitSet.push_back(var);

            //Set prop has been done
            propagated[var] = true;

            //Set propValue
            if (solver->value(var).getBool())
                propValue[var] = true;
            else
                propValue[var] = false;
        } else if (propagated[var]) {
            if (propValue[var] == solver->value(var).getBool()) {

                //they both imply the same
                const Lit litToEnq = Lit(var, !propValue[var]);
                toEnqueue.push_back(litToEnq);
                #ifdef DRUP
                if (solver->drup) {
                    if (solver->conf.verbosity >= 6) {
                        cout
                        << "c bprop:"
                        << litToEnq
                        << endl;
                    }
                    (*solver->drup)
                    << litToEnq
                    << " 0\n";
                }
                #endif
            }
        }

        visitedAlready[thisLit.toInt()] = 1;

        if (!solver->conf.otfHyperbin)
            continue;

        //Update cache, if the trail was within limits (cacheUpdateCutoff)
        const Lit ancestor = solver->varData[thisLit.var()].reason.getAncestor();
        if (solver->conf.doCache
            && thisLit != lit
            && numElemsSet <= solver->conf.cacheUpdateCutoff
            //&& cacheUpdated[(~ancestor).toInt()] == 0
        ) {
            //Update stats/markings
            //cacheUpdated[(~ancestor).toInt()]++;
            extraTime += 1;
            extraTimeCache += solver->implCache[(~ancestor).toInt()].lits.size()/30;
            extraTimeCache += solver->implCache[(~thisLit).toInt()].lits.size()/30;

            const bool learntStep = solver->varData[thisLit.var()].reason.getLearntStep();

            //Update the cache now
            assert(ancestor != lit_Undef);
            bool taut = solver->implCache[(~ancestor).toInt()].merge(
                solver->implCache[(~thisLit).toInt()].lits
                , thisLit
                , learntStep
                , ancestor.var()
                , solver->seen
            );

            //If tautology according to cache we can
            //enqueue ~ancestor at toplevel since both
            //~ancestor V OTHER, and ~ancestor V ~OTHER are technically in
            if (taut
                && (solver->varData[ancestor.var()].removed == Removed::none
                    || solver->varData[ancestor.var()].removed == Removed::queued_replacer)
            ) {
                toEnqueue.push_back(~ancestor);
            }

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "The impl cache of " << (~ancestor) << " is now: ";
            cout << solver->implCache[(~ancestor).toInt()] << endl;
            #endif
        }
    }

    if (!solver->conf.otfHyperbin
        && solver->conf.doCache
    ) {
        tmp.clear();
        for (int64_t c = solver->trail.size()-1
            ; c != (int64_t)solver->trail_lim[0] - 1
            ; c--
        ) {
            extraTime += 2;
            const Lit thisLit = solver->trail[c];
            tmp.push_back(thisLit);
        }

        bool taut = solver->implCache[(~lit).toInt()].merge(
            tmp
            , lit_Undef
            , true //Learnt step -- we don't know, so we assume
            , lit.var()
            , solver->seen
        );

        //If tautology according to cache we can
        //enqueue ~lit at toplevel since both
        //~lit V OTHER, and ~lit V ~OTHER are technically in
        if (taut) {
            toEnqueue.push_back(~lit);
            #ifdef DRUP
            if (solver->drup) {
                (*solver->drup)
                << (~lit)
                << " 0\n";
            }
            #endif
        }
    }

    solver->cancelZeroLight();
    runStats.addedBin += solver->hyperBinResAll();
    std::pair<size_t, size_t> tmp = solver->removeUselessBins();
    runStats.removedIrredBin += tmp.first;
    runStats.removedRedBin += tmp.second;
    #ifdef DEBUG_REMOVE_USELESS_BIN
    testBinRemoval(lit);
    #endif

    //Add toEnqueue
    assert(solver->ok);
    runStats.bothSameAdded += toEnqueue.size();
    extraTime += 3*toEnqueue.size();
    return solver->enqueueThese(toEnqueue);
}

uint64_t Prober::memUsed() const
{
    uint64_t mem = 0;
    mem += visitedAlready.capacity()*sizeof(char);
    mem += propagatedBitSet.capacity()*sizeof(uint32_t);
    mem += toEnqueue.capacity()*sizeof(Lit);
    mem += tmp.capacity()*sizeof(Lit);
    mem += propagated.capacity()/8;
    mem += propValue.capacity()/8;
    //mem += candidates.capacity()*sizeof(TwoSignVar);

    return mem;
}

/*void Prober::sortAndResetCandidates()
{
    candidates.clear();
    candidates.resize(solver->nVars());
    for(size_t i = 0; i < solver->nVars(); i++) {
        Lit lit = Lit(i, false);
        candidates[i].var = lit.var();

        //Calculate approx number of literals propagated for positive polarity
        //TODO stamping -- replace '0'
        size_t posPolar =
            std::max<size_t>(
                solver->watches[(~lit).toInt()].size()
                , 0//solver->implCache[(~lit).toInt()].lits.size()
            );

        //Calculate approx number of literals propagated for negative polarity
        //TODO stamping -- replace '0'
        size_t negPolar =
            std::max<size_t>(
                solver->watches[lit.toInt()].size()
                , 0 //solver->implCache[lit.toInt()].lits.size()
            );

        //Minimim of the two polarities
        candidates[i].minOfPolarities = std::min(posPolar, negPolar);
        //cout << "candidate size: " << candidates[i].minOfPolarities << endl;
    }

    //Sort candidates from MAX to MIN of 'minOfPolarities'
    std::sort(candidates.begin(), candidates.end());
}*/

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
//             || (solver->varData[var].removed != Removed::none
//                 && solver->varData[var].removed != Removed::queued_replacer)
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


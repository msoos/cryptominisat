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
#include "sqlstats.h"

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
    {}

    const vector<uint32_t>& activities;

    bool operator()(uint32_t var1, uint32_t var2) const
    {
        //Least active vars first
        return (activities[var1] < activities[var2]);
    }
};

uint64_t Prober::limit_used() const
{
    return solver->propStats.bogoProps + solver->propStats.otfHyperTime + extraTime + extraTimeCache;
}

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
        && !solver->drup->enabled()
    ) {
        solver->conf.otfHyperbin = false;
        if (solver->conf.verbosity >= 2) {
            cout << "c [probe] no longer doing OTF hyper-bin&trans-red" << endl;
        }
        solver->needToAddBinClause.clear();
        solver->uselessBin.clear();
    }
}

void Prober::reset_stats_and_state()
{
    extraTime = 0;
    extraTimeCache = 0;
    solver->propStats.clear();
    runStats.clear();
    runStats.origNumBins = solver->binTri.redBins + solver->binTri.irredBins;

    visitedAlready.clear();
    visitedAlready.resize(solver->nVars()*2, 0);
    propagatedBitSet.clear();
    propagated.clear();
    propagated.resize(solver->nVars(), 0);
    propValue.resize(solver->nVars());
}

uint64_t Prober::calc_numpropstodo()
{
    uint64_t numPropsTodo = solver->conf.probe_bogoprops_timeoutM*1000ULL*1000ULL;

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
    if (solver->litStats.redLits + solver->litStats.irredLits  < 2LL*1000LL*1000LL) {
        numPropsTodo *= 1.2;
    }
    if (numActiveVars > 600LL*1000LL) {
        numPropsTodo *= 0.8;
    }
    if (solver->litStats.redLits + solver->litStats.irredLits > 20LL*1000LL*1000LL) {
        numPropsTodo *= 0.8;
    }

    runStats.origNumFreeVars = numActiveVars;
    if (solver->conf.verbosity >= 2) {
    cout
        << "c [probe] lits : "
        << std::setprecision(2) << (double)(solver->litStats.redLits + solver->litStats.irredLits)/(1000.0*1000.0)
        << "M"
        << " act vars: "
        << std::setprecision(2) << (double)numActiveVars/1000.0 << "K"
        << " BP+HP todo: "
        << std::setprecision(2) << (double)numPropsTodo/(1000.0*1000.0) << "M"
        << endl;
    }

    return numPropsTodo;
}

void Prober::clean_clauses_before_probe()
{
    if (solver->conf.verbosity >= 6) {
        cout << "c Cleaning clauses before probing." << endl;
    }
    solver->clauseCleaner->removeAndCleanAll();
    if (solver->conf.verbosity >= 6) {
        cout << "c Cleaning clauses before probing finished." << endl;
    }
}

uint64_t Prober::update_numpropstodo_based_on_prev_performance(uint64_t numPropsTodo)
{
     //If failed var searching is going good, do successively more and more of it
    double percentEffectLast = (double)lastTimeZeroDepthAssings/(double)runStats.origNumFreeVars * 100.0;
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

    numPropsTodo = (double)numPropsTodo * numPropsMultiplier;
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

    return numPropsTodo;
}

void Prober::clean_clauses_after_probe()
{
    double time = cpuTime();
    bool advancedCleanup = false;

    //If more than 10% were set, detach&reattach. It's faster
    if ((double)runStats.origNumFreeVars - (double)solver->getNumFreeVars()
            >  (double)runStats.origNumFreeVars/10.0
        && solver->getNumLongClauses() > 200000
    ) {
        if (solver->conf.verbosity >= 5)
            cout << "c Advanced cleanup after probing" << endl;

        advancedCleanup = true;
        CompleteDetachReatacher reattacher(solver);
        reattacher.detachNonBinsNonTris();
        const bool ret = reattacher.reattachLongs();
        release_assert(ret == true);
    } else {
        if (solver->conf.verbosity >= 5)
            cout << "c Standard cleanup after probing" << endl;

        solver->clauseCleaner->removeAndCleanAll();
    }

    if (solver->conf.verbosity  >= 1 &&
        (runStats.zeroDepthAssigns > 100 || advancedCleanup)
    ) {
        cout
        << "c [probe] cleaning up after T: "
        << std::setw(8) << std::fixed << std::setprecision(2)
        << cpuTime() - time << " s "
        << endl;
    }
}

void Prober::check_if_must_disable_otf_hyperbin_and_tred(const uint64_t numPropsTodo)
{
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
}

void Prober::check_if_must_disable_cache_update()
{
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
}

Lit Prober::update_lit_for_dominator(
    Lit lit
    , vector<Var>& poss_choice
    , const vector<size_t>& fast_rnd_lookup
) {
    if (solver->conf.doStamp) {
        //If this lit is reachable from somewhere else, then reach it from there
        if (solver->stamp.tstamp[lit.toInt()].dominator[STAMP_IRRED] != lit_Undef) {
            const Lit betterlit = solver->stamp.tstamp[lit.toInt()].dominator[STAMP_IRRED];
            if (solver->value(betterlit.var()) == l_Undef
                && solver->varData[betterlit.var()].is_decision
            ) {
                //Update lit
                lit = betterlit;

                //Blacklist new lit
                poss_choice[fast_rnd_lookup[lit.var()]] = std::numeric_limits<Var>::max();

                //Must not have visited it already, otherwise the stamp dominator would be incorrect
                assert(!visitedAlready[lit.toInt()]);
            }
        }
    } else  if (solver->conf.doCache) {
        if (solver->litReachable[lit.toInt()].lit != lit_Undef) {
            const Lit betterlit = solver->litReachable[lit.toInt()].lit;
            if (solver->value(betterlit.var()) == l_Undef
                && solver->varData[betterlit.var()].is_decision
            ) {
                //Update lit
                lit = betterlit;
            }
        }
    }

    return lit;
}

vector<Var> Prober::randomize_possible_choices()
{
    vector<Var> poss_choice;
    for(size_t i = 0; i < solver->nVars(); i++) {
        if (solver->value(i) == l_Undef
            && (solver->varData[i].removed == Removed::none
                || solver->varData[i].removed == Removed::queued_replacer)
        ) {
            poss_choice.push_back(i);
        }
    }

    //Random swap
    for (size_t i = 0
        ; i + 1< poss_choice.size()
        ; i++
    ) {
        std::swap(
            poss_choice[i]
            , poss_choice[i+solver->mtrand.randInt(poss_choice.size()-1-i)]
        );
    }

    return poss_choice;
}

vector<size_t> Prober::create_fast_random_lookup(const vector<Var>& poss_choice)
{
    vector<size_t> lookup(solver->nVars(), std::numeric_limits<size_t>::max());
    for (size_t i = 0; i < poss_choice.size(); i++) {
        lookup[poss_choice[i]] = i;
    }

    return lookup;
}

bool Prober::probe()
{
    assert(solver->decisionLevel() == 0);
    assert(solver->nVars() > 0);
    solver->testAllClauseAttach();

    clean_clauses_before_probe();
    reset_stats_and_state();
    uint64_t numPropsTodo = calc_numpropstodo();

    const double myTime = cpuTime();
    const size_t origTrailSize = solver->trail_size();
    numPropsTodo = update_numpropstodo_based_on_prev_performance(numPropsTodo);

    vector<Var> poss_choice = randomize_possible_choices();
    const vector<size_t> fast_rnd_lookup = create_fast_random_lookup(poss_choice);

    assert(solver->propStats.bogoProps == 0);
    assert(solver->propStats.otfHyperTime == 0);
    for(size_t i = 0
        ; i < poss_choice.size()
        && limit_used() < numPropsTodo
        && cpuTime() <= solver->conf.maxTime
        && !solver->must_interrupt_asap()
        ; i++
    ) {
        extraTime += 20;
        runStats.numLoopIters++;
        const Var var = poss_choice[i];

        //Check if already blacklisted
        if (var == std::numeric_limits<Var>::max())
            continue;

        //Probe 'false' first --> this is not critical
        Lit lit = Lit(var, false);

        //Check if var is set already
        if (solver->value(lit.var()) != l_Undef
            || !solver->varData[lit.var()].is_decision
            || visitedAlready[lit.toInt()]
        ) {
            continue;
        }

        lit = update_lit_for_dominator(lit, poss_choice, fast_rnd_lookup);
        runStats.numVarProbed++;
        extraTime += 20;

        if (!tryThis(lit, true))
            goto end;

        if (solver->value(lit) == l_Undef
            && !tryThis((~lit), false)
        ) {
            goto end;
        }
    }

end:

    //Delete any remaining binaries to add or remove
    //next time, variables will be renumbered/etc. so it will be wrong
    //to add/remove them
    solver->needToAddBinClause.clear();
    solver->uselessBin.clear();

    runStats.zeroDepthAssigns = solver->trail_size() - origTrailSize;
    if (solver->ok && runStats.zeroDepthAssigns) {
        clean_clauses_after_probe();
    }

    update_and_print_stats(myTime, numPropsTodo);
    check_if_must_disable_otf_hyperbin_and_tred(numPropsTodo);
    check_if_must_disable_cache_update();

    solver->testAllClauseAttach();
    return solver->ok;
}

void Prober::update_and_print_stats(const double myTime, const uint64_t numPropsTodo)
{
    for(size_t i = 0; i < visitedAlready.size(); i++) {
        if (visitedAlready[i])
            runStats.numVisited++;
    }
    lastTimeZeroDepthAssings = runStats.zeroDepthAssigns;
    const double time_used = cpuTime() - myTime;
    const bool time_out = (limit_used() < numPropsTodo);
    const double time_remain = ((double)numPropsTodo-(double)limit_used())/numPropsTodo;
    runStats.cpu_time = time_used;
    runStats.propStats = solver->propStats;
    runStats.timeAllocated += numPropsTodo;
    runStats.numCalls = 1;
    globalStats += runStats;

    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "probe"
            , time_used
            , time_out
            , time_remain
        );
    }
}

void Prober::clearUpBeforeFirstSet()
{
    extraTime += propagatedBitSet.size();
    for(size_t varset: propagatedBitSet) {
        propagated[varset] = false;
    }
    propagatedBitSet.clear();
}

void Prober::updateCache(Lit thisLit, Lit lit, size_t numElemsSet)
{
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

        const bool redStep = solver->varData[thisLit.var()].reason.isRedStep();

        //Update the cache now
        assert(ancestor != lit_Undef);
        bool taut = solver->implCache[(~ancestor).toInt()].merge(
            solver->implCache[(~thisLit).toInt()].lits
            , thisLit
            , redStep
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
            if (solver->conf.verbosity >= 10)
                cout << "c Tautology from cache indicated we can enqueue " << (~ancestor) << endl;
        }

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "The impl cache of " << (~ancestor) << " is now: ";
        cout << solver->implCache[(~ancestor).toInt()] << endl;
        #endif
    }
}

void Prober::checkAndSetBothProp(Var var, bool first)
{
    //If this is the first, set what is propagated
    if (first) {
        //Visited this var, needs clear later on
        propagatedBitSet.push_back(var);

        //Set prop has been done
        propagated[var] = true;

        //Set propValue
        if (solver->value(var) == l_True)
            propValue[var] = true;
        else
            propValue[var] = false;
    } else if (propagated[var]) {
        if (propValue[var] == (solver->value(var) == l_True)) {

            //they both imply the same
            const Lit litToEnq = Lit(var, !propValue[var]);
            toEnqueue.push_back(litToEnq);
            (*solver->drup) << litToEnq << fin;

            if (solver->conf.verbosity >= 10)
                cout << "c Bothprop indicated to enqueue " << litToEnq << endl;
        }
    }
}

void Prober::addRestOfLitsToCache(Lit lit)
{
    tmp_lits.clear();
    for (int64_t c = solver->trail_size()-1
        ; c != (int64_t)solver->trail_lim[0] - 1
        ; c--
    ) {
        extraTime += 2;
        const Lit thisLit = solver->trail[c];
        tmp_lits.push_back(thisLit);
    }

    bool taut = solver->implCache[(~lit).toInt()].merge(
        tmp_lits
        , lit_Undef
        , true //Red step -- we don't know, so we assume
        , lit.var()
        , solver->seen
    );

    //If tautology according to cache we can
    //enqueue ~lit at toplevel since both
    //~lit V OTHER, and ~lit V ~OTHER are technically in
    if (taut) {
        toEnqueue.push_back(~lit);
        (*solver->drup) << ~lit << fin;
    }
}

void Prober::handleFailedLit(Lit lit, Lit failed)
{
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
    clearUpBeforeFirstSet();
}

bool Prober::check_timeout_due_to_hyperbin()
{
    //If we timed out on ONE call, turn otf hyper-bin off
    //and return --> the "visitedAlready" will be wrong
    if (solver->timedOutPropagateFull
        && !solver->drup->enabled()
    ) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [probe] intra-propagation timout,"
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
        return true;
    }

    return false;
}

bool Prober::tryThis(const Lit lit, const bool first)
{
    //Clean state if this is the 1st of two
    if (first) {
        clearUpBeforeFirstSet();
    }
    toEnqueue.clear();
    runStats.numProbed++;
    solver->newDecisionLevel();
    solver->enqueue(lit);
    solver->varData[lit.var()].depth = 0;
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

        if (check_timeout_due_to_hyperbin()) {
            return solver->okay();
        }
    } else {
        //No hyper-bin so we use regular propagate and regular analyze

        PropBy confl = solver->propagate();
        if (!confl.isNULL()) {
            uint32_t  glue;
            uint32_t  backtrack_level;
            solver->analyze_conflict(
                confl
                , backtrack_level  //return backtrack level here
                , glue             //return glue here
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
        handleFailedLit(lit, failed);
        return solver->ok;
    } else {
        if (solver->conf.verbosity >= 6)
            cout << "c Did not fail on lit " << lit << endl;
    }

    //Fill bothprop, cache
    assert(solver->decisionLevel() > 0);
    size_t numElemsSet = solver->trail_size() - solver->trail_lim[0];
    for (int64_t c = solver->trail_size()-1
        ; c != (int64_t)solver->trail_lim[0] - 1
        ; c--
    ) {
        extraTime += 2;
        const Lit thisLit = solver->trail[c];
        const Var var = thisLit.var();

        checkAndSetBothProp(var, first);
        visitedAlready[thisLit.toInt()] = 1;
        if (!solver->conf.otfHyperbin)
            continue;
        updateCache(thisLit, lit, numElemsSet);
    }

    if (!solver->conf.otfHyperbin
        && solver->conf.doCache
    ) {
        addRestOfLitsToCache(lit);
    }

    solver->cancelZeroLight();
    runStats.addedBin += solver->hyperBinResAll();
    std::pair<size_t, size_t> tmp = solver->removeUselessBins();
    runStats.removedIrredBin += tmp.first;
    runStats.removedRedBin += tmp.second;

    //Add toEnqueue
    assert(solver->ok);
    runStats.bothSameAdded += toEnqueue.size();
    extraTime += 3*toEnqueue.size();
    return solver->enqueueThese(toEnqueue);
}

size_t Prober::memUsed() const
{
    size_t mem = 0;
    mem += visitedAlready.capacity()*sizeof(char);
    mem += propagatedBitSet.capacity()*sizeof(uint32_t);
    mem += toEnqueue.capacity()*sizeof(Lit);
    mem += tmp_lits.capacity()*sizeof(Lit);
    mem += propagated.capacity()/8;
    mem += propValue.capacity()/8;

    return mem;
}

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
//     uint32_t backupNumUnits = solver->trail_size();
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
//     << " units: " << (solver->trail_size() - backupNumUnits)
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
//         for (int sublevel = solver->trail_size()-1; sublevel > (int)solver->trail_lim[0]; sublevel--) {
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
//     //std::exit(-1);
//
//     return solver->ok;
// }


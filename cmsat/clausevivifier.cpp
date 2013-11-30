/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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

#include "clausevivifier.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"

#include <iomanip>
using namespace CMSat;
using std::cout;
using std::endl;

//#define ASSYM_DEBUG
//#define DEBUG_STAMPING

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

ClauseVivifier::ClauseVivifier(Solver* _solver) :
    solver(_solver)
    , seen(solver->seen)
    , seen_subs(solver->seen2)
    , numCalls(0)
{}

bool ClauseVivifier::vivify(const bool alsoStrengthen)
{
    assert(solver->ok);
    #ifdef VERBOSE_DEBUG
    cout << "c clauseVivifier started" << endl;
    #endif //VERBOSE_DEBUG
    numCalls++;

    solver->clauseCleaner->cleanClauses(solver->longIrredCls);

    if (!vivifyClausesCache(solver->longIrredCls, false, false))
        goto end;

    if (!vivifyClausesCache(solver->longRedCls, true, false))
        goto end;

    if (alsoStrengthen) {
        if (!vivifyClausesCache(solver->longIrredCls, false, true))
            goto end;

        if (!vivifyClausesCache(solver->longRedCls, true, true))
            goto end;
    }

    if (alsoStrengthen
        && !asymmClausesLongIrred()
    ) {
        goto end;
    }

    if (alsoStrengthen
        && !vivifyClausesTriIrred()
    ) {
        goto end;
    }

end:
    //Stats
    globalStats += runStats;
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }
    runStats.clear();

    return solver->ok;
}

bool ClauseVivifier::vivifyClausesTriIrred()
{
    if (solver->conf.verbosity >= 6) {
        cout
        << "c Doing asymm branch for tri irred clauses"
        << endl;
    }

    uint64_t origShorten = runStats.numClShorten;
    uint64_t origLitRem = runStats.numLitsRem;
    double myTime = cpuTime();
    uint64_t maxNumProps = 2LL*1000LL*1000LL;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    size_t origTrailSize = solver->trail.size();

    //Randomize start in the watchlist
    size_t upI;
    upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size()
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps) {
            break;
        }

        Lit lit = Lit::toLit(upI);
        for (size_t i = 0; i < solver->watches[upI].size(); i++) {
            if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps) {
                break;
            }

            Watched ws = solver->watches[upI][i];

            //Only irred TRI and each TRI only once
            if (ws.isTri()
                && !ws.red()
                && lit < ws.lit2()
                && ws.lit2() < ws.lit3()
            ) {
                uselessLits.clear();
                lits.resize(3);
                lits[0] = lit;
                lits[1] = ws.lit2();
                lits[2] = ws.lit3();
                testVivify(
                    CL_OFFSET_MAX
                    , ws.red()
                    , 2
                );

                //We could have modified the watchlist, better exit now
                break;
            }
        }

        if (!solver->okay()) {
            break;
        }
    }

    if (solver->conf.verbosity >= 3) {
        cout
        << "c [vivif] tri "
        << " tri-shorten: " << runStats.numClShorten - origShorten
        << " lit-rem: " << runStats.numLitsRem - origLitRem
        << " 0-depth ass: " << solver->trail.size() - origTrailSize
        << " T: " << std::setprecision(2) << cpuTime() - myTime
        << endl;
    }

    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;

    return solver->ok;
}

struct ClauseSizeSorter
{
    ClauseSizeSorter(const ClauseAllocator& _clAllocator, const bool _invert = false) :
        clAllocator(_clAllocator)
        , invert(_invert)
    {}

    const ClauseAllocator& clAllocator;
    const bool invert;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = clAllocator.getPointer(off1);
        const Clause* cl2 = clAllocator.getPointer(off2);

        if (!invert)
            return cl1->size() > cl2->size();
        else
            return cl1->size() < cl2->size();
    }
};

/**
@brief Performs clause vivification (by Hamadi et al.)
*/
bool ClauseVivifier::asymmClausesLongIrred()
{
    assert(solver->ok);
    if (solver->conf.verbosity >= 6) {
        cout
        << "c Doing asymm branch for long irred clauses"
        << endl;
    }

    double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();

    //Time-limiting
    uint64_t maxNumProps = 20LL*1000LL*1000LL;
    if (solver->litStats.irredLits + solver->litStats.redLits < 500000)
        maxNumProps *=2;

    extraTime = 0;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    bool needToFinish = false;
    runStats.potentialClauses = solver->longIrredCls.size();
    runStats.numCalled = 1;

    std::sort(solver->longIrredCls.begin(), solver->longIrredCls.end(), ClauseSizeSorter(solver->clAllocator));
    uint64_t origLitRem = runStats.numLitsRem;
    uint64_t origClShorten = runStats.numClShorten;

    uint32_t queueByBy = 2;
    if (numCalls > 8
        && (solver->litStats.irredLits + solver->litStats.redLits < 4000000)
        && (solver->longIrredCls.size() < 50000))
        queueByBy = 1;

    vector<ClOffset>::iterator i, j;
    i = j = solver->longIrredCls.begin();
    for (vector<ClOffset>::iterator end = solver->longIrredCls.end()
        ; i != end
        ; i++
    ) {
        //Check if we are in state where we only copy offsets around
        if (needToFinish || !solver->ok) {
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps) {
            if (solver->conf.verbosity >= 3) {
                cout
                << "c Need to finish asymm -- ran out of prop (=allocated time)"
                << endl;
            }
            runStats.timeOut++;
            needToFinish = true;
        }

        //Get pointer
        ClOffset offset = *i;
        Clause& cl = *solver->clAllocator.getPointer(offset);
        //Time to dereference
        extraTime += 5;

        //If we already tried this clause, then move to next
        if (cl.getAsymmed()) {
            *j++ = *i;
            continue;
        } else {
            //Otherwise, this clause has been tried for sure
            cl.setAsymmed(true);
        }

        extraTime += cl.size();
        runStats.checkedClauses++;

        //Sanity check
        assert(cl.size() > 3);
        assert(!cl.red());

        //Copy literals
        uselessLits.clear();
        lits.resize(cl.size());
        std::copy(cl.begin(), cl.end(), lits.begin());

        //Try to vivify clause
        ClOffset offset2 = testVivify(
            offset
            , cl.red()
            , queueByBy
        );

        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    solver->longIrredCls.resize(solver->longIrredCls.size()- (i-j));

    //If went through the whole list, then reset 'asymmed' flag
    //so next time we start from the beginning
    if (!needToFinish) {
        for (vector<ClOffset>::const_iterator
            it = solver->longIrredCls.begin(), end = solver->longIrredCls.end()
            ; it != end
            ; it++
        ) {
            Clause* cl = solver->clAllocator.getPointer(*it);
            cl->setAsymmed(false);
        }
    }

    if (solver->conf.verbosity >= 2) {
        cout << "c [asymm] longirred"
        << " tried: "
        << runStats.checkedClauses << "/" << solver->longIrredCls.size()
        << " cl-rem:"
        << runStats.numClShorten- origClShorten
        << " lits-rem:"
        << runStats.numLitsRem - origLitRem
        << " T: "
        << std::setprecision(2) << cpuTime() - myTime
        << " T-out: " << (needToFinish ? "Y" : "N")
        << endl;
    }

    //Update stats
    runStats.timeNorm = cpuTime() - myTime;
    runStats.zeroDepthAssigns = solver->trail.size() - origTrailSize;

    return solver->ok;
}

ClOffset ClauseVivifier::testVivify(
    ClOffset offset
    , const bool red
    , const uint32_t queueByBy
) {
    #ifdef DRUP_DEBUG
    if (solver->conf.verbosity >= 6) {
        cout
        << "Trying to vivify clause:";
        for(size_t i = 0; i < lits.size(); i++) {
            cout << lits[i] << " ";
        }
        cout << endl;
    }
    #endif

    //Try to enqueue the literals in 'queueByBy' amounts and see if we fail
    bool failed = false;
    uint32_t done = 0;
    solver->newDecisionLevel();
    for (; done < lits.size();) {
        uint32_t i2 = 0;
        for (; (i2 < queueByBy) && ((done+i2) < lits.size()); i2++) {
            lbool val = solver->value(lits[done+i2]);
            if (val == l_Undef) {
                solver->enqueue(~lits[done+i2]);
            } else if (val == l_False) {
                //Record that there is no use for this literal
                uselessLits.push_back(lits[done+i2]);
            }
        }
        done += i2;
        extraTime += 5;
        failed = (!solver->propagate().isNULL());
        if (failed) {
            break;
        }
    }
    solver->cancelZeroLight();
    assert(solver->ok);

    if (uselessLits.size() > 0 || (failed && done < lits.size())) {
        //Stats
        runStats.numClShorten++;
        extraTime += 20;
        const uint32_t origSize = lits.size();

        //Remove useless literals from 'lits'
        lits.resize(done);
        for (uint32_t i2 = 0; i2 < uselessLits.size(); i2++) {
            remove(lits, uselessLits[i2]);
        }

        //Make new clause
        Clause *cl2 = solver->addClauseInt(lits, red);

        //Print results
        if (solver->conf.verbosity >= 5) {
            cout
            << "c Asymm branch effective." << endl;
            if (offset != CL_OFFSET_MAX) {
                cout
                << "c --> orig clause:" <<
                 *solver->clAllocator.getPointer(offset)
                 << endl;
            } else {
                cout
                << "c --> orig clause: TRI/BIN" << endl;
            }
            cout
            << "c --> orig size:" << origSize << endl
            << "c --> new size:" << (cl2 == NULL ? 0 : cl2->size()) << endl
            << "c --> removing lits from end:" << origSize - done << endl
            << "c --> useless lits in middle:" << uselessLits.size()
            << endl;
        }

        //Detach and free old clause
        if (offset != CL_OFFSET_MAX) {
            solver->detachClause(offset);
            solver->clAllocator.clauseFree(offset);
        }

        runStats.numLitsRem += origSize - lits.size();

        if (cl2 != NULL) {
            //The new clause has been asymm-tried
            cl2->setAsymmed(true);

            return solver->clAllocator.getOffset(cl2);
        } else {
            return CL_OFFSET_MAX;
        }
    } else {
        return offset;
    }
}

void ClauseVivifier::strengthen_clause_with_watch(
    const Lit lit
    , const Watched* wit
) {
    //Strengthening w/ bin
    if (wit->isBinary()
        && seen[lit.toInt()] //We haven't yet removed it
    ) {
        if (seen[(~wit->lit2()).toInt()]) {
            thisRemLitBinTri++;
            seen[(~wit->lit2()).toInt()] = 0;
        }
    }

    //Strengthening w/ tri
    if (wit->isTri()
        && seen[lit.toInt()] //We haven't yet removed it
    ) {
        if (seen[(wit->lit2()).toInt()]) {
            if (seen[(~wit->lit3()).toInt()]) {
                thisRemLitBinTri++;
                seen[(~wit->lit3()).toInt()] = 0;
            }
        } else if (seen[wit->lit3().toInt()]) {
            if (seen[(~wit->lit2()).toInt()]) {
                thisRemLitBinTri++;
                seen[(~wit->lit2()).toInt()] = 0;
            }
        }
    }
}

bool ClauseVivifier::subsume_clause_with_watch(
    const Lit lit
    , Watched* wit
    , const Clause& cl
) {
    //Subsumption w/ bin
    if (wit->isBinary() &&
        seen_subs[wit->lit2().toInt()]
    ) {
        //If subsuming irred with redundant, make the redundant into irred
        if (wit->red() && !cl.red()) {
            wit->setRed(false);
            timeAvailable -= solver->watches[wit->lit2().toInt()].size()*3;
            findWatchedOfBin(solver->watches, wit->lit2(), lit, true).setRed(false);
            solver->binTri.redBins--;
            solver->binTri.irredBins++;
        }
        cache_based_data.subBinTri++;
        isSubsumed = true;
        return true;
    }

    //Extension w/ bin
    if (wit->isBinary()
        && !wit->red()
        && !seen_subs[(~(wit->lit2())).toInt()]
    ) {
        seen_subs[(~(wit->lit2())).toInt()] = 1;
        lits2.push_back(~(wit->lit2()));
    }

    if (wit->isTri()) {
        assert(wit->lit2() < wit->lit3());
    }

    //Subsumption w/ tri
    if (wit->isTri()
        && lit < wit->lit2() //Check only one instance of the TRI clause
        && seen_subs[wit->lit2().toInt()]
        && seen_subs[wit->lit3().toInt()]
    ) {
        //If subsuming irred with redundant, make the redundant into irred
        if (!cl.red() && wit->red()) {
            wit->setRed(false);
            timeAvailable -= solver->watches[wit->lit2().toInt()].size()*3;
            timeAvailable -= solver->watches[wit->lit3().toInt()].size()*3;
            findWatchedOfTri(solver->watches, wit->lit2(), lit, wit->lit3(), true).setRed(false);
            findWatchedOfTri(solver->watches, wit->lit3(), lit, wit->lit2(), true).setRed(false);
            solver->binTri.redTris--;
            solver->binTri.irredTris++;
        }
        cache_based_data.subBinTri++;
        isSubsumed = true;
        return true;
    }

    //Extension w/ tri (1)
    if (wit->isTri()
        && lit < wit->lit2() //Check only one instance of the TRI clause
        && !wit->red()
        && seen_subs[wit->lit2().toInt()]
        && !seen_subs[(~(wit->lit3())).toInt()]
    ) {
        seen_subs[(~(wit->lit3())).toInt()] = 1;
        lits2.push_back(~(wit->lit3()));
    }

    //Extension w/ tri (2)
    if (wit->isTri()
        && lit < wit->lit2() //Check only one instance of the TRI clause
        && !wit->red()
        && !seen_subs[(~(wit->lit2())).toInt()]
        && seen_subs[wit->lit3().toInt()]
    ) {
        seen_subs[(~(wit->lit2())).toInt()] = 1;
        lits2.push_back(~(wit->lit2()));
    }

    return false;
}

bool ClauseVivifier::strenghten_clause_with_cache(const Lit lit)
{
    timeAvailable -= 2*solver->implCache[lit.toInt()].lits.size();
    for (const LitExtra elit: solver->implCache[lit.toInt()].lits) {
         if (seen[(~(elit.getLit())).toInt()]) {
            seen[(~(elit.getLit())).toInt()] = 0;
            thisRemLitCache++;
         }

         if (seen_subs[elit.getLit().toInt()]
             && elit.getOnlyIrredBin()
         ) {
             isSubsumed = true;
             cache_based_data.subCache++;
             return true;
         }
     }

     return false;
}

void ClauseVivifier::vivify_clause_with_lit(
    Clause& cl
    , const Lit lit
    , const bool alsoStrengthen
) {
    //Use cache
    if (alsoStrengthen
        && solver->conf.doCache
        && seen[lit.toInt()] //We haven't yet removed this literal from the clause
     ) {
         const bool subsumed = strenghten_clause_with_cache(lit);
         if (subsumed)
             return;
     }

    //Go through the watchlist
    watch_subarray thisW = solver->watches[lit.toInt()];
    timeAvailable -= thisW.size()*2 + 5;
    for(watch_subarray::iterator
        wit = thisW.begin(), wend = thisW.end()
        ; wit != wend
        ; wit++
    ) {
        //Can't do anything with a clause
        if (wit->isClause())
            continue;

        timeAvailable -= 5;

        if (alsoStrengthen) {
            strengthen_clause_with_watch(lit, wit);
        }

        const bool subsumed = subsume_clause_with_watch(lit, wit, cl);
        if (subsumed)
            return;
    }
}

void ClauseVivifier::try_subsuming_by_stamping(const bool red)
{
    if (solver->conf.doStamp
        && !isSubsumed
        && !red
    ) {
        timeAvailable -= lits2.size()*3 + 10;
        if (solver->stamp.stampBasedClRem(lits2)) {
            isSubsumed = true;
            cache_based_data.subsumedStamp++;
        }
    }
}

void ClauseVivifier::remove_lits_through_stamping_red()
{
    if (lits.size() > 1) {
        timeAvailable -= lits.size()*3 + 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        cache_based_data.remLitTimeStampTotal += tmp.first;
        cache_based_data.remLitTimeStampTotalInv += tmp.second;
    }
}

void ClauseVivifier::remove_lits_through_stamping_irred()
{
    if (lits.size() > 1) {
        timeAvailable -= lits.size()*3 + 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
        cache_based_data.remLitTimeStampTotal += tmp.first;
        cache_based_data.remLitTimeStampTotalInv += tmp.second;
    }
}

bool ClauseVivifier::vivify_clause(
    ClOffset& offset
    , bool red
    , const bool alsoStrengthen
) {
    Clause& cl = *solver->clAllocator.getPointer(offset);
    assert(cl.size() > 3);

    timeAvailable -= cl.size()*2;
    tmpStats.totalLits += cl.size();
    tmpStats.triedCls++;
    isSubsumed = false;
    thisRemLitCache = 0;
    thisRemLitBinTri = 0;

    //Fill 'seen'
    lits2.clear();
    for (const Lit lit: cl) {
        seen[lit.toInt()] = 1;
        seen_subs[lit.toInt()] = 1;
        lits2.push_back(lit);
    }

    //Go through each literal and subsume/strengthen with it
    for (const Lit
        *lit = cl.begin(), *end = cl.end()
        ; lit != end && !isSubsumed
        ; lit++
    ) {
        vivify_clause_with_lit(cl, *lit, alsoStrengthen);
    }
    assert(lits2.size() > 1);

    try_subsuming_by_stamping(red);

    //Clear 'seen_subs'
    timeAvailable -= lits2.size()*3;
    for (const Lit lit: lits2) {
        seen_subs[lit.toInt()] = 0;
    }

    //Clear 'seen' and fill new clause data
    lits.clear();
    timeAvailable -= cl.size()*3;
    for (const Lit lit: cl) {
        if (!isSubsumed
            && seen[lit.toInt()]
        ) {
            lits.push_back(lit);
        }
        seen[lit.toInt()] = 0;
    }

    if (isSubsumed)
        return true;

    if (alsoStrengthen
        && solver->conf.doStamp
    ) {
        remove_lits_through_stamping_red();
        remove_lits_through_stamping_irred();
    }

    //Nothing to do
    if (lits.size() == cl.size()) {
        return false;
    }

    //Remove or shrink clause
    timeAvailable -= cl.size()*10;
    cache_based_data.remLitCache += thisRemLitCache;
    cache_based_data.remLitBinTri += thisRemLitBinTri;
    tmpStats.shrinked++;
    timeAvailable -= lits.size()*2 + 50;
    Clause* c2 = solver->addClauseInt(lits, cl.red(), cl.stats);
    if (!solver->ok) {
        needToFinish = true;
    }

    if (c2 != NULL) {
        solver->detachClause(offset);
        solver->clAllocator.clauseFree(offset);
        offset = solver->clAllocator.getOffset(c2);
        return false;
    }

    //Implicit clause or non-existent after addition, remove
    return true;
}

void ClauseVivifier::randomise_order_of_clauses(
    vector<ClOffset>& clauses
) {
    if (!clauses.empty()) {
        timeAvailable -= clauses.size()*2;
        for(size_t i = 0; i < clauses.size()-1; i++) {
            std::swap(
                clauses[i]
                , clauses[i + solver->mtrand.randInt(clauses.size()-i-1)]
            );
        }
    }
}

uint64_t ClauseVivifier::calc_time_available(
    const bool alsoStrengthen
    , const bool red
) const {
    //If it hasn't been to successful until now, don't do it so much
    const Stats::CacheBased* stats = NULL;
    if (red) {
        stats = &(globalStats.redCacheBased);
    } else {
        stats = &(globalStats.irredCacheBased);
    }

    uint64_t maxCountTime = 700ULL*1000ULL*1000ULL;
    if (!alsoStrengthen) {
        maxCountTime *= 4;
    }
    if (stats->numCalled > 2
        && (double)stats->numClSubsumed/(double)stats->triedCls < 0.05
        && (double)stats->numLitsRem/(double)stats->totalLits < 0.05
    ) {
        maxCountTime /= 2;
    }

    return maxCountTime;
}

bool ClauseVivifier::vivifyClausesCache(
    vector<ClOffset>& clauses
    , bool red
    , bool alsoStrengthen
) {
    assert(solver->ok);

    //Stats
    double myTime = cpuTime();

    timeAvailable = calc_time_available(alsoStrengthen, red);
    tmpStats = Stats::CacheBased();
    tmpStats.totalCls = clauses.size();
    tmpStats.numCalled = 1;
    cache_based_data.clear();
    needToFinish = false;
    randomise_order_of_clauses(clauses);

    size_t i = 0;
    size_t j = i;
    const size_t end = clauses.size();
    for (
        ; i < end
        ; i++
    ) {
        //Timeout?
        if (timeAvailable < 0) {
            needToFinish = true;
            tmpStats.ranOutOfTime++;
        }

        //Check status
        if (needToFinish) {
            clauses[j++] = clauses[i];
            continue;
        }

        ClOffset offset = clauses[i];
        const bool remove = vivify_clause(offset, red, alsoStrengthen);
        if (remove) {
            solver->detachClause(offset);
            solver->clAllocator.clauseFree(offset);
        } else {
            clauses[j++] = offset;
        }
    }
    clauses.resize(clauses.size() - (i-j));
    #ifdef DEBUG_IMPLICIT_STATS
    solver->checkImplicitStats();
    #endif

    //Set stats
    tmpStats.numClSubsumed += cache_based_data.get_cl_subsumed();
    tmpStats.numLitsRem += cache_based_data.get_lits_rem();
    tmpStats.cpu_time = cpuTime() - myTime;
    if (red) {
        runStats.redCacheBased = tmpStats;
    } else {
        runStats.irredCacheBased = tmpStats;
    }

    if (solver->conf.verbosity >= 2) {
        cache_based_data.print();
    }

    return solver->ok;
}

void ClauseVivifier::strengthen_bin_with_bin(
    const Lit lit
    , Watched*& i
    , Watched*& j
    , const Watched* end
) {
    lits.clear();
    lits.push_back(lit);
    lits.push_back(i->lit2());
    if (solver->conf.doStamp) {
        timeAvailable -= 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        str_impl_data.stampRem += tmp.first;
        str_impl_data.stampRem += tmp.second;
        assert(!lits.empty());
        if (lits.size() == 1) {
            str_impl_data.toEnqueue.push_back(lits[0]);
            (*solver->drup) << lits[0] << fin;

            str_impl_data.remLitFromBin++;
            str_impl_data.stampRem++;
            *j++ = *i;
            return;
        }
    }

    //If inverted, then the inverse will never be found, because
    //watches are sorted
    if (i->lit2().sign()) {
        *j++ = *i;
        return;
    }

    //Try to look for a binary in this same watchlist
    //that has ~i->lit2() inside. Everything is sorted, so we are
    //lucky, this is speedy
    bool rem = false;
    watch_subarray::const_iterator i2 = i;
    while(i2 != end
        && (i2->isBinary() || i2->isTri())
        && i->lit2().var() == i2->lit2().var()
    ) {
        timeAvailable -= 2;
        //Yay, we have found what we needed!
        if (i2->isBinary() && i2->lit2() == ~i->lit2()) {
            rem = true;
            break;
        }

        i2++;
    }

    //Enqeue literal
    if (rem) {
        str_impl_data.remLitFromBin++;
        str_impl_data.toEnqueue.push_back(lit);
        (*solver->drup) << lit << fin;
    }
    *j++ = *i;
}

void ClauseVivifier::strengthen_tri_with_bin_tri_stamp(
    const Lit lit
    , Watched*& i
    , Watched*& j
) {
    const Lit lit1 = i->lit2();
    const Lit lit2 = i->lit3();
    bool rem = false;

    timeAvailable -= solver->watches[(~lit).toInt()].size();
    for(watch_subarray::const_iterator
        it2 = solver->watches[(~lit).toInt()].begin(), end2 = solver->watches[(~lit).toInt()].end()
        ; it2 != end2 && timeAvailable > 0
        ; it2++
    ) {
        if (it2->isBinary()
            && (it2->lit2() == lit1 || it2->lit2() == lit2)
        ) {
            rem = true;
            str_impl_data.remLitFromTriByBin++;
            break;
        }

        if (it2->isTri()
            && (
                (it2->lit2() == lit1 && it2->lit3() == lit2)
                ||
                (it2->lit2() == lit2 && it2->lit3() == lit1)
            )

        ) {
            rem = true;
            str_impl_data.remLitFromTriByTri++;
            break;
        }

        //watches are sorted, so early-abort
        if (it2->isClause())
            break;
    }

    if (rem) {
        solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
        str_impl_data.remLitFromTri++;
        str_impl_data.binsToAdd.push_back(BinaryClause(i->lit2(), i->lit3(), i->red()));

        (*solver->drup)
        << i->lit2()  << i->lit3() << fin
        << del << lit << i->lit2() << i->lit3() << fin;
        return;
    }

    if (solver->conf.doStamp) {
        //Strengthen TRI using stamps
        lits.clear();
        lits.push_back(lit);
        lits.push_back(i->lit2());
        lits.push_back(i->lit3());

        //Try both stamp types to reduce size
        timeAvailable -= 15;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        str_impl_data.stampRem += tmp.first;
        str_impl_data.stampRem += tmp.second;
        if (lits.size() > 1) {
            timeAvailable -= 15;
            std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
            str_impl_data.stampRem += tmp.first;
            str_impl_data.stampRem += tmp.second;
        }

        if (lits.size() == 2) {
            solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
            str_impl_data.remLitFromTri++;
            str_impl_data.binsToAdd.push_back(BinaryClause(lits[0], lits[1], i->red()));

            //Drup
            (*solver->drup)
            << lits[0] << lits[1] << fin
            << del << lit << i->lit2() << i->lit3() << fin;

            return;
        } else if (lits.size() == 1) {
            solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
            str_impl_data.remLitFromTri+=2;
            str_impl_data.toEnqueue.push_back(lits[0]);
            (*solver->drup)
            << lits[0] << fin
            << del << lit << i->lit2() << i->lit3() << fin;

            return;
        }
    }


    //Nothing to do, copy
    *j++ = *i;
}

void ClauseVivifier::strengthen_implicit_lit(const Lit lit)
{
    watch_subarray ws = solver->watches[lit.toInt()];

    Watched* i = ws.begin();
    Watched* j = i;
    for (const Watched* end = ws.end()
        ; i != end
        ; i++
    ) {
        timeAvailable -= 2;
        if (timeAvailable < 0) {
            *j++ = *i;
            continue;
        }

        switch(i->getType()) {
            case CMSat::watch_clause_t:
                *j++ = *i;
                break;

            case CMSat::watch_binary_t:
                timeAvailable -= 20;
                strengthen_bin_with_bin(lit, i, j, end);
                break;

            case CMSat::watch_tertiary_t:
                timeAvailable -= 20;
                strengthen_tri_with_bin_tri_stamp(lit, i, j);
                break;

            default:
                assert(false);
                break;
        }
    }
    ws.shrink(i-j);
}

bool ClauseVivifier::strengthenImplicit()
{
    str_impl_data.clear();

    const size_t origTrailSize = solver->trail.size();
    timeAvailable = 1000LL*1000LL*1000LL;
    double myTime = cpuTime();

    //Cannot handle empty
    if (solver->watches.size() == 0)
        return solver->okay();

    //Randomize starting point
    size_t upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size() && timeAvailable > 0
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        str_impl_data.numWatchesLooked++;
        const Lit lit = Lit::toLit(upI);
        strengthen_implicit_lit(lit);
    }

    //Enqueue delayed values
    if (!solver->enqueueThese(str_impl_data.toEnqueue))
        goto end;

    //Add delayed binary clauses
    for(const BinaryClause& bin: str_impl_data.binsToAdd) {
        lits.clear();
        lits.push_back(bin.getLit1());
        lits.push_back(bin.getLit2());
        timeAvailable -= 5;
        solver->addClauseInt(lits, bin.isRed());
        if (!solver->okay())
            goto end;
    }

end:

    solver->checkStats();
    if (solver->conf.verbosity >= 1) {
        str_impl_data.print(
            solver->trail.size() - origTrailSize
            , myTime
            , timeAvailable
        );
    }

    //Update stats
    solver->solveStats.subsBinWithBinTime += cpuTime() - myTime;

    return solver->okay();
}

void ClauseVivifier::StrImplicitData::print(
    const size_t trail_diff
    , const double myTime
    , const int64_t timeAvailable
) const {
    cout
    << "c [implicit] str"
    << " lit bin: " << remLitFromBin
    << " lit tri: " << remLitFromTri
    << " (by tri: " << remLitFromTriByTri << ")"
    << " (by stamp: " << stampRem << ")"
    << " set-var: " << trail_diff

    << " T: " << std::fixed << std::setprecision(2)
    << (cpuTime() - myTime)
    << " T-out: " << (timeAvailable < 0 ? "Y" : "N")
    << " w-visit: " << numWatchesLooked
    << endl;
}

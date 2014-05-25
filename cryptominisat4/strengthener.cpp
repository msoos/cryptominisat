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

#include "strengthener.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "sqlstats.h"

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

Strengthener::Strengthener(Solver* _solver) :
    solver(_solver)
    , seen(solver->seen)
    , seen_subs(solver->seen2)
    , numCalls(0)
{}

bool Strengthener::strengthen(const bool alsoStrengthen)
{
    assert(solver->ok);
    numCalls++;

    solver->clauseCleaner->cleanClauses(solver->longIrredCls);

    runStats.redCacheBased.clear();
    runStats.irredCacheBased.clear();

    if (!shorten_all_clauses_with_cache_watch_stamp(solver->longIrredCls, false, false))
        goto end;

    if (!shorten_all_clauses_with_cache_watch_stamp(solver->longRedCls, true, false))
        goto end;

    if (alsoStrengthen) {
        if (!shorten_all_clauses_with_cache_watch_stamp(solver->longIrredCls, false, true))
            goto end;

        if (!shorten_all_clauses_with_cache_watch_stamp(solver->longRedCls, true, true))
            goto end;
    }

end:
    globalStats += runStats;
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print();
        else
            runStats.printShort();
    }
    runStats.clear();

    return solver->ok;
}

void Strengthener::strengthen_clause_with_watch(
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

bool Strengthener::subsume_clause_with_watch(
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
            timeAvailable -= (long)solver->watches[wit->lit2().toInt()].size()*3;
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
            timeAvailable -= (long)solver->watches[wit->lit2().toInt()].size()*3;
            timeAvailable -= (long)solver->watches[wit->lit3().toInt()].size()*3;
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

bool Strengthener::str_and_sub_clause_with_cache(const Lit lit)
{
    if (solver->conf.doCache
        && seen[lit.toInt()] //We haven't yet removed this literal from the clause
     ) {
        timeAvailable -= 2*(long)solver->implCache[lit.toInt()].lits.size();
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

     return false;
}

void Strengthener::str_and_sub_using_watch(
    Clause& cl
    , const Lit lit
    , const bool alsoStrengthen
) {
    //Go through the watchlist
    watch_subarray thisW = solver->watches[lit.toInt()];
    timeAvailable -= (long)thisW.size()*2 + 5;
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

void Strengthener::try_subsuming_by_stamping(const bool red)
{
    if (solver->conf.doStamp
        && solver->conf.otfHyperbin
        && !isSubsumed
        && !red
    ) {
        timeAvailable -= (long)lits2.size()*3 + 10;
        if (solver->stamp.stampBasedClRem(lits2)) {
            isSubsumed = true;
            cache_based_data.subsumedStamp++;
        }
    }
}

void Strengthener::remove_lits_through_stamping_red()
{
    if (lits.size() > 1) {
        timeAvailable -= (long)lits.size()*3 + 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        cache_based_data.remLitTimeStampTotal += tmp.first;
        cache_based_data.remLitTimeStampTotalInv += tmp.second;
    }
}

void Strengthener::remove_lits_through_stamping_irred()
{
    if (lits.size() > 1) {
        timeAvailable -= (long)lits.size()*3 + 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
        cache_based_data.remLitTimeStampTotal += tmp.first;
        cache_based_data.remLitTimeStampTotalInv += tmp.second;
    }
}

bool Strengthener::shorten_clause_with_cache_watch_stamp(
    ClOffset& offset
    , bool red
    , const bool alsoStrengthen
) {
    Clause& cl = *solver->clAllocator.getPointer(offset);
    assert(cl.size() > 3);

    timeAvailable -= (long)cl.size()*2;
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
        if (alsoStrengthen) {
            bool subsumed = str_and_sub_clause_with_cache(*lit);
            if (subsumed)
                break;
        }

        str_and_sub_using_watch(cl, *lit, alsoStrengthen);
    }
    assert(lits2.size() > 1);

    try_subsuming_by_stamping(red);

    //Clear 'seen_subs'
    timeAvailable -= (long)lits2.size()*3;
    for (const Lit lit: lits2) {
        seen_subs[lit.toInt()] = 0;
    }

    //Clear 'seen' and fill new clause data
    lits.clear();
    timeAvailable -= (long)cl.size()*3;
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
    timeAvailable -= (long)cl.size()*10;
    cache_based_data.remLitCache += thisRemLitCache;
    cache_based_data.remLitBinTri += thisRemLitBinTri;
    tmpStats.shrinked++;
    timeAvailable -= (long)lits.size()*2 + 50;
    Clause* c2 = solver->addClauseInt(lits, cl.red(), cl.stats);
    if (c2 != NULL) {
        solver->detachClause(offset);
        solver->clAllocator.clauseFree(offset);
        offset = solver->clAllocator.getOffset(c2);
        return false;
    }

    //Implicit clause or non-existent after addition, remove
    return true;
}

void Strengthener::randomise_order_of_clauses(
    vector<ClOffset>& clauses
) {
    if (clauses.empty())
        return;

    timeAvailable -= (long)clauses.size()*2;
    for(size_t i = 0; i < clauses.size()-1; i++) {
        std::swap(
            clauses[i]
            , clauses[i + solver->mtrand.randInt(clauses.size()-i-1)]
        );
    }
}

uint64_t Strengthener::calc_time_available(
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

    uint64_t maxCountTime = solver->conf.watch_cache_stamp_based_str_timeoutM*1000LL*1000LL;
    if (!alsoStrengthen) {
        maxCountTime *= 2;
    }
    if (stats->numCalled > 2
        && (double)stats->numClSubsumed/(double)stats->triedCls < 0.05
        && (double)stats->numLitsRem/(double)stats->totalLits < 0.05
    ) {
        maxCountTime *= 0.5;
    }

    return maxCountTime;
}

bool Strengthener::shorten_all_clauses_with_cache_watch_stamp(
    vector<ClOffset>& clauses
    , bool red
    , bool alsoStrengthen
) {
    assert(solver->ok);

    //Stats
    double myTime = cpuTime();

    const int64_t orig_time_available = calc_time_available(alsoStrengthen, red);
    timeAvailable = orig_time_available;
    tmpStats = Stats::CacheBased();
    tmpStats.totalCls = clauses.size();
    tmpStats.numCalled = 1;
    cache_based_data.clear();
    bool need_to_finish = false;
    randomise_order_of_clauses(clauses);

    size_t i = 0;
    size_t j = i;
    const size_t end = clauses.size();
    for (
        ; i < end
        ; i++
    ) {
        //Timeout?
        if (timeAvailable <= 0
            || !solver->okay()
        ) {
            need_to_finish = true;
            tmpStats.ranOutOfTime++;
        }

        //Check status
        if (need_to_finish) {
            clauses[j++] = clauses[i];
            continue;
        }

        ClOffset offset = clauses[i];
        const bool remove = shorten_clause_with_cache_watch_stamp(offset, red, alsoStrengthen);
        if (remove) {
            solver->detachClause(offset);
            solver->clAllocator.clauseFree(offset);
        } else {
            clauses[j++] = offset;
        }
    }
    clauses.resize(clauses.size() - (i-j));
    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif

    //Set stats
    const double time_used = cpuTime() - myTime;
    const bool time_out = timeAvailable < 0;
    const double time_remain = (double)timeAvailable/(double)orig_time_available;
    tmpStats.numClSubsumed += cache_based_data.get_cl_subsumed();
    tmpStats.numLitsRem += cache_based_data.get_lits_rem();
    tmpStats.cpu_time = time_used;
    if (red) {
        runStats.redCacheBased += tmpStats;
    } else {
        runStats.irredCacheBased += tmpStats;
    }
    if (solver->conf.verbosity >= 2) {
        cache_based_data.print();
    }
    if (solver->conf.doSQL) {
        std::stringstream ss;
        ss << "shorten"
        << (alsoStrengthen ? " and str" : "")
        << (red ? " red" : " irred")
        <<  " cls with cache and stamp"
        ;
        solver->sqlStats->time_passed(
            solver
            , ss.str()
            , time_used
            , time_out
            , time_remain
        );
    }

    return solver->ok;
}

void Strengthener::strengthen_bin_with_bin(
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

void Strengthener::strengthen_tri_with_bin_tri_stamp(
    const Lit lit
    , Watched*& i
    , Watched*& j
) {
    const Lit lit1 = i->lit2();
    const Lit lit2 = i->lit3();
    bool rem = false;

    timeAvailable -= (long)solver->watches[(~lit).toInt()].size();
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
            std::pair<size_t, size_t> tmp2 = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
            str_impl_data.stampRem += tmp2.first;
            str_impl_data.stampRem += tmp2.second;
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

void Strengthener::strengthen_implicit_lit(const Lit lit)
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

bool Strengthener::strengthenImplicit()
{
    str_impl_data.clear();

    const size_t origTrailSize = solver->trail_size();
    timeAvailable = 1000LL*1000LL*1000LL;
    const int64_t orig_time = timeAvailable;
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

    if (solver->conf.verbosity >= 1) {
        str_impl_data.print(
            solver->trail_size() - origTrailSize
            , cpuTime() - myTime
            , timeAvailable
            , orig_time
            , solver
        );
    }
    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_stats();
    #endif

    return solver->okay();
}

void Strengthener::StrImplicitData::print(
    const size_t trail_diff
    , const double time_used
    , const int64_t timeAvailable
    , const int64_t orig_time
    , Solver* solver
) const {
    bool time_out = timeAvailable <= 0;
    const double time_remain = (double)timeAvailable/(double)orig_time;

    cout
    << "c [implicit] str"
    << " lit bin: " << remLitFromBin
    << " lit tri: " << remLitFromTri
    << " (by tri: " << remLitFromTriByTri << ")"
    << " (by stamp: " << stampRem << ")"
    << " set-var: " << trail_diff

    << " T: " << std::fixed << std::setprecision(2) << time_used
    << " T-out: " << (time_out ? "Y" : "N")
    << " T-r: " << time_remain * 100.0
    << " w-visit: " << numWatchesLooked
    << endl;

    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "implicit str"
            , time_used
            , time_out
            , time_remain
        );
    }
}

Strengthener::Stats& Strengthener::Stats::operator+=(const Stats& other)
{
    irredCacheBased += other.irredCacheBased;
    redCacheBased += other.redCacheBased;
    return *this;
}

void Strengthener::Stats::printShort() const
{
    irredCacheBased.printShort("irred");
    redCacheBased.printShort("red");
}

void Strengthener::Stats::print() const
{
    cout << "c -------- STRENGTHEN STATS --------" << endl;
    cout << "c --> cache-based on irred cls" << endl;
    irredCacheBased.print();

    cout << "c --> cache-based on red cls" << endl;
    redCacheBased.print();
    cout << "c -------- STRENGTHEN STATS END --------" << endl;
}

/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#include "distillerwithimplicit.h"
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

//#define DEBUG_STAMPING

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

DistillWithImplicit::DistillWithImplicit(Solver* _solver) :
    solver(_solver)
    , seen(solver->seen)
    , seen_subs(solver->seen2)
    , numCalls(0)
{}

bool DistillWithImplicit::distill_long_with_implicit(const bool alsoStrengthen)
{
    assert(solver->ok);
    numCalls++;

    solver->clauseCleaner->clean_clauses(solver->longIrredCls);

    runStats.redCacheBased.clear();
    runStats.irredCacheBased.clear();

    if (!shorten_all_cl_with_cache_watch_stamp(solver->longIrredCls, false, false))
        goto end;

    if (!shorten_all_cl_with_cache_watch_stamp(solver->longRedCls, true, false))
        goto end;

    solver->clauseCleaner->remove_and_clean_all();
    if (alsoStrengthen) {
        if (!shorten_all_cl_with_cache_watch_stamp(solver->longIrredCls, false, true))
            goto end;

        if (!shorten_all_cl_with_cache_watch_stamp(solver->longRedCls, true, true))
            goto end;

        solver->clauseCleaner->remove_and_clean_all();
    }

end:
    globalStats += runStats;
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print();
        else
            runStats.print_short(solver);
    }
    runStats.clear();

    return solver->ok;
}

void DistillWithImplicit::strengthen_clause_with_watch(
    const Lit lit
    , const Watched* wit
) {
    //Strengthening w/ bin
    if (wit->isBin()
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

bool DistillWithImplicit::subsume_clause_with_watch(
    const Lit lit
    , Watched* wit
    , const Clause& cl
) {
    //Subsumption w/ bin
    if (wit->isBin() &&
        seen_subs[wit->lit2().toInt()]
    ) {
        //If subsuming irred with redundant, make the redundant into irred
        if (wit->red() && !cl.red()) {
            wit->setRed(false);
            timeAvailable -= (long)solver->watches[wit->lit2()].size()*3;
            findWatchedOfBin(solver->watches, wit->lit2(), lit, true).setRed(false);
            solver->binTri.redBins--;
            solver->binTri.irredBins++;
        }
        cache_based_data.subBinTri++;
        isSubsumed = true;
        return true;
    }

    //Extension w/ bin
    if (wit->isBin()
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
            timeAvailable -= (long)solver->watches[wit->lit2()].size()*3;
            timeAvailable -= (long)solver->watches[wit->lit3()].size()*3;
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

bool DistillWithImplicit::str_and_sub_clause_with_cache(const Lit lit, const bool alsoStrengthen)
{
    if (solver->conf.doCache
        && seen[lit.toInt()] //We haven't yet removed this literal from the clause
     ) {
        timeAvailable -= (1+(int)alsoStrengthen)*(long)solver->implCache[lit].lits.size();
        for (const LitExtra elit: solver->implCache[lit].lits) {
             if (alsoStrengthen
                && seen[(~(elit.getLit())).toInt()]
            ) {
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

void DistillWithImplicit::str_and_sub_using_watch(
    Clause& cl
    , const Lit lit
    , const bool alsoStrengthen
) {
    //Go through the watchlist
    watch_subarray thisW = solver->watches[lit];
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

void DistillWithImplicit::try_subsuming_by_stamping(const bool red)
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

void DistillWithImplicit::remove_lits_through_stamping_red()
{
    if (lits.size() > 1) {
        timeAvailable -= (long)lits.size()*3 + 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_RED);
        cache_based_data.remLitTimeStampTotal += tmp.first;
        cache_based_data.remLitTimeStampTotalInv += tmp.second;
    }
}

void DistillWithImplicit::remove_lits_through_stamping_irred()
{
    if (lits.size() > 1) {
        timeAvailable -= (long)lits.size()*3 + 10;
        std::pair<size_t, size_t> tmp = solver->stamp.stampBasedLitRem(lits, STAMP_IRRED);
        cache_based_data.remLitTimeStampTotal += tmp.first;
        cache_based_data.remLitTimeStampTotalInv += tmp.second;
    }
}

void DistillWithImplicit::strsub_with_cache_and_watch(
    bool alsoStrengthen
    , Clause& cl
) {
    //Go through each literal and subsume/strengthen with it
    Lit *lit2 = cl.begin();
    lit2++;
    for (const Lit
        *lit = cl.begin(), *end = cl.end()
        ; lit != end && !isSubsumed
        ; lit++, lit2++
    ) {
        if (lit2 < end) {
            solver->watches.prefetch(lit2->toInt());
        }

        bool subsumed = str_and_sub_clause_with_cache(*lit, alsoStrengthen);
        if (subsumed)
            break;

        str_and_sub_using_watch(cl, *lit, alsoStrengthen);
    }
    assert(lits2.size() > 1);
}

bool DistillWithImplicit::sub_str_cl_with_cache_watch_stamp(
    ClOffset& offset
    , bool red
    , const bool alsoStrengthen
) {
    Clause& cl = *solver->cl_alloc.ptr(offset);
    assert(cl.size() > 3);

    if (solver->conf.verbosity >= 10) {
        cout << "Examining str clause:" << cl << endl;
    }

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

    strsub_with_cache_and_watch(alsoStrengthen, cl);
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

    return remove_or_shrink_clause(cl, offset);
}

bool DistillWithImplicit::remove_or_shrink_clause(Clause& cl, ClOffset& offset)
{
    //Remove or shrink clause
    timeAvailable -= (long)cl.size()*10;
    cache_based_data.remLitCache += thisRemLitCache;
    cache_based_data.remLitBinTri += thisRemLitBinTri;
    tmpStats.shrinked++;
    timeAvailable -= (long)lits.size()*2 + 50;
    Clause* c2 = solver->add_clause_int(lits, cl.red(), cl.stats);
    if (c2 != NULL) {
        solver->detachClause(offset);
        solver->cl_alloc.clauseFree(offset);
        offset = solver->cl_alloc.get_offset(c2);
        return false;
    }

    //Implicit clause or non-existent after addition, remove
    return true;
}

void DistillWithImplicit::randomise_order_of_clauses(
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

uint64_t DistillWithImplicit::calc_time_available(
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

    uint64_t maxCountTime =
        solver->conf.watch_cache_stamp_based_str_time_limitM*1000LL*1000LL
        *solver->conf.global_timeout_multiplier;
    if (!alsoStrengthen) {
        maxCountTime *= 2;
    }
    if (stats->numCalled > 2
        && stats->triedCls > 0 //avoid division by zero
        && stats->totalLits > 0 //avoid division by zero
        && float_div(stats->numClSubsumed, stats->triedCls) < 0.05
        && float_div(stats->numLitsRem, stats->totalLits) < 0.05
    ) {
        maxCountTime *= 0.5;
    }

    return maxCountTime;
}

bool DistillWithImplicit::shorten_all_cl_with_cache_watch_stamp(
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
        const bool remove = sub_str_cl_with_cache_watch_stamp(offset, red, alsoStrengthen);
        if (remove) {
            solver->detachClause(offset);
            solver->cl_alloc.clauseFree(offset);
        } else {
            clauses[j++] = offset;
        }
    }
    clauses.resize(clauses.size() - (i-j));
    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif

    dump_stats_for_shorten_all_cl_with_cache_stamp(red
        , alsoStrengthen
        , myTime
        , orig_time_available
    );

    return solver->ok;
}

void DistillWithImplicit::dump_stats_for_shorten_all_cl_with_cache_stamp(
    bool red
    , bool alsoStrengthen
    , double myTime
    , double orig_time_available
) {
    //Set stats
    const double time_used = cpuTime() - myTime;
    const bool time_out = timeAvailable < 0;
    const double time_remain = float_div(timeAvailable, orig_time_available);
    tmpStats.numClSubsumed += cache_based_data.get_cl_subsumed();
    tmpStats.numLitsRem += cache_based_data.get_lits_rem();
    tmpStats.cpu_time = time_used;
    if (red) {
        runStats.redCacheBased += tmpStats;
    } else {
        runStats.irredCacheBased += tmpStats;
    }
    if (solver->conf.verbosity >= 3) {
        if (solver->conf.verbosity >= 10) {
            cout << "red:" << red << " alsostrenghten:" << alsoStrengthen << endl;
        }
        cache_based_data.print();

        cout << "c [distill-with-bin-ext]"
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
    if (solver->sqlStats) {
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
}

void DistillWithImplicit::CacheBasedData::clear()
{
    CacheBasedData tmp;
    *this = tmp;
}

size_t DistillWithImplicit::CacheBasedData::get_cl_subsumed() const
{
    return subBinTri + subsumedStamp + subCache;
}

size_t DistillWithImplicit::CacheBasedData::get_lits_rem() const
{
    return remLitBinTri + remLitCache
        + remLitTimeStampTotal + remLitTimeStampTotalInv;
}

void DistillWithImplicit::CacheBasedData::print() const
{
    cout
    << "c [distill-with-bin-ext] stamp-based"
    << " lit-rem: " << remLitTimeStampTotal
    << " inv-lit-rem: " << remLitTimeStampTotalInv
    << " stamp-cl-rem: " << subsumedStamp
    << endl;

    cout
    << "c [distill-with-bin-ext] bintri-based"
    << " lit-rem: " << remLitBinTri
    << " cl-sub: " << subBinTri
    << endl;

    cout
    << "c [distill-with-bin-ext] cache-based"
    << " lit-rem: " << remLitCache
    << " cl-sub: " << subCache
    << endl;
}

DistillWithImplicit::Stats& DistillWithImplicit::Stats::operator+=(const Stats& other)
{
    irredCacheBased += other.irredCacheBased;
    redCacheBased += other.redCacheBased;
    return *this;
}

void DistillWithImplicit::Stats::print_short(const Solver* solver) const
{
    irredCacheBased.print_short("irred", solver);
    redCacheBased.print_short("red", solver);
}

void DistillWithImplicit::Stats::print() const
{
    cout << "c -------- STRENGTHEN STATS --------" << endl;
    cout << "c --> cache-based on irred cls" << endl;
    irredCacheBased.print();

    cout << "c --> cache-based on red cls" << endl;
    redCacheBased.print();
    cout << "c -------- STRENGTHEN STATS END --------" << endl;
}


void DistillWithImplicit::Stats::CacheBased::print_short(const string type, const Solver* solver) const
{
    cout << "c [distill] cache-based "
    << std::setw(5) << type
    << "-- "
    << " cl tried " << std::setw(8) << triedCls
    << " cl-sh " << std::setw(5) << shrinked
    << " cl-rem " << std::setw(4) << numClSubsumed
    << " lit-rem " << std::setw(6) << numLitsRem
    << solver->conf.print_times(cpu_time, ranOutOfTime)
    << endl;
}

void DistillWithImplicit::Stats::CacheBased::print() const
{
    print_stats_line("c time"
        , cpu_time
        , ratio_for_stat(cpu_time, numCalled)
        , "s/call"
    );

    print_stats_line("c shrinked/tried/total"
        , shrinked
        , triedCls
        , totalCls
    );

    print_stats_line("c subsumed/tried/total"
        , numClSubsumed
        , triedCls
        , totalCls
    );

    print_stats_line("c lits-rem"
        , numLitsRem
        , stats_line_percent(numLitsRem, totalLits)
        , "% of lits tried"
    );

    print_stats_line("c called "
        , numCalled
        , stats_line_percent(ranOutOfTime, numCalled)
        , "% ran out of time"
    );

}

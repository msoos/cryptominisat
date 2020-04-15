/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "distillerlongwithimpl.h"
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

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

DistillerLongWithImpl::DistillerLongWithImpl(Solver* _solver) :
    solver(_solver)
    , seen(solver->seen)
    , seen2(solver->seen2)
    , numCalls(0)
{}

bool DistillerLongWithImpl::distill_long_with_implicit(const bool alsoStrengthen)
{
    assert(solver->ok);
    numCalls++;

    solver->clauseCleaner->remove_and_clean_all();

    runStats.redWatchBased.clear();
    runStats.irredWatchBased.clear();

    if (!shorten_all_cl_with_watch(solver->longIrredCls, false, false))
        goto end;

    if (solver->longRedCls[0].size() > 0
        && !shorten_all_cl_with_watch(solver->longRedCls[0], true, false)
    ) {
        goto end;
    }

    if (alsoStrengthen) {
        if (!shorten_all_cl_with_watch(solver->longIrredCls, false, true))
            goto end;

        if (solver->longRedCls[0].size() > 0
            && !shorten_all_cl_with_watch(solver->longRedCls[0], true, true)
        ) {
            goto end;
        }
    }

end:
    globalStats += runStats;
    if (solver->conf.verbosity) {
        if (solver->conf.verbosity >= 3)
            runStats.print();
        else
            runStats.print_short(solver);
    }
    runStats.clear();

    return solver->okay();
}

void DistillerLongWithImpl::strengthen_clause_with_watch(
    const Lit lit
    , const Watched* wit
) {
    //Strengthening w/ bin
    if (wit->isBin()
        && seen[lit.toInt()] //We haven't yet removed it
    ) {
        if (seen[(~wit->lit2()).toInt()]) {
            thisremLitBin++;
            seen[(~wit->lit2()).toInt()] = 0;
        }
    }
}

bool DistillerLongWithImpl::subsume_clause_with_watch(
    const Lit lit
    , Watched* wit
    , const Clause& cl
) {
    //Subsumption w/ bin
    if (wit->isBin() &&
        seen2[wit->lit2().toInt()]
    ) {
        //If subsuming irred with redundant, make the redundant into irred
        if (wit->red() && !cl.red()) {
            wit->setRed(false);
            timeAvailable -= (long)solver->watches[wit->lit2()].size()*3;
            findWatchedOfBin(solver->watches, wit->lit2(), lit, true).setRed(false);
            solver->binTri.redBins--;
            solver->binTri.irredBins++;
        }
        watch_based_data.subBin++;
        isSubsumed = true;
        return true;
    }

    //Extension w/ bin
    if (wit->isBin()
        && !wit->red()
        && !seen2[(~(wit->lit2())).toInt()]
    ) {
        seen2[(~(wit->lit2())).toInt()] = 1;
        lits2.push_back(~(wit->lit2()));
    }

    return false;
}

void DistillerLongWithImpl::str_and_sub_using_watch(
    Clause& cl
    , const Lit lit
    , const bool alsoStrengthen
) {
    //Go through the watchlist
    watch_subarray thisW = solver->watches[lit];
    timeAvailable -= (long)thisW.size()*2 + 5;
    for(Watched* wit = thisW.begin(), *wend = thisW.end()
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

void DistillerLongWithImpl::strsub_with_watch(
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
        str_and_sub_using_watch(cl, *lit, alsoStrengthen);
    }
    assert(lits2.size() > 1);
}

bool DistillerLongWithImpl::sub_str_cl_with_watch(
    ClOffset& offset
    , bool red
    , const bool alsoStrengthen
) {
    Clause& cl = *solver->cl_alloc.ptr(offset);
    assert(cl.size() > 2);

    if (solver->conf.verbosity >= 10) {
        cout << "Examining str clause:" << cl << endl;
    }

    timeAvailable -= (long)cl.size()*2;
    tmpStats.totalLits += cl.size();
    tmpStats.triedCls++;
    isSubsumed = false;
    thisremLitBin = 0;

    //Fill 'seen'
    lits2.clear();
    for (const Lit lit: cl) {
        seen[lit.toInt()] = 1;
        seen2[lit.toInt()] = 1;
        lits2.push_back(lit);
    }

    strsub_with_watch(alsoStrengthen, cl);

    //Clear 'seen2'
    timeAvailable -= (long)lits2.size()*3;
    for (const Lit lit: lits2) {
        seen2[lit.toInt()] = 0;
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

    //Nothing to do
    if (lits.size() == cl.size()) {
        return false;
    }

    return remove_or_shrink_clause(cl, offset);
}

//returns FALSE in case clause is shortened, and TRUE in case it is removed
bool DistillerLongWithImpl::remove_or_shrink_clause(Clause& cl, ClOffset& offset)
{
    //Remove or shrink clause
    timeAvailable -= (long)cl.size()*10;
    watch_based_data.remLitBin += thisremLitBin;
    tmpStats.shrinked++;
    timeAvailable -= (long)lits.size()*2 + 50;
    Clause* c2 = solver->add_clause_int(lits, cl.red(), cl.stats);
    if (c2 != NULL) {
        solver->detachClause(offset);
        solver->free_cl(offset);
        offset = solver->cl_alloc.get_offset(c2);
        return false;
    }

    //Implicit clause or non-existent after addition, remove
    return true;
}

void DistillerLongWithImpl::randomise_order_of_clauses(
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

uint64_t DistillerLongWithImpl::calc_time_available(
    const bool alsoStrengthen
    , const bool red
) const {
    //If it hasn't been to successful until now, don't do it so much
    const Stats::WatchBased* stats = NULL;
    if (red) {
        stats = &(globalStats.redWatchBased);
    } else {
        stats = &(globalStats.irredWatchBased);
    }

    uint64_t maxCountTime =
        solver->conf.watch_based_str_time_limitM*1000LL*1000LL
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

bool DistillerLongWithImpl::shorten_all_cl_with_watch(
    vector<ClOffset>& clauses
    , bool red
    , bool alsoStrengthen
) {
    assert(solver->ok);

    //Stats
    double myTime = cpuTime();

    const int64_t orig_time_available = calc_time_available(alsoStrengthen, red);
    timeAvailable = orig_time_available;
    tmpStats = Stats::WatchBased();
    tmpStats.totalCls = clauses.size();
    tmpStats.numCalled = 1;
    watch_based_data.clear();
    bool need_to_finish = false;

    //don't randomise if it's too large.
    if (clauses.size() < 100*10000*1000) {
        randomise_order_of_clauses(clauses);
    }

    size_t i = 0;
    size_t j = i;
    ClOffset offset;
    #ifdef USE_GAUSS
    Clause* cl;
    #endif
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
        offset = clauses[i];
        if (need_to_finish) {
            goto copy;
        }

        #ifdef USE_GAUSS
        cl = solver->cl_alloc.ptr(offset);
        if (cl->used_in_xor() &&
            solver->conf.force_preserve_xors)
        {
            goto copy;
        }
        #endif

        if (sub_str_cl_with_watch(offset, red, alsoStrengthen)) {
            solver->detachClause(offset);
            solver->free_cl(offset);
            continue;
        }

        copy:
        clauses[j++] = offset;
    }
    clauses.resize(clauses.size() - (i-j));
    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif

    dump_stats_for_shorten_all_cl_with_watch(red
        , alsoStrengthen
        , myTime
        , orig_time_available
    );

    return solver->okay();
}

void DistillerLongWithImpl::dump_stats_for_shorten_all_cl_with_watch(
    bool red
    , bool alsoStrengthen
    , double myTime
    , double orig_time_available
) {
    //Set stats
    const double time_used = cpuTime() - myTime;
    const bool time_out = timeAvailable < 0;
    const double time_remain = float_div(timeAvailable, orig_time_available);
    tmpStats.numClSubsumed += watch_based_data.get_cl_subsumed();
    tmpStats.numLitsRem += watch_based_data.get_lits_rem();
    tmpStats.cpu_time = time_used;
    if (red) {
        runStats.redWatchBased += tmpStats;
    } else {
        runStats.irredWatchBased += tmpStats;
    }
    if (solver->conf.verbosity >= 2) {
        if (solver->conf.verbosity >= 10) {
            cout << "red:" << red << " alsostrenghten:" << alsoStrengthen << endl;
        }
        watch_based_data.print();

        cout << "c [distill-with-bin-ext]"
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
    if (solver->sqlStats) {
        std::stringstream ss;
        ss << "shorten"
        << (alsoStrengthen ? " and str" : "")
        << (red ? " red" : " irred")
        <<  " cls"
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

void DistillerLongWithImpl::WatchBasedData::clear()
{
    WatchBasedData tmp;
    *this = tmp;
}

size_t DistillerLongWithImpl::WatchBasedData::get_cl_subsumed() const
{
    return subBin;
}

size_t DistillerLongWithImpl::WatchBasedData::get_lits_rem() const
{
    return remLitBin;
}

void DistillerLongWithImpl::WatchBasedData::print() const
{
    cout
    << "c [distill-with-bin-ext] bin-based"
    << " lit-rem: " << remLitBin
    << " cl-sub: " << subBin
    << endl;
}

DistillerLongWithImpl::Stats& DistillerLongWithImpl::Stats::operator+=(const Stats& other)
{
    irredWatchBased += other.irredWatchBased;
    redWatchBased += other.redWatchBased;
    return *this;
}

void DistillerLongWithImpl::Stats::print_short(const Solver* _solver) const
{
    irredWatchBased.print_short("irred", _solver);
    redWatchBased.print_short("red", _solver);
}

void DistillerLongWithImpl::Stats::print() const
{
    cout << "c -------- STRENGTHEN STATS --------" << endl;
    cout << "c --> watch-based on irred cls" << endl;
    irredWatchBased.print();

    cout << "c --> watch-based on red cls" << endl;
    redWatchBased.print();
    cout << "c -------- STRENGTHEN STATS END --------" << endl;
}


void DistillerLongWithImpl::Stats::WatchBased::print_short(const string type, const Solver* _solver) const
{
    cout << "c [distill] watch-based "
    << std::setw(5) << type
    << "-- "
    << " cl tried " << std::setw(8) << triedCls
    << " cl-sh " << std::setw(5) << shrinked
    << " cl-rem " << std::setw(4) << numClSubsumed
    << " lit-rem " << std::setw(6) << numLitsRem
    << _solver->conf.print_times(cpu_time, ranOutOfTime)
    << endl;
}

void DistillerLongWithImpl::Stats::WatchBased::print() const
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

double DistillerLongWithImpl::mem_used() const
{
    double mem = sizeof(DistillerLongWithImpl);
    mem+= lits.size()*sizeof(Lit);
    mem +=lits2.size()*sizeof(Lit);

    return mem;
}

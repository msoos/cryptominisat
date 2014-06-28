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

#include "vivifier.h"
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

Vivifier::Vivifier(Solver* _solver) :
    solver(_solver)
{}

bool Vivifier::vivify(const bool alsoStrengthen)
{
    assert(solver->ok);
    numCalls++;

    solver->clauseCleaner->cleanClauses(solver->longIrredCls);

    if (alsoStrengthen
        && !vivify_long_irred_cls()
    ) {
        goto end;
    }

    if (alsoStrengthen
        && !vivify_tri_irred_cls()
    ) {
        goto end;
    }

end:
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

bool Vivifier::vivify_tri_irred_cls()
{
    if (solver->conf.verbosity >= 6) {
        cout
        << "c Doing vivif for tri irred clauses"
        << endl;
    }

    uint64_t origShorten = runStats.numClShorten;
    uint64_t origLitRem = runStats.numLitsRem;
    double myTime = cpuTime();
    uint64_t maxNumProps = 2LL*1000LL*1000LL;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    size_t origTrailSize = solver->trail_size();

    //Randomize start in the watchlist
    size_t upI;
    upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (; numDone < solver->watches.size()
        ; upI = (upI +1) % solver->watches.size(), numDone++

    ) {
        if (solver->propStats.bogoProps-oldBogoProps + extraTime > maxNumProps
            || solver->must_interrupt_asap()
        ) {
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
                try_vivify_clause_and_return_new(
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

    int64_t diff_bogoprops = (int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps;
    const bool time_out =  diff_bogoprops + extraTime > maxNumProps;
    const double time_used = cpuTime() - myTime;
    const double time_remain = 1.0 - (double)(diff_bogoprops + extraTime)/(double)maxNumProps;
    if (solver->conf.verbosity >= 3) {
        cout
        << "c [vivif] tri irred"
        << " shorten: " << runStats.numClShorten - origShorten
        << " lit-rem: " << runStats.numLitsRem - origLitRem
        << " 0-depth ass: " << solver->trail_size() - origTrailSize
        << " T: " << std::setprecision(2) << time_used
        << " T-out: " << std::setprecision(2) << (time_out ? "Y" : "N")
        << " T-rem: " << std::setprecision(2) << time_remain *100.0 << "%"
        << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "vivif tri irred"
            , time_used
            , time_out
            , time_remain
        );
    }

    runStats.zeroDepthAssigns = solver->trail_size() - origTrailSize;

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

bool Vivifier::vivify_long_irred_cls()
{
    assert(solver->ok);
    if (solver->conf.verbosity >= 6) {
        cout
        << "c Doing asymm branch for long irred clauses"
        << endl;
    }

    double myTime = cpuTime();
    const size_t origTrailSize = solver->trail_size();

    //Time-limiting
    uint64_t maxNumProps = solver->conf.max_props_vivif_long_irred_clsM*1000LL*1000ULL;
    if (solver->litStats.irredLits + solver->litStats.redLits < 500000)
        maxNumProps *=2;

    extraTime = 0;
    uint64_t oldBogoProps = solver->propStats.bogoProps;
    bool time_out = false;
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
        if (time_out || !solver->ok) {
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        if (solver->propStats.bogoProps-oldBogoProps + extraTime >= maxNumProps
            || solver->must_interrupt_asap()
        ) {
            if (solver->conf.verbosity >= 3) {
                cout
                << "c Need to finish asymm -- ran out of prop (=allocated time)"
                << endl;
            }
            runStats.timeOut++;
            time_out = true;
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
        ClOffset offset2 = try_vivify_clause_and_return_new(
            offset
            , cl.red()
            , queueByBy
        );

        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    solver->longIrredCls.resize(solver->longIrredCls.size()- (i-j));

    //Didn't time out, so it went through the whole list. Reset asymm for all.
    if (!time_out) {
        for (vector<ClOffset>::const_iterator
            it = solver->longIrredCls.begin(), end = solver->longIrredCls.end()
            ; it != end
            ; it++
        ) {
            Clause* cl = solver->clAllocator.getPointer(*it);
            cl->setAsymmed(false);
        }
    }

    const double time_used = cpuTime() - myTime;
    const double time_remain = (double)(solver->propStats.bogoProps-oldBogoProps + extraTime)/(double)maxNumProps;
    if (solver->conf.verbosity >= 2) {
        cout << "c [vivif] longirred"
        << " tried: " << runStats.checkedClauses << "/" << solver->longIrredCls.size()
        << " cl-rem:" << runStats.numClShorten- origClShorten
        << " lits-rem:" << runStats.numLitsRem - origLitRem
        << " T: " << std::setprecision(2) << time_used
        << " T-out: " << (time_out ? "Y" : "N")
        << " T-rem: " << std::setprecision(2) << time_remain*100.0 << "%"
        << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "vivif long irred"
            , time_used
            , time_out
            , time_remain
        );
    }

    //Update stats
    runStats.time_used = cpuTime() - myTime;
    runStats.zeroDepthAssigns = solver->trail_size() - origTrailSize;

    return solver->ok;
}

ClOffset Vivifier::try_vivify_clause_and_return_new(
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

Vivifier::Stats& Vivifier::Stats::operator+=(const Stats& other)
{
    time_used += other.time_used;
    timeOut += other.timeOut;
    zeroDepthAssigns += other.zeroDepthAssigns;
    numClShorten += other.numClShorten;
    numLitsRem += other.numLitsRem;
    checkedClauses += other.checkedClauses;
    potentialClauses += other.potentialClauses;
    numCalled += other.numCalled;

    return *this;
}

void Vivifier::Stats::printShort() const
{
    cout
    << "c [vivif] asymm (tri+long)"
    << " useful: "<< numClShorten
    << "/" << checkedClauses << "/" << potentialClauses
    << " lits-rem: " << numLitsRem
    << " 0-depth-assigns: " << zeroDepthAssigns
    << " T: " << time_used << " s"
    << " T-out: " << (timeOut ? "Y" : "N")
    << endl;
}

void Vivifier::Stats::print(const size_t nVars) const
{
    //Asymm
    cout << "c -------- ASYMM STATS --------" << endl;
    printStatsLine("c time"
        , time_used
        , time_used/(double)numCalled
        , "per call"
    );

    printStatsLine("c timed out"
        , timeOut
        , stats_line_percent(timeOut, numCalled)
        , "% of calls"
    );

    printStatsLine("c asymm/checked/potential"
        , numClShorten
        , checkedClauses
        , potentialClauses
    );

    printStatsLine("c lits-rem",
        numLitsRem
    );
    printStatsLine("c 0-depth-assigns",
        zeroDepthAssigns
        , stats_line_percent(zeroDepthAssigns, nVars)
        , "% of vars"
    );
    cout << "c -------- ASYMM STATS END --------" << endl;
}

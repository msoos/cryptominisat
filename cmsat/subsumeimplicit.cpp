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

#include "subsumeimplicit.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"

#include <iomanip>
using std::cout;
using std::endl;
using namespace CMSat;

SubsumeImplicit::SubsumeImplicit(Solver* _solver) :
    solver(_solver)
{
}

void SubsumeImplicit::try_subsume_tri(
    const Lit lit
    , Watched*& i
    , Watched*& j
    , const bool doStamp
) {
    //Only treat one of the TRI's instances
    if (lit > i->lit2()) {
        *j++ = *i;
        return;
    }

    bool remove = false;

    //Subsumed by bin
    if (lastLit2 == i->lit2()
        && lastLit3 == lit_Undef
        && lastLit2 == i->lit2()
    ) {
        if (lastRed && !i->red()) {
            assert(lastBin->isBinary());
            assert(lastBin->red());
            assert(lastBin->lit2() == lastLit2);

            lastBin->setRed(false);
            timeAvailable -= 20;
            timeAvailable -= solver->watches[lastLit2.toInt()].size();
            findWatchedOfBin(solver->watches, lastLit2, lit, true).setRed(false);
            solver->binTri.redBins--;
            solver->binTri.irredBins++;
            lastRed = false;
        }

        remove = true;
    }

    //Subsumed by Tri
    if (!remove
        && lastLit2 == i->lit2()
        && lastLit3 == i->lit3()
    ) {
        //The sorting algorithm prefers irred to red, so it is
        //impossible to have irred before red
        assert(!(i->red() == false && lastRed == true));

        remove = true;
    }

    tmplits.clear();
    tmplits.push_back(lit);
    tmplits.push_back(i->lit2());
    tmplits.push_back(i->lit3());

    //Subsumed by stamp
    if (doStamp && !remove) {
        timeAvailable -= 15;
        remove = solver->stamp.stampBasedClRem(tmplits);
        runStats.stampTriRem += remove;
    }

    //Subsumed by cache
    if (!remove
        && solver->conf.doCache
    ) {
        for(size_t i = 0; i < tmplits.size() && !remove; i++) {
            timeAvailable -= solver->implCache[lit.toInt()].lits.size();
            for (vector<LitExtra>::const_iterator
                it2 = solver->implCache[tmplits[i].toInt()].lits.begin()
                , end2 = solver->implCache[tmplits[i].toInt()].lits.end()
                ; it2 != end2
                ; it2++
            ) {
                if ((   it2->getLit() == tmplits[0]
                        || it2->getLit() == tmplits[1]
                        || it2->getLit() == tmplits[2]
                    )
                    && it2->getOnlyIrredBin()
                ) {
                    remove = true;
                    runStats.cacheTriRem++;
                    break;
                 }
            }
        }
    }

    if (remove) {
        timeAvailable -= 30;
        solver->remove_tri_but_lit1(lit, i->lit2(), i->lit3(), i->red(), timeAvailable);
        runStats.remTris++;
        (*solver->drup) << del << lit  << i->lit2()  << i->lit3() << fin;
        return;
    }

    //Don't remove
    lastLit2 = i->lit2();
    lastLit3 = i->lit3();
    lastRed = i->red();

    *j++ = *i;
    return;
}

void SubsumeImplicit::try_subsume_bin(
    const Lit lit
    , Watched*& i
    , Watched*& j
) {
    //Subsume bin with bin
    if (i->lit2() == lastLit2
        && lastLit3 == lit_Undef
    ) {
        //The sorting algorithm prefers irred to red, so it is
        //impossible to have irred before red
        assert(!(i->red() == false && lastRed == true));

        runStats.remBins++;
        assert(i->lit2().var() != lit.var());
        timeAvailable -= 30;
        timeAvailable -= solver->watches[i->lit2().toInt()].size();
        removeWBin(solver->watches, i->lit2(), lit, i->red());
        if (i->red()) {
            solver->binTri.redBins--;
        } else {
            solver->binTri.irredBins--;
        }
        (*solver->drup) << del << lit << i->lit2() << fin;

        return;
    } else {
        lastBin = j;
        lastLit2 = i->lit2();
        lastLit3 = lit_Undef;
        lastRed = i->red();
        *j++ = *i;
    }
}

void SubsumeImplicit::subsume_implicit()
{
    assert(solver->okay());
    const double myTime = cpuTime();
    timeAvailable = 1900LL*1000LL*1000LL;
    const bool doStamp = solver->conf.doStamp;
    runStats.clear();

    //Randomize starting point
    const size_t rnd_start = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (;numDone < solver->watches.size() && timeAvailable > 0
         ;numDone++
    ) {
        const size_t at = (rnd_start + numDone)  % solver->watches.size();
        runStats.numWatchesLooked++;
        const Lit lit = Lit::toLit(at);
        watch_subarray ws = solver->watches[lit.toInt()];

        //We can't do much when there is nothing, or only one
        if (ws.size() < 2)
            continue;

        timeAvailable -= ws.size()*std::ceil(std::log((double)ws.size())) + 20;
        std::sort(ws.begin(), ws.end(), WatchSorter());
        /*cout << "---> Before" << endl;
        printWatchlist(ws, lit);*/

        Watched* i = ws.begin();
        Watched* j = i;
        clear();

        for (Watched* end = ws.end(); i != end; i++) {
            if (timeAvailable < 0) {
                *j++ = *i;
                continue;
            }

            switch(i->getType()) {
                case CMSat::watch_clause_t:
                    *j++ = *i;
                    break;

                case CMSat::watch_tertiary_t:
                    try_subsume_tri(lit, i, j, doStamp);
                    break;

                case CMSat::watch_binary_t:
                    try_subsume_bin(lit, i, j);
                    break;

                default:
                    assert(false);
                    break;
            }
        }
        ws.shrink(i-j);
    }
    solver->checkStats();

    runStats.numCalled++;
    runStats.time_used += cpuTime() - myTime;
    runStats.time_out += (timeAvailable < 0);
    if (solver->conf.verbosity >= 1) {
        runStats.printShort();
    }

    globalStats += runStats;
}

SubsumeImplicit::Stats SubsumeImplicit::Stats::operator+=(const SubsumeImplicit::Stats& other)
{
    numCalled+= other.numCalled;
    time_out += other.time_out;
    time_used += other.time_used;
    remBins += other.remBins;
    remTris += other.remTris;
    stampTriRem += other.stampTriRem;
    cacheTriRem += other.cacheTriRem;
    numWatchesLooked += other.numWatchesLooked;

    return *this;
}

void SubsumeImplicit::Stats::printShort() const
{
    cout
    << "c [impl sub]"
    << " bin: " << remBins
    << " tri: " << remTris
    << " (stamp: " << stampTriRem << ", cache: " << cacheTriRem << ")"

    << " T: " << std::fixed << std::setprecision(2)
    << time_used
    << " T-out: " << (time_out ? "Y" : "N")
    << " w-visit: " << numWatchesLooked
    << endl;
}

void SubsumeImplicit::Stats::print() const
{
    //Asymm
    cout << "c -------- IMPLICIT SUB STATS --------" << endl;
    printStatsLine("c time"
        , time_used
        , time_used/(double)numCalled
        , "per call"
    );

    printStatsLine("c timed out"
        , time_out
        , (double)time_out/(double)numCalled*100.0
        , "% of calls"
    );

    printStatsLine("c rem bins"
        , remBins
    );

    printStatsLine("c rem tris"
        , remTris
    );
    cout << "c -------- IMPLICIT SUB STATS END --------" << endl;
}

SubsumeImplicit::Stats SubsumeImplicit::getStats() const
{
    return globalStats;
}

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

#include "calcdefpolars.h"
#include "assert.h"
#include "time_mem.h"
#include "solver.h"

using namespace CMSat;
using std::cout;
using std::endl;

CalcDefPolars::CalcDefPolars(Solver* _solver) :
    solver(_solver)
{
}

/**
@brief Tally votes for a default TRUE or FALSE value for the variable using the Jeroslow-Wang method

@p votes[inout] Votes are tallied at this place for each variable
@p cs The clause to tally votes for
*/
void CalcDefPolars::tallyVotes(const vector<ClOffset>& cs)
{
    for (vector<ClOffset>::const_iterator
        it = cs.begin(), end = it + cs.size()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *solver->clAllocator->getPointer(*it);

        //Only count irred
        if (cl.red())
            continue;

        double divider;
        if (cl.size() > 63) divider = 0.0;
        else divider = 1.0/(double)((uint64_t)1<<(cl.size()-1));

        for (const Lit *it2 = cl.begin(), *end2 = cl.end(); it2 != end2; it2++) {
            if (it2->sign()) votes[it2->var()] += divider;
            else votes[it2->var()] -= divider;
        }
    }
}

void CalcDefPolars::tallyVotesBinTri(const vector<vec<Watched> >& watches)
{
    size_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {

            //Only count bins once
            if (it2->isBinary()
                && lit < it2->lit2()
                && !it2->red()
            ) {

                if (lit.sign()) votes[lit.var()] += 0.5;
                else votes[lit.var()] -= 0.5;

                Lit lit2 = it2->lit2();
                if (lit2.sign()) votes[lit2.var()] += 0.5;
                else votes[lit2.var()] -= 0.5;
            }

            //Only count TRI-s once
            if (it2->isTri()
                && lit < it2->lit2()
                && it2->lit2() < it2->lit3()
                && it2->red()
            ) {
                if (lit.sign()) votes[lit.var()] += 0.3;
                else votes[lit.var()] -= 0.3;

                Lit lit2 = it2->lit2();
                if (lit2.sign()) votes[lit2.var()] += 0.3;
                else votes[lit2.var()] -= 0.3;

                Lit lit3 = it2->lit3();
                if (lit3.sign()) votes[lit3.var()] += 0.3;
                else votes[lit3.var()] -= 0.3;
            }
        }
    }
}

/**
@brief Tallies votes for a TRUE/FALSE default polarity using Jeroslow-Wang

Voting is only used if polarity_mode is "PolarityMode::automatic". This is the default.
Uses the tallyVotes() functions to tally the votes
*/
const vector<char> CalcDefPolars::calculate()
{
    assert(solver->decisionLevel() == 0);

    //Setup
    votes.clear();
    votes.resize(solver->nVars(), 0.0);
    vector<char> ret(solver->nVars(), 0);
    const double myTime = cpuTime();

    //Tally votes
    tallyVotes(solver->longIrredCls);
    tallyVotesBinTri(solver->watches);

    //Set polarity according to tally
    uint32_t posPolars = 0;
    uint32_t undecidedPolars = 0;
    size_t i = 0;
    for (vector<double>::iterator it = votes.begin(), end = votes.end(); it != end; it++, i++) {
        ret[i] = (*it >= 0.0);
        posPolars += (*it < 0.0);
        undecidedPolars += (*it == 0.0);
    }

    //Print results
    if (solver->conf.verbosity >= 3) {
        #ifdef USE_OPENMP
        #pragma omp critical
        #endif

        cout << "c Calc default polars - "
        << " T: " << std::fixed << std::setprecision(2) << (cpuTime() - myTime) << " s"
        << " pos: " << std::setw(7) << posPolars
        << " undec: " << std::setw(7) << undecidedPolars
        << " neg: " << std::setw(7) << solver->nVars()-  undecidedPolars - posPolars
        << std:: endl;
    }

    return ret;
}
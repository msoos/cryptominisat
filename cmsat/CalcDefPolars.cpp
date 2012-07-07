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

#include "CalcDefPolars.h"
#include "assert.h"
#include "time_mem.h"
#include "Solver.h"
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
void CalcDefPolars::tallyVotes(const vector<Clause*>& cs, vector<double>& votes) const
{
    for (vector<Clause*>::const_iterator it = cs.begin(), end = it + cs.size(); it != end; it++) {
        const Clause& c = **it;
        if (c.learnt()) continue;

        double divider;
        if (c.size() > 63) divider = 0.0;
        else divider = 1.0/(double)((uint64_t)1<<(c.size()-1));

        for (const Lit *it2 = c.begin(), *end2 = c.end(); it2 != end2; it2++) {
            if (it2->sign()) votes[it2->var()] += divider;
            else votes[it2->var()] -= divider;
        }
    }
}

void CalcDefPolars::tallyVotesBin(vector<double>& votes, const vector<vec<Watched> >& watches) const
{
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                if (!it2->learnt()) {
                    if (lit.sign()) votes[lit.var()] += 0.5;
                    else votes[lit.var()] -= 0.5;

                    Lit lit2 = it2->getOtherLit();
                    if (lit2.sign()) votes[lit2.var()] += 0.5;
                    else votes[lit2.var()] -= 0.5;
                }
            }
        }
    }
}

/**
@brief Tallies votes for a TRUE/FALSE default polarity using Jeroslow-Wang

Voting is only used if polarity_mode is "polarity_auto". This is the default.
Uses the tallyVotes() functions to tally the votes
*/
const vector<char> CalcDefPolars::calculate()
{
    assert(solver->decisionLevel() == 0);

    //Setup
    vector<char> ret(solver->nVars(), 0);
    vector<double> votes(solver->nVars(), 0.0);
    const double myTime = cpuTime();

    //Tally votes
    tallyVotes(solver->clauses, votes);
    tallyVotesBin(votes, solver->watches);

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
        #pragma omp critical
        cout << "c Calc default polars - "
        << " time: " << std::fixed << std::setw(6) << std::setprecision(2) << (cpuTime() - myTime) << " s"
        << " pos: " << std::setw(7) << posPolars
        << " undec: " << std::setw(7) << undecidedPolars
        << " neg: " << std::setw(7) << solver->nVars()-  undecidedPolars - posPolars
        << std:: endl;
    }

    return ret;
}
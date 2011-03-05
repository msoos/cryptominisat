/***************************************************************************
CryptoMiniSat -- Copyright (c) 2011 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "CalcDefPolars.h"
#include "assert.h"
#include "time_mem.h"
#include "Solver.h"

CalcDefPolars::CalcDefPolars(Solver& _solver) :
    solver(_solver)
{
}

/**
@brief Tally votes for a default TRUE or FALSE value for the variable using the Jeroslow-Wang method

@p votes[inout] Votes are tallied at this place for each variable
@p cs The clause to tally votes for
*/
void CalcDefPolars::tallyVotes(const vec<Clause*>& cs, vec<double>& votes) const
{
    for (const Clause * const*it = cs.getData(), * const*end = it + cs.size(); it != end; it++) {
        const Clause& c = **it;
        if (c.learnt()) continue;

        double divider;
        if (c.size() > 63) divider = 0.0;
        else divider = 1.0/(double)((uint64_t)1<<(c.size()-1));

        for (const Lit *it2 = c.getData(), *end2 = c.getDataEnd(); it2 != end2; it2++) {
            if (it2->sign()) votes[it2->var()] += divider;
            else votes[it2->var()] -= divider;
        }
    }
}

void CalcDefPolars::tallyVotesBin(vec<double>& votes, const vec<vec2<Watched> >& watches) const
{
    uint32_t wsLit = 0;
    for (const vec2<Watched> *it = watches.getData(), *end = watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                if (!it2->getLearnt()) {
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
@brief Tally votes a default TRUE or FALSE value for the variable using the Jeroslow-Wang method

For XOR clause, we simply add some weight for a FALSE default, i.e. being in
xor clauses makes the variabe more likely to be FALSE by default
*/
void CalcDefPolars::tallyVotes(const vec<XorClause*>& cs, vec<double>& votes) const
{
    for (const XorClause * const*it = cs.getData(), * const*end = it + cs.size(); it != end; it++) {
        const XorClause& c = **it;
        double divider;
        if (c.size() > 63) divider = 0.0;
        else divider = 1.0/(double)((uint64_t)1<<(c.size()-1));

        for (const Lit *it2 = c.getData(), *end2 = c.getDataEnd(); it2 != end2; it2++) {
            votes[it2->var()] += divider;
        }
    }
}

/**
@brief Tallies votes for a TRUE/FALSE default polarity using Jeroslow-Wang

Voting is only used if polarity_mode is "polarity_auto". This is the default.
Uses the tallyVotes() functions to tally the votes
*/
void CalcDefPolars::calculate()
{
    #ifdef VERBOSE_DEBUG_POLARITIES
    std::cout << "Default polarities: " << std::endl;
    #endif

    assert(solver.decisionLevel() == 0);
    if (solver.conf.polarity_mode == polarity_auto) {
        double myTime = cpuTime();

        vec<double> votes(solver.nVars(), 0.0);

        tallyVotes(solver.clauses, votes);
        tallyVotesBin(votes, solver.watches);
        tallyVotes(solver.xorclauses, votes);

        Var i = 0;
        uint32_t posPolars = 0;
        uint32_t undecidedPolars = 0;
        for (const double *it = votes.getData(), *end = votes.getDataEnd(); it != end; it++, i++) {
            solver.varData[i].polarity.setLastVal(*it >= 0.0);
            posPolars += (*it < 0.0);
            undecidedPolars += (*it == 0.0);
            #ifdef VERBOSE_DEBUG_POLARITIES
            std::cout << !defaultPolarities[i] << ", ";
            #endif //VERBOSE_DEBUG_POLARITIES
        }

        if (solver.conf.verbosity >= 2) {
            std::cout << "c Calc default polars - "
            << " time: " << std::fixed << std::setw(6) << std::setprecision(2) << (cpuTime() - myTime) << " s"
            << " pos: " << std::setw(7) << posPolars
            << " undec: " << std::setw(7) << undecidedPolars
            << " neg: " << std::setw(7) << solver.nVars()-  undecidedPolars - posPolars
            << std:: endl;
        }
    } else {
        for (uint32_t i = 0; i < solver.varData.size(); i++) {
            solver.varData[i].polarity.setLastVal(solver.getPolarity(i));
        }
    }

    #ifdef VERBOSE_DEBUG_POLARITIES
    std::cout << std::endl;
    #endif //VERBOSE_DEBUG_POLARITIES
}
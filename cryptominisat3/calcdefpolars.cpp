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

#include "calcdefpolars.h"
#include "assert.h"
#include "time_mem.h"
#include "solver.h"
#include "clauseallocator.h"

using namespace CMSat;
using std::cout;
using std::endl;

CalcDefPolars::CalcDefPolars(Solver* _solver) :
    solver(_solver)
{
}

void CalcDefPolars::add_vote(const Lit lit, const double value)
{
    if (lit.sign()) {
        votes[lit.var()] -= value;
    } else {
        votes[lit.var()] += value;
    }
}

void CalcDefPolars::tally_clause_votes(const vector<ClOffset>& cs)
{
    for (const ClOffset offset: cs) {
        const Clause& cl = *solver->clAllocator.getPointer(offset);

        //Only count irred
        if (cl.red())
            continue;

        if (cl.size() > 63)
            continue;

        double divider = 1.0/(double)((uint64_t)1<<(cl.size()-1));

        for (const Lit lit: cl) {
            add_vote(lit, divider);

        }
    }
}

void CalcDefPolars::tally_implicit_votes(const watch_array& watches)
{
    size_t wsLit = 0;
    for (watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {

            //Only count bins once
            if (it2->isBinary()
                && lit < it2->lit2()
                && !it2->red()
            ) {
                add_vote(lit, 0.5);
                add_vote(it2->lit2(), 0.5);
            }

            //Only count TRI-s once
            if (it2->isTri()
                && lit < it2->lit2()
                && it2->lit2() < it2->lit3()
                && !it2->red()
            ) {
                add_vote(lit, 0.33);
                add_vote(it2->lit2(), 0.33);
                add_vote(it2->lit3(), 0.33);
            }
        }
    }
}

const vector<unsigned char> CalcDefPolars::calculate()
{
    assert(solver->decisionLevel() == 0);

    //Setup
    votes.clear();
    votes.resize(solver->nVars(), 0.0);
    vector<unsigned char> ret_polar(solver->nVars(), 0);
    const double myTime = cpuTime();

    //Tally votes
    tally_clause_votes(solver->longIrredCls);
    tally_implicit_votes(solver->watches);

    //Set polarity according to tally
    size_t pos_polars = 0;
    size_t neg_polars = 0;
    size_t undecided_polars = 0;
    for (size_t i = 0; i < votes.size(); i++) {
        if (votes[i] > 0) {
            ret_polar[i] = true;
            pos_polars ++;
        } else {
            if (votes[i] == 0) {
                undecided_polars ++;
            } else {
                neg_polars++;
            }
            ret_polar[i] = false;
        }
    }

    //Print results
    if (solver->conf.verbosity >= 2) {
        cout
        << "c [polar] default polars - "
        << " pos: " << std::setw(7) << pos_polars
        << " neg: " << std::setw(7) << neg_polars
        << " undec: " << std::setw(7) << undecided_polars
        << " T: " << std::fixed << std::setprecision(2) << (cpuTime() - myTime) << " s"
        << std:: endl;
    }

    return ret_polar;
}
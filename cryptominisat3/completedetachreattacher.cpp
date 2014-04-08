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

#include "completedetachreattacher.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "clauseallocator.h"

using namespace CMSat;

CompleteDetachReatacher::CompleteDetachReatacher(Solver* _solver) :
    solver(_solver)
{
}

/**
@brief Completely detach all non-binary clauses
*/
void CompleteDetachReatacher::detachNonBinsNonTris()
{
    ClausesStay stay;

    for (watch_array::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it
    ) {
        stay += clearWatchNotBinNotTri(*it);
    }

    solver->litStats.redLits = 0;
    solver->litStats.irredLits = 0;

    assert(stay.redBins % 2 == 0);
    solver->binTri.redBins = stay.redBins/2;

    assert(stay.irredBins % 2 == 0);
    solver->binTri.irredBins = stay.irredBins/2;

    assert(stay.redTris % 3 == 0);
    solver->binTri.redTris = stay.redTris/3;

    assert(stay.irredTris % 3 == 0);
    solver->binTri.irredTris = stay.irredTris/3;
}

/**
@brief Helper function for detachPointerUsingClauses()
*/
CompleteDetachReatacher::ClausesStay CompleteDetachReatacher::clearWatchNotBinNotTri(
    watch_subarray ws
) {
    ClausesStay stay;

    watch_subarray::iterator i = ws.begin();
    watch_subarray::iterator j = i;
    for (watch_subarray::iterator end = ws.end(); i != end; i++) {
        if (i->isBinary()) {
            if (i->red())
                stay.redBins++;
            else
                stay.irredBins++;

            *j++ = *i;
        } else if (i->isTri()) {
            if (i->red())
                stay.redTris++;
            else
                stay.irredTris++;

            *j++ = *i;
        }
    }
    ws.shrink_(i-j);

    return stay;
}

bool CompleteDetachReatacher::reattachLongs(bool removeStatsFirst)
{
    if (solver->conf.verbosity >= 6) {
        cout << "Cleaning and reattaching clauses" << endl;
    }

    cleanAndAttachClauses(solver->longIrredCls, removeStatsFirst);
    cleanAndAttachClauses(solver->longRedCls, removeStatsFirst);
    solver->clauseCleaner->clean_implicit_clauses();

    if (solver->ok) {
        solver->ok = (solver->propagate().isNULL());
    }
    assert(!solver->drup->something_delayed());

    return solver->ok;
}

/**
@brief Cleans clauses from failed literals/removes satisfied clauses from cs

May change solver->ok to FALSE (!)
*/
void CompleteDetachReatacher::cleanAndAttachClauses(
    vector<ClOffset>& cs
    , bool removeStatsFirst
) {
    assert(!solver->drup->something_delayed());
    vector<ClOffset>::iterator i = cs.begin();
    vector<ClOffset>::iterator j = i;
    for (vector<ClOffset>::iterator end = cs.end(); i != end; i++) {
        Clause* cl = solver->clAllocator.getPointer(*i);

        //Handle stat removal if need be
        if (removeStatsFirst) {
            if (cl->red()) {
                solver->litStats.redLits -= cl->size();
            } else {
                solver->litStats.irredLits -= cl->size();
            }
        }

        if (cleanClause(cl)) {
            solver->attachClause(*cl);
            *j++ = *i;
        } else {
            solver->clAllocator.clauseFree(*i);
        }
    }
    cs.resize(cs.size() - (i-j));
}

/**
@brief Not only cleans a clause from false literals, but if clause is satisfied, it reports it
*/
bool CompleteDetachReatacher::cleanClause(Clause* cl)
{
    Clause& ps = *cl;
    (*solver->drup) << deldelay << ps << fin;
    if (ps.size() <= 3) {
        cout
        << "ERROR, clause is too small, and linked in: "
        << *cl
        << endl;
    }
    assert(ps.size() > 3);

    Lit *i = ps.begin();
    Lit *j = i;
    for (Lit *end = ps.end(); i != end; i++) {
        if (solver->value(*i) == l_True) {

            //Drup
            if (i != j) {
                (*solver->drup) << findelay;
            }
            return false;
        }
        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);

    //Drup
    if (i != j) {
        (*solver->drup) << *cl << fin << findelay;
    } else {
        solver->drup->forget_delay();
    }

    switch (ps.size()) {
        case 0:
            solver->ok = false;
            return false;

        case 1:
            solver->enqueue(ps[0]);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
            return false;

        case 2: {
            solver->attachBinClause(ps[0], ps[1], ps.red());
            return false;
        }

        case 3: {
            solver->attachTriClause(ps[0], ps[1], ps[2], ps.red());
            return false;
        }

        default: {
            break;
        }
    }

    return true;
}

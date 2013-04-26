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

#include "completedetachreattacher.h"
#include "solver.h"
#include "varreplacer.h"
#include "clausecleaner.h"

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

    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++
    ) {
        stay += clearWatchNotBinNotTri(*it);
    }

    solver->binTri.redLits = stay.learntBins + stay.learntTris;
    solver->binTri.irredLits = stay.nonLearntBins + stay.nonLearntTris;
    solver->binTri.redBins = stay.learntBins/2;
    solver->binTri.irredBins = stay.nonLearntBins/2;
    solver->binTri.redTris = stay.learntTris/3;
    solver->binTri.irredTris = stay.nonLearntTris/3;
}

/**
@brief Helper function for detachPointerUsingClauses()
*/
CompleteDetachReatacher::ClausesStay CompleteDetachReatacher::clearWatchNotBinNotTri(
    vec<Watched>& ws
) {
    ClausesStay stay;

    vec<Watched>::iterator i = ws.begin();
    vec<Watched>::iterator j = i;
    for (vec<Watched>::iterator end = ws.end(); i != end; i++) {
        if (i->isBinary()) {
            if (i->learnt())
                stay.learntBins++;
            else
                stay.nonLearntBins++;

            *j++ = *i;
        } else if (i->isTri()) {
            if (i->learnt())
                stay.learntTris++;
            else
                stay.nonLearntTris++;

            *j++ = *i;
        }
    }
    ws.shrink_(i-j);

    return stay;
}

/**
@brief Completely attach all clauses
*/
bool CompleteDetachReatacher::reattachLongs(bool removeStatsFirst)
{
    #ifdef DRUP
    if (solver->conf.verbosity >= 6) {
        cout << "Cleaning and reattaching clauses" << endl;
    }
    #endif
    cleanAndAttachClauses(solver->longIrredCls, removeStatsFirst);
    cleanAndAttachClauses(solver->longRedCls, removeStatsFirst);

    //Treat implicits
    solver->clauseCleaner->treatImplicitClauses();

    if (solver->ok) {
        solver->ok = (solver->propagate().isNULL());
    }

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
    vector<ClOffset>::iterator i = cs.begin();
    vector<ClOffset>::iterator j = i;
    for (vector<ClOffset>::iterator end = cs.end(); i != end; i++) {
        Clause* cl = solver->clAllocator->getPointer(*i);

        //Handle stat removal if need be
        if (removeStatsFirst) {
            if (cl->learnt()) {
                solver->binTri.redLits -= cl->size();
            } else {
                solver->binTri.irredLits -= cl->size();
            }
        }

        if (cleanClause(cl)) {
            solver->attachClause(*cl);
            *j++ = *i;
        } else {
            solver->clAllocator->clauseFree(*i);
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
    if (ps.size() <= 3) {
        cout
        << "ERROR, clause is too small, and linked in: "
        << *cl
        << endl;
    }
    assert(ps.size() > 3);
    #ifdef DRUP
    vector<Lit> origCl(cl->size());
    std::copy(cl->begin(), cl->end(), origCl.begin());
    #endif

    Lit *i = ps.begin();
    Lit *j = i;
    for (Lit *end = ps.end(); i != end; i++) {
        if (solver->value(*i) == l_True) {
            #ifdef DRUP
            if (solver->drup && i != j) {
                (*solver->drup)
                << "d "
                << origCl
                << endl;
            }
            #endif

            return false;
        }
        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);

    #ifdef DRUP
    if (solver->drup && i != j) {
        (*solver->drup)
        << cl
        << " 0\n"

        //Delete old one
        << "d "
        << origCl
        << " 0\n";
    }
    #endif

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
            solver->attachBinClause(ps[0], ps[1], ps.learnt());
            return false;
        }

        case 3: {
            solver->attachTriClause(ps[0], ps[1], ps[2], ps.learnt());
            return false;
        }

        default: {
            break;
        }
    }

    return true;
}

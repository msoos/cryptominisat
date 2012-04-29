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

#include "CompleteDetachReattacher.h"
#include "ThreadControl.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"

CompleteDetachReatacher::CompleteDetachReatacher(ThreadControl* _control) :
    control(_control)
{
}

/**
@brief Completely detach all non-binary clauses
*/
void CompleteDetachReatacher::detachNonBinsNonTris(const bool removeTri)
{
    uint64_t oldNumBinsLearnt = control->numBinsLearnt;
    uint64_t oldNumBinsNonLearnt = control->numBinsNonLearnt;
    ClausesStay stay;

    for (vector<vec<Watched> >::iterator it = control->watches.begin(), end = control->watches.end(); it != end; it++) {
        stay += clearWatchNotBinNotTri(*it, removeTri);
    }

    control->learntsLits = stay.learntBins;
    control->clausesLits = stay.nonLearntBins;
    control->numBinsLearnt = stay.learntBins/2;
    control->numBinsNonLearnt = stay.nonLearntBins/2;
    release_assert(control->numBinsLearnt == oldNumBinsLearnt);
    release_assert(control->numBinsNonLearnt == oldNumBinsNonLearnt);
}

/**
@brief Helper function for detachPointerUsingClauses()
*/
CompleteDetachReatacher::ClausesStay CompleteDetachReatacher::clearWatchNotBinNotTri(vec<Watched>& ws, const bool removeTri)
{
    ClausesStay stay;

    vec<Watched>::iterator i = ws.begin();
    vec<Watched>::iterator j = i;
    for (vec<Watched>::iterator end = ws.end(); i != end; i++) {
        if (i->isBinary()) {
            if (i->getLearnt())
                stay.learntBins++;
            else
                stay.nonLearntBins++;

            *j++ = *i;
        } else if (!removeTri && i->isTriClause()) {
            stay.tris++;
            *j++ = *i;
        }
    }
    ws.shrink_(i-j);

    return stay;
}

/**
@brief Completely attach all clauses
*/
bool CompleteDetachReatacher::reattachNonBins()
{
    cleanAndAttachClauses(control->clauses);
    cleanAndAttachClauses(control->learnts);
    control->clauseCleaner->removeSatisfiedBins();

    if (control->ok)
        control->ok = (control->propagate().isNULL());

    return control->ok;
}

/**
@brief Cleans clauses from failed literals/removes satisfied clauses from cs

May change control->ok to FALSE (!)
*/
void CompleteDetachReatacher::cleanAndAttachClauses(vector<Clause*>& cs)
{
    vector<Clause*>::iterator i = cs.begin();
    vector<Clause*>::iterator j = i;
    for (vector<Clause*>::iterator end = cs.end(); i != end; i++) {
        if (cleanClause(*i)) {
            control->attachClause(**i);
            *j++ = *i;
        } else {
            control->clAllocator->clauseFree(*i);
        }
    }
    cs.resize(cs.size() - (i-j));
}

/**
@brief Not only cleans a clause from false literals, but if clause is satisfied, it reports it
*/
bool CompleteDetachReatacher::cleanClause(Clause*& cl)
{
    Clause& ps = *cl;
    assert(ps.size() > 2);

    Lit *i = ps.begin();
    Lit *j = i;
    for (Lit *end = ps.end(); i != end; i++) {
        if (control->value(*i) == l_True) return false;
        if (control->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);

    switch (ps.size()) {
        case 0:
            control->ok = false;
            return false;

        case 1:
            control->enqueue(ps[0]);
            control->propStats.propsUnit++;
            return false;

        case 2: {
            control->attachBinClause(ps[0], ps[1], ps.learnt());
            return false;
        }

        default: {
            break;
        }
    }

    return true;
}

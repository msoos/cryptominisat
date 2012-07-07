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

#include "ClauseCleaner.h"

//#define DEBUG_CLEAN
//#define VERBOSE_DEBUG

ClauseCleaner::ClauseCleaner(Solver* _solver) :
    solver(_solver)
{
}

bool ClauseCleaner::satisfied(const Watched& watched, Lit lit)
{
    assert(watched.isBinary());
    if (solver->value(lit) == l_True) return true;
    if (solver->value(watched.lit1()) == l_True) return true;
    return false;
}

void ClauseCleaner::removeSatisfiedBins()
{
    #ifdef DEBUG_CLEAN
    assert(solver->decisionLevel() == 0);
    #endif

    uint32_t numRemovedHalfNonLearnt = 0;
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isBinary() && satisfied(*i, lit)) {
                if (i->learnt())
                    numRemovedHalfLearnt++;
                else
                    numRemovedHalfNonLearnt++;
            } else {
                *j++ = *i;
            }
        }
        ws.shrink_(i - j);
    }

    //cout << "removedHalfLeart: " << numRemovedHalfLearnt << endl;
    //cout << "removedHalfNonLeart: " << numRemovedHalfNonLearnt << endl;
    assert(numRemovedHalfLearnt % 2 == 0);
    assert(numRemovedHalfNonLearnt % 2 == 0);
    solver->clausesLits -= numRemovedHalfNonLearnt;
    solver->learntsLits -= numRemovedHalfLearnt;
    solver->numBinsLearnt -= numRemovedHalfLearnt/2;
    solver->numBinsNonLearnt -= numRemovedHalfNonLearnt/2;
}

void ClauseCleaner::cleanClauses(vector<Clause*>& cs)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->qhead == solver->trail.size());

    #ifdef VERBOSE_DEBUG
    cout << "Cleaning " << (type==binaryClauses ? "binaryClauses" : "normal clauses" ) << endl;
    #endif //VERBOSE_DEBUG

    vector<Clause*>::iterator s, ss, end;
    for (s = ss = cs.begin(), end = cs.end();  s != end; s++) {
        if (s+1 != end)
            __builtin_prefetch(*(s+1));

        if (cleanClause(*s)) {
            solver->clAllocator->clauseFree(*s);
        } else {
            *ss++ = *s;
        }
    }
    cs.resize(cs.size() - (s-ss));

    #ifdef VERBOSE_DEBUG
    cout << "cleanClauses(Clause) useful ?? Removed: " << s-ss << endl;
    #endif
}

inline bool ClauseCleaner::cleanClause(Clause*& cc)
{
    Clause& c = *cc;
    assert(c.size() > 2);
    const uint32_t origSize = c.size();

    Lit origLit1 = c[0];
    Lit origLit2 = c[1];
    Lit origLit3 = c[2];

    Lit *i, *j, *end;
    uint32_t num = 0;
    for (i = j = c.begin(), end = i + c.size();  i != end; i++, num++) {
        lbool val = solver->value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (val == l_True) {
            solver->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);
            return true;
        }
    }
    c.shrink(i-j);

    assert(c.size() > 1);
    if (i != j) {
        if (c.size() == 2) {
            solver->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);
            solver->attachBinClause(c[0], c[1], c.learnt());
            return true;
        } else if (c.size() == 3) {
            solver->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);
            solver->attachClause(c);
        } else {
            if (c.learnt())
                solver->learntsLits -= i-j;
            else
                solver->clausesLits -= i-j;
        }
    }

    return false;
}

bool ClauseCleaner::satisfied(const Clause& c) const
{
    for (uint32_t i = 0; i != c.size(); i++)
        if (solver->value(c[i]) == l_True)
            return true;
        return false;
}

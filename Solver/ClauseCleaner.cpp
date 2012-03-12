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

ClauseCleaner::ClauseCleaner(ThreadControl* _control) :
    control(_control)
{
    for (uint32_t i = 0; i < 6; i++) {
        lastNumUnitarySat[i] = control->getNumUnitaries();
        lastNumUnitaryClean[i] = control->getNumUnitaries();
    }
}

bool ClauseCleaner::satisfied(const Watched& watched, Lit lit)
{
    assert(watched.isBinary());
    if (control->value(lit) == l_True) return true;
    if (control->value(watched.getOtherLit()) == l_True) return true;
    return false;
}

void ClauseCleaner::removeSatisfiedBins(const uint32_t limit)
{
    #ifdef DEBUG_CLEAN
    assert(control->decisionLevel() == 0);
    #endif

    if (lastNumUnitarySat[binaryClauses] + limit >= control->getNumUnitaries())
        return;

    uint32_t numRemovedHalfNonLearnt = 0;
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::iterator it = control->watches.begin(), end = control->watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isBinary() && satisfied(*i, lit)) {
                if (i->getLearnt()) numRemovedHalfLearnt++;
                else {
                    numRemovedHalfNonLearnt++;
                }
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
    control->clausesLits -= numRemovedHalfNonLearnt;
    control->learntsLits -= numRemovedHalfLearnt;
    control->numBins -= (numRemovedHalfLearnt + numRemovedHalfNonLearnt)/2;

    lastNumUnitarySat[binaryClauses] = control->getNumUnitaries();
}

void ClauseCleaner::cleanClauses(vector<Clause*>& cs, ClauseSetType type, const uint32_t limit)
{
    assert(control->decisionLevel() == 0);
    assert(control->qhead == control->trail.size());

    if (lastNumUnitaryClean[type] + limit >= control->getNumUnitaries())
        return;

    #ifdef VERBOSE_DEBUG
    cout << "Cleaning " << (type==binaryClauses ? "binaryClauses" : "normal clauses" ) << endl;
    #endif //VERBOSE_DEBUG

    vector<Clause*>::iterator s, ss, end;
    for (s = ss = cs.begin(), end = cs.end();  s != end; s++) {
        if (s+1 != end)
            __builtin_prefetch(*(s+1));

        if (cleanClause(*s)) {
            control->clAllocator->clauseFree(*s);
        } else {
            *ss++ = *s;
        }
    }
    cs.resize(cs.size() - (s-ss));

    lastNumUnitaryClean[type] = control->getNumUnitaries();

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
        lbool val = control->value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (val == l_True) {
            control->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);
            return true;
        }
    }
    c.shrink(i-j);

    assert(c.size() > 1);
    if (i != j) {
        if (c.size() == 2) {
            control->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);
            control->attachBinClause(c[0], c[1], c.learnt());
            return true;
        } else if (c.size() == 3) {
            control->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);
            control->attachClause(c);
        } else {
            if (c.learnt())
                control->learntsLits -= i-j;
            else
                control->clausesLits -= i-j;
        }
    }

    return false;
}

bool ClauseCleaner::satisfied(const Clause& c) const
{
    for (uint32_t i = 0; i != c.size(); i++)
        if (control->value(c[i]) == l_True)
            return true;
        return false;
}

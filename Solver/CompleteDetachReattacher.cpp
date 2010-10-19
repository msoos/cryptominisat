/**************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

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

#include "CompleteDetachReattacher.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"

CompleteDetachReatacher::CompleteDetachReatacher(Solver& _solver) :
    solver(_solver)
{
}

/**
@brief Completely detach all clauses
*/
void CompleteDetachReatacher::detachNonBins()
{
    solver.clauses_literals = 0;
    solver.learnts_literals = 0;

    std::pair<uint64_t, uint64_t> tmp1, tmp2;
    for (uint32_t i = 0; i < solver.nVars(); i++) {
        tmp1 = clearWatchNotBin(solver.watches[i*2]);
        tmp2 = clearWatchNotBin(solver.watches[i*2+1]);
    }

    solver.learnts_literals += tmp1.first + tmp2.first;
    solver.clauses_literals += tmp1.second + tmp2.second;
}

/**
@brief Helper function for detachPointerUsingClauses()
*/
const std::pair<uint32_t, uint32_t> CompleteDetachReatacher::clearWatchNotBin(vec<Watched>& ws)
{
    uint32_t numRemainNonLearnt = 0;
    uint32_t numRemainLearnt = 0;

    Watched* i = ws.getData();
    Watched* j = i;
    for (Watched *end = ws.getDataEnd(); i != end; i++) {
        if (i->isBinary()) {
            *j++ = *i;
        } else {
            if (i->getLearnt()) numRemainLearnt++;
            else numRemainNonLearnt++;
        }
    }
    ws.shrink_(i-j);

    return std::make_pair(numRemainLearnt, numRemainNonLearnt);
}

/**
@brief Completely attach all clauses
*/
const bool CompleteDetachReatacher::reattachNonBins()
{
    assert(solver.ok);

    cleanAndAttachClauses(solver.clauses);
    cleanAndAttachClauses(solver.learnts);
    cleanAndAttachClauses(solver.xorclauses);
    solver.clauseCleaner->removeSatisfiedBins();

    if (solver.ok) solver.ok = (solver.propagate().isNULL());

    return solver.ok;
}

/**
@brief Cleans clauses from failed literals/removes satisfied clauses from cs

May change solver.ok to FALSE (!)
*/
inline void CompleteDetachReatacher::cleanAndAttachClauses(vec<Clause*>& cs)
{
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end = cs.getDataEnd(); i != end; i++) {
        if (cleanClause(*i)) {
            solver.attachClause(**i);
            if ((**i).size() == 2) {
                solver.numBins++;
            } else
                *j++ = *i;
        } else {
            solver.clauseAllocator.clauseFree(*i);
        }
    }
    cs.shrink(i-j);
}

/**
@brief Cleans clauses from failed literals/removes satisfied clauses from cs
*/
inline void CompleteDetachReatacher::cleanAndAttachClauses(vec<XorClause*>& cs)
{
    XorClause **i = cs.getData();
    XorClause **j = i;
    for (XorClause **end = cs.getDataEnd(); i != end; i++) {
        if (cleanClause(**i)) {
            solver.attachClause(**i);
            *j++ = *i;
        } else {
            solver.clauseAllocator.clauseFree(*i);
        }
    }
    cs.shrink(i-j);
}

/**
@brief Not only cleans a clause from false literals, but if clause is satisfied, it reports it
*/
inline const bool CompleteDetachReatacher::cleanClause(Clause*& cl)
{
    Clause& ps = *cl;
    Lit *i = ps.getData();
    Lit *j = i;
    for (Lit *end = ps.getDataEnd(); i != end; i++) {
        if (solver.value(*i) == l_True) return false;
        if (solver.value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);

    switch (ps.size()) {
        case 0:
            solver.ok = false;
            return false;
        case 1:
            solver.uncheckedEnqueue(ps[0]);
            return false;

        case 2: if (i != j) {
            solver.attachBinClause(ps[0], ps[1], ps.learnt());
            return false;
        }

        default:;
    }

    return true;
}

/**
@brief Not only cleans a clause from false literals, but if clause is satisfied, it reports it
*/
inline const bool CompleteDetachReatacher::cleanClause(XorClause& ps)
{
    Lit *i = ps.getData(), *j = i;
    for (Lit *end = ps.getDataEnd(); i != end; i++) {
        if (solver.assigns[i->var()] == l_True) ps.invert(true);
        if (solver.assigns[i->var()] == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);

    switch (ps.size()) {
        case 0:
            if (ps.xorEqualFalse() == false) solver.ok = false;
            return false;
        case 1:
            solver.uncheckedEnqueue(Lit(ps[0].var(), ps.xorEqualFalse()));
            return false;

        case 2: {
            ps[0] = ps[0].unsign();
            ps[1] = ps[1].unsign();
            solver.varReplacer->replace(ps, ps.xorEqualFalse(), ps.getGroup());
            return false;
        }

        default:;
    }

    return true;
}

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

CompleteDetachReatacher::CompleteDetachReatacher(Solver& _solver) :
    solver(_solver)
{
}

void CompleteDetachReatacher::completelyDetach()
{
    solver.clauses_literals = 0;
    solver.learnts_literals = 0;
    for (uint32_t i = 0; i < solver.nVars(); i++) {
        solver.watches[i*2].clear();
        solver.watches[i*2+1].clear();
    }
}

void CompleteDetachReatacher::detachPointerUsingClauses()
{
    solver.clauses_literals = 0;
    solver.learnts_literals = 0;
    for (uint32_t i = 0; i < solver.nVars(); i++) {
        clearWatchOfPointerUsers(solver.watches[i*2]);
        clearWatchOfPointerUsers(solver.watches[i*2+1]);
    }
}

void CompleteDetachReatacher::clearWatchOfPointerUsers(vec<Watched>& ws)
{
    Watched* i = ws.getData();
    Watched* j = i;
    for (Watched *end = ws.getDataEnd(); i != end; i++) {
        if (i->isBinary() || i->isTriClause()) {
            *j++ = *i;
        }
    }
    ws.shrink_(i-j);
}

const bool CompleteDetachReatacher::completelyReattach()
{
    assert(solver.ok);

    cleanAndAttachClauses(solver.binaryClauses, true);
    cleanAndAttachClauses(solver.clauses, false);
    cleanAndAttachClauses(solver.learnts, false);

    solver.varReplacer->reattachInternalClauses();
    cleanAndAttachClauses(solver.xorclauses);

    if (solver.ok) solver.ok = (solver.propagate().isNULL());

    return solver.ok;
}

inline void CompleteDetachReatacher::cleanAndAttachClauses(vec<Clause*>& cs, const bool lookingThroughBinary)
{
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end = cs.getDataEnd(); i != end; i++) {
        if (cleanClause(*i)) {
            solver.attachClause(**i);
            if (!lookingThroughBinary && (**i).size() == 2)
                solver.binaryClauses.push(*i);
            else
                *j++ = *i;
        } else {
            solver.clauseAllocator.clauseFree(*i);
        }
    }
    cs.shrink(i-j);
}

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
            Clause *c2 = solver.clauseAllocator.Clause_new(ps, ps.getGroup(), ps.learnt());
            solver.becameBinary++;
            solver.clauseAllocator.clauseFree(cl);
            cl = c2;
        }

        default:;
    }

    return true;
}

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

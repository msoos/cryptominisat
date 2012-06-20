/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "cmsat/CompleteDetachReattacher.h"
#include "cmsat/VarReplacer.h"
#include "cmsat/ClauseCleaner.h"

using namespace CMSat;

CompleteDetachReatacher::CompleteDetachReatacher(Solver& _solver) :
    solver(_solver)
{
}

/**
@brief Completely detach all non-binary clauses
*/
void CompleteDetachReatacher::detachNonBinsNonTris(const bool removeTri)
{
    uint32_t oldNumBins = solver.numBins;
    ClausesStay stay;

    for (vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++) {
        stay += clearWatchNotBinNotTri(*it, removeTri);
    }

    solver.learnts_literals = stay.learntBins;
    solver.clauses_literals = stay.nonLearntBins;
    solver.numBins = (stay.learntBins + stay.nonLearntBins)/2;
    release_assert(solver.numBins == oldNumBins);
}

/**
@brief Helper function for detachPointerUsingClauses()
*/
const CompleteDetachReatacher::ClausesStay CompleteDetachReatacher::clearWatchNotBinNotTri(vec<Watched>& ws, const bool removeTri)
{
    ClausesStay stay;

    vec<Watched>::iterator i = ws.getData();
    vec<Watched>::iterator j = i;
    for (vec<Watched>::iterator end = ws.getDataEnd(); i != end; i++) {
        if (i->isBinary()) {
            if (i->getLearnt()) stay.learntBins++;
            else stay.nonLearntBins++;
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
    assert(solver.ok);

    cleanAndAttachClauses(solver.clauses);
    cleanAndAttachClauses(solver.learnts);
    cleanAndAttachClauses(solver.xorclauses);
    solver.clauseCleaner->removeSatisfiedBins();

    if (solver.ok) solver.ok = (solver.propagate<false>().isNULL());

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
    PolaritySorter sorter(solver.polarity);
    for (Clause **end = cs.getDataEnd(); i != end; i++) {
        std::sort((*i)->getData(), (*i)->getDataEnd(), sorter);
        if (cleanClause(*i)) {
            solver.attachClause(**i);
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
inline bool CompleteDetachReatacher::cleanClause(Clause*& cl)
{
    Clause& ps = *cl;
    assert(ps.size() > 2);

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

        case 2: {
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
inline bool CompleteDetachReatacher::cleanClause(XorClause& ps)
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
            solver.varReplacer->replace(ps, ps.xorEqualFalse());
            return false;
        }

        default:;
    }

    return true;
}

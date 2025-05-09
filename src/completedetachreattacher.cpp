/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "completedetachreattacher.h"
#include "constants.h"
#include "solver.h"
#include "sqlstats.h"
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
void CompleteDetachReatacher::detach_nonbins()
{
    assert(!solver->frat->something_delayed());
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
}

/**
@brief Helper function for detachPointerUsingClauses()
*/
CompleteDetachReatacher::ClausesStay CompleteDetachReatacher::clearWatchNotBinNotTri(
    watch_subarray ws
) {
    ClausesStay stay;

    Watched* i = ws.begin();
    Watched* j = i;
    for (Watched* end = ws.end(); i != end; i++) {
        if (i->isBin()) {
            if (i->red())
                stay.redBins++;
            else
                stay.irredBins++;

            *j++ = *i;
        }
    }
    ws.shrink_(i-j);

    return stay;
}

bool CompleteDetachReatacher::reattachLongs(bool removeStatsFirst) {
    verb_print(6, "Cleaning and reattaching clauses");
    cleanAndAttachClauses(solver->longIrredCls, removeStatsFirst);
    for(auto& lredcls: solver->longRedCls) cleanAndAttachClauses(lredcls, removeStatsFirst);
    solver->clauseCleaner->clean_implicit_clauses();
    assert(!solver->frat->something_delayed());

    if (solver->okay()) solver->ok = (solver->propagate<true>().isnullptr());
    solver->attach_xorclauses();
    return solver->okay();
}

void CompleteDetachReatacher::attachClauses( vector<ClOffset>& cs) {
    for (ClOffset offs: cs) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        bool satisfied = false;
        for(Lit lit: *cl) if (solver->value(lit) == l_True) satisfied = true;
        if (!satisfied) {
            assert(solver->value((*cl)[0]) == l_Undef);
            assert(solver->value((*cl)[1]) == l_Undef);
        }
        solver->attachClause(*cl, false);
    }
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
    for (vector<ClOffset>::iterator end = cs.end(); i != end; ++i) {
        assert(!solver->frat->something_delayed());
        Clause* cl = solver->cl_alloc.ptr(*i);

        //Handle stat removal if need be
        if (removeStatsFirst) {
            if (cl->red()) {
                solver->litStats.redLits -= cl->size();
            } else {
                solver->litStats.irredLits -= cl->size();
            }
        }

        if (clean_clause(cl)) {
            solver->attachClause(*cl);
            *j++ = *i;
        } else {
            solver->free_cl(*i);
        }
    }
    cs.resize(cs.size() - (i-j));
}

/**
@brief Not only cleans a clause from false literals, but if clause is satisfied, it reports it
*/
bool CompleteDetachReatacher::clean_clause(Clause* cl)
{
    (*solver->frat) << deldelay << *cl << fin;
    Clause& ps = *cl;
    if (ps.size() <= 2) {
        cout
        << "ERROR, clause is too small, and linked in: "
        << *cl
        << endl;
    }
    assert(ps.size() > 2);

    Lit *i = ps.begin();
    Lit *j = i;
    for (Lit *end = ps.end(); i != end; i++) {
        if (solver->value(*i) == l_True) {
            (*solver->frat) << findelay;
            return false;
        }
        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);

    //Drat
    if (i != j) {
        INC_ID(*cl);
        (*solver->frat) << add << *cl << fin << findelay;
    } else {
        solver->frat->forget_delay();
    }

    switch (ps.size()) {
        case 0:
            set_unsat_cl_id(cl->stats.id);
            solver->ok = false;
            return false;

        case 1:
            solver->enqueue<true>(ps[0]);
            (*solver->frat) << del << *cl << fin; //double unit delete
            return false;

        case 2: {
            solver->attach_bin_clause(ps[0], ps[1], ps.red(), cl->stats.id);
            return false;
        }

        default: {
            break;
        }
    }

    return true;
}

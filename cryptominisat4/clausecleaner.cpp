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

#include "clausecleaner.h"
#include "clauseallocator.h"
#include "solver.h"

using namespace CMSat;

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
    if (solver->value(watched.lit2()) == l_True) return true;
    return false;
}

void ClauseCleaner::clean_binary_implicit(
    Watched& ws
    , watch_subarray::iterator& j
    , const Lit lit
) {
    if (satisfied(ws, lit)) {
        //Only delete once
        if (lit < ws.lit2()) {
            (*solver->drup) << del << lit << ws.lit2() << fin;
        }

        if (ws.red()) {
            impl_data.remLBin++;
        } else {
            impl_data.remNonLBin++;
        }
    } else {
        assert(solver->value(ws.lit2()) == l_Undef);
        assert(solver->value(lit) == l_Undef);
        *j++ = ws;
    }
}

void ClauseCleaner::clean_tertiary_implicit(
    Watched& ws
    , watch_subarray::iterator& j
    , const Lit lit
) {
    bool remove = false;

    //Satisfied?
    if (solver->value(lit) == l_True
        || solver->value(ws.lit2()) == l_True
        || solver->value(ws.lit3()) == l_True
    ) {
        remove = true;
    }

    //Shortened -- attach bin, but only *once*
    Lit lits[2];
    bool needAttach = false;
    if (!remove
        && solver->value(lit) == l_False
    ) {
        if (lit < ws.lit2()) {
            lits[0] = ws.lit2();
            lits[1] = ws.lit3();
            needAttach = true;
        }
        remove = true;
    }
    if (!remove
        && solver->value(ws.lit2()) == l_False
    ) {
        if (lit < ws.lit2()) {
            lits[0] = lit;
            lits[1] = ws.lit3();
            needAttach = true;
        }
        remove = true;
    }
    if (!remove
        && solver->value(ws.lit3()) == l_False
    ) {
        if (lit < ws.lit2()) {
            lits[0] = lit;
            lits[1] = ws.lit2();
            needAttach = true;
        }
        remove = true;
    }
    if (needAttach) {
        impl_data.toAttach.push_back(BinaryClause(lits[0], lits[1], ws.red()));
        (*solver->drup) << lits[0] << lits[1] << fin;
    }

    if (remove) {
        //Drup
        if (//Only remove once --> exactly when adding
            lit < ws.lit2()
            && ws.lit2() < ws.lit3()
        ) {
            (*solver->drup)
            << del << lit << ws.lit2() << ws.lit3() << fin;
        }

        if (ws.red())
            impl_data.remLTri++;
        else
            impl_data.remNonLTri++;
    } else {
        *j++ = ws;
    }
}

void ClauseCleaner::clean_implicit_watchlist(
    watch_subarray& watch_list
    , const Lit lit
) {
    watch_subarray::iterator i = watch_list.begin();
    watch_subarray::iterator j = i;
    for (watch_subarray::iterator end2 = watch_list.end(); i != end2; i++) {
        if (i->isClause()) {
            *j++ = *i;
            continue;
        }
        assert(!solver->drup->something_delayed());

        if (i->isBinary()) {
            clean_binary_implicit(*i, j, lit);
            continue;
        }

        assert(i->isTri());
        clean_tertiary_implicit(*i, j, lit);
    }
    watch_list.shrink_(i - j);
}

void ClauseCleaner::clean_implicit_clauses()
{
    assert(!solver->drup->something_delayed());
    assert(solver->decisionLevel() == 0);
    impl_data = ImplicitData();
    size_t wsLit = 0;
    for (size_t end = solver->watches.size()
        ; wsLit != end
        ; wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        watch_subarray ws = solver->watches[lit.toInt()];
        if (ws.empty())
            continue;

        clean_implicit_watchlist(ws, lit);
    }
    impl_data.update_solver_stats(solver);

    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif
}

void ClauseCleaner::cleanClauses(vector<ClOffset>& cs)
{
    assert(!solver->drup->something_delayed());
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());

    #ifdef VERBOSE_DEBUG
    cout << "Cleaning  clauses" << endl;
    #endif //VERBOSE_DEBUG

    vector<ClOffset>::iterator s, ss, end;
    size_t at = 0;
    for (s = ss = cs.begin(), end = cs.end();  s != end; s++, at++) {
        if (at + 1 < cs.size()) {
            Clause* cl = solver->clAllocator.getPointer(cs[at+1]);
            __builtin_prefetch(cl);
        }
        if (cleanClause(*s)) {
            solver->clAllocator.clauseFree(*s);
        } else {
            *ss++ = *s;
        }
    }
    cs.resize(cs.size() - (s-ss));

    #ifdef VERBOSE_DEBUG
    cout << "cleanClauses(Clause) useful ?? Removed: " << s-ss << endl;
    #endif
}

inline bool ClauseCleaner::cleanClause(ClOffset offset)
{
    assert(!solver->drup->something_delayed());
    Clause& cl = *solver->clAllocator.getPointer(offset);
    assert(cl.size() > 3);
    const uint32_t origSize = cl.size();

    (*solver->drup) << deldelay << cl << fin;
    Lit origLit1 = cl[0];
    Lit origLit2 = cl[1];

    Lit *i, *j, *end;
    uint32_t num = 0;
    for (i = j = cl.begin(), end = i + cl.size();  i != end; i++, num++) {
        lbool val = solver->value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (val == l_True) {
            solver->detachModifiedClause(origLit1, origLit2, origSize, &cl);
            (*solver->drup) << findelay;
            return true;
        }
    }
    if (i != j) {
        cl.shrink(i-j);
        (*solver->drup) << cl << fin << findelay;
    } else {
        solver->drup->forget_delay();
    }

    assert(cl.size() > 1);
    if (i != j) {
        if (cl.size() == 2) {
            solver->detachModifiedClause(origLit1, origLit2, origSize, &cl);
            solver->attachBinClause(cl[0], cl[1], cl.red());
            return true;
        } else if (cl.size() == 3) {
            solver->detachModifiedClause(origLit1, origLit2, origSize, &cl);
            solver->attachTriClause(cl[0], cl[1], cl[2], cl.red());
            return true;
        } else {
            if (cl.red())
                solver->litStats.redLits -= i-j;
            else
                solver->litStats.irredLits -= i-j;
        }
    }

    return false;
}

bool ClauseCleaner::satisfied(const Clause& cl) const
{
    for (uint32_t i = 0; i != cl.size(); i++)
        if (solver->value(cl[i]) == l_True)
            return true;
        return false;
}

void ClauseCleaner::ImplicitData::update_solver_stats(Solver* solver)
{
    for(const BinaryClause& bincl: toAttach) {
        assert(solver->value(bincl.getLit1()) == l_Undef);
        assert(solver->value(bincl.getLit2()) == l_Undef);
        solver->attachBinClause(bincl.getLit1(), bincl.getLit2(), bincl.isRed());
    }

    assert(remNonLBin % 2 == 0);
    assert(remLBin % 2 == 0);
    assert(remNonLTri % 3 == 0);
    assert(remLTri % 3 == 0);
    solver->binTri.irredBins -= remNonLBin/2;
    solver->binTri.redBins -= remLBin/2;
    solver->binTri.irredTris -= remNonLTri/3;
    solver->binTri.redTris -= remLTri/3;
}

void ClauseCleaner::removeAndCleanAll()
{
    double myTime = cpuTime();
    clean_implicit_clauses();
    cleanClauses(solver->longIrredCls);
    cleanClauses(solver->longRedCls);

    #ifndef NDEBUG
    //Once we have cleaned the watchlists
    //no watchlist whose lit is set may be non-empty
    size_t wsLit = 0;
    for(watch_array::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        if (solver->value(lit) != l_Undef) {
            assert((*it).empty());
        }
    }
    #endif

    if (solver->conf.verbosity >= 2) {
        cout
        << "c [clean] T: "
        << std::fixed << std::setprecision(4)
        << (cpuTime() - myTime)
        << " s" << endl;
    }
}
/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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
#include "cryptominisat4/solvertypesmini.h"

using namespace CMSat;

//#define DEBUG_CLEAN
//#define VERBOSE_DEBUG

ClauseCleaner::ClauseCleaner(Solver* _solver) :
    solver(_solver)
{
}

bool ClauseCleaner::satisfied(const Watched& watched, Lit lit)
{
    assert(watched.isBin());
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
            (*solver->drat) << del << lit << ws.lit2() << fin;
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
        (*solver->drat) << lits[0] << lits[1] << fin;
    }

    if (remove) {
        //Drat
        if (//Only remove once --> exactly when adding
            lit < ws.lit2()
            && ws.lit2() < ws.lit3()
        ) {
            (*solver->drat)
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
        assert(!solver->drat->something_delayed());

        if (i->isBin()) {
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
    assert(!solver->drat->something_delayed());
    assert(solver->decisionLevel() == 0);
    impl_data = ImplicitData();
    size_t wsLit = 0;
    size_t wsLit2 = 2;
    for (size_t end = solver->watches.size()
        ; wsLit != end
        ; wsLit++, wsLit2++
    ) {
        if (wsLit2 < end
            && !solver->watches[Lit::toLit(wsLit2)].empty()
        ) {
            solver->watches.prefetch(Lit::toLit(wsLit2).toInt());
        }

        const Lit lit = Lit::toLit(wsLit);
        watch_subarray ws = solver->watches[lit];
        if (ws.empty())
            continue;

        clean_implicit_watchlist(ws, lit);
    }
    impl_data.update_solver_stats(solver);

    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif
}

void ClauseCleaner::clean_clauses(vector<ClOffset>& cs)
{
    clean_clauses_pre();
    clean_clauses_inter(cs);
    clean_clauses_post();
}

void ClauseCleaner::clean_clauses_inter(vector<ClOffset>& cs)
{
    assert(!solver->drat->something_delayed());
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());

    #ifdef VERBOSE_DEBUG
    cout << "Cleaning  clauses" << endl;
    #endif //VERBOSE_DEBUG

    vector<ClOffset>::iterator s, ss, end;
    size_t at = 0;
    for (s = ss = cs.begin(), end = cs.end();  s != end; ++s, ++at) {
        if (at + 1 < cs.size()) {
            Clause* pre_cl = solver->cl_alloc.ptr(cs[at+1]);
            __builtin_prefetch(pre_cl);
        }

        const ClOffset off = *s;
        Clause& cl = *solver->cl_alloc.ptr(off);

        const Lit origLit1 = cl[0];
        const Lit origLit2 = cl[1];
        const auto origSize = cl.size();
        const bool red = cl.red();

        if (clean_clause(cl)) {
            solver->watches.smudge(origLit1);
            solver->watches.smudge(origLit2);
            cl.setRemoved();
            if (red) {
                solver->litStats.redLits -= origSize;
                if (solver->red_long_cls_is_reducedb(cl)) {
                    solver->num_red_cls_reducedb--;
                }
            } else {
                solver->litStats.irredLits -= origSize;
            }
            delayed_free.push_back(off);
        } else {
            *ss++ = *s;
        }
    }
    cs.resize(cs.size() - (s-ss));
}

inline bool ClauseCleaner::clean_clause(Clause& cl)
{
    assert(!solver->drat->something_delayed());
    assert(cl.size() > 3);
    (*solver->drat) << deldelay << cl << fin;


    Lit *i, *j, *end;
    uint32_t num = 0;
    for (i = j = cl.begin(), end = i + cl.size();  i != end; i++, num++) {
        lbool val = solver->value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (val == l_True) {
            (*solver->drat) << findelay;
            return true;
        }
    }
    if (i != j) {
        cl.shrink(i-j);
        (*solver->drat) << cl << fin << findelay;
    } else {
        solver->drat->forget_delay();
    }

    assert(cl.size() > 1);
    if (i != j) {
        if (cl.size() == 2) {
            solver->attach_bin_clause(cl[0], cl[1], cl.red());
            return true;
        } else if (cl.size() == 3) {
            solver->attach_tri_clause(cl[0], cl[1], cl[2], cl.red());
            return true;
        } else {
            if (cl.red()) {
                solver->litStats.redLits -= i-j;
            } else {
                solver->litStats.irredLits -= i-j;
            }
            assert(solver->value(cl[0]) == l_Undef);
            assert(solver->value(cl[1]) == l_Undef);
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
        solver->attach_bin_clause(bincl.getLit1(), bincl.getLit2(), bincl.isRed());
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

void ClauseCleaner::clean_clauses_pre()
{
    assert(solver->watches.get_smudged_list().empty());
    assert(delayed_free.empty());
}

void ClauseCleaner::clean_clauses_post()
{
    solver->clean_occur_from_removed_clauses_only_smudged();
    for(ClOffset off: delayed_free) {
        solver->cl_alloc.clauseFree(off);
    }
    delayed_free.clear();
}

void ClauseCleaner::remove_and_clean_all()
{
    double myTime = cpuTime();

    clean_implicit_clauses();

    clean_clauses_pre();
    clean_clauses_inter(solver->longIrredCls);
    clean_clauses_inter(solver->longRedCls);
    clean_clauses_post();


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


bool ClauseCleaner::clean_one_xor(Xor& x)
{
    bool rhs = x.rhs;
    size_t i = 0;
    size_t j = 0;
    for(size_t size = x.size(); i < size; i++) {
        uint32_t var = x[i];
        if (solver->value(var) != l_Undef) {
            rhs ^= solver->value(var) == l_True;
        } else {
            x[j++] = var;
        }
    }
    x.resize(j);
    x.rhs = rhs;

    switch(x.size()) {
        case 0:
            solver->ok &= !x.rhs;
            return false;

        case 1: {
            solver->fully_enqueue_this(Lit(x[0], !x.rhs));
            return false;
        }
        case 2: {
            solver->add_xor_clause_inter(vars_to_lits(x), x.rhs, true);
            return false;
        }
        default: {
            return true;
            break;
        }
    }
}

bool ClauseCleaner::clean_xor_clauses(vector<Xor>& xors)
{
    assert(solver->ok);
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") Cleaning gauss clauses" << endl;
    for(Xor& x : xors) {
        cout << "orig XOR: " << x << endl;
    }
    #endif

    size_t i = 0;
    size_t j = 0;
    for(size_t size = xors.size(); i < size; i++) {
        Xor& x = xors[i];
        const bool keep = clean_one_xor(x);
        if (!solver->ok) {
            return false;
        }

        if (keep) {
            xors[j++] = x;
        }
    }
    xors.resize(j);

    #ifdef VERBOSE_DEBUG
    for(Xor& x : xors) {
        cout << "cleaned XOR: " << x << endl;
    }
    #endif
    return solver->ok;
}

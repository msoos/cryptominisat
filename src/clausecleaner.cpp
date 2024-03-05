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

#include "clausecleaner.h"
#include "clauseallocator.h"
#include "solver.h"
#include "sqlstats.h"
#include "solvertypesmini.h"

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
    const Watched* i
    , Watched*& j
    , const Lit lit
) {
    if (satisfied(*i, lit)) {
        //Only delete once
        if (lit < i->lit2()) {
            (*solver->frat) << del << i->get_ID() << lit << i->lit2() << fin;
        }

        if (i->red()) {
            impl_data.remLBin++;
        } else {
            impl_data.remNonLBin++;
        }
    } else {
        #ifdef SLOW_DEBUG
        if (solver->value(i->lit2()) != l_Undef
            || solver->value(lit) != l_Undef
        ) {
            cout << "ERROR binary during cleaning has non-l-Undef "
            << " Bin clause: " << lit << " " << i->lit2() << endl
            << " values: " << solver->value(lit)
            << " " << solver->value(i->lit2())
            << endl;
        }
        #endif

        assert(solver->value(i->lit2()) == l_Undef);
        assert(solver->value(lit) == l_Undef);
        *j++ = *i;
    }
}

void ClauseCleaner::clean_implicit_watchlist(
    watch_subarray& watch_list
    , const Lit lit
) {
    Watched* i = watch_list.begin();
    Watched* j = i;
    for (Watched* end2 = watch_list.end(); i != end2; i++) {
        if (i->isClause() || i->isBNN()) {
            *j++ = *i;
            continue;
        }

        if (i->isBin()) {
            clean_binary_implicit(i, j, lit);
            continue;
        }
    }
    watch_list.shrink_(i - j);
}

void ClauseCleaner::clean_implicit_clauses()
{
    if (solver->conf.verbosity > 15) {
        cout << "c cleaning implicit clauses" << endl;
    }

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

//return True if it's to be removed.
bool ClauseCleaner::clean_bnn(BNN& bnn, uint32_t bnn_idx) {
    if (solver->conf.verbosity > 15) {
        cout << "Cleaning BNN: " << bnn << endl;
    }

    uint32_t i = 0;
    uint32_t j = 0;
    for(; i < bnn.size(); i++) {
        Lit l = bnn[i];
        if (solver->value(l) == l_Undef) {
            bnn[j++] = bnn[i];
            continue;
        }
        removeWBNN(solver->watches, l, bnn_idx);
        removeWBNN(solver->watches, ~l, bnn_idx);

        if (solver->value(l) == l_False) {
            //nothing
        } else if (solver->value(l) == l_True) {
            bnn.cutoff--;
        }
    }
    bnn.resize(j);

    if (!bnn.set && solver->value(bnn.out) != l_Undef) {
        removeWBNN(solver->watches, bnn.out, bnn_idx);
        removeWBNN(solver->watches, ~bnn.out, bnn_idx);
        if (solver->value(bnn.out) == l_False) {
            for (auto& l: bnn) {
                l = ~l;
            }
            bnn.cutoff = (int32_t)bnn.size()+1-bnn.cutoff;
        }
        bnn.set = true;
        bnn.out = lit_Undef;
    }

    lbool ret = solver->bnn_eval(bnn);
    if (ret != l_Undef) {
        if (ret == l_False) {
            assert(false && "Not handled yet, but it's possible!!");
            solver->ok = false;
            return true;
        }
        //remove
        return true;
    }

    //translate into clauses
    if (solver->bnn_to_cnf(bnn)) {
        return true;
    }

    //cannot be removed
    return false;
}

void ClauseCleaner::clean_bnns_inter(vector<BNN*>& bnns)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());

    if (solver->conf.verbosity > 15) {
        cout << "Cleaning BNNs" << endl;
    }

    for (uint32_t i = 0; i < bnns.size() && solver->okay(); i++) {
        BNN* bnn = solver->bnns[i];
        if (!bnn || bnn->isRemoved)
            continue;

        if (clean_bnn(*bnn, i)) {
            for(const auto& l: *bnn) {
                solver->watches.smudge(l);
                solver->watches.smudge(~l);
            }
            if (bnn->out != lit_Undef) {
                solver->watches.smudge(bnn->out);
                solver->watches.smudge(~bnn->out);
            }
            bnn->isRemoved = true;
//             cout << "Removed BNN" << endl;
        }
        bnn->undefs = bnn->size();
        bnn->ts = 0;
    }
}

void ClauseCleaner::clean_clauses_inter(vector<ClOffset>& cs)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());

    if (solver->conf.verbosity > 15) {
        cout << "Cleaning clauses in vector<>" << endl;
    }

    vector<ClOffset>::iterator s, ss, end;
    size_t at = 0;
    for (s = ss = cs.begin(), end = cs.end();  s != end; ++s, ++at) {
        if (at + 1 < cs.size()) {
            Clause* pre_cl = solver->cl_alloc.ptr(cs[at+1]);
            cmsat_prefetch(pre_cl);
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

bool ClauseCleaner::clean_clause(Clause& cl)
{
    //Don't clean if detached. We'll deal with it during re-attach.
    if (cl._xor_is_detached) {
        return false;
    }

    assert(cl.size() > 2);
    (*solver->frat) << deldelay << cl << fin;
    solver->chain.clear();

    #ifdef SLOW_DEBUG
    uint32_t num_false_begin = 0;
    Lit l1 = cl[0];
    Lit l2 = cl[1];
    num_false_begin += solver->value(cl[0]) == l_False;
    num_false_begin += solver->value(cl[1]) == l_False;
    #endif

    Lit *i, *j, *end;
    uint32_t num = 0;
    for (i = j = cl.begin(), end = i + cl.size();  i != end; i++, num++) {
        lbool val = solver->value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (val == l_True) {
            (*solver->frat) << findelay;
            return true;
        } else {
            solver->chain.push_back(solver->unit_cl_IDs[i->var()]);
        }
    }

    if (i != j) {
        const auto orig_ID = cl.stats.ID;
        INC_ID(cl);
        cl.shrink(i-j);
        (*solver->frat) << add << cl << chain << orig_ID;
        for(auto const& id: solver->chain) (*solver->frat) << id;
        (*solver->frat) << fin << findelay;
    } else {
        solver->frat->forget_delay();
    }

    assert(cl.size() != 0);
    assert(cl.size() != 1);
    assert(cl.size() > 1);
    assert(solver->value(cl[0]) == l_Undef);
    assert(solver->value(cl[1]) == l_Undef);

    #ifdef SLOW_DEBUG
    //no l_True, so first 2 of orig must have been l_Undef
    if (num_false_begin != 0) {
        cout << "val " << l1 << ":" << solver->value(l1) << endl;
        cout << "val " << l2 << ":" << solver->value(l2) << endl;
    }
    assert(num_false_begin == 0 && "Propagation wasn't full? Watch lit was l_False and clause wasn't satisfied");
    #endif

    if (i != j) {
        cl.setStrenghtened();
        if (cl.size() == 2) {
            solver->attach_bin_clause(cl[0], cl[1], cl.red(), cl.stats.ID);
            return true;
        } else {
            if (cl.red()) {
                solver->litStats.redLits -= i-j;
            } else {
                solver->litStats.irredLits -= i-j;
            }
        }
    }

    return false;
}

void ClauseCleaner::ImplicitData::update_solver_stats(Solver* solver)
{
    for(const BinaryClause& bincl: toAttach) {
        assert(solver->value(bincl.getLit1()) == l_Undef);
        assert(solver->value(bincl.getLit2()) == l_Undef);
        solver->attach_bin_clause(bincl.getLit1(),
                                  bincl.getLit2(),
                                  bincl.isRed(),
                                  bincl.getID());
    }

    assert(remNonLBin % 2 == 0);
    assert(remLBin % 2 == 0);
    solver->binTri.irredBins -= remNonLBin/2;
    solver->binTri.redBins -= remLBin/2;
}

void ClauseCleaner::clean_clauses_pre()
{
    assert(solver->watches.get_smudged_list().empty());
    assert(delayed_free.empty());
}

void ClauseCleaner::clean_clauses_post()
{
    for(ClOffset off: delayed_free) {
        solver->free_cl(off);
    }
    delayed_free.clear();
}

void ClauseCleaner::clean_bnns_post()
{
    for(BNN*& bnn: solver->bnns) {
        if (bnn && bnn->isRemoved) {
            free(bnn);
            bnn = NULL;
        }
    }
}

bool ClauseCleaner::remove_and_clean_all()
{
    double myTime = cpuTime();
    assert(solver->okay());
    assert(solver->prop_at_head());
    assert(solver->decisionLevel() == 0);
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";

    size_t last_trail = numeric_limits<size_t>::max();
    while(last_trail != solver->trail_size()) {
        last_trail = solver->trail_size();
        solver->ok = solver->propagate<false>().isNULL();
        if (!solver->okay()) break;
        if (!clean_all_xor_clauses()) break;

        clean_implicit_clauses();
        clean_clauses_pre();
        clean_bnns_inter(solver->bnns);
        if (!solver->okay()) break;

        clean_clauses_inter(solver->longIrredCls);
        for(auto& lredcls: solver->longRedCls) clean_clauses_inter(lredcls);
        solver->clean_occur_from_removed_clauses_only_smudged();
        clean_clauses_post();
        clean_bnns_post();
    }

    #ifndef NDEBUG
    if (solver->okay()) {
        //Once we have cleaned the watchlists
        //no watchlist whose lit is set may be non-empty
        size_t wsLit = 0;
        for(watch_array::const_iterator
            it = solver->watches.begin(), end = solver->watches.end()
            ; it != end
            ; ++it, wsLit++
        ) {
            const Lit lit = Lit::toLit(wsLit);
            if (solver->value(lit) != l_Undef) {
                if (!it->empty()) {
                    cout << "ERROR watches size: " << it->size() << endl;
                    for(const auto& w: *it) {
                        cout << "ERROR w: " << w << endl;
                    }
                }
                assert(it->empty());
            }
        }
    }
    #endif

    verb_print(2, "[clean]" << solver->conf.print_times(cpuTime() - myTime));
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";

    return solver->okay();
}


bool ClauseCleaner::clean_one_xor(Xor& x)
{
    // they encode information (see NOTE in cnf.h) so they MUST be in BDDs
    //      otherwise FRAT will fail
    TBUDDY_DO(if (solver->frat->enabled()) assert(x.bdd));

    bool rhs = x.rhs;
    size_t i = 0;
    size_t j = 0;
    VERBOSE_PRINT("Trying to clean XOR: " << x);
    for(size_t size = x.clash_vars.size(); i < size; i++) {
        const auto& v = x.clash_vars[i];
        if (solver->value(v) == l_Undef) {
            x.clash_vars[j++] = v;
        }
    }
    x.clash_vars.resize(j);

    i = 0;
    j = 0;
    for(size_t size = x.size(); i < size; i++) {
        uint32_t var = x[i];
        if (solver->value(var) != l_Undef) {
            rhs ^= solver->value(var) == l_True;
        } else {
            x[j++] = var;
        }
    }
    if (j < x.size()) {
        x.resize(j);
        x.rhs = rhs;
        VERBOSE_PRINT("cleaned XOR: " << x);
    }

    if (x.size() <= 2) {
        solver->frat->flush();
        TBUDDY_DO(delete x.bdd);
        TBUDDY_DO(x.bdd = NULL);
    }

    switch(x.size()) {
        case 0:
            if (x.rhs == true) solver->ok = false;
            if (!solver->ok) {
                assert(solver->unsat_cl_ID == 0);
                *solver->frat << add << ++solver->clauseID << fin;
                solver->unsat_cl_ID = solver->clauseID;
            }
            return false;
        case 1: {
            assert(solver->okay());
            solver->enqueue<true>(Lit(x[0], !x.rhs));
            solver->ok = solver->propagate<true>().isNULL();
            return false;
        }
        case 2:
            assert(solver->okay());
            solver->add_xor_clause_inter(vars_to_lits(x), x.rhs, true);
            return false;
        default:
            return true;
    }
}

bool ClauseCleaner::clean_all_xor_clauses()
{
    assert(solver->okay());
    assert(solver->decisionLevel() == 0);

    size_t last_trail = numeric_limits<size_t>::max();
    while(last_trail != solver->trail_size()) {
        last_trail = solver->trail_size();
        if (!clean_xor_clauses(solver->xorclauses)) return false;
        if (!clean_xor_clauses(solver->xorclauses_unused)) return false;
        if (!clean_xor_clauses(solver->xorclauses_orig)) return false;
        solver->ok = solver->propagate<false>().isNULL();
    }

    // clean up removed_xorclauses_clash_vars
    uint32_t j = 0;
    for(uint32_t i = 0; i < solver->removed_xorclauses_clash_vars.size(); i++) {
        if (solver->value(solver->removed_xorclauses_clash_vars[i]) == l_Undef) {
            solver->removed_xorclauses_clash_vars[j++] = solver->removed_xorclauses_clash_vars[i];
        }
    }
    solver->removed_xorclauses_clash_vars.resize(j);

    return solver->okay();
}

bool ClauseCleaner::clean_xor_clauses(vector<Xor>& xors)
{
    assert(solver->ok);
    VERBOSE_DEBUG_DO(for(Xor& x : xors) cout << "orig XOR: " << x << endl);

    size_t last_trail = numeric_limits<size_t>::max();
    while(last_trail != solver->trail_size()) {
        last_trail = solver->trail_size();
        size_t i = 0;
        size_t j = 0;
        for(size_t size = xors.size(); i < size; i++) {
            Xor& x = xors[i];
            if (!solver->okay()) {
                xors[j++] = x;
                continue;
            }

            VERBOSE_PRINT("Checking to keep xor: " << x);
            const bool keep = clean_one_xor(x);
            if (keep) {
                assert(x.size() > 2);
                xors[j++] = x;
            } else {
                solver->removed_xorclauses_clash_vars.insert(
                    solver->removed_xorclauses_clash_vars.end()
                    , x.clash_vars.begin()
                    , x.clash_vars.end()
                );
                VERBOSE_PRINT("NOT keeping XOR");
            }
        }
        xors.resize(j);
        if (!solver->okay()) break;
        solver->ok = solver->propagate<false>().isNULL();
    }
    VERBOSE_PRINT("clean_xor_clauses() finished");

    return solver-> okay();
}

//returns TRUE if removed or solver is UNSAT
bool ClauseCleaner::full_clean(Clause& cl)
{
    (*solver->frat) << deldelay << cl << fin;

    Lit *i = cl.begin();
    Lit *j = i;
    for (Lit *end = cl.end(); i != end; i++) {
        if (solver->value(*i) == l_True) {
            return true;
        }

        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }

    if (i != j) {
        cl.shrink(i-j);
        INC_ID(cl);
        (*solver->frat) << add << cl << fin << findelay;
    } else {
        solver->frat->forget_delay();
        return false;
    }

    if (cl.size() == 0) {
        assert(solver->unsat_cl_ID == 0);
        solver->unsat_cl_ID = cl.stats.ID;
        solver->ok = false;
        return true;
    }

    if (cl.size() == 1) {
        solver->enqueue<true>(cl[0]);
        *solver->frat << del << cl << del; // double unit delete
        return true;
    }

    if (cl.size() == 2) {
        solver->attach_bin_clause(cl[0], cl[1], cl.red(), cl.stats.ID);
        return true;
    }

    return false;
}

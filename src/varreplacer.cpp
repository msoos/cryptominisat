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

#include "varreplacer.h"
#include "varupdatehelper.h"
#include "solver.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "clauseallocator.h"
#include "sqlstats.h"
#include "sccfinder.h"
#ifdef USE_BREAKID
#include "cms_breakid.h"
#endif

#include <iostream>
#include <iomanip>
#include <set>
using std::cout;
using std::endl;
using std::make_pair;

#ifdef VERBOSE_DEBUG
#define REPLACE_STATISTICS
#define VERBOSE_DEBUG_BIN_REPLACER
#endif

using namespace CMSat;

//#define VERBOSE_DEBUG
//#define REPLACE_STATISTICS
//#define DEBUG_BIN_REPLACER
//#define VERBOSE_DEBUG_BIN_REPLACER

VarReplacer::VarReplacer(Solver* _solver) :
    solver(_solver)
{
    scc_finder = new SCCFinder(_solver);
    ps_tmp.resize(2);
}

VarReplacer::~VarReplacer()
{
    delete scc_finder;
}

void VarReplacer::new_var(const uint32_t orig_outer)
{
    if (orig_outer == numeric_limits<uint32_t>::max()) {
        table.push_back(Lit(table.size(), false));
    }
}

void VarReplacer::check_no_replaced_var_set() const
{
    for(uint32_t var = 0; var < solver->nVarsOuter(); var++) {
        if (solver->value(var) != l_Undef) {
            if (solver->varData[var].removed != Removed::none)
            {
                cout << "ERROR: var " << var + 1 << " has removed: "
                << removed_type_to_string(solver->varData[var].removed)
                << " but is set to " << solver->value(var) << endl;
                assert(solver->varData[var].removed == Removed::none);
                exit(-1);
            }
        }
    }
}

void VarReplacer::new_vars(const size_t n)
{
    size_t oldsize = table.size();
    table.insert(table.end(), n, lit_Undef);
    for(size_t i = oldsize; i < table.size(); i++) {
        table[i] = Lit(i, false);
    }
}

void VarReplacer::save_on_var_memory()
{
}

void VarReplacer::updateVars(
    const std::vector< uint32_t >& /*outerToInter*/
    , const std::vector< uint32_t >& /*interToOuter*/
) {
    //Nothing to do, we keep OUTER in all these data structures
    //hence, it needs no update
}

void VarReplacer::printReplaceStats() const
{
    uint32_t i = 0;
    for (vector<Lit>::const_iterator
        it = table.begin(); it != table.end()
        ; ++it, i++
    ) {
        if (it->var() == i) continue;
        cout << "Replacing var " << i+1 << " with Lit " << *it << endl;
    }
}

void VarReplacer::update_vardata(
    const Lit orig
    , const Lit replaced_with
) {
    uint32_t orig_var = orig.var();
    uint32_t replaced_with_var = replaced_with.var();

    //Not replaced_with, or not replaceable, so skip
    if (orig_var == replaced_with_var
        || solver->varData[replaced_with_var].removed == Removed::elimed
    ) {
        return;
    }

    //Has already been handled previously, just skip
    if (solver->varData[orig_var].removed == Removed::replaced) {
        return;
    }

    //Okay, so unset decision, and set the other one decision
    assert(orig_var != replaced_with_var);
    solver->varData[orig_var].removed = Removed::replaced;
    assert(solver->varData[replaced_with_var].removed == Removed::none);
    assert(solver->value(replaced_with_var) == l_Undef);
    assert(orig_var <= solver->nVars() && replaced_with_var <= solver->nVars());
}

bool VarReplacer::enqueueDelayedEnqueue()
{
    for(auto& l: delayedEnqueue) {
        l.first = get_lit_replaced_with(l.first);

        if (!solver->ok) {
            //if we are UNSAT, just delete them
            *solver->frat << del << l.second << l.first << fin;
            continue;
        }

        if (solver->value(l.first) == l_Undef) {
            solver->enqueue<false>(l.first);
            *solver->frat << del << l.second << l.first << fin; // double unit delete
        } else if (solver->value(l.first) == l_False) {
            *solver->frat
            << add << ++solver->clauseID << fin
            << del << l.second << l.first << fin;
            assert(solver->unsat_cl_ID == 0);
            solver->unsat_cl_ID = solver->clauseID;
            solver->ok = false;
        } else {
            //it's already set, delete
            *solver->frat << del << l.second << l.first << fin;
        }
    }
    delayedEnqueue.clear();

    if (!solver->ok) return false;

    solver->ok = solver->propagate<false>().isNULL();
    return solver->okay();
}

void VarReplacer::attach_delayed_attach()
{
    for(Clause* c: delayed_attach_or_free) {
        if (c->size() <= 2) {
            solver->free_cl(c);
        } else {
            c->unset_removed();
            solver->attachClause(*c);
        }
    }
    delayed_attach_or_free.clear();
}

void VarReplacer::update_all_vardata()
{
    uint32_t var = 0;
    for (vector<Lit>::const_iterator
        it = table.begin(); it != table.end()
        ; ++it, var++
    ) {
        const uint32_t orig = solver->map_outer_to_inter(var);
        const Lit orig_lit = Lit(orig, false);

        const uint32_t repl = solver->map_outer_to_inter(it->var());
        const Lit repl_lit = Lit(repl, it->sign());

        update_vardata(orig_lit, repl_lit);
    }
}

bool VarReplacer::perform_replace()
{
    assert(solver->ok);
    checkUnsetSanity();
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";

    //Set up stats
    runStats.clear();
    runStats.numCalls = 1;
    const double myTime = cpuTime();
    const size_t origTrailSize = solver->trail_size();

    if (!solver->clauseCleaner->remove_and_clean_all()) {
        return false;
    }
    #ifdef DEBUG_ATTACH_MORE
    solver->test_all_clause_attached();
    #endif

    //Printing stats
    if (solver->conf.verbosity >= 5)
        printReplaceStats();

    update_all_vardata();
    check_no_replaced_var_set();

    runStats.actuallyReplacedVars = replacedVars -lastReplacedVars;
    lastReplacedVars = replacedVars;

    #ifdef DEBUG_ATTACH_MORE
    solver->test_all_clause_attached();
    #endif
    assert(solver->prop_at_head());

    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif
    build_fast_inter_replace_lookup();

    //Replace implicits
    if (!replaceImplicit()) {
        goto end;
    }

    //Replace longs
    assert(solver->watches.get_smudged_list().empty());
    assert(delayed_attach_or_free.empty());
    if (!replace_set(solver->longIrredCls)) {
        goto end;
    }
    for(auto& lredcls: solver->longRedCls) {
        if (!replace_set(lredcls)) {
            goto end;
        }
    }
    replace_bnns();

    solver->clean_occur_from_removed_clauses_only_smudged();
    attach_delayed_attach();

    //Replace XORs
    if (!replace_xor_clauses(solver->xorclauses)) goto end;
    if (!replace_xor_clauses(solver->xorclauses_orig)) goto end;
    if (!replace_xor_clauses(solver->xorclauses_unused)) goto end;

    assert(solver->gmatrices.empty() && "Cannot replace vars inside GJ elim");

    for(auto& v: solver->removed_xorclauses_clash_vars) {
        v = get_var_replaced_with_fast(v);
    }

    //While replacing the clauses
    //we cannot(for implicits) and/or shouldn't (for implicit & long cls) enqueue
    //* We cannot because we are going through a struct and we might change it
    //* We shouldn't because then non-dominators would end up in the 'trail'
    if (!enqueueDelayedEnqueue()) {
        goto end;
    }

    solver->update_assumptions_after_varreplace();
#ifdef USE_BREAKID
    if (solver->breakid) {
        solver->breakid->update_var_after_varreplace();
    }
#endif

end:
    delayed_attach_or_free.clear();
    destroy_fast_inter_replace_lookup();
    assert(solver->prop_at_head() || !solver->ok);

    //Update stats
    const double time_used = cpuTime() - myTime;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;
    runStats.cpu_time = time_used;
    globalStats += runStats;
    if (solver->conf.verbosity) {
        if (solver->conf.verbosity  >= 3)
            runStats.print(solver->nVarsOuter());
        else
            runStats.print_short(solver);
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "vrep"
            , time_used
        );
    }
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";

    if (solver->okay()) {
        #ifdef DEBUG_ATTACH_MORE
        solver->test_all_clause_attached();
        #endif
        solver->check_wrong_attach();
        #ifdef DEBUG_IMPLICIT_STATS
        solver->check_stats();
        #endif
        checkUnsetSanity();
    }
    delete_frat_cls();

    return solver->okay();
}

void VarReplacer::delete_frat_cls()
{
    for(const auto& f: bins_for_frat) {
        *solver->frat << del << std::get<0>(f) << std::get<1>(f) << std::get<2>(f) << fin;
    }
    bins_for_frat.clear();
}

bool VarReplacer::replace_one_xor_clause(Xor& x)
{
    uint32_t j = 0;
    for(uint32_t i = 0; i < x.clash_vars.size(); i++) {
        uint32_t v = x.clash_vars[i];
        uint32_t upd = get_var_replaced_with_fast(v);
        if (!solver->seen[upd]) {
            solver->seen[upd] = 1;
            x.clash_vars[j++] = upd;
        }
    }
    x.clash_vars.resize(j);
    for(auto& v: x.clash_vars) solver->seen[v] = 0;

    for(uint32_t& v: x) {
        assert(v < solver->nVars());

        Lit l = Lit(v, false);
        if (get_lit_replaced_with_fast(l) != l) {
            const Lit l2 = get_lit_replaced_with_fast(l);
            #ifdef USE_TBUDDY
            if (solver->frat->enabled()) {
                solver->frat->flush();
                ilist bin = ilist_new(2);
                ilist_resize(bin, 2);
                bin[0] = v+1;
                bin[1] = l2.var()+1;
                tbdd::xor_constraint* bdd2 = new tbdd::xor_constraint(bin, l2.sign());
                tbdd::xor_set xs;
                xs.add(*x.bdd);
                xs.add(*bdd2);
                const auto new_bdd = xs.sum();
                delete bdd2;
                delete x.bdd;
                x.bdd = new_bdd;
            }
            #endif
            x.rhs ^= l2.sign();
            v = l2.var();
            runStats.replacedLits++;
        }
    }

    solver->clean_xor_vars_no_prop(x.get_vars(), x.rhs);
    switch (x.size()) {
        case 0:
            if (x.rhs == true) solver->ok = false;
            return false;
            break;
        case 1: {
            Lit l(x[0], !x.rhs);
            *solver->frat << add << ++solver->clauseID << l << fin;
            delayedEnqueue.push_back(make_pair(l, solver->clauseID));
            return false;
            break;
        }
        default:
            return true;
            break;
    }
}

bool VarReplacer::replace_xor_clauses(vector<Xor>& xors)
{
    uint32_t j = 0;

    for(uint32_t i = 0; i < xors.size(); i++) {
        Xor& x = xors[i];
        if (replace_one_xor_clause(x)) {
            std::swap(xors[j], xors[i]);
            j++;
        } else {
            TBUDDY_DO(delete x.bdd);
            TBUDDY_DO(x.bdd = NULL);
        }
    }
    xors.resize(j);

    return solver->okay();
}

inline void VarReplacer::updateBin(
    Watched* i
    , Watched*& j
    , const Lit origLit1
    , const Lit origLit2
    , Lit lit1
    , Lit lit2
) {
    bool remove = false;

    //Two lits are the same in BIN
    if (lit1 == lit2) {
        int32_t ID = ++solver->clauseID;
        (*solver->frat) << add << ID << lit2 << fin;
        delayedEnqueue.push_back(make_pair(lit2, ID));
        remove = true;
    }

    //Tautology
    if (lit1 == ~lit2) remove = true;
    if (remove) {
        impl_tmp_stats.remove(*i);

        //Drat -- Delete only once
        if (origLit1 < origLit2) {
            (*solver->frat) << del << i->get_ID() << origLit1 << origLit2 << fin;
        }

        return;
    }

    //Drat
    if (//Changed
        (lit1 != origLit1 || lit2 != origLit2)
        //Delete&attach only once
        && (origLit1 < origLit2)
    ) {
        //WARNING TODO beware, this make post-FRAT parsing for ML fail.
        //we need a better mechanism than reloc, or we need to teach the tool reloc
        //solver->clauseID+1 is used and then unused immediately
        (*solver->frat)
        << reloc << i->get_ID() << solver->clauseID+1 << fin
        << add << i->get_ID() << lit1 << lit2 << fin
        << del << solver->clauseID+1 << origLit1 << origLit2 << fin;
    }

    if (lit1 != origLit1) {
        solver->watches[lit1].push(*i);
    } else {
        *j++ = *i;
    }
}

void VarReplacer::updateStatsFromImplStats()
{
    assert(impl_tmp_stats.removedRedBin % 2 == 0);
    solver->binTri.redBins -= impl_tmp_stats.removedRedBin/2;

    assert(impl_tmp_stats.removedIrredBin % 2 == 0);
    solver->binTri.irredBins -= impl_tmp_stats.removedIrredBin/2;

    #ifdef DEBUG_IMPLICIT_STATS
    solver->check_implicit_stats();
    #endif

    runStats.removedBinClauses += impl_tmp_stats.removedRedBin/2 + impl_tmp_stats.removedIrredBin/2;

    impl_tmp_stats.clear();
}

bool VarReplacer::replaceImplicit()
{
    impl_tmp_stats.clear();
    delayedEnqueue.clear();
    delayed_attach_bin.clear();
    assert(solver->watches.get_smudged_list().empty());

    for(size_t i = 0; i < solver->nVars()*2; i++) {
        const Lit lit = Lit::toLit(i);
        if (get_lit_replaced_with_fast(lit) != lit) {
            solver->watches.smudge(lit);
        }
    }

    for(size_t at = 0; at < solver->watches.get_smudged_list().size(); at++) {
        const Lit origLit1 = solver->watches.get_smudged_list()[at];
        watch_subarray ws = solver->watches[origLit1];

        Watched* i = ws.begin();
        Watched* j = i;
        for (Watched *end2 = ws.end(); i != end2; i++) {
            //Don't bother non-bin
            if (!i->isBin()) {
                *j++ = *i;
                continue;
            }
            runStats.bogoprops += 1;

            const Lit origLit2 = i->lit2();
            assert(solver->value(origLit1) == l_Undef);
            assert(solver->value(origLit2) == l_Undef);
            assert(origLit1.var() != origLit2.var());

            //Update main lit
            Lit lit1 = origLit1;
            if (get_lit_replaced_with_fast(lit1) != lit1) {
                lit1 = get_lit_replaced_with_fast(lit1);
                runStats.replacedLits++;
                solver->watches.smudge(origLit2);
            }

            //Update lit2
            Lit lit2 = origLit2;
            if (get_lit_replaced_with_fast(lit2) != lit2) {
                lit2 = get_lit_replaced_with_fast(lit2);
                i->setLit2(lit2);
                runStats.replacedLits++;
            }

            assert(i->isBin());
            updateBin(i, j, origLit1, origLit2, lit1, lit2);
        }
        ws.shrink_(i-j);
    }

    for(const BinaryClause& bincl : delayed_attach_bin) {
        solver->attach_bin_clause(
            bincl.getLit1(), bincl.getLit2(), bincl.isRed(), bincl.getID());
    }
    delayed_attach_bin.clear();

    #ifdef VERBOSE_DEBUG_BIN_REPLACER
    cout << "c debug bin replacer start" << endl;
    cout << "c debug bin replacer end" << endl;
    #endif

    updateStatsFromImplStats();
    solver->watches.clear_smudged();

    return solver->okay();
}


void VarReplacer::replace_bnn_lit(Lit& l, uint32_t idx, bool& changed)
{
    removeWBNN(solver->watches, l, idx);
    removeWBNN(solver->watches, ~l, idx);
    changed = true;
    l = get_lit_replaced_with_fast(l);
    runStats.replacedLits++;
}

bool VarReplacer::replace_bnns()
{
    assert(!solver->frat->something_delayed());
    for (uint32_t idx = 0; idx < solver->bnns.size(); idx++) {
        BNN* bnn = solver->bnns[idx];
        if (bnn == NULL) {
            continue;
        }
        assert(!bnn->isRemoved);
        runStats.bogoprops += 3;

        bool changed = false;

        for (Lit& l: *bnn) {
            if (isReplaced_fast(l)) {
                replace_bnn_lit(l, idx, changed);
                solver->watches[l].push(Watched(idx, WatchType::watch_bnn_t, bnn_pos_t));
                solver->watches[~l].push(Watched(idx, WatchType::watch_bnn_t, bnn_neg_t));
            }
        }
        if (!bnn->set) {
            if (isReplaced_fast(bnn->out)) {
                replace_bnn_lit(bnn->out, idx, changed);
                solver->watches[bnn->out].push(
                    Watched(idx, WatchType::watch_bnn_t, bnn_out_t));
                solver->watches[~bnn->out].push(
                    Watched(idx, WatchType::watch_bnn_t, bnn_out_t));
            }
        }

        if (changed) {
            // TODO fix up BNN: p + ~p
        }

    }

    assert(solver->okay() && "Beware, we don't check return value of this function");
    return solver->okay();
}

/**
@brief Replaces variables in long clauses
*/
bool VarReplacer::replace_set(vector<ClOffset>& cs)
{
    assert(!solver->frat->something_delayed());
    vector<ClOffset>::iterator i = cs.begin();
    vector<ClOffset>::iterator j = i;
    for (vector<ClOffset>::iterator end = cs.end(); i != end; ++i) {
        runStats.bogoprops += 3;
        assert(!solver->frat->something_delayed());

        //Finish up if UNSAT
        if (!solver->ok) {
            *j++ = *i;
            continue;
        }

        Clause& c = *solver->cl_alloc.ptr(*i);
        assert(!c.getRemoved());
        assert(c.size() > 2);

        bool changed = false;
        (*solver->frat) << deldelay << c << fin;

        const Lit origLit1 = c[0];
        const Lit origLit2 = c[1];

        for (Lit& l: c) {
            if (isReplaced_fast(l)) {
                changed = true;
                l = get_lit_replaced_with_fast(l);
                runStats.replacedLits++;
            }
        }

        if (changed && handleUpdatedClause(c, origLit1, origLit2)) {
            runStats.removedLongClauses++;
            if (!solver->ok) {
                //if it became UNSAT, then don't delete.
                *j++ = *i;
            }
        } else {
            *j++ = *i;
            solver->frat->forget_delay();
        }

    }
    cs.resize(cs.size() - (i-j));
    assert(!solver->frat->something_delayed());

    return solver->okay();
}

Lit* my_lit_find(Clause& cl, const Lit lit)
{
    for(Lit* a = cl.begin(); a != cl.end(); a++) {
        if (*a == lit)
            return a;
    }
    return NULL;
}

/**
@returns TRUE if needs removal
*/
bool VarReplacer::handleUpdatedClause(
    Clause& c
    , const Lit origLit1
    , const Lit origLit2
) {
    assert(!c.getRemoved());
    bool satisfied = false;
    std::sort(c.begin(), c.end());
    Lit p;
    uint32_t i, j;
    const uint32_t origSize = c.size();
    for (i = j = 0, p = lit_Undef; i != origSize; i++) {
        assert(solver->varData[c[i].var()].removed == Removed::none);
        if (solver->value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (solver->value(c[i]) != l_False && c[i] != p) {
            c[j++] = p = c[i];
        }
    }
    c.shrink(i - j);
    c.setStrenghtened();

    runStats.bogoprops += 10;
    if (c.red()) {
        solver->litStats.redLits -= origSize;
    } else {
        solver->litStats.irredLits -= origSize;
    }
    delayed_attach_or_free.push_back(&c);
    VERBOSE_PRINT("clause after var-replacement: " << c);

    if (satisfied) {
        (*solver->frat) << findelay;
        c.shrink(c.size()); //needed to make clause cleaner happy
        solver->watches.smudge(origLit1);
        solver->watches.smudge(origLit2);
        c.setRemoved();
        return true;
    }

    INC_ID(c);
    (*solver->frat) << add << c << fin << findelay;

    runStats.bogoprops += 3;
    switch(c.size()) {
    case 0:
        assert(solver->unsat_cl_ID == 0);
        solver->unsat_cl_ID = c.stats.ID;
        solver->ok = false;
        return true;
    case 1 :
        c.setRemoved();
        solver->watches.smudge(origLit1);
        solver->watches.smudge(origLit2);

        delayedEnqueue.push_back(make_pair(c[0], c.stats.ID));
        runStats.removedLongLits += origSize;
        return true;
    case 2:
        c.setRemoved();
        solver->watches.smudge(origLit1);
        solver->watches.smudge(origLit2);

        solver->attach_bin_clause(c[0], c[1], c.red(), c.stats.ID);
        runStats.removedLongLits += origSize;
        return true;

    default:
        Lit* at = my_lit_find(c, origLit1);
        if (at != NULL) {
            std::swap(c[0], *at);
        }
        Lit* at2 = my_lit_find(c, origLit2);
        if (at2 != NULL) {
            std::swap(c[1], *at2);
        }
        if (at != NULL && at2 != NULL) {
            delayed_attach_or_free.pop_back();
            if (c.red()) {
                solver->litStats.redLits += c.size();
            } else {
                solver->litStats.irredLits += c.size();
            }
        } else {
            c.setRemoved();
            solver->watches.smudge(origLit1);
            solver->watches.smudge(origLit2);
        }

        runStats.removedLongLits += origSize - c.size();
        return false;
    }

    release_assert(false);
}

void VarReplacer::set_sub_var_during_solution_extension(uint32_t var, const uint32_t sub_var)
{
    const lbool to_set = solver->model[var] ^ table[sub_var].sign();
    const uint32_t sub_var_inter = solver->map_outer_to_inter(sub_var);
    assert(solver->varData[sub_var_inter].removed == Removed::replaced);
    #ifdef VERBOSE_DEBUG
    if (solver->model_value(sub_var) != l_Undef) {
        cout << "ERROR: var " << sub_var +1 << " is set but it's replaced!" << endl;
    }
    #endif
    assert(solver->model_value(sub_var) == l_Undef);

    if (solver->conf.verbosity > 10) {
        cout << "Varreplace-extend: setting outer " << sub_var+1
        << " to " << to_set << " because of " << var+1 << endl;
    }
    solver->model[sub_var] = to_set;
}

//NOTE: 'var' is OUTER
void VarReplacer::extend_model(const uint32_t var)
{
    assert(solver->model[var] != l_Undef);
    auto it = reverseTable.find(var);
    if (it == reverseTable.end())
        return;

    assert(it->first == var);
    for(const uint32_t sub_var: it->second)
    {
        set_sub_var_during_solution_extension(var, sub_var);
    }
}

void VarReplacer::extend_pop_queue(vector<Lit>& pop)
{
    vector<Lit> extra;
    for (Lit p: pop) {
        const auto& repl = reverseTable[p.var()];
        for(uint32_t x: repl) {
            extra.push_back(Lit(x, table[x].sign() ^ p.sign()));
        }
    }

    for(Lit x: extra) {
        pop.push_back(x);
    }
}

void VarReplacer::extend_model_already_set()
{
    assert(solver->model.size() == solver->nVarsOuter());
    for (auto it = reverseTable.begin() , end = reverseTable.end()
        ; it != end
        ; ++it
    ) {
        if (solver->model_value(it->first) == l_Undef) {
            continue;
        }

        for(const uint32_t sub_var: it->second)
        {
            set_sub_var_during_solution_extension(it->first, sub_var);
        }
    }
}

void VarReplacer::extend_model_set_undef()
{
    assert(solver->model.size() == solver->nVarsOuter());
    for (auto it = reverseTable.begin() , end = reverseTable.end()
        ; it != end
        ; ++it
    ) {
        if (solver->model_value(it->first) == l_Undef) {
            solver->model[it->first] = l_False;
            for(const uint32_t sub_var: it->second)
            {
                set_sub_var_during_solution_extension(it->first, sub_var);
            }
        }
    }
}

void VarReplacer::replaceChecks(const uint32_t var1, const uint32_t var2) const
{

    assert(solver->ok);
    assert(solver->decisionLevel() == 0);
    assert(solver->value(var1) == l_Undef);
    assert(solver->value(var2) == l_Undef);

    assert(solver->varData[var1].removed == Removed::none);
    assert(solver->varData[var2].removed == Removed::none);
}

bool VarReplacer::handleAlreadyReplaced(const Lit lit1, const Lit lit2)
{
    //OOps, already inside, but with inverse polarity, UNSAT
    if (lit1.sign() != lit2.sign()) {
        (*solver->frat)
        << add << ++solver->clauseID << ~lit1 << lit2 << fin
        << add << ++solver->clauseID << lit1 << ~lit2 << fin
        << add << ++solver->clauseID << lit1 << fin
        << add << ++solver->clauseID << ~lit1 << fin
        << add << ++solver->clauseID << fin
        << del << solver->clauseID-1 << ~lit1 << fin
        << del << solver->clauseID-2 << lit1 << fin
        << del << solver->clauseID-3 << lit1 << ~lit2 << fin
        << del << solver->clauseID-4 << ~lit1 << lit2 << fin;
        // the UNSAT one, i.e. solver->clauseID-1 does not need to be deleted,
        //   it's automatically deleted
        assert(solver->unsat_cl_ID == 0);
        solver->unsat_cl_ID = solver->clauseID;
        solver->ok = false;
        return false;
    }

    //Already inside in the correct way, return
    return true;
}

bool VarReplacer::replace_vars_already_set(
    const Lit lit1
    , const lbool val1
    , const Lit /*lit2*/
    , const lbool val2
) {
    if (val1 != val2) {

        (*solver->frat)
        << add << ++solver->clauseID << ~lit1 << fin
        << add << ++solver->clauseID << lit1 << fin
        << add << ++solver->clauseID << fin
        << del << solver->clauseID-1 << lit1 << fin
        << del << solver->clauseID-2 << ~lit1 << fin;
        assert(solver->unsat_cl_ID == 0);
        solver->unsat_cl_ID = solver->clauseID;
        solver->ok = false;
    }

    //Already set, return with correct code
    return solver->okay();
}

bool VarReplacer::handleOneSet(
    const Lit lit1
    , const lbool val1
    , const Lit lit2
    , const lbool val2
) {
    if (solver->ok) {
        Lit toEnqueue;
        if (val1 != l_Undef) {
            toEnqueue = lit2 ^ (val1 == l_False);
        } else {
            toEnqueue = lit1 ^ (val2 == l_False);
        }
        solver->enqueue<false>(toEnqueue);
        solver->ok = (solver->propagate<false>().isNULL());
    }
    return solver->okay();
}

/**
@brief Replaces two two lits with one another
*/
bool VarReplacer::replace(
    uint32_t var1
    , uint32_t var2
    , const bool xor_is_true
)
{
    #ifdef VERBOSE_DEBUG
    cout
    << "replace() called with var " <<  Lit(var1, false)
    << " and var " << Lit(var2, false)
    << " with xor_is_true " << xor_is_true << endl;
    #endif

    replaceChecks(var1, var2);

    //Move forward
    const Lit lit1 = get_lit_replaced_with(Lit(var1, false));
    const Lit lit2 = get_lit_replaced_with(Lit(var2, false)) ^ xor_is_true;

    //Already inside?
    if (lit1.var() == lit2.var()) {
        return handleAlreadyReplaced(lit1, lit2);
    }

    int32_t ID = ++solver->clauseID;
    int32_t ID2 = ++solver->clauseID;
    (*solver->frat)
    << add << ID << ~lit1 << lit2 << fin
    << add << ID2 << lit1 << ~lit2 << fin;
    bins_for_frat.push_back(std::tuple<int32_t, Lit, Lit>{ID, ~lit1, lit2});
    bins_for_frat.push_back(std::tuple<int32_t, Lit, Lit>{ID2, lit1, ~lit2});

    //None should be removed, only maybe queued for replacement
    assert(solver->varData[lit1.var()].removed == Removed::none);
    assert(solver->varData[lit2.var()].removed == Removed::none);

    const lbool val1 = solver->value(lit1);
    const lbool val2 = solver->value(lit2);

    //Both are set
    if (val1 != l_Undef && val2 != l_Undef) {
        return replace_vars_already_set(lit1, val1, lit2, val2);
    }

    //exactly one set
    if ((val1 != l_Undef && val2 == l_Undef)
        || (val2 != l_Undef && val1 == l_Undef)
    ) {
        return handleOneSet(lit1, val1, lit2, val2);
    }

    assert(val1 == l_Undef && val2 == l_Undef);

    const Lit lit1_outer = solver->map_inter_to_outer(lit1);
    const Lit lit2_outer = solver->map_inter_to_outer(lit2);
    return update_table_and_reversetable(lit1_outer, lit2_outer);
}

bool VarReplacer::update_table_and_reversetable(const Lit lit1, const Lit lit2)
{
    if (reverseTable.find(lit1.var()) == reverseTable.end()) {
        reverseTable[lit2.var()].push_back(lit1.var());
        table[lit1.var()] = lit2 ^ lit1.sign();
        replacedVars++;
        return true;
    }

    if (reverseTable.find(lit2.var()) == reverseTable.end()) {
        reverseTable[lit1.var()].push_back(lit2.var());
        table[lit2.var()] = lit1 ^ lit2.sign();
        replacedVars++;
        return true;
    }

    //both have children
    setAllThatPointsHereTo(lit1.var(), lit2 ^ lit1.sign());
    replacedVars++;
    return true;
}

/**
@brief Changes internal graph to set everything that pointed to var to point to lit
*/
void VarReplacer::setAllThatPointsHereTo(const uint32_t var, const Lit lit)
{
    map<uint32_t, vector<uint32_t> >::iterator it = reverseTable.find(var);
    if (it != reverseTable.end()) {
        for(const uint32_t var2: it->second) {
            assert(table[var2].var() == var);
            if (lit.var() != var2) {
                table[var2] = lit ^ table[var2].sign();
                reverseTable[lit.var()].push_back(var2);
            }
        }
        reverseTable.erase(it);
    }
    table[var] = lit;
    reverseTable[lit.var()].push_back(var);
}

void VarReplacer::checkUnsetSanity()
{
    for(size_t i = 0; i < solver->nVarsOuter(); i++) {
        const Lit repLit = get_lit_replaced_with(Lit(i, false));
        const uint32_t repVar = get_var_replaced_with(i);

        if (solver->varData[i].removed == Removed::none
            && solver->varData[repVar].removed == Removed::none
            && solver->value(i) != solver->value(repLit)
        ) {
            cout
            << "Variable " << (i+1)
            << " has been set to " << solver->value(i)
            << " but it has been replaced with lit "
            << get_lit_replaced_with(Lit(i, false))
            << " and that has been set to "
            << solver->value(get_lit_replaced_with(Lit(i, false)))
            << endl;

            assert(solver->value(i) == solver->value(repLit));
            std::exit(-1);
        }
    }

    #ifdef SLOW_DEBUG
    check_no_replaced_var_set();
    #endif
}

bool VarReplacer::add_xor_as_bins(const BinaryXor& bin_xor)
{
    ps_tmp[0] = Lit(bin_xor.vars[0], false);
    ps_tmp[1] = Lit(bin_xor.vars[1], true ^ bin_xor.rhs);
    solver->add_clause_int(ps_tmp);
    if (!solver->ok) return false;

    ps_tmp[0] = Lit(bin_xor.vars[0], true);
    ps_tmp[1] = Lit(bin_xor.vars[1], false ^ bin_xor.rhs);
    solver->add_clause_int(ps_tmp);
    if (!solver->ok) return false;

    return true;
}

bool VarReplacer::replace_if_enough_is_found(const size_t limit, uint64_t* bogoprops_given, bool *replaced)
{
    if (replaced)
        *replaced = false;

    scc_finder->performSCC(bogoprops_given);
    if (scc_finder->get_num_binxors_found() < limit) {
        scc_finder->clear_binxors();
        return solver->okay();
    }

    assert(solver->gmatrices.empty());
    assert(solver->gqueuedata.empty());

    if (replaced)
        *replaced = true;

    const set<BinaryXor>& xors_found = scc_finder->get_binxors();
    for(BinaryXor bin_xor: xors_found) {
        if (!add_xor_as_bins(bin_xor)) return false;

        if (solver->value(bin_xor.vars[0]) == l_Undef
            && solver->value(bin_xor.vars[1]) == l_Undef
        ) {
            replace(bin_xor.vars[0], bin_xor.vars[1], bin_xor.rhs);
            if (!solver->okay()) {
                return false;
            }
        }
    }

    const bool ret = perform_replace();
    if (bogoprops_given) {
        *bogoprops_given += runStats.bogoprops;
    }
    scc_finder->clear_binxors();

    return ret;
}

size_t VarReplacer::mem_used() const
{
    size_t b = 0;
    b += scc_finder->mem_used();
    b += delayedEnqueue.capacity()*sizeof(Lit);
    b += table.capacity()*sizeof(Lit);
    for(map<uint32_t, vector<uint32_t> >::const_iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; ++it
    ) {
        b += it->second.capacity()*sizeof(Lit);
    }
    //TODO under-counting
    b += reverseTable.size()*(sizeof(uint32_t) + sizeof(vector<uint32_t>));

    return b;
}

uint32_t VarReplacer::print_equivalent_literals(bool outer_numbering, std::ostream *os) const
{
    uint32_t num = 0;
    vector<Lit> tmpCl;
    for (uint32_t var = 0; var < table.size(); var++) {
        const Lit lit = table[var];
        if (lit.var() == var)
            continue;

        //They have been renumbered in a way that cannot be dumped
        Lit lit1;
        Lit lit2;
        if (outer_numbering) {
            lit1 = lit;
            lit2 = Lit(var, false);
        } else {
            lit1 = solver->map_outer_to_inter(lit);
            lit2 = solver->map_outer_to_inter(Lit(var, false));

            if (lit1.var() >= solver->nVars() ||
                lit2.var() >= solver->nVars()
            ) {
                continue;
            }
        }

        if (os) {
            tmpCl.clear();
            tmpCl.push_back(~lit1);
            tmpCl.push_back(lit2);
            std::sort(tmpCl.begin(), tmpCl.end());

            *os
            << tmpCl[0] << " "
            << tmpCl[1]
            << " 0\n";

            tmpCl[0] ^= true;
            tmpCl[1] ^= true;

            *os
            << tmpCl[0] << " "
            << tmpCl[1]
            << " 0\n";
        }
        num++;
    }
    return num;
}

void VarReplacer::print_some_stats(const double global_cpu_time) const
{
    print_stats_line("c vrep replace time"
        , globalStats.cpu_time
        , stats_line_percent(globalStats.cpu_time, global_cpu_time)
        , "% time"
    );

    print_stats_line("c vrep tree roots"
        , getNumTrees()
    );

    print_stats_line("c vrep trees' crown"
        , get_num_replaced_vars()
        , float_div(get_num_replaced_vars(), getNumTrees())
        , "leafs/tree"
    );
}

void VarReplacer::Stats::print(const size_t nVars) const
{
        cout << "c --------- VAR REPLACE STATS ----------" << endl;
        print_stats_line("c time"
            , cpu_time
            , float_div(cpu_time, numCalls)
            , "per call"
        );

        print_stats_line("c trees' crown"
            , actuallyReplacedVars
            , stats_line_percent(actuallyReplacedVars, nVars)
            , "% of vars"
        );

        print_stats_line("c 0-depth assigns"
            , zeroDepthAssigns
            , stats_line_percent(zeroDepthAssigns, nVars)
            , "% vars"
        );

        print_stats_line("c lits replaced"
            , replacedLits
        );

        print_stats_line("c bin cls removed"
            , removedBinClauses
        );

        print_stats_line("c long cls removed"
            , removedLongClauses
        );

        print_stats_line("c long lits removed"
            , removedLongLits
        );

         print_stats_line("c bogoprops"
            , bogoprops
        );
        cout << "c --------- VAR REPLACE STATS END ----------" << endl;
}

void VarReplacer::Stats::print_short(const Solver* solver) const
{
    cout
    << "c [vrep]"
    << " vars " << actuallyReplacedVars
    << " lits " << replacedLits
    << " rem-bin-cls " << removedBinClauses
    << " rem-long-cls " << removedLongClauses
    << " BP " << bogoprops/(1000*1000) << "M"
    << solver->conf.print_times(cpu_time)
    << endl;
}

VarReplacer::Stats& VarReplacer::Stats::operator+=(const Stats& other)
{
    numCalls += other.numCalls;
    cpu_time += other.cpu_time;
    replacedLits += other.replacedLits;
    zeroDepthAssigns += other.zeroDepthAssigns;
    actuallyReplacedVars += other.actuallyReplacedVars;
    removedBinClauses += other.removedBinClauses;
    removedLongClauses += other.removedLongClauses;
    removedLongLits += other.removedLongLits;
    bogoprops += other.bogoprops;

    return *this;
}

void VarReplacer::build_fast_inter_replace_lookup()
{
    fast_inter_replace_lookup.clear();
    fast_inter_replace_lookup.reserve(solver->nVars());
    for(uint32_t var = 0; var < solver->nVars(); var++) {
        fast_inter_replace_lookup.push_back(get_lit_replaced_with(Lit(var, false)));
    }
}

void VarReplacer::destroy_fast_inter_replace_lookup()
{
    vector<Lit> tmp;
    fast_inter_replace_lookup.swap(tmp);
}

Lit VarReplacer::get_lit_replaced_with(Lit lit) const
{
    lit = solver->map_inter_to_outer(lit);
    Lit lit2 = get_lit_replaced_with_outer(lit);
    return solver->map_outer_to_inter(lit2);
}

uint32_t VarReplacer::get_var_replaced_with(uint32_t var) const
{
    var = solver->map_inter_to_outer(var);
    uint32_t var2 = table[var].var();
    return solver->map_outer_to_inter(var2);
}

vector<uint32_t> VarReplacer::get_vars_replacing(uint32_t var) const
{
    vector<uint32_t> ret;
    var = solver->map_inter_to_outer(var);
    map<uint32_t, vector<uint32_t> >::const_iterator it = reverseTable.find(var);
    if (it != reverseTable.end()) {
        for(uint32_t v: it->second) {
            ret.push_back(solver->map_outer_to_inter(v));
        }
    }

    return ret;
}

vector<pair<Lit, Lit> > VarReplacer::get_all_binary_xors_outer() const
{
    vector<pair<Lit, Lit> > ret;
    for(size_t i = 0; i < table.size(); i++) {
        if (table[i] != Lit(i, false)) {
            ret.push_back(make_pair(Lit(i, false), table[i]));
        }
    }

    return ret;
}

bool VarReplacer::get_scc_depth_warning_triggered() const
{
    return scc_finder->depth_warning_triggered();
}

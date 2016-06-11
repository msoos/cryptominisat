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

#include "searcher.h"
#include "occsimplifier.h"
#include "time_mem.h"
#include "solver.h"
#include <iomanip>
#include "varreplacer.h"
#include "clausecleaner.h"
#include "propbyforgraph.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <ratio>
#include "sqlstats.h"
#include "datasync.h"
#include "reducedb.h"
#include "gaussian.h"
#include "sqlstats.h"
#include "watchalgos.h"
#include "hasher.h"
//#define DEBUG_RESOLV

using namespace CMSat;
using std::cout;
using std::endl;

#ifdef USE_OMP
#include <omp.h>
#endif

//#define VERBOSE_DEBUG_GEN_CONFL_DOT

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_GEN_CONFL_DOT
#endif

/**
@brief Sets a sane default config and allocates handler classes
*/
Searcher::Searcher(const SolverConf *_conf, Solver* _solver, std::atomic<bool>* _must_interrupt_inter) :
        HyperEngine(
            _conf
            , _must_interrupt_inter
        )

        //variables
        , solver(_solver)
        , order_heap_glue(VarOrderLt(activ_glue))
        , cla_inc(1)
{
    var_decay = conf.var_decay_start;
    var_inc = conf.var_inc_start;
    more_red_minim_limit_binary_actual = conf.more_red_minim_limit_binary;
    more_red_minim_limit_cache_actual = conf.more_red_minim_limit_cache;
    mtrand.seed(conf.origSeed);
    hist.setSize(conf.shortTermHistorySize, conf.blocking_restart_trail_hist_length);
    conf.cur_max_temp_red_cls = conf.max_temporary_learnt_clauses;
}

Searcher::~Searcher()
{
    clear_gauss();
    delete[] hits;
}

void Searcher::new_var(const bool bva, const uint32_t orig_outer)
{
    PropEngine::new_var(bva, orig_outer);

    activ_glue.push_back(0);
    insertVarOrder((int)nVars()-1);
}

void Searcher::new_vars(size_t n)
{
    PropEngine::new_vars(n);

    activ_glue.resize(activ_glue.size() + n, 0);
    for(int i = n-1; i >= 0; i--) {
        insertVarOrder((int)nVars()-i-1);
    }
}

void Searcher::save_on_var_memory()
{
    PropEngine::save_on_var_memory();
    activ_glue.resize(nVars());
    activ_glue.shrink_to_fit();
}

void Searcher::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    updateArray(activ_glue, interToOuter);
    //activ_glue are not updated, they are taken from backup, which is updated

    renumber_assumptions(outerToInter);
}

void Searcher::renumber_assumptions(const vector<uint32_t>& outerToInter)
{
    solver->unfill_assumptions_set_from(assumptions);
    for(AssumptionPair& lit_pair: assumptions) {
        assert(lit_pair.lit_inter.var() < outerToInter.size());
        lit_pair.lit_inter = getUpdatedLit(lit_pair.lit_inter, outerToInter);
    }
    solver->fill_assumptions_set_from(assumptions);
}

template<bool update_bogoprops>
inline void Searcher::add_lit_to_learnt(
    const Lit lit
) {
    antec_data.vsids_all_incoming_vars.push(activ_glue[lit.var()]/var_inc);
    const uint32_t var = lit.var();
    assert(varData[var].removed == Removed::none);

    //If var is at level 0, don't do anything with it, just skip
    if (seen[var] || varData[var].level == 0) {
        return;
    }

    //Update our state of going through the conflict
    bump_var_activity<update_bogoprops>(var);
    seen[var] = 1;
    if (!update_bogoprops && conf.doOTFSubsume) {
        tmp_learnt_clause_size++;
        seen2[lit.toInt()] = 1;
        tmp_learnt_clause_abst |= abst_var(lit.var());
    }

    if (varData[var].level >= decisionLevel()) {
        pathC++;

        if (!update_bogoprops && varData[var].reason != PropBy()) {
            if (varData[var].reason.getType() == clause_t) {
                Clause* cl = cl_alloc.ptr(varData[var].reason.get_offset());
                if (cl->red()) {
                    const uint32_t glue = cl->stats.glue;
                    implied_by_learnts.push_back(std::make_pair(var, glue));
                }
            } else if (varData[var].reason.getType() == binary_t
                && varData[var].reason.isRedStep()
            ) {
                implied_by_learnts.push_back(std::make_pair(var, 2));
            } else if (varData[var].reason.getType() == tertiary_t
                && varData[var].reason.isRedStep()
            ) {
                implied_by_learnts.push_back(std::make_pair(var, 3));
            }
        }
    } else {
        learnt_clause.push_back(lit);
    }
}

inline void Searcher::recursiveConfClauseMin()
{
    uint32_t abstract_level = 0;
    for (size_t i = 1; i < learnt_clause.size(); i++) {
        //(maintain an abstraction of levels involved in conflict)
        abstract_level |= abstractLevel(learnt_clause[i].var());
    }

    size_t i, j;
    for (i = j = 1; i < learnt_clause.size(); i++) {
        if (varData[learnt_clause[i].var()].reason.isNULL()
            || !litRedundant(learnt_clause[i], abstract_level)
        ) {
            learnt_clause[j++] = learnt_clause[i];
        }
    }
    learnt_clause.resize(j);
}

void Searcher::create_otf_subsuming_implicit_clause(const Clause& cl)
{
    OTFClause newCl;
    newCl.size = 0;
    for(const Lit
        *it = cl.begin(), *end = cl.end()
        ; it != end
        ; ++it
    ) {
        if (seen2[it->toInt()]) {
            assert(newCl.size < 3);
            newCl.lits[newCl.size] = *it;
            newCl.size++;
        }
    }
    otf_subsuming_short_cls.push_back(newCl);
    if (conf.verbosity >= 6) {
        cout << "New implicit clause that subsumes a long clause:";
        for(unsigned  i = 0; i < newCl.size; i++) {
            cout
            << newCl.lits[i] << " ";
        }
        cout  << endl;
    }

    if (drat->enabled()) {
        for(unsigned  i = 0; i < newCl.size; i++) {
            *drat << newCl.lits[i];
        }
        *drat << fin;
    }

    stats.otfSubsumed++;
    stats.otfSubsumedImplicit++;
    stats.otfSubsumedRed += cl.red();
    stats.otfSubsumedLitsGained += cl.size() - newCl.size;
}

void Searcher::create_otf_subsuming_long_clause(
    Clause& cl
    , const ClOffset offset
) {
    (*solver->drat) << deldelay << cl << fin;
    solver->detachClause(cl, false);
    stats.otfSubsumed++;
    stats.otfSubsumedLong++;
    stats.otfSubsumedRed += cl.red();
    stats.otfSubsumedLitsGained += cl.size() - tmp_learnt_clause_size;

    size_t i = 0;
    size_t i2 = 0;
    for (; i < cl.size(); i++) {
        if (seen2[cl[i].toInt()]) {
            cl[i2++] = cl[i];
        }
    }
    cl.shrink(i-i2);
    assert(cl.size() == tmp_learnt_clause_size);
    if (conf.verbosity >= 6) {
        cout
        << "New smaller clause OTF:" << cl << endl;
    }
    *drat << cl << fin << findelay;
    otf_subsuming_long_cls.push_back(offset);
}

void Searcher::check_otf_subsume(const ClOffset offset, Clause& cl)
{
    size_t num_lits_from_cl = 0;
    for (const Lit lit: cl) {
        if (seen2[lit.toInt()]) {
            num_lits_from_cl++;
        }
    }
    if (num_lits_from_cl != tmp_learnt_clause_size)
        return;

    if (num_lits_from_cl <= 3) {
        create_otf_subsuming_implicit_clause(cl);
    } else {
        create_otf_subsuming_long_clause(cl, offset);
    }
}

void Searcher::normalClMinim()
{
    size_t i,j;
    for (i = j = 1; i < learnt_clause.size(); i++) {
        const PropBy& reason = varData[learnt_clause[i].var()].reason;
        size_t size;
        Clause* cl = NULL;
        PropByType type = reason.getType();
        if (type == null_clause_t) {
            learnt_clause[j++] = learnt_clause[i];
            continue;
        }

        switch (type) {
            case clause_t:
                cl = cl_alloc.ptr(reason.get_offset());
                size = cl->size()-1;
                break;

            case binary_t:
                size = 1;
                break;

            case tertiary_t:
                size = 2;
                break;

            default:
                release_assert(false);
                std::exit(-1);
        }

        for (size_t k = 0; k < size; k++) {
            Lit p;
            switch (type) {
                case clause_t:
                    p = (*cl)[k+1];
                    break;

                case binary_t:
                    p = reason.lit2();
                    break;

                case tertiary_t:
                    if (k == 0) {
                        p = reason.lit2();
                    } else {
                        p = reason.lit3();
                    }
                    break;

                default:
                    release_assert(false);
                    std::exit(-1);
            }

            if (!seen[p.var()] && varData[p.var()].level > 0) {
                learnt_clause[j++] = learnt_clause[i];
                break;
            }
        }
    }
    learnt_clause.resize(j);
}

void Searcher::debug_print_resolving_clause(const PropBy confl) const
{
#ifndef DEBUG_RESOLV
    //Avoid unused parameter warning
    (void) confl;
#else
    switch(confl.getType()) {
        case tertiary_t: {
            cout << "resolv (tri): " << confl.lit2() << ", " << confl.lit3() << endl;
            break;
        }

        case binary_t: {
            cout << "resolv bin: " << confl.lit2() << endl;
            break;
        }

        case clause_t: {
            Clause* cl = cl_alloc.ptr(confl.get_offset());
            cout << "resolv (long): " << *cl << endl;
            break;
        }

        case xor_t: {
            //in the future, we'll have XOR clauses. Not yet.
            assert(false);
            exit(-1);
            break;
        }

        case null_clause_t: {
            assert(false);
            break;
        }
    }
#endif
}

void Searcher::update_clause_glue_from_analysis(Clause* cl)
{
    assert(cl->red());
    const unsigned new_glue = calc_glue(*cl);

    if (new_glue + 1 < cl->stats.glue) {
        cl->stats.glue = new_glue;

        if (new_glue <= conf.protect_cl_if_improved_glue_below_this_glue_for_one_turn) {
            cl->stats.ttl = 1;
        }
    }
}

template<bool update_bogoprops>
Clause* Searcher::add_literals_from_confl_to_learnt(
    const PropBy confl
    , const Lit p
) {
    #ifdef VERBOSE_DEBUG
    debug_print_resolving_clause(confl);
    #endif

    Clause* cl = NULL;
    switch (confl.getType()) {
        case tertiary_t : {
            if (confl.isRedStep()) {
                antec_data.triRed++;
                stats.resolvs.triRed++;
            } else {
                antec_data.triIrred++;
                stats.resolvs.triIrred++;
            }
            break;
        }

        case binary_t : {
            if (confl.isRedStep()) {
                antec_data.binRed++;
                stats.resolvs.binRed++;
            } else {
                antec_data.binIrred++;
                stats.resolvs.binIrred++;
            }
            break;
        }

        case clause_t : {
            cl = cl_alloc.ptr(confl.get_offset());
            if (cl->red()) {
                stats.resolvs.longRed++;
                #ifdef STATS_NEEDED
                antec_data.vsids_of_ants.push(cl->stats.antec_data.vsids_vars.avg());
                antec_data.longRed++;
                antec_data.age_long_reds.push(sumConflicts() - cl->stats.introduced_at_conflict);
                antec_data.glue_long_reds.push(cl->stats.glue);
                #endif
            } else {
                antec_data.longIrred++;
                stats.resolvs.longRed++;
            }
            antec_data.size_longs.push(cl->size());

            #ifdef STATS_NEEDED
            cl->stats.used_for_uip_creation++;
            #endif
            if (!update_bogoprops
                && cl->red()
                && cl->stats.glue > conf.glue_must_keep_clause_if_below_or_eq
                && cl->stats.ttl == 0
            ) {
                bumpClauseAct(cl);
            }

            if (!update_bogoprops
                && conf.update_glues_on_analyze
                && cl->red()
                && cl->stats.glue > conf.glue_must_keep_clause_if_below_or_eq
            ) {
                update_clause_glue_from_analysis(cl);
            }
            break;
        }

        case null_clause_t:
        default:
            assert(false && "Error in conflict analysis (otherwise should be UIP)");
    }

    size_t i = 0;
    bool cont = true;
    Lit x = lit_Undef;
    while(cont) {
        switch (confl.getType()) {
            case tertiary_t:
                if (i == 0) {
                    x = failBinLit;
                } else if (i == 1) {
                    x = confl.lit2();
                } else {
                    x = confl.lit3();
                    cont = false;
                }
                break;
            case binary_t:
                if (i == 0) {
                    x = failBinLit;
                } else {
                    x = confl.lit2();
                    cont = false;
                }
                break;

            case clause_t:
                assert(!cl->getRemoved());
                x = (*cl)[i];
                if (i == cl->size()-1) {
                    cont = false;
                }
                break;
            case null_clause_t:
                assert(false);
        }
        if (p == lit_Undef || i > 0) {
            add_lit_to_learnt<update_bogoprops>(x);
        }
        i++;
    }
    return cl;
}

template<bool update_bogoprops>
inline void Searcher::minimize_learnt_clause()
{
    const size_t origSize = learnt_clause.size();

    toClear = learnt_clause;
    if (conf.doRecursiveMinim) {
        recursiveConfClauseMin();
    } else {
        normalClMinim();
    }
    for (const Lit lit: toClear) {
        if (!update_bogoprops
            && conf.doOTFSubsume
        ) {
            seen2[lit.toInt()] = 0;
        }
        seen[lit.var()] = 0;
    }
    toClear.clear();

    stats.recMinCl += ((origSize - learnt_clause.size()) > 0);
    stats.recMinLitRem += origSize - learnt_clause.size();
}

inline void Searcher::minimize_using_permdiff()
{
    if (conf.doMinimRedMore
        && learnt_clause.size() > 1
    ) {
        stats.moreMinimLitsStart += learnt_clause.size();
        watch_based_learnt_minim();

        stats.moreMinimLitsEnd += learnt_clause.size();
    }
}

inline void Searcher::watch_based_learnt_minim()
{
    MYFLAG++;
    const auto& ws  = watches[~learnt_clause[0]];
    uint32_t nb = 0;
    for (const Watched& w: ws) {
        if (w.isBin()) {
            Lit imp = w.lit2();
            if (permDiff[imp.var()] == MYFLAG && value(imp) == l_True) {
                nb++;
                permDiff[imp.var()] = MYFLAG - 1;
            }
        } else {
            break;
        }
    }
    uint32_t l = learnt_clause.size() - 1;
    if (nb > 0) {
        for (uint32_t i = 1; i < learnt_clause.size() - nb; i++) {
            if (permDiff[learnt_clause[i].var()] != MYFLAG) {
                Lit p = learnt_clause[l];
                learnt_clause[l] = learnt_clause[i];
                learnt_clause[i] = p;
                l--;
                i--;
            }
        }
        learnt_clause.resize(learnt_clause.size()-nb);
    }
}

void Searcher::print_fully_minimized_learnt_clause() const
{
    if (conf.verbosity >= 6) {
        cout << "Final clause: " << learnt_clause << endl;
        for (uint32_t i = 0; i < learnt_clause.size(); i++) {
            cout << "lev learnt_clause[" << i << "]:" << varData[learnt_clause[i].var()].level << endl;
        }
    }
}

size_t Searcher::find_backtrack_level_of_learnt()
{
    if (learnt_clause.size() <= 1)
        return 0;
    else {
        uint32_t max_i = 1;
        for (uint32_t i = 2; i < learnt_clause.size(); i++) {
            if (varData[learnt_clause[i].var()].level > varData[learnt_clause[max_i].var()].level)
                max_i = i;
        }
        std::swap(learnt_clause[max_i], learnt_clause[1]);
        return varData[learnt_clause[1].var()].level;
    }
}

template<bool update_bogoprops>
inline Clause* Searcher::create_learnt_clause(PropBy confl)
{
    pathC = 0;
    int index = trail.size() - 1;
    Lit p = lit_Undef;
    Clause* last_resolved_cl = NULL;

    learnt_clause.push_back(lit_Undef); //make space for ~p
    do {
        #ifdef DEBUG_RESOLV
        cout << "p is: " << p << endl;
        #endif

        //This is for OTF subsumption ("OTF clause improvement" by Han&Somezi)
        //~p is essentially popped from the temporary learnt clause
        if (p != lit_Undef) {
            antec_data.vsids_of_resolving_literals.push(activ_glue[p.var()]/var_inc);
            if (!update_bogoprops && conf.doOTFSubsume) {
                tmp_learnt_clause_size--;
                assert(seen2[(~p).toInt()] == 1);
                seen2[(~p).toInt()] = 0;
            }

            //We MUST under-estimate
            tmp_learnt_clause_abst &= ~(abst_var((~p).var()));
        }

        last_resolved_cl = add_literals_from_confl_to_learnt<update_bogoprops>(confl, p);

        // Select next implication to look at
        while (!seen[trail[index--].var()]);

        p = trail[index+1];
        assert(p != lit_Undef);

        if (!update_bogoprops
            && pathC > 1
            && conf.doOTFSubsume
            //A long clause
            && last_resolved_cl != NULL
            //Good enough clause to try to minimize
            && (!last_resolved_cl->red() || last_resolved_cl->stats.glue <= conf.doOTFSubsumeOnlyAtOrBelowGlue)
            //Must subsume, so must be smaller
            && last_resolved_cl->size() > tmp_learnt_clause_size
            //Must not be a temporary clause
            && !last_resolved_cl->gauss_temp_cl()
            && !last_resolved_cl->used_in_xor()
        ) {
            last_resolved_cl->recalc_abst_if_needed();
            //Everything in learnt_cl_2 seems to be also in cl
            if ((last_resolved_cl->abst & tmp_learnt_clause_abst) ==  tmp_learnt_clause_abst
            ) {
                check_otf_subsume(confl.get_offset(), *last_resolved_cl);
            }
        }

        confl = varData[p.var()].reason;
        assert(varData[p.var()].level > 0);

        //This clears out vars that haven't been added to learnt_clause,
        //but their 'seen' has been set
        seen[p.var()] = 0;

        //Okay, one more path done
        pathC--;
    } while (pathC > 0);
    assert(pathC == 0);
    learnt_clause[0] = ~p;

    if (conf.doOTFSubsume
        && !update_bogoprops
    ) {
        for(const Lit lit: learnt_clause) {
            seen2[lit.toInt()] = 0;
        }
    }

    return last_resolved_cl;
}

template<bool update_bogoprops>
void Searcher::bump_var_activities_based_on_implied_by_learnts(const uint32_t glue)
{
    assert(!update_bogoprops);
    for (const auto dat :implied_by_learnts) {
        const uint32_t v_glue = dat.second;
        if (v_glue < glue) {
            bump_var_activity<update_bogoprops>(dat.first);
        }
    }
}

Clause* Searcher::otf_subsume_last_resolved_clause(Clause* last_resolved_cl)
{
    //We can only on-the-fly subsume with clauses that are not 2- or 3-long
    //furthermore, we cannot subsume a clause that is marked for deletion
    //due to its high glue value
    if (!conf.doOTFSubsume
        //Last was a lont clause
        || last_resolved_cl == NULL
        //Final clause will not be implicit
        || learnt_clause.size() <= 3
        //Larger or equivalent clauses cannot subsume the clause
        || learnt_clause.size() >= last_resolved_cl->size()
    ) {
        return NULL;
    }

    //Does it subsume?
    if (!subset(learnt_clause, *last_resolved_cl))
        return NULL;

    //on-the-fly subsumed the original clause
    stats.otfSubsumed++;
    stats.otfSubsumedLong++;
    stats.otfSubsumedRed += last_resolved_cl->red();
    stats.otfSubsumedLitsGained += last_resolved_cl->size() - learnt_clause.size();
    return last_resolved_cl;
}

void Searcher::print_debug_resolution_data(const PropBy confl)
{
#ifndef DEBUG_RESOLV
    //Avoid unused parameter warning
    (void) confl;
#else
    cout << "Before resolution, trail is: " << endl;
    print_trail();
    cout << "Conflicting clause: " << confl << endl;
    cout << "Fail bin lit: " << failBinLit << endl;
#endif
}

template<bool update_bogoprops>
Clause* Searcher::analyze_conflict(
    const PropBy confl
    , uint32_t& out_btlevel
    , uint32_t& glue
) {
    //Set up environment
    learnt_clause.clear();
    assert(toClear.empty());
    implied_by_learnts.clear();
    otf_subsuming_short_cls.clear();
    otf_subsuming_long_cls.clear();
    tmp_learnt_clause_size = 0;
    tmp_learnt_clause_abst = 0;
    assert(decisionLevel() > 0);

    print_debug_resolution_data(confl);
    Clause* last_resolved_cl = create_learnt_clause<update_bogoprops>(confl);
    stats.litsRedNonMin += learnt_clause.size();
    minimize_learnt_clause<update_bogoprops>();
    stats.litsRedFinal += learnt_clause.size();
    if (learnt_clause.size() <= conf.max_size_more_minim) {
        glue = calc_glue(learnt_clause);
        if (glue <= conf.max_glue_more_minim) {
            minimize_using_permdiff();
        }
    }
    glue = calc_glue(learnt_clause);
    print_fully_minimized_learnt_clause();

    if (learnt_clause.size() > conf.max_size_more_minim
        && glue <= conf.glue_must_keep_clause_if_below_or_eq
    ) {
        minimise_redundant_more(learnt_clause);
    }

    out_btlevel = find_backtrack_level_of_learnt();
    if (!update_bogoprops
        && conf.extra_bump_var_activities_based_on_glue
    ) {
        bump_var_activities_based_on_implied_by_learnts<update_bogoprops>(glue);
    }
    implied_by_learnts.clear();

    for(const Lit l: learnt_clause) {
        antec_data.vsids_vars.push(activ_glue[l.var()]/var_inc);
    }

    return otf_subsume_last_resolved_clause(last_resolved_cl);

}

bool Searcher::litRedundant(const Lit p, uint32_t abstract_levels)
{
    #ifdef DEBUG_LITREDUNDANT
    cout << "c " << __func__ << " called" << endl;
    #endif

    analyze_stack.clear();
    analyze_stack.push(p);

    size_t top = toClear.size();
    while (!analyze_stack.empty()) {
        #ifdef DEBUG_LITREDUNDANT
        cout << "At point in litRedundant: " << analyze_stack.top() << endl;
        #endif

        const PropBy reason = varData[analyze_stack.top().var()].reason;
        PropByType type = reason.getType();
        analyze_stack.pop();

        //Must have a reason
        assert(!reason.isNULL());

        size_t size;
        Clause* cl = NULL;
        switch (type) {
            case clause_t:
                cl = cl_alloc.ptr(reason.get_offset());
                size = cl->size()-1;
                break;

            case binary_t:
                size = 1;
                break;

            case tertiary_t:
                size = 2;
                break;

            case null_clause_t:
            default:
                release_assert(false);
        }

        for (size_t i = 0
            ; i < size
            ; i++
        ) {
            Lit p2;
            switch (type) {
                case clause_t:
                    p2 = (*cl)[i+1];
                    break;

                case binary_t:
                    p2 = reason.lit2();
                    break;

                case tertiary_t:
                    if (i == 0) {
                        p2 = reason.lit2();
                    } else {
                        p2 = reason.lit3();
                    }
                    break;

                case null_clause_t:
                default:
                    release_assert(false);
            }
            stats.recMinimCost++;

            if (!seen[p2.var()] && varData[p2.var()].level > 0) {
                if (!varData[p2.var()].reason.isNULL()
                    && (abstractLevel(p2.var()) & abstract_levels) != 0
                ) {
                    seen[p2.var()] = 1;
                    analyze_stack.push(p2);
                    toClear.push_back(p2);
                } else {
                    //Return to where we started before function executed
                    for (size_t j = top; j < toClear.size(); j++) {
                        seen[toClear[j].var()] = 0;
                    }
                    toClear.resize(top);

                    return false;
                }
            }
        }
    }

    return true;
}
template Clause* Searcher::analyze_conflict<true>(const PropBy confl
    , uint32_t& out_btlevel
    , uint32_t& glue
);
template Clause* Searcher::analyze_conflict<false>(const PropBy confl
    , uint32_t& out_btlevel
    , uint32_t& glue
);

bool Searcher::subset(const vector<Lit>& A, const Clause& B)
{
    //Set seen
    for (uint32_t i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 1;

    bool ret = true;
    for (uint32_t i = 0; i != A.size(); i++) {
        if (!seen[A[i].toInt()]) {
            ret = false;
            break;
        }
    }

    //Clear seen
    for (uint32_t i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 0;

    return ret;
}

void Searcher::analyze_final_confl_with_assumptions(const Lit p, vector<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push_back(p);

    if (decisionLevel() == 0) {
        return;
    }

    //It's been set at level 0. The seen[] may not be large enough to do
    //seen[p.var()] -- we might have mem-saved that
    if (varData[p.var()].level == 0) {
        return;
    }

    seen[p.var()] = 1;

    assert(!trail_lim.empty());
    for (int64_t i = (int64_t)trail.size() - 1; i >= (int64_t)trail_lim[0]; i--) {
        const uint32_t x = trail[i].var();
        if (seen[x]) {
            const PropBy reason = varData[x].reason;
            if (reason.isNULL()) {
                assert(varData[x].level > 0);
                out_conflict.push_back(~trail[i]);
            } else {
                switch(reason.getType()) {
                    case PropByType::clause_t : {
                        const Clause& cl = *cl_alloc.ptr(reason.get_offset());
                        assert(value(cl[0]) == l_True);
                        for(const Lit lit: cl) {
                            if (varData[lit.var()].level > 0) {
                                seen[lit.var()] = 1;
                            }
                        }
                        break;
                    }
                    case PropByType::tertiary_t: {
                        const Lit lit = reason.lit3();
                        if (varData[lit.var()].level > 0) {
                            seen[lit.var()] = 1;
                        }
                        //NO break!
                    }
                    case PropByType::binary_t: {
                        const Lit lit = reason.lit2();
                        if (varData[lit.var()].level > 0) {
                            seen[lit.var()] = 1;
                        }
                        break;
                    }

                    default:
                        assert(false);
                        break;
                }
            }
            seen[x] = 0;
        }
    }
    seen[p.var()] = 0;
}

void Searcher::update_assump_conflict_to_orig_outside(vector<Lit>& out_conflict)
{
    if (assumptions.empty()) {
        return;
    }

    std::sort(assumptions.begin(), assumptions.end());
    std::sort(out_conflict.begin(), out_conflict.end());
    assert(out_conflict.size() <= assumptions.size());
    //They now are in the order where we can go through them linearly

    /*cout << "out_conflict: " << out_conflict << endl;
    cout << "assumptions: ";
    for(AssumptionPair p: assumptions) {
        cout << "inter: " << p.lit_inter << " , outer: " << p.lit_orig_outside << " , ";
    }
    cout << endl;*/

    uint32_t at_assump = 0;
    for(size_t i = 0; i < out_conflict.size(); i++) {
        Lit& lit = out_conflict[i];
        while(lit != ~assumptions[at_assump].lit_inter) {
            at_assump++;
            assert(at_assump < assumptions.size() && "final conflict contains literals that are not from the assumptions!");
        }
        assert(lit == ~assumptions[at_assump].lit_inter);

        //Update to correct outside lit
        lit = ~assumptions[at_assump].lit_orig_outside;
    }
}

void Searcher::check_blocking_restart()
{
    if (conf.do_blocking_restart
        && sumConflicts() > conf.lower_bound_for_blocking_restart
        && hist.glueHist.isvalid()
        && hist.trailDepthHistLonger.isvalid()
        && decisionLevel() > 0
        && trail.size() > hist.trailDepthHistLonger.avg()*conf.blocking_restart_multip
    ) {
        hist.glueHist.clear();
        if (!blocked_restart) {
            stats.blocked_restart_same++;
        }
        blocked_restart = true;
        stats.blocked_restart++;
    }
}

template<bool update_bogoprops>
lbool Searcher::search()
{
    assert(ok);
    const double myTime = cpuTime();

    //Stats reset & update
    if (!update_bogoprops) {
        stats.numRestarts++;
        stats.clauseID_at_start_inclusive = clauseID;
    }
    hist.clear();
    hist.reset_glue_hist_size(conf.shortTermHistorySize);

    assert(solver->prop_at_head());

    //Loop until restart or finish (SAT/UNSAT)
    blocked_restart = false;
    PropBy confl;

    lbool dec_ret = l_Undef;
    while (!params.needToStopSearch
        || !confl.isNULL() //always finish the last conflict
    ) {
        if (!confl.isNULL()) {
            if (((stats.conflStats.numConflicts & 0xfff) == 0xfff)
                && var_decay < conf.var_decay_max
                && !update_bogoprops
            ) {
                var_decay += 0.01;
            }

            #ifdef STATS_NEEDED
            stats.conflStats.update(lastConflictCausedBy);
            #endif

            #ifdef USE_GAUSS
            confl:
            #endif
            print_restart_stat();
            if (!update_bogoprops) {
                #ifdef STATS_NEEDED
                hist.trailDepthHist.push(trail.size()); //TODO  - trail_lim[0]
                #endif
                hist.trailDepthHistLonger.push(trail.size()); //TODO  - trail_lim[0]
                check_blocking_restart();
            }
            if (!handle_conflict<update_bogoprops>(confl)) {
                dump_search_sql(myTime);
                dump_search_loop_stats();
                return l_False;
            }
            reduce_db_if_needed();
            check_need_restart();
        } else {
            assert(ok);
            #ifdef USE_GAUSS
            bool at_least_one_continue = false;
            for (Gaussian* g: gauss_matrixes) {
                gauss_ret ret = g->find_truths();
                switch (ret) {
                    case gauss_cont:
                        at_least_one_continue = true;
                        break;
                    case gauss_false:
                        return l_False;
                    case gauss_confl:
                        confl = g->found_conflict;
                        goto confl;
                    case gauss_nothing:
                        ;
                }
            }
            if (at_least_one_continue) {
                goto prop;
            }
            assert(ok);
            #endif //USE_GAUSS

            if (decisionLevel() == 0
                && !clean_clauses_if_needed()
            ) {
                return l_False;
            };

            dec_ret = new_decision();
            if (dec_ret != l_Undef) {
                dump_search_sql(myTime);
                dump_search_loop_stats();
                return dec_ret;
            }
        }

        #ifdef USE_GAUSS
        prop:
        #endif

        if (update_bogoprops) {
            confl = propagate<update_bogoprops>();
        } else {
            confl = propagate_any_order_fast();
        }
    }
    max_confl_this_phase -= (int64_t)params.conflictsDoneThisRestart;

    cancelUntil(0);
    assert(solver->prop_at_head());
    if (!solver->datasync->syncData()) {
        return l_False;
    }
    dump_search_sql(myTime);
    dump_search_loop_stats();

    return l_Undef;
}

void Searcher::dump_search_sql(const double myTime)
{
    if (solver->sqlStats && conf.dump_individual_search_time) {
        solver->sqlStats->time_passed_min(
            solver
            , "search"
            , cpuTime()-myTime
        );
    }
}

/**
@brief Picks a new decision variable to branch on

@returns l_Undef if it should restart instead. l_False if it reached UNSAT
         (through simplification)
*/
lbool Searcher::new_decision()
{
    Lit next = lit_Undef;
    while (decisionLevel() < assumptions.size()) {
        // Perform user provided assumption:
        Lit p = assumptions[decisionLevel()].lit_inter;
        assert(varData[p.var()].removed == Removed::none);

        if (value(p) == l_True) {
            // Dummy decision level:
            new_decision_level();
            //To store matrix in case it needs to be stored. No new information
            //is meant to be available at this point.
            #ifdef USE_GAUSS
            //These are not supposed to be changed *at all* by the funciton
            //since it has already been called before
            for (Gaussian* g: gauss_matrixes) {
                gauss_ret ret = g->find_truths();
                assert(ret == gauss_nothing);
            }
            #endif //USE_GAUSS

        } else if (value(p) == l_False) {
            analyze_final_confl_with_assumptions(~p, conflict);
            return l_False;
        } else {
            assert(p.var() < nVars());
            stats.decisionsAssump++;
            next = p;
            break;
        }
    }

    if (next == lit_Undef) {
        // New variable decision:
        next = pickBranchLit();

        //No decision taken, because it's SAT
        if (next == lit_Undef)
            return l_True;

        //Update stats
        stats.decisions++;
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    new_decision_level();
    enqueue(next);

    return l_Undef;
}

double Searcher::luby(double y, int x)
{
    int size, seq;
    for (size = 1, seq = 0
        ; size < x + 1
        ; seq++, size = 2 * size + 1
    );

    while (size - 1 != x) {
        size = (size - 1) >> 1;
        seq--;
        x = x % size;
    }

    return std::pow(y, seq);
}

void Searcher::check_need_restart()
{
    if ((stats.conflStats.numConflicts & 0xff) == 0xff) {
        //It's expensive to check the time the time
        if (cpuTime() > conf.maxTime) {
            params.needToStopSearch = true;
        }

        if (must_interrupt_asap())  {
            if (conf.verbosity >= 3)
                cout << "c must_interrupt_asap() is set, restartig as soon as possible!" << endl;
            params.needToStopSearch = true;
        }
    }

    if (params.rest_type == Restart::glue) {
        if (hist.glueHist.isvalid()
            && conf.local_glue_multiplier * hist.glueHist.avg() > hist.glueHistLT.avg()
        ) {
            params.needToStopSearch = true;
        }
    }
    if ((conf.restartType == Restart::glue_geom
        || conf.restartType == Restart::geom
        || conf.restartType == Restart::luby)
        && (int64_t)params.conflictsDoneThisRestart > max_confl_this_phase
    ) {
        params.needToStopSearch = true;
    }

    //Conflict limit reached?
    if (params.conflictsDoneThisRestart > params.conflictsToDo) {
        if (conf.verbosity >= 3) {
            cout
            << "c Over limit of conflicts for this restart"
            << " -- restarting as soon as possible!" << endl;
        }
        params.needToStopSearch = true;
    }
}

void Searcher::add_otf_subsume_long_clauses()
{
    //Hande long OTF subsumption
    for(size_t i = 0; i < otf_subsuming_long_cls.size(); i++) {
        const ClOffset offset = otf_subsuming_long_cls[i];
        Clause& cl = *solver->cl_alloc.ptr(offset);
        #ifdef STATS_NEEDED
        cl.stats.conflicts_made += conf.rewardShortenedClauseWithConfl;
        #endif

        //Find the l_Undef
        size_t at = std::numeric_limits<size_t>::max();
        for(size_t i2 = 0; i2 < cl.size(); i2++) {
            if (value(cl[i2]) == l_Undef) {
                at = i2;
                break;
            }
        }
        assert(at != std::numeric_limits<size_t>::max());
        std::swap(cl[at], cl[0]);
        assert(value(cl[0]) == l_Undef);

        //Find another l_Undef or an l_True
        at = 0;
        for(size_t i2 = 1; i2 < cl.size(); i2++) {
            if (value(cl[i2]) == l_Undef || value(cl[i2]) == l_True) {
                at = i2;
                break;
            }
        }
        assert(cl.size() > 3);

        if (at == 0) {
            //If none found, we have a propagating clause_t
            enqueue(cl[0], decisionLevel() == 0 ? PropBy() : PropBy(offset));

            //Drat
            if (decisionLevel() == 0) {
                *drat << cl[0] << fin;
            }
        } else {
            //We have a non-propagating clause

            std::swap(cl[at], cl[1]);
            assert(value(cl[1]) == l_Undef || value(cl[1]) == l_True);
        }
        solver->attachClause(cl, false);
        cl.setStrenghtened();
    }
    otf_subsuming_long_cls.clear();
}

void Searcher::add_otf_subsume_implicit_clause()
{
    //Handle implicit OTF subsumption
    for(vector<OTFClause>::iterator
        it = otf_subsuming_short_cls.begin(), end = otf_subsuming_short_cls.end()
        ; it != end
        ; ++it
    ) {
        assert(it->size > 1);
        //Find the l_Undef
        size_t at = std::numeric_limits<size_t>::max();
        for(size_t i2 = 0; i2 < it->size; i2++) {
            if (value(it->lits[i2]) == l_Undef) {
                at = i2;
                break;
            }
        }
        assert(at != std::numeric_limits<size_t>::max());
        std::swap(it->lits[at], it->lits[0]);
        assert(value(it->lits[0]) == l_Undef);

        //Find another l_Undef or an l_True
        at = 0;
        for(size_t i2 = 1; i2 < it->size; i2++) {
            if (value(it->lits[i2]) == l_Undef
                || value(it->lits[i2]) == l_True
            ) {
                at = i2;
                break;
            }
        }

        if (at == 0) {
            //If none found, we have a propagation
            //Calculate reason
            PropBy by = PropBy();

            //if decision level is non-zero, we have to be more careful
            if (decisionLevel() != 0) {
                if (it->size == 2) {
                    by = PropBy(it->lits[1], true);
                } else {
                    by = PropBy(it->lits[1], it->lits[2], true);
                }
            }

            //Enqueue this literal, finally
            enqueue(
                it->lits[0]
                , by
            );

            //Drat
            if (decisionLevel() == 0) {
                *drat << it->lits[0] << fin;
            }
        } else {
            //We have a non-propagating clause
            std::swap(it->lits[at], it->lits[1]);
            assert(value(it->lits[1]) == l_Undef
                || value(it->lits[1]) == l_True
            );

            //Attach new binary/tertiary clause
            if (it->size == 2) {
                solver->datasync->signalNewBinClause(it->lits);
                solver->attach_bin_clause(it->lits[0], it->lits[1], true);
            } else {
                solver->attach_tri_clause(it->lits[0], it->lits[1], it->lits[2], true);
            }
        }
    }
    otf_subsuming_short_cls.clear();
}

void Searcher::update_history_stats(size_t backtrack_level, size_t glue)
{
    assert(decisionLevel() > 0);

    //short-term averages
    hist.branchDepthHist.push(decisionLevel());
    hist.branchDepthDeltaHist.push(decisionLevel() - backtrack_level);
    hist.conflSizeHist.push(learnt_clause.size());
    hist.numResolutionsHist.push(antec_data.num());
    hist.trailDepthDeltaHist.push(trail.size() - trail_lim[backtrack_level]);

    //long-term averages
    hist.decisionLevelHistLT.push(decisionLevel());
    hist.backtrackLevelHistLT.push(backtrack_level);

    hist.vsidsVarsAvgLT.push(antec_data.vsids_vars.avg());
    hist.numResolutionsHistLT.push(antec_data.num());
    hist.conflSizeHistLT.push(learnt_clause.size());

    hist.trailDepthHistLT.push(trail.size());
    if (params.rest_type == Restart::glue) {
        hist.glueHistLT.push(glue);
        hist.glueHist.push(glue);
    }
}

void Searcher::attach_and_enqueue_learnt_clause(Clause* cl, bool enq)
{
    switch (learnt_clause.size()) {
        case 0:
            assert(false);
        case 1:
            //Unitary learnt
            stats.learntUnits++;
            if (enq) enqueue(learnt_clause[0]);
            assert(decisionLevel() == 0);

            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif

            break;
        case 2:
            //Binary learnt
            stats.learntBins++;
            solver->datasync->signalNewBinClause(learnt_clause);
            solver->attach_bin_clause(learnt_clause[0], learnt_clause[1], true, enq);
            if (enq) enqueue(learnt_clause[0], PropBy(learnt_clause[1], true));

            #ifdef STATS_NEEDED
            propStats.propsBinRed++;
            #endif
            break;

        case 3:
            //3-long learnt
            stats.learntTris++;
            solver->attach_tri_clause(learnt_clause[0], learnt_clause[1], learnt_clause[2], true, enq);
            if (enq) enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2], true));

            #ifdef STATS_NEEDED
            propStats.propsTriRed++;
            #endif
            break;

        default:
            //Long learnt
            stats.learntLongs++;
            solver->attachClause(*cl, enq);
            if (enq) enqueue(learnt_clause[0], PropBy(cl_alloc.get_offset(cl)));
            bumpClauseAct(cl);

            #ifdef STATS_NEEDED
            cl->stats.antec_data = antec_data;
            propStats.propsLongRed++;
            #endif

            break;
    }
}

inline void Searcher::print_learning_debug_info() const
{
    #ifndef VERBOSE_DEBUG
    return;
    #else
    cout
    << "Learning:" << learnt_clause
    << endl
    << "reverting var " << learnt_clause[0].var()+1
    << " to " << !learnt_clause[0].sign()
    << endl;
    #endif
}

void Searcher::print_learnt_clause() const
{
    if (conf.verbosity >= 6) {
        cout
        << "c learnt clause: "
        << learnt_clause
        << endl;
    }
}

#ifdef STATS_NEEDED
void Searcher::dump_sql_clause_data(
    const uint32_t glue
    , const uint32_t backtrack_level
) {
    solver->sqlStats->dump_clause_stats(
        solver
        , clauseID
        , glue
        , backtrack_level
        , learnt_clause.size()
        , antec_data
        , decisionLevel()
        , trail.size()
        , params.conflictsDoneThisRestart
        , hist
    );
}
#endif

bool Searcher::check_and_insert_into_hash_learnt_cl()
{
    if (!conf.hash_relearn_check) {
        return false;
    }

    if (hits == NULL
        && solver->get_num_long_irred_cls() > 30000 //TODO: MAGIC CONSTANT
    ) {
        hits = new uint32_t[hash_size];
    }

    //Don't do it for very large clauses, like 400 etc.
    if (learnt_clause.size() > 40  //TODO MAGIC CONSTANT
        || hits == NULL
    ) {
        return false;
    }

    learnt_clause_sorted.resize(learnt_clause.size());
    std::copy(learnt_clause.begin(), learnt_clause.end()
        , learnt_clause_sorted.begin());
    std::sort(learnt_clause_sorted.begin(), learnt_clause_sorted.end());

    uint64_t hash = clause_hash(learnt_clause_sorted);
    uint32_t hash_i = hash & hash_mask;
    uint32_t hash_j = (1UL << (hash >> hash_bits) ) & (8 * sizeof(hits[0]) - 1);

    if (hits[hash_i] & hash_j) {
        //It seems we've already learnt this clause before.
        //Let's make sure we don't forget it this time.
        hits[hash_i] &= ~hash_j;
        return true;
    } else {
        //Even if we forget this learnt clause, we can still
        //prevent us from unlearning it again in the future
        hits[hash_i] |= hash_j;
        return false;
    }
}

Clause* Searcher::handle_last_confl_otf_subsumption(
    Clause* cl
    , const uint32_t glue
    , const uint32_t
    backtrack_level
) {
    //Cannot make a non-implicit into an implicit
    if (learnt_clause.size() <= 3) {
        *drat << learnt_clause << fin;
        return NULL;
    }

    //No on-the-fly subsumption
    if (cl == NULL || cl->gauss_temp_cl()) {
        cl = cl_alloc.Clause_new(learnt_clause
        #ifdef STATS_NEEDED
        , sumConflicts()
        , clauseID
        #endif
        );
        cl->makeRed(glue);
        ClOffset offset = cl_alloc.get_offset(cl);
        unsigned which_arr;
        if (cl->stats.glue <= conf.glue_must_keep_clause_if_below_or_eq) {
            which_arr = 0;
        } else {
            which_arr = 1;
        }
        if (which_arr == 0) {
            stats.red_cl_in_which0++;
        }
        if (conf.guess_cl_effectiveness) {
            unsigned guess = guess_clause_array(cl->stats.glue, backtrack_level, 7.5, 0.2);
            if (guess < which_arr) {
                stats.guess_different++;
            }
            if (guess == 0) {
                cl->stats.ttl = 1;
            }
        }
        const bool inside_hash = check_and_insert_into_hash_learnt_cl();
        if (inside_hash && which_arr > 0) {
            which_arr = 0;
            stats.cache_hit++;
        }

        cl->stats.which_red_array = which_arr;
        solver->longRedCls[cl->stats.which_red_array].push_back(offset);
        *drat << *cl << fin;

        #ifdef STATS_NEEDED
        if (solver->sqlStats
            && drat
            && conf.dump_individual_restarts_and_clauses
            && learnt_clause.size() > 3
        ) {
            dump_sql_clause_data(
                glue
                , backtrack_level
            );
        }
        #endif
        clauseID++;

        return cl;
    }

    assert(cl->size() > 3);
    *drat << learnt_clause << fin;
    if (conf.verbosity >= 6) {
        cout
        << "Detaching OTF subsumed (LAST) clause:"
        << *cl
        << endl;
    }
    solver->detachClause(*cl);
    assert(cl->size() > learnt_clause.size());

    //Shrink clause
    for (uint32_t i = 0; i < learnt_clause.size(); i++) {
        (*cl)[i] = learnt_clause[i];
    }
    cl->resize(learnt_clause.size());
    assert(cl->size() == learnt_clause.size());

    //Update stats
    if (cl->red() && cl->stats.glue > glue) {
        cl->stats.glue = glue;
    }
    #ifdef STATS_NEEDED
    cl->stats.conflicts_made += conf.rewardShortenedClauseWithConfl;
    #endif

    return cl;
}

template<bool update_bogoprops>
bool Searcher::handle_conflict(const PropBy confl)
{
    uint32_t backtrack_level;
    uint32_t glue;
    antec_data.clear();
    stats.conflStats.numConflicts++;
    params.conflictsDoneThisRestart++;
    if (conf.doPrintConflDot)
        create_graphviz_confl_graph(confl);

    if (decisionLevel() == 0)
        return false;

    Clause* cl = analyze_conflict<update_bogoprops>(
        confl
        , backtrack_level  //return backtrack level here
        , glue             //return glue here
    );
    print_learnt_clause();


    //Add decision-based clause in case it's short
    decision_clause.clear();
    if (!update_bogoprops
        && learnt_clause.size() > 50 //TODO MAGIC parameter
        && decisionLevel() <= 9
        && decisionLevel() >= 2
    ) {
        for(int i = (int)trail_lim.size()-1; i >= 0; i--) {
            decision_clause.push_back(~trail[trail_lim[i]]);
        }
    }

    if (!update_bogoprops) {
        update_history_stats(backtrack_level, glue);
    }
    cancelUntil(backtrack_level);

    add_otf_subsume_long_clauses();
    add_otf_subsume_implicit_clause();
    print_learning_debug_info();
    assert(value(learnt_clause[0]) == l_Undef);
    glue = std::min<uint32_t>(glue, std::numeric_limits<uint32_t>::max());
    cl = handle_last_confl_otf_subsumption(cl, glue, backtrack_level);
    assert(learnt_clause.size() <= 3 || cl != NULL);
    attach_and_enqueue_learnt_clause(cl);

    //Add decision-based clause
    if (!update_bogoprops
        && decision_clause.size() > 0
    ) {
        int i = decision_clause.size();
        while(--i >= 0) {
            if (value(decision_clause[i]) == l_True
                || value(decision_clause[i]) == l_Undef
            ) {
                break;
            }
        }
        std::swap(decision_clause[0], decision_clause[i]);
        learnt_clause = decision_clause;
        cl = handle_last_confl_otf_subsumption(NULL, learnt_clause.size(), decisionLevel());
        attach_and_enqueue_learnt_clause(cl, false);
    }

    if (!update_bogoprops) {
        varDecayActivity();
        decayClauseAct();
    }

    return true;
}
template bool Searcher::handle_conflict<true>(const PropBy confl);
template bool Searcher::handle_conflict<false>(const PropBy confl);

void Searcher::resetStats()
{
    startTime = cpuTime();

    //Rest solving stats
    stats.clear();
    propStats.clear();
    #ifdef STATS_NEEDED
    lastSQLPropStats = propStats;
    lastSQLGlobalStats = stats;
    #endif

    lastCleanZeroDepthAssigns = trail.size();
}

lbool Searcher::burst_search()
{
    const double myTime = cpuTime();
    const size_t numUnitsUntilNow = stats.learntUnits;
    const size_t numBinsUntilNow = stats.learntBins;

    //Save old config
    const double backup_rand = conf.random_var_freq;
    const PolarityMode backup_polar_mode = conf.polarity_mode;
    Restart backup_restart_type = params.rest_type;
    double backup_var_inc = var_inc;
    double backup_var_decay = var_decay;

    //Set burst config
    conf.random_var_freq = 1;
    conf.polarity_mode = PolarityMode::polarmode_rnd;

    //Do burst
    params.clear();
    params.conflictsToDo = conf.burst_search_len;
    params.rest_type = Restart::never;
    lbool status = search<true>();

    //Restore config
    conf.random_var_freq = backup_rand;
    conf.polarity_mode = backup_polar_mode;
    params.rest_type = backup_restart_type;
    assert(var_inc == backup_var_inc);
    assert(var_decay == backup_var_decay);

    //Print what has happened
    const double time_used = cpuTime() - myTime;
    if (conf.verbosity) {
        cout
        << "c "
        << conf.burst_search_len << "-long burst search "
        << " learnt units:" << (stats.learntUnits - numUnitsUntilNow)
        << " learnt bins: " << (stats.learntBins - numBinsUntilNow)
        << solver->conf.print_times(time_used)
        << endl;
    }

    return status;
}

void Searcher::print_restart_header() const
{
    cout
    << "c"
    << " " << std::setw(5) << "rest"
    << " " << std::setw(5) << "conf"
    << " " << std::setw(5) << "freevar"
    << " " << std::setw(5) << "IrrL"
    << " " << std::setw(5) << "IrrT"
    << " " << std::setw(5) << "IrrB"
    << " " << std::setw(7) << "l/longC"
    << " " << std::setw(7) << "l/allC";

    for(size_t i = 0; i < longRedCls.size(); i++) {
        cout << " " << std::setw(4) << "RedL" << i;
    }

    cout << " " << std::setw(5) << "RedT"
    << " " << std::setw(5) << "RedB"
    << " " << std::setw(7) << "l/longC"
    << " " << std::setw(7) << "l/allC"
    << endl;
}

void Searcher::print_restart_stat_line() const
{
    printBaseStats();
    if (conf.print_all_stats) {
        solver->print_clause_stats();
        hist.print();
    } else {
        solver->print_clause_stats();
    }

    cout << endl;
}

void Searcher::printBaseStats() const
{
    cout
    << "c"
    //<< omp_get_thread_num()
    << " " << std::setw(5) << sumRestarts();

    if (sumConflicts() >  20000) {
        cout
        << " " << std::setw(4) << sumConflicts()/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << sumConflicts();
    }

    cout
    << " " << std::setw(7) << solver->get_num_free_vars()
    ;
}

struct MyInvSorter {
    bool operator()(size_t num, size_t num2)
    {
        return num > num2;
    }
};

struct MyPolarData
{
    MyPolarData (size_t _pos, size_t _neg, size_t _flipped) :
        pos(_pos)
        , neg(_neg)
        , flipped(_flipped)
    {}

    size_t pos;
    size_t neg;
    size_t flipped;

    bool operator<(const MyPolarData& other) const
    {
        return (pos + neg) > (other.pos + other.neg);
    }
};

#ifdef STATS_NEEDED
inline void Searcher::dump_restart_sql()
{
    //Propagation stats
    PropStats thisPropStats = propStats - lastSQLPropStats;
    SearchStats thisStats = stats - lastSQLGlobalStats;
    thisStats.clauseID_at_start_inclusive = stats.clauseID_at_start_inclusive;
    thisStats.clauseID_at_end_exclusive = clauseID;

    solver->sqlStats->restart(
        thisPropStats
        , thisStats
        , solver
        , this
    );

    lastSQLPropStats = propStats;
    lastSQLGlobalStats = stats;
}
#endif

void Searcher::print_restart_stat()
{
    //Print restart stat
    if (conf.verbosity
        && ((lastRestartPrint + conf.print_restart_line_every_n_confl)
          < sumConflicts())
    ) {
        //Print restart output header
        if (lastRestartPrintHeader == 0
            ||(lastRestartPrintHeader + 20000) < sumConflicts()
        ) {
            print_restart_header();
            lastRestartPrintHeader = sumConflicts();
        }
        print_restart_stat_line();
        lastRestartPrint = sumConflicts();
    }
}

void Searcher::reset_temp_cl_num()
{
    conf.cur_max_temp_red_cls = conf.max_temporary_learnt_clauses;
}

void Searcher::reduce_db_if_needed()
{
    if (longRedCls[1].size() <= conf.cur_max_temp_red_cls) {
        return;
    }

    if (conf.verbosity >= 3) {
        cout
        << "c "
        << " cleaning"
        << " numConflicts : " << stats.conflStats.numConflicts
        << " SumConfl: " << sumConflicts()
        << " max_confl_per_search_solve_call:" << max_confl_per_search_solve_call
        << " Trail size: " << trail.size() << endl;
    }
    solver->reduceDB->reduce_db_and_update_reset_stats();
    if (conf.verbosity >= 3) {
        watches.print_stat();
    }

    cl_alloc.consolidate(solver);
    conf.cur_max_temp_red_cls *= conf.inc_max_temp_red_cls;
}

bool Searcher::clean_clauses_if_needed()
{
    assert(decisionLevel() == 0);

    if (!ok || !propagate_any_order_fast().isNULL()) {
        return ok = false;
    }

    const size_t newZeroDepthAss = trail.size() - lastCleanZeroDepthAssigns;
    if (newZeroDepthAss > 0
        && simpDB_props < 0
    ) {
        if (conf.verbosity) {
            cout << "c newZeroDepthAss : " << newZeroDepthAss  << endl;
        }
        lastCleanZeroDepthAssigns = trail.size();
        solver->clauseCleaner->remove_and_clean_all();

        cl_alloc.consolidate(solver);
        rebuildOrderHeap(); //TODO only filter is needed!
        sortWatched();
        simpDB_props = (litStats.redLits + litStats.irredLits)<<5;
    }

    return true;
}

void Searcher::rebuildOrderHeap()
{
    vec<uint32_t> vs;
    for (uint32_t v = 0; v < nVars(); v++) {
        if (varData[v].removed == Removed::none
            && value(v) == l_Undef
        ) {
            vs.push(v);
        }
    }
    order_heap_glue.build(vs);
}

//NOTE: as per AWS check, doing this in Searcher::solve() loop is _detrimental_
//      to performance. Solved 2 less in 3600s of SATRace'14
lbool Searcher::perform_scc_and_varreplace_if_needed()
{
    if (conf.doFindAndReplaceEqLits
            && (solver->binTri.numNewBinsSinceSCC
                > ((double)solver->get_num_free_vars()*conf.sccFindPercent))
    ) {
        if (conf.verbosity) {
            cout
            << "c new bins since last SCC: "
            << std::setw(2)
            << solver->binTri.numNewBinsSinceSCC
            << " free vars %:"
            << std::fixed << std::setprecision(2) << std::setw(4)
            << stats_line_percent(solver->binTri.numNewBinsSinceSCC, solver->get_num_free_vars())
            << endl;
        }

        solver->clauseCleaner->remove_and_clean_all();

        lastCleanZeroDepthAssigns = trail.size();
        if (!solver->varReplacer->replace_if_enough_is_found(std::floor((double)solver->get_num_free_vars()*0.001))) {
            return l_False;
        }
        #ifdef SLOW_DEBUG
        assert(solver->check_order_heap_sanity());
        #endif
    }

    return l_Undef;
}

inline void Searcher::dump_search_loop_stats()
{
    #ifdef STATS_NEEDED
    if (sqlStats
        && conf.dump_individual_restarts_and_clauses
    ) {
        dump_restart_sql();
    }
    #endif
}

bool Searcher::must_abort(const lbool status) {
    if (status != l_Undef) {
        if (conf.verbosity >= 6) {
            cout
            << "c Returned status of search() is non-l_Undef at loop "
            << loop_num
            << " confl:"
            << sumConflicts()
            << endl;
        }
        return true;
    }

    if (stats.conflStats.numConflicts >= max_confl_per_search_solve_call) {
        if (conf.verbosity >= 3) {
            cout
            << "c search over max conflicts"
            << endl;
        }
        return true;
    }

    if (cpuTime() >= conf.maxTime) {
        if (conf.verbosity >= 3) {
            cout
            << "c search over max time"
            << endl;
        }
        return true;
    }

    if (solver->must_interrupt_asap()) {
        if (conf.verbosity >= 3) {
            cout
            << "c search interrupting as requested"
            << endl;
        }
        return true;
    }

    return false;
}

void Searcher::print_search_loop_num()
{
    if (conf.verbosity >= 6) {
        cout
        << "c search loop " << loop_num
        << endl;
    }
}

lbool Searcher::solve(
    const uint64_t _maxConfls
    , const unsigned upper_level_iteration_num
) {
    assert(ok);
    assert(qhead == trail.size());
    max_confl_per_search_solve_call = _maxConfls;
    num_search_called++;
    #ifdef SLOW_DEBUG
    //When asking for a lot of simple soluitons, search() gets called a lot
    check_no_removed_or_freed_cl_in_watch();
    #endif

    if (solver->conf.verbosity >= 6) {
        cout
        << "c Searcher::solve() called"
        << endl;
    }

    #ifdef USE_GAUSS
    for (Gaussian* g : gauss_matrixes) {
        if (!g->init_until_fixedpoint()) {
            return l_False;
        }
    }
    #endif

    resetStats();
    lbool status = l_Undef;
    if (conf.burst_search_len > 0
        && upper_level_iteration_num > 0
    ) {
        assert(solver->check_order_heap_sanity());
        status = burst_search();
        if (status != l_Undef)
            goto end;
    }

    if (conf.restartType == Restart::geom) {
        max_confl_phase = conf.restart_first;
        max_confl_this_phase = conf.restart_first;
        params.rest_type = Restart::geom;
    }

    if (conf.restartType == Restart::luby) {
        max_confl_this_phase = conf.restart_first;
        params.rest_type = Restart::luby;
    }

    assert(solver->check_order_heap_sanity());
    for(loop_num = 0
        ; stats.conflStats.numConflicts < max_confl_per_search_solve_call
        ; loop_num ++
    ) {
        #ifdef SLOW_DEBUG
        assert(order_heap_glue.heap_property());
        assert(solver->check_order_heap_sanity());
        #endif

        assert(watches.get_smudged_list().empty());
        print_search_loop_num();

        lastRestartConfl = sumConflicts();
        params.clear();
        params.conflictsToDo = max_confl_per_search_solve_call-stats.conflStats.numConflicts;
        if (params.rest_type == Restart::glue_geom) {
            params.rest_type = Restart::geom;
        }
        status = search<false>();

        if (conf.restartType == Restart::geom
            && max_confl_this_phase <= 0
        ) {
            max_confl_phase *= conf.restart_inc;
            max_confl_this_phase = max_confl_phase;
        }

        if (conf.restartType == Restart::luby) {
            max_confl_this_phase = luby(conf.restart_inc*1.5, loop_num) * (double)conf.restart_first/2.0;
        }

        if (conf.restartType == Restart::glue_geom
            && max_confl_this_phase <= 0
        ) {
            if (params.rest_type == Restart::geom) {
                params.rest_type = Restart::glue;
            } else {
                params.rest_type = Restart::geom;
            }
            switch (params.rest_type) {
                case Restart::geom:
                    max_confl_phase = (double)max_confl_phase * conf.restart_inc;
                    max_confl_this_phase = max_confl_phase;
                    break;

                case Restart::glue:
                    max_confl_this_phase = 2*max_confl_phase;
                    break;

                default:
                    release_assert(false);
            }
            if (conf.verbosity >= 3) {
                cout << "Phase is now " << std::setw(10) << getNameOfRestartType(params.rest_type)
                << " this phase size: " << max_confl_this_phase
                << " global phase size: " << max_confl_phase << endl;
            }

            //don't go into the geom phase in case we would stop it due to simplification
            if (conf.abort_searcher_solve_on_geom_phase
                && params.rest_type  == Restart::geom
                && max_confl_this_phase + stats.conflStats.numConflicts  > max_confl_per_search_solve_call
            ) {
                if (conf.verbosity) {
                    cout << "c Returning from Searcher::solve() due to phase change and insufficient conflicts left" << endl;
                }
                goto end;
            }
        }

        if (must_abort(status)) {
            goto end;
        }
    }

    end:
    finish_up_solve(status);

    return status;
}

void Searcher::print_solution_varreplace_status() const
{
    for(size_t var = 0; var < nVarsOuter(); var++) {
        if (varData[var].removed == Removed::replaced
            || varData[var].removed == Removed::elimed
        ) {
            assert(value(var) == l_Undef || varData[var].level == 0);
        }

        if (conf.verbosity >= 6
            && varData[var].removed == Removed::replaced
            && value(var) != l_Undef
        ) {
            cout
            << "var: " << var
            << " value: " << value(var)
            << " level:" << varData[var].level
            << " type: " << removed_type_to_string(varData[var].removed)
            << endl;
        }
    }
}

void Searcher::print_solution_type(const lbool status) const
{
    if (conf.verbosity >= 6) {
        if (status == l_True) {
            cout << "Solution from Searcher is SAT" << endl;
        } else if (status == l_False) {
            cout << "Solution from Searcher is UNSAT" << endl;
            cout << "OK is: " << okay() << endl;
        } else {
            cout << "Solutions from Searcher is UNKNOWN" << endl;
        }
    }
}

void Searcher::finish_up_solve(const lbool status)
{
    print_solution_type(status);

    if (status == l_True) {
        double myTime = cpuTime();
        model = assigns;
        full_model = assigns;
        if (conf.greedy_undef) {
            vector<uint32_t> trail_lim_vars;
            for(size_t i = 0; i < decisionLevel(); i++) {
                uint32_t at = trail_lim[i];

                //Yes, it can be equal -- when dummy decision levels are added
                //and a solution is found
                if (at < trail.size()) {
                    uint32_t v = trail[at].var();
                    trail_lim_vars.push_back(v);
                    //cout << "var at " << i << " of trail_lim : "<< v+1 << endl;
                }
            }
            cancelUntil(0);
            const uint32_t unset = solver->undefine(trail_lim_vars);

            if (conf.verbosity) {
                cout << "c [undef] Freed up " << unset << " var(s)"
                << conf.print_times(cpuTime()-myTime)
                << endl;
            }
            if (sqlStats) {
                sqlStats->time_passed_min(
                    solver
                    , "greedy undefine"
                    , cpuTime()-myTime
                );
            }
        } else {
            cancelUntil(0);
        }
        print_solution_varreplace_status();
    } else if (status == l_False) {
        if (conflict.size() == 0) {
            ok = false;
        }
        cancelUntil(0);
    }

    stats.cpu_time = cpuTime() - startTime;
    if (conf.verbosity >= 4) {
        cout << "c Searcher::solve() finished"
        << " status: " << status
        << " numConflicts : " << stats.conflStats.numConflicts
        << " SumConfl: " << sumConflicts()
        << " max_confl_per_search_solve_call:" << max_confl_per_search_solve_call
        << endl;
    }

    print_iteration_solving_stats();
}

void Searcher::print_iteration_solving_stats()
{
    if (conf.verbosity >= 3) {
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
        stats.print(propStats.propagations);
        propStats.print(stats.cpu_time);
        print_stats_line("c props/decision"
            , float_div(propStats.propagations, stats.decisions)
        );
        print_stats_line("c props/conflict"
            , float_div(propStats.propagations, stats.conflStats.numConflicts)
        );
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
    }
}

Lit Searcher::pickBranchLit()
{
    #ifdef VERBOSE_DEBUG
    cout << "picking decision variable, dec. level: " << decisionLevel() << " ";
    #endif

    Lit next = lit_Undef;

    // Random decision:
    if (conf.random_var_freq > 0) {
        double rand = mtrand.randDblExc();
        double frq = conf.random_var_freq;
        if (rand < frq && !order_heap_glue.empty()) {
            const uint32_t next_var = order_heap_glue.random_element(mtrand);

            if (value(next_var) == l_Undef
                && solver->varData[next_var].removed == Removed::none
            ) {
                stats.decisionsRand++;
                next = Lit(next_var, !pickPolarity(next_var));
            }
        }
    }

    // Activity based decision:
    if (next == lit_Undef) {
        uint32_t next_var = var_Undef;
        while (next_var == var_Undef
          || value(next_var) != l_Undef
        ) {
            //There is no more to branch on. Satisfying assignment found.
            if (order_heap_glue.empty()) {
                return lit_Undef;
            }
            next_var = order_heap_glue.removeMin();
        }
        next = Lit(next_var, !pickPolarity(next_var));
    }

    //Flip polaritiy if need be
    /*if (mtrand.randInt(50) == 1
        && next != lit_Undef
    ) {
        next ^= true;
        stats.decisionFlippedPolar++;
    }*/

    //No vars in heap: solution found
    #ifdef SLOW_DEBUG
    if (next != lit_Undef) {
        assert(solver->varData[next.var()].removed == Removed::none);
    }
    #endif
    return next;
}

void Searcher::cache_based_more_minim(vector<Lit>& cl)
{
    int64_t limit = more_red_minim_limit_cache_actual;
    const size_t first_n_lits_of_cl =
        std::min<size_t>(conf.max_num_lits_more_red_min, cl.size());
    for (size_t at_lit = 0; at_lit < first_n_lits_of_cl; at_lit++) {
        Lit lit = cl[at_lit];

        //Timeout
        if (limit < 0)
            break;

        //Already removed this literal
        if (seen[lit.toInt()] == 0)
            continue;

        assert(solver->implCache.size() > lit.toInt());
        const TransCache& cache1 = solver->implCache[lit];
        limit -= (int64_t)cache1.lits.size()/2;
        for (const LitExtra litExtra: cache1.lits) {
            assert(seen.size() > litExtra.getLit().toInt());
            if (seen[(~(litExtra.getLit())).toInt()]) {
                stats.cacheShrinkedClause++;
                seen[(~(litExtra.getLit())).toInt()] = 0;
            }
        }
    }
}

void Searcher::binary_based_more_minim(vector<Lit>& cl)
{
    int64_t limit  = more_red_minim_limit_binary_actual;
    const size_t first_n_lits_of_cl =
        std::min<size_t>(conf.max_num_lits_more_red_min, cl.size());
    for (size_t at_lit = 0; at_lit < first_n_lits_of_cl; at_lit++) {
        Lit lit = cl[at_lit];
        //Already removed this literal
        if (seen[lit.toInt()] == 0)
            continue;

        //Watchlist-based minimisation
        watch_subarray_const ws = watches[lit];
        for (const Watched* i = ws.begin() , *end = ws.end()
            ; i != end && limit > 0
            ; i++
        ) {
            limit--;
            if (i->isBin()) {
                if (seen[(~i->lit2()).toInt()]) {
                    stats.binTriShrinkedClause++;
                    seen[(~i->lit2()).toInt()] = 0;
                }
                continue;
            }

            if (i->isTri()) {
                if (seen[i->lit3().toInt()]) {
                    if (seen[(~i->lit2()).toInt()]) {
                        stats.binTriShrinkedClause++;
                        seen[(~i->lit2()).toInt()] = 0;
                    }
                }
                if (seen[i->lit2().toInt()]) {
                    if (seen[(~i->lit3()).toInt()]) {
                        stats.binTriShrinkedClause++;
                        seen[(~i->lit3()).toInt()] = 0;
                    }
                }
                continue;
            }
            break;
        }
    }
}

void Searcher::minimise_redundant_more(vector<Lit>& cl)
{
    /*if (conf.doStamp&& conf.more_otf_shrink_with_stamp) {
        stamp_based_more_minim(learnt_clause);
    }*/

    stats.furtherShrinkAttempt++;
    for (const Lit lit: cl) {
        seen[lit.toInt()] = 1;
    }

    if (conf.doCache && conf.more_otf_shrink_with_cache) {
        cache_based_more_minim(cl);
    }

    binary_based_more_minim(cl);

    //Finally, remove the literals that have seen[literal] = 0
    //Here, we can count do stats, etc.
    bool changedClause  = false;
    vector<Lit>::iterator i = cl.begin();
    vector<Lit>::iterator j= i;

    //never remove the 0th literal -- TODO this is a bad thing
    //we should be able to remove this, but I can't figure out how to
    //reorder the clause then
    seen[cl[0].toInt()] = 1;
    for (vector<Lit>::iterator end = cl.end(); i != end; i++) {
        if (seen[i->toInt()]) {
            *j++ = *i;
        } else {
            changedClause = true;
        }
        seen[i->toInt()] = 0;
    }
    stats.furtherShrinkedSuccess += changedClause;
    cl.resize(cl.size() - (i-j));
}

void Searcher::stamp_based_more_minim(vector<Lit>& cl)
{
    //Stamp-based minimization
    stats.stampShrinkAttempt++;
    const size_t origSize = cl.size();

    Lit firstLit = cl[0];
    std::pair<size_t, size_t> tmp;
    tmp = stamp.stampBasedLitRem(cl, STAMP_RED);
    if (tmp.first || tmp.second) {
        //cout << "Rem RED: " << tmp.first + tmp.second << endl;
    }
    tmp = stamp.stampBasedLitRem(cl, STAMP_IRRED);
    if (tmp.first || tmp.second) {
        //cout << "Rem IRRED: " << tmp.first + tmp.second << endl;
    }

    //Handle removal or moving of the first literal
    size_t at = std::numeric_limits<size_t>::max();
    for(size_t i = 0; i < cl.size(); i++) {
        if (cl[i] == firstLit) {
            at = i;
            break;
        }
    }
    if (at != std::numeric_limits<size_t>::max()) {
        //Make original first lit first in the final clause, too
        std::swap(cl[0], cl[at]);
    } else {
        //Re-add first lit
        cl.push_back(lit_Undef);
        std::swap(cl[0], cl[cl.size()-1]);
        cl[0] = firstLit;
    }

    stats.stampShrinkCl += ((origSize - cl.size()) > 0);
    stats.stampShrinkLit += origSize - cl.size();
}

bool Searcher::VarFilter::operator()(uint32_t var) const
{
    return (cc->value(var) == l_Undef && solver->varData[var].removed == Removed::none);
}

uint64_t Searcher::sumConflicts() const
{
    return solver->sumSearchStats.conflStats.numConflicts + stats.conflStats.numConflicts;
}

uint64_t Searcher::sumRestarts() const
{
    return stats.numRestarts + solver->get_stats().numRestarts;
}

size_t Searcher::hyper_bin_res_all(const bool check_for_set_values)
{
    size_t added = 0;

    for(std::set<BinaryClause>::const_iterator
        it = solver->needToAddBinClause.begin()
        , end = solver->needToAddBinClause.end()
        ; it != end
        ; ++it
    ) {
        lbool val1 = value(it->getLit1());
        lbool val2 = value(it->getLit2());

        if (conf.verbosity >= 6) {
            cout
            << "c Attached hyper-bin: "
            << it->getLit1() << "(val: " << val1 << " )"
            << ", " << it->getLit2() << "(val: " << val2 << " )"
            << endl;
        }

        //If binary is satisfied, skip
        if (check_for_set_values
            && (val1 == l_True || val2 == l_True)
        ) {
            continue;
        }

        if (check_for_set_values) {
            assert(val1 == l_Undef && val2 == l_Undef);
        }

        solver->attach_bin_clause(it->getLit1(), it->getLit2(), true, false);
        added++;
    }
    solver->needToAddBinClause.clear();

    return added;
}

std::pair<size_t, size_t> Searcher::remove_useless_bins(bool except_marked)
{
    size_t removedIrred = 0;
    size_t removedRed = 0;

    if (conf.doTransRed) {
        for(std::set<BinaryClause>::iterator
            it = uselessBin.begin()
            , end = uselessBin.end()
            ; it != end
            ; ++it
        ) {
            propStats.otfHyperTime += 2;
            if (solver->conf.verbosity >= 10) {
                cout << "Removing binary clause: " << *it << endl;
            }
            propStats.otfHyperTime += solver->watches[it->getLit1()].size()/2;
            propStats.otfHyperTime += solver->watches[it->getLit2()].size()/2;
            bool removed;
            if (except_marked) {
                bool rem1 = removeWBin_except_marked(solver->watches, it->getLit1(), it->getLit2(), it->isRed());
                bool rem2 = removeWBin_except_marked(solver->watches, it->getLit2(), it->getLit1(), it->isRed());
                assert(rem1 == rem2);
                removed = rem1;
            } else {
                removeWBin(solver->watches, it->getLit1(), it->getLit2(), it->isRed());
                removeWBin(solver->watches, it->getLit2(), it->getLit1(), it->isRed());
                removed = true;
            }

            if (!removed) {
                continue;
            }

            //Update stats
            if (it->isRed()) {
                solver->binTri.redBins--;
                removedRed++;
            } else {
                solver->binTri.irredBins--;
                removedIrred++;
            }
            *drat << del << it->getLit1() << it->getLit2() << fin;

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Removed bin: "
            << it->getLit1() << " , " << it->getLit2()
            << " , red: " << it->isRed() << endl;
            #endif
        }
    }
    uselessBin.clear();

    return std::make_pair(removedIrred, removedRed);
}

string Searcher::analyze_confl_for_graphviz_graph(
    PropBy conflHalf
    , uint32_t& out_btlevel
    , uint32_t &glue
) {
    pathC = 0;
    Lit p = lit_Undef;

    learnt_clause.clear();
    learnt_clause.push_back(lit_Undef);      // (leave room for the asserting literal)
    int index   = trail.size() - 1;
    out_btlevel = 0;
    std::stringstream resolutions_str;

    PropByForGraph confl(conflHalf, failBinLit, cl_alloc);
    do {
        assert(!confl.isNULL());          // (otherwise should be UIP)

        //Update antec_data output
        if (p != lit_Undef) {
            resolutions_str << " | ";
        }
        resolutions_str << "{ " << confl << " | " << pathC << " -- ";

        for (uint32_t j = (p == lit_Undef) ? 0 : 1, size = confl.size(); j != size; j++) {
            Lit q = confl[j];
            const uint32_t my_var = q.var();

            if (!seen[my_var] //if already handled, don't care
                && varData[my_var].level > 0 //if it's assigned at level 0, it's assigned FALSE, so leave it out
            ) {
                seen[my_var] = 1;
                assert(varData[my_var].level <= decisionLevel());

                if (varData[my_var].level == decisionLevel()) {
                    pathC++;
                } else {
                    learnt_clause.push_back(q);

                    //Backtracking level is largest of thosee inside the clause
                    if (varData[my_var].level > out_btlevel)
                        out_btlevel = varData[my_var].level;
                }
            }
        }
        resolutions_str << pathC << " }";

        //Go through the trail backwards, select the one that is to be resolved
        while (!seen[trail[index--].var()]);

        p = trail[index+1];
        confl = PropByForGraph(varData[p.var()].reason, p, cl_alloc);
        seen[p.var()] = 0; // this one is resolved
        pathC--;
    } while (pathC > 0); //UIP when eveything goes through this one
    assert(pathC == 0);
    learnt_clause[0] = ~p;

    // clear out seen
    for (uint32_t j = 0; j != learnt_clause.size(); j++)
        seen[learnt_clause[j].var()] = 0;    // ('seen[]' is now cleared)

    //Calculate glue
    glue = calc_glue(learnt_clause);

    return resolutions_str.str();
}

void Searcher::print_edges_for_graphviz_file(std::ofstream& file) const
{
    for (const Lit lit: trail) {

        //0-decision level means it's pretty useless to put into the impl. graph
        if (varData[lit.var()].level == 0) continue;

        //Not directly connected with the conflict, drop
        if (!seen[lit.var()]) continue;

        PropBy reason = varData[lit.var()].reason;

        //A decision variable, it is not propagated by any clause
        if (reason.isNULL()) continue;

        PropByForGraph prop(reason, lit, cl_alloc);
        for (uint32_t i = 0; i < prop.size(); i++) {
            if (prop[i] == lit //This is being propagated, don't make a circular line
                || varData[prop[i].var()].level == 0 //'clean' clauses of 0-level lits
            ) continue;

            file << "x" << prop[i].unsign() << " -> x" << lit.unsign() << " "
            << "[ "
            << " label=\"";
            for(uint32_t i2 = 0; i2 < prop.size();) {
                //'clean' clauses of 0-level lits
                if (varData[prop[i2].var()].level == 0) {
                    i2++;
                    continue;
                }

                file << prop[i2];
                i2++;
                if (i2 != prop.size()) file << " ";
            }
            file << "\""
            << " , fontsize=8"
            << " ];" << endl;
        }
    }
}

void Searcher::print_vertex_definitions_for_graphviz_file(std::ofstream& file)
{
    for (size_t i = 0; i < trail.size(); i++) {
        Lit lit = trail[i];

        //Only vertexes that really have been used
        if (seen[lit.var()] == 0) continue;
        seen[lit.var()] = 0;

        file << "x" << lit.unsign()
        << " [ "
        << " shape=\"box\""
        //<< ", size = 0.8"
        << ", style=\"filled\"";
        if (varData[lit.var()].reason.isNULL())
            file << ", color=\"darkorange2\""; //decision var
        else
            file << ", color=\"darkseagreen4\""; //propagated var

        //Print label
        file
        << ", label=\"" << (lit.sign() ? "-" : "") << "x" << lit.unsign()
        << " @ " << varData[lit.var()].level << "\""
        << " ];" << endl;
    }
}

void Searcher::fill_seen_for_lits_connected_to_conflict_graph(
    vector<Lit>& lits
) {
    while(!lits.empty())
    {
        vector<Lit> newLits;
        for (size_t i = 0; i < lits.size(); i++) {
            PropBy reason = varData[lits[i].var()].reason;
            //Reason in NULL, so remove: it's got no antedecent
            if (reason.isNULL()) continue;

            #ifdef VERBOSE_DEBUG_GEN_CONFL_DOT
            cout << "Reason for lit " << lits[i] << " : " << reason << endl;
            #endif

            PropByForGraph prop(reason, lits[i], cl_alloc);
            for (uint32_t i2 = 0; i2 < prop.size(); i2++) {
                const Lit lit = prop[i2];
                assert(value(lit) != l_Undef);

                //Don't put into the impl. graph lits at 0 decision level
                if (varData[lit.var()].level == 0) continue;

                //Already added, just drop
                if (seen[lit.var()]) continue;

                seen[lit.var()] = true;
                newLits.push_back(lit);
            }
        }
        lits = newLits;
    }
}

vector<Lit> Searcher::get_lits_from_conflict(const PropBy conflPart)
{
    vector<Lit> lits;
    PropByForGraph confl(conflPart, failBinLit, cl_alloc);
    for (uint32_t i = 0; i < confl.size(); i++) {
        const Lit lit = confl[i];
        assert(value(lit) == l_False);
        lits.push_back(lit);

        //Put these into the impl. graph for sure
        seen[lit.var()] = true;
    }

    return lits;
}

void Searcher::create_graphviz_confl_graph(const PropBy conflPart)
{
    assert(ok);
    assert(!conflPart.isNULL());

    std::stringstream s;
    s << "confls/" << "confl" << solver->sumConflicts() << ".dot";
    std::string filename = s.str();

    std::ofstream file;
    file.open(filename.c_str());
    if (!file) {
        cout << "Couldn't open filename " << filename << endl;
        cout << "Maybe you forgot to create subdirectory 'confls'" << endl;
        std::exit(-1);
    }
    file << "digraph G {" << endl;

    //Special vertex indicating final conflict clause (to help us)
    uint32_t out_btlevel, glue;
    const std::string res = analyze_confl_for_graphviz_graph(conflPart, out_btlevel, glue);
    file << "vertK -> dummy;";
    file << "dummy "
    << "[ "
    << " shape=record"
    << " , label=\"{"
    << " clause: " << learnt_clause
    << " | btlevel: " << out_btlevel
    << " | glue: " << glue
    << " | {resol: | " << res << " }"
    << "}\""
    << " , fontsize=8"
    << " ];" << endl;

    vector<Lit> lits = get_lits_from_conflict(conflPart);
    for (const Lit lit: lits) {
        file << "x" << lit.unsign() << " -> vertK "
        << "[ "
        << " label=\"" << lits << "\""
        << " , fontsize=8"
        << " ];" << endl;
    }

    //Special conflict vertex
    file << "vertK"
    << " [ "
    << "shape=\"box\""
    << ", style=\"filled\""
    << ", color=\"darkseagreen\""
    << ", label=\"K : " << lits << "\""
    << "];" << endl;

    fill_seen_for_lits_connected_to_conflict_graph(lits);
    print_edges_for_graphviz_file(file);
    print_vertex_definitions_for_graphviz_file(file);

    file  << "}" << endl;
    file.close();

    if (conf.verbosity >= 6) {
        cout
        << "c Printed implication graph (with conflict clauses) to file "
        << filename << endl;
    };
}

template<bool update_bogoprops>
PropBy Searcher::propagate() {
    const size_t origTrailSize = trail.size();

    PropBy ret;
    if (conf.propBinFirst) {
        ret = propagate_strict_order();
    } else {
        ret = propagate_any_order<update_bogoprops>();
    }

    //Drat -- If declevel 0 propagation, we have to add the unitaries
    if (decisionLevel() == 0 && drat->enabled()) {
        for(size_t i = origTrailSize; i < trail.size(); i++) {
            #ifdef DEBUG_DRAT
            if (conf.verbosity >= 6) {
                cout
                << "c 0-level enqueue:"
                << trail[i]
                << endl;
            }
            #endif
            *drat << trail[i] << fin;
        }
        if (!ret.isNULL()) {
            *drat << fin;
        }
    }

    return ret;
}
template PropBy Searcher::propagate<true>();
template PropBy Searcher::propagate<false>();

size_t Searcher::mem_used() const
{
    size_t mem = HyperEngine::mem_used();
    mem += otf_subsuming_short_cls.capacity()*sizeof(OTFClause);
    mem += otf_subsuming_long_cls.capacity()*sizeof(ClOffset);
    mem += activ_glue.capacity()*sizeof(uint32_t);
    mem += order_heap_glue.mem_used();
    mem += learnt_clause.capacity()*sizeof(Lit);
    mem += hist.mem_used();
    mem += conflict.capacity()*sizeof(Lit);
    mem += model.capacity()*sizeof(lbool);
    mem += analyze_stack.mem_used();
    mem += assumptions.capacity()*sizeof(Lit);
    mem += assumptionsSet.capacity()*sizeof(char);

    if (conf.verbosity >= 3) {
        cout
        << "c otfMustAttach bytes: "
        << otf_subsuming_short_cls.capacity()*sizeof(OTFClause)
        << endl;

        cout
        << "c toAttachLater bytes: "
        << otf_subsuming_long_cls.capacity()*sizeof(ClOffset)
        << endl;

        cout
        << "c toclear bytes: "
        << toClear.capacity()*sizeof(Lit)
        << endl;

        cout
        << "c trail bytes: "
        << trail.capacity()*sizeof(Lit)
        << endl;

        cout
        << "c trail_lim bytes: "
        << trail_lim.capacity()*sizeof(Lit)
        << endl;

        cout
        << "c activ_glue bytes: "
        << activ_glue.capacity()*sizeof(uint32_t)
        << endl;

        cout
        << "c order_heap_glue bytes: "
        << order_heap_glue.mem_used()
        << endl;

        cout
        << "c learnt clause bytes: "
        << learnt_clause.capacity()*sizeof(Lit)
        << endl;

        cout
        << "c hist bytes: "
        << hist.mem_used()
        << endl;

        cout
        << "c conflict bytes: "
        << conflict.capacity()*sizeof(Lit)
        << endl;

        cout
        << "c Stack bytes: "
        << analyze_stack.capacity()*sizeof(Lit)
        << endl;
    }

    return mem;
}

void Searcher::fill_assumptions_set_from(const vector<AssumptionPair>& fill_from)
{
    #ifdef SLOW_DEBUG
    for(auto x: assumptionsSet) {
        assert(!x);
    }
    #endif

    if (fill_from.empty()) {
        return;
    }

    for(const AssumptionPair lit_pair: assumptions) {
        const Lit lit = lit_pair.lit_inter;
        if (lit.var() < assumptionsSet.size()) {
            if (assumptionsSet[lit.var()]) {
                //Assumption contains the same literal twice. Shouldn't really be allowed...
                //assert(false && "Either the assumption set contains the same literal twice, or something is very wrong in the solver.");
            } else {
                assumptionsSet[lit.var()] = true;
            }
        } else {
            if (value(lit) == l_Undef) {
                std::cerr
                << "ERROR: Lit " << lit
                << " varData[lit.var()].removed: " << removed_type_to_string(varData[lit.var()].removed)
                << " value: " << value(lit)
                << " -- value should NOT be l_Undef"
                << endl;
            }
            assert(value(lit) != l_Undef);
        }
    }
}


void Searcher::unfill_assumptions_set_from(const vector<AssumptionPair>& unfill_from)
{
    if (unfill_from.empty()) {
        return;
    }

    for(const AssumptionPair lit_pair: unfill_from) {
        const Lit lit = lit_pair.lit_inter;
        if (lit.var() < assumptionsSet.size()) {
            assumptionsSet[lit.var()] = false;
        }
    }

    #ifdef SLOW_DEBUG
    for(auto x: assumptionsSet) {
        assert(!x);
    }
    #endif
}

inline void Searcher::varDecayActivity()
{
    var_inc *= (1.0 / var_decay);
}

template<bool update_bogoprops>
inline void Searcher::bump_var_activity(uint32_t var)
{
    if (update_bogoprops) {
        return;
    }

    activ_glue[var] += var_inc;

    #ifdef SLOW_DEBUG
    bool rescaled = false;
    #endif
    if (activ_glue[var] > 1e100) {
        // Rescale:
        for (double& act : activ_glue) {
            act *= 1e-100;
        }
        #ifdef SLOW_DEBUG
        rescaled = true;
        #endif

        //Reset var_inc
        var_inc *= 1e-100;
    }

    // Update order_heap with respect to new activity:
    if (order_heap_glue.inHeap(var)) {
        order_heap_glue.decrease(var);
    }

    #ifdef SLOW_DEBUG
    if (rescaled) {
        assert(order_heap_glue.heap_property());
    }
    #endif
}

void Searcher::update_var_decay()
{
    if (var_decay >= conf.var_decay_max) {
        var_decay = conf.var_decay_max;
    }
}

void Searcher::consolidate_watches()
{
    double t = cpuTime();
    watches.consolidate();
    double time_used = cpuTime() - t;

    if (conf.verbosity) {
        cout
        << "c [consolidate]"
        << conf.print_times(time_used)
        << endl;
    }

    if (sqlStats) {
        sqlStats->time_passed_min(
            solver
            , "consolidate watches"
            , time_used
        );
    }
}

void Searcher::write_long_cls(
    const vector<ClOffset>& clauses
    , SimpleOutFile& f
    , const bool red
) const {
    f.put_uint64_t(clauses.size());
    for(ClOffset c: clauses)
    {
        Clause& cl = *cl_alloc.ptr(c);
        assert(cl.size() > 3);
        f.put_uint32_t(cl.size());
        for(const Lit l: cl)
        {
            f.put_lit(l);
        }
        if (red) {
            assert(cl.red());
            f.put_struct(cl.stats);
        }
    }
}

void Searcher::read_long_cls(
    SimpleInFile& f
    , const bool red
) {
    uint64_t num_cls = f.get_uint64_t();

    vector<Lit> tmp_cl;
    for(size_t i = 0; i < num_cls; i++)
    {
        tmp_cl.clear();

        uint32_t sz = f.get_uint32_t();
        for(size_t i = 0; i < sz; i++)
        {
            tmp_cl.push_back(f.get_lit());
        }
        ClauseStats cl_stats;
        if (red) {
            f.get_struct(cl_stats);
        }

        Clause* cl = cl_alloc.Clause_new(tmp_cl
        #ifdef STATS_NEEDED
        , cl_stats.introduced_at_conflict
        , cl_stats.ID
        #endif
        );
        if (red) {
            cl->makeRed(cl_stats.glue);
        }
        cl->stats = cl_stats;
        attachClause(*cl);
        const ClOffset offs = cl_alloc.get_offset(cl);
        if (red) {
            if (cl->stats.glue <= conf.glue_must_keep_clause_if_below_or_eq) {
                cl->stats.which_red_array = 0;
            } else{
                cl->stats.which_red_array = 1;
            }

            longRedCls[0].push_back(cl->stats.which_red_array);
        } else {
            longIrredCls.push_back(offs);
        }
    }
}

unsigned Searcher::guess_clause_array(
    const uint32_t /*glue*/
    , const uint32_t backtrack_lev
    , const double vsids_cutoff
    , double backtrack_cutoff
    , const double offset_percent
    , bool count_antec_glue_long_reds
) const {
    uint32_t votes = 0;
    double perc_trail_depth = (double)trail.size()/hist.trailDepthHistLT.avg();
    if (perc_trail_depth < (0.3-offset_percent)) {
        votes++;
    }

    double perc_dec_lev = (double)decisionLevel()/hist.decisionLevelHistLT.avg();
    if (perc_dec_lev < (0.3-offset_percent)) {
        votes++;
    }

    double perc_backtrack_lev = (double)backtrack_lev/hist.decisionLevelHistLT.avg();
    if (perc_backtrack_lev < (backtrack_cutoff-offset_percent)) {
        votes++;
    }

    if (count_antec_glue_long_reds) {
        if (antec_data.glue_long_reds.avg() > 12) {
            votes += 1;
        }
    }

    if (antec_data.vsids_vars.avg() > vsids_cutoff) {
        votes += 2;
    }

    if (votes > 2) {
        return 0;
    }
    return 1;
}

void Searcher::write_binary_cls(
    SimpleOutFile& f
    , bool red
) const {
    if (red) {
        f.put_uint64_t(binTri.redBins);
    } else {
        f.put_uint64_t(binTri.irredBins);
    }

    size_t at = 0;
    for(watch_subarray_const ws: watches)
    {
        Lit lit1 = Lit::toLit(at);
        at++;
        for(Watched w: ws)
        {
            if (w.isBin() && w.red() == red) {
                assert(lit1 != w.lit2());
                if (lit1 < w.lit2()) {
                    f.put_lit(lit1);
                    f.put_lit(w.lit2());
                }
            }
        }
    }
}

void Searcher::write_tri_cls(
    SimpleOutFile& f
    , bool red
) const {
    if (red) {
        f.put_uint64_t(binTri.redTris);
    } else {
        f.put_uint64_t(binTri.irredTris);
    }

    size_t at = 0;
    for(watch_subarray_const ws: watches)
    {
        Lit lit1 = Lit::toLit(at);
        at++;
        for(Watched w: ws)
        {
            if (w.isTri() && w.red() == red) {
                if (lit1 < w.lit2()
                    && w.lit2() < w.lit3()
                ) {
                    f.put_lit(lit1);
                    f.put_lit(w.lit2());
                    f.put_lit(w.lit3());
                }
            }
        }
    }
}


void Searcher::read_binary_cls(
    SimpleInFile& f
    , bool red
) {
    uint64_t num = f.get_uint64_t();
    for(uint64_t i = 0; i < num; i++)
    {
        const Lit lit1 = f.get_lit();
        const Lit lit2 = f.get_lit();
        attach_bin_clause(lit1, lit2, red);
    }
}

void Searcher::read_tri_cls(
    SimpleInFile& f
    , bool red
) {
    uint64_t num = f.get_uint64_t();
    for(uint64_t i = 0; i < num; i++)
    {
        const Lit lit1 = f.get_lit();
        const Lit lit2 = f.get_lit();
        const Lit lit3 = f.get_lit();
        attach_tri_clause(lit1, lit2, lit3, red);
    }
}

void Searcher::save_state(SimpleOutFile& f, const lbool status) const
{
    assert(decisionLevel() == 0);
    PropEngine::save_state(f);

    f.put_vector(activ_glue);
    f.put_vector(model);
    f.put_vector(full_model);
    f.put_vector(conflict);

    //Clauses
    if (status == l_Undef) {
        write_binary_cls(f, false);
        write_binary_cls(f, true);
        write_tri_cls(f, false);
        write_tri_cls(f, true);
        write_long_cls(longIrredCls, f, false);
        for(auto& lredcls: longRedCls) {
            write_long_cls(lredcls, f, true);
        }
    }
}

void Searcher::load_state(SimpleInFile& f, const lbool status)
{
    assert(decisionLevel() == 0);
    PropEngine::load_state(f);

    f.get_vector(activ_glue);
    for(size_t i = 0; i < nVars(); i++) {
        if (varData[i].removed == Removed::none
            && value(i) == l_Undef
        ) {
            insertVarOrder(i);
        }
    }
    f.get_vector(model);
    f.get_vector(full_model);
    f.get_vector(conflict);

    //Clauses
    if (status == l_Undef) {
        read_binary_cls(f, false);
        read_binary_cls(f, true);
        read_tri_cls(f, false);
        read_tri_cls(f, true);
        read_long_cls(f, false);
        for(size_t i = 0; i < longRedCls.size(); i++) {
            read_long_cls(f, true);
        }
    }
}


template
void Searcher::cancelUntil<true>(uint32_t level);
template
void Searcher::cancelUntil<false>(uint32_t level);

template<bool also_insert_varorder>
void Searcher::cancelUntil(uint32_t level)
{
    #ifdef VERBOSE_DEBUG
    cout << "Canceling until level " << level;
    if (level > 0) cout << " sublevel: " << trail_lim[level];
    cout << endl;
    #endif

    if (decisionLevel() > level) {
        #ifdef USE_GAUSS
        for (Gaussian* gauss : gauss_matrixes) {
            gauss->canceling(trail_lim[level]);
        }
        #endif //USE_GAUSS

        //Go through in reverse order, unassign & insert then
        //back to the vars to be branched upon
        for (int sublevel = trail.size()-1
            ; sublevel >= (int)trail_lim[level]
            ; sublevel--
        ) {
            #ifdef VERBOSE_DEBUG
            cout
            << "Canceling lit " << trail[sublevel]
            << " sublevel: " << sublevel
            << endl;
            #endif

            #ifdef ANIMATE3D
            std:cerr << "u " << var << endl;
            #endif

            const uint32_t var = trail[sublevel].var();
            assert(value(var) != l_Undef);
            assigns[var] = l_Undef;
            if (also_insert_varorder) {
                insertVarOrder(var);
            }
        }
        qhead = trail_lim[level];
        trail.resize(trail_lim[level]);
        trail_lim.resize(level);
    }

    #ifdef VERBOSE_DEBUG
    cout
    << "Canceling finished. Now at level: " << decisionLevel()
    << " sublevel: " << trail.size()-1
    << endl;
    #endif
}

#ifdef USE_GAUSS
void Searcher::clearGaussMatrixes()
{
    /*assert(decisionLevel() == 0);
    for (uint32_t i = 0; i < gauss_matrixes.size(); i++)
        delete gauss_matrixes[i];
    gauss_matrixes.clear();*/

    /*
    for (uint32_t i = 0; i != freeLater.size(); i++)
        clauseAllocator.clauseFree(freeLater[i]);
    freeLater.clear();
    */
}
#endif

#ifdef USE_GAUSS
void Searcher::clear_gauss()
{
    for(Gaussian* g: gauss_matrixes) {
        if (conf.verbosity) {
            g->print_stats();
        }
        delete g;
    }
    gauss_matrixes.clear();
}
#endif

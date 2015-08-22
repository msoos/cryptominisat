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
#include "calcdefpolars.h"
#include "time_mem.h"
#include "solver.h"
#include <iomanip>
#include "varreplacer.h"
#include "clausecleaner.h"
#include "propbyforgraph.h"
#include <algorithm>
#include <cstddef>
#include "sqlstats.h"
#include "datasync.h"
#include "reducedb.h"
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
Searcher::Searcher(const SolverConf *_conf, Solver* _solver, bool* _needToInterrupt) :
        HyperEngine(
            _conf
            , _needToInterrupt
        )

        //variables
        , solver(_solver)
        , order_heap(VarOrderLt(activities))
        , clauseActivityIncrease(1)
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
}

void Searcher::new_var(const bool bva, const Var orig_outer)
{
    PropEngine::new_var(bva, orig_outer);

    activities.push_back(0);
    insertVarOrder((int)nVars()-1);
}

void Searcher::new_vars(size_t n)
{
    PropEngine::new_vars(n);

    activities.resize(activities.size() + n, 0);
    for(int i = n-1; i >= 0; i--) {
        insertVarOrder((int)nVars()-i-1);
    }
}

void Searcher::save_on_var_memory()
{
    PropEngine::save_on_var_memory();
    activities.resize(nVars());
    activities.shrink_to_fit();
}

void Searcher::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    updateArray(activities, interToOuter);
    //activities are not updated, they are taken from backup, which is updated

    renumber_assumptions(outerToInter);
}

void Searcher::renumber_assumptions(const vector<Var>& outerToInter)
{
    solver->unfill_assumptions_set_from(assumptions);
    for(AssumptionPair& lit_pair: assumptions) {
        assert(lit_pair.lit_inter.var() < outerToInter.size());
        lit_pair.lit_inter = getUpdatedLit(lit_pair.lit_inter, outerToInter);
    }
    solver->fill_assumptions_set_from(assumptions);
}

void Searcher::add_lit_to_learnt(
    const Lit lit
) {
    const Var var = lit.var();
    assert(varData[var].removed == Removed::none);

    //If var is at level 0, don't do anything with it, just skip
    if (varData[var].level == 0)
        return;

    //Update our state of going through the conflict
    if (!seen[var]) {
        seen[var] = 1;

        bump_var_activitiy(var);
        tmp_learnt_clause_size++;
        seen2[lit.toInt()] = 1;
        tmp_learnt_clause_abst |= abst_var(lit.var());

        if (varData[var].level == decisionLevel()) {
            pathC++;

            //Glucose 2.1
            if (update_polarity_and_activity
                && params.rest_type != Restart::geom
                && varData[var].reason != PropBy()
            ) {
                if (varData[var].reason.getType() == clause_t) {
                    Clause* cl = cl_alloc.ptr(varData[var].reason.get_offset());
                    if (cl->red()) {
                        const uint32_t glue = cl->stats.glue;
                        implied_by_learnts.push_back(std::make_pair(lit, glue));
                    }
                } else if (varData[var].reason.getType() == binary_t
                    && varData[var].reason.isRedStep()
                ) {
                    implied_by_learnts.push_back(std::make_pair(lit, 2));
                } else if (varData[var].reason.getType() == tertiary_t
                    && varData[var].reason.isRedStep()
                ) {
                    implied_by_learnts.push_back(std::make_pair(lit, 3));
                }
            }
        }
        else {
            learnt_clause.push_back(lit);
        }
    }
}

void Searcher::recursiveConfClauseMin()
{
    uint32_t abstract_level = 0;
    for (size_t i = 1; i < learnt_clause.size(); i++) {
        //(maintain an abstraction of levels involved in conflict)
        abstract_level |= abstractLevel(learnt_clause[i].var());
    }

    size_t i, j;
    for (i = j = 1; i < learnt_clause.size(); i++) {
        #ifdef DEBUG_LITREDUNDANT
        cout << "Calling litRedundant at i = " << i << endl;
        #endif
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

    if (drup->enabled()) {
        for(unsigned  i = 0; i < newCl.size; i++) {
            *drup << newCl.lits[i];
        }
        *drup << fin;
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
    (*solver->drup) << deldelay << cl << fin;
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
    *drup << cl << fin << findelay;
    otf_subsuming_long_cls.push_back(offset);
}

void Searcher::check_otf_subsume(const PropBy confl)
{
    ClOffset offset = confl.get_offset();
    Clause& cl = *cl_alloc.ptr(offset);

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
    return;
    #endif

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

        case null_clause_t: {
            assert(false);
            break;
        }
    }
}

void Searcher::update_clause_glue_from_analysis(Clause* cl)
{
    assert(cl->red());
    if (cl->stats.glue == 2) {
        return;
    }

    const unsigned new_glue = calc_glue_using_seen2_upper_bit_no_zero_lev(*cl);

    if (new_glue + 1 < cl->stats.glue) {
        //tot_lbds = tot_lbds - c.lbd() + lbd;
        //c.delta_lbd(c.delta_lbd() + c.lbd() - lbd);

        if (new_glue <= conf.glue_must_keep_clause_if_below_or_eq && red_long_cls_is_reducedb(*cl)) {
            num_red_cls_reducedb--;
        }
        cl->stats.glue = new_glue;

        if (new_glue <= conf.protect_clause_if_imrpoved_glue_below_this_glue_for_one_turn) {
            if (red_long_cls_is_reducedb(*cl)) {
                num_red_cls_reducedb--;
            }
            cl->stats.ttl = 1;
        }
    }
}

Clause* Searcher::add_literals_from_confl_to_learnt(
    const PropBy confl
    , const Lit p
) {
    debug_print_resolving_clause(confl);

    Clause* cl = NULL;
    switch (confl.getType()) {
        case tertiary_t : {
            resolutions.tri++;
            stats.resolvs.tri++;
            add_lit_to_learnt(confl.lit3());

            if (p == lit_Undef) {
                add_lit_to_learnt(failBinLit);
            }

            add_lit_to_learnt(confl.lit2());
            break;
        }

        case binary_t : {
            resolutions.bin++;
            stats.resolvs.bin++;
            if (p == lit_Undef) {
                add_lit_to_learnt(failBinLit);
            }
            add_lit_to_learnt(confl.lit2());
            break;
        }

        case clause_t : {
            cl = cl_alloc.ptr(confl.get_offset());
            if (cl->red()) {
                resolutions.redL++;
                stats.resolvs.redL++;
            } else {
                resolutions.irredL++;
                stats.resolvs.irredL++;
            }
            #ifdef STATS_NEEDED
            cl->stats.used_for_uip_creation++;
            #endif
            if (cl->red() && update_polarity_and_activity) {
                bumpClauseAct(cl);
                if (conf.update_glues_on_analyze) {
                    update_clause_glue_from_analysis(cl);
                }
            }

            assert(!cl->getRemoved());
            for (size_t j = 0; j < cl->size(); j++) {
                //Will be resolved away, skip
                if (p != lit_Undef && j == 0)
                    continue;

                add_lit_to_learnt((*cl)[j]);
            }
            break;
        }

        case null_clause_t:
        default:
            //otherwise should be UIP
            assert(false && "Error in conflict analysis");
            break;
    }
    return cl;
}

void Searcher::minimize_learnt_clause()
{
    const size_t origSize = learnt_clause.size();

    toClear = learnt_clause;
    if (conf.doRecursiveMinim) {
        recursiveConfClauseMin();
    } else {
        normalClMinim();
    }
    for (const Lit lit: toClear) {
        seen2[lit.toInt()] = 0;
        seen[lit.var()] = 0;
    }
    toClear.clear();

    stats.recMinCl += ((origSize - learnt_clause.size()) > 0);
    stats.recMinLitRem += origSize - learnt_clause.size();
}

void Searcher::mimimize_learnt_clause_more_maybe()
{
    if (conf.doMinimRedMore
        && learnt_clause.size() > 1
        && (conf.doAlwaysFMinim
            //|| (calc_glue_using_seen2(learnt_clause) < 0.45*hist.glueHistLT.avg()
            //    && learnt_clause.size() < 0.45*hist.conflSizeHistLT.avg())
            || (learnt_clause.size() <= conf.max_size_more_minim
                && calc_glue_using_seen2(learnt_clause) <= conf.max_glue_more_minim)
            )
    ) {
        stats.moreMinimLitsStart += learnt_clause.size();

        //Binary&cache-based minim
        minimise_redundant_more(learnt_clause);

        //Stamp-based minimization -- cheap, so do it anyway
        if (conf.doStamp&& conf.more_otf_shrink_with_stamp) {
            stamp_based_more_minim(learnt_clause);
        }

        stats.moreMinimLitsEnd += learnt_clause.size();
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
        for (uint32_t i = 2; i < learnt_clause.size(); i++)
            if (varData[learnt_clause[i].var()].level > varData[learnt_clause[max_i].var()].level)
                max_i = i;
        std::swap(learnt_clause[max_i], learnt_clause[1]);
        return varData[learnt_clause[1].var()].level;
    }
}

inline Clause* Searcher::create_learnt_clause(PropBy confl)
{
    pathC = 0;
    int index = trail.size() - 1;
    Lit p = lit_Undef;
    Clause* last_resolved_long_cl = NULL;

    learnt_clause.push_back(lit_Undef); //make space for ~p
    do {
        #ifdef DEBUG_RESOLV
        cout << "p is: " << p << endl;
        #endif

        //This is for OTF subsumption ("OTF clause improvement" by Han&Somezi)
        //~p is essentially popped from the temporary learnt clause
        if (p != lit_Undef) {
            tmp_learnt_clause_size--;
            assert(seen2[(~p).toInt()] == 1);
            seen2[(~p).toInt()] = 0;

            //We MUST under-estimate
            tmp_learnt_clause_abst &= ~(abst_var((~p).var()));
        }

        last_resolved_long_cl = add_literals_from_confl_to_learnt(confl, p);

        // Select next implication to look at
        while (!seen[trail[index--].var()]);

        p = trail[index+1];
        assert(p != lit_Undef);

        if (update_polarity_and_activity
            && conf.doOTFSubsume
            //A long clause
            && last_resolved_long_cl != NULL
            //Good enough clause to try to minimize
            && last_resolved_long_cl->stats.glue <= conf.doOTFSubsumeOnlyAtOrBelowGlue
            //Must subsume, so must be smaller
            && last_resolved_long_cl->size() > tmp_learnt_clause_size
        ) {
            last_resolved_long_cl->recalc_abst_if_needed();
            //Everything in learnt_cl_2 seems to be also in cl
            if (
                ((last_resolved_long_cl->abst & tmp_learnt_clause_abst) ==  tmp_learnt_clause_abst)
                && pathC > 1
            ) {
                check_otf_subsume(confl);
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
    for(const Lit lit: learnt_clause) {
        seen2[lit.toInt()] = 0;
    }

    return last_resolved_long_cl;
}

void Searcher::bump_var_activities_based_on_implied_by_learnts(const uint32_t glue)
{
    for (const auto dat :implied_by_learnts) {
        const uint32_t v_glue = dat.second;
        if (v_glue < glue) {
            bump_var_activitiy(dat.first.var());
        }
    }
}

Clause* Searcher::otf_subsume_last_resolved_clause(Clause* last_resolved_long_cl)
{
    //We can only on-the-fly subsume with clauses that are not 2- or 3-long
    //furthermore, we cannot subsume a clause that is marked for deletion
    //due to its high glue value
    if (!conf.doOTFSubsume
        //Last was a lont clause
        || last_resolved_long_cl == NULL
        //Final clause will not be implicit
        || learnt_clause.size() <= 3
        //Larger or equivalent clauses cannot subsume the clause
        || learnt_clause.size() >= last_resolved_long_cl->size()
    ) {
        return NULL;
    }

    //Does it subsume?
    if (!subset(learnt_clause, *last_resolved_long_cl))
        return NULL;

    //on-the-fly subsumed the original clause
    stats.otfSubsumed++;
    stats.otfSubsumedLong++;
    stats.otfSubsumedRed += last_resolved_long_cl->red();
    stats.otfSubsumedLitsGained += last_resolved_long_cl->size() - learnt_clause.size();
    return last_resolved_long_cl;
}

void Searcher::print_debug_resolution_data(const PropBy confl)
{
    #ifndef DEBUG_RESOLV
    return;
    #endif

    cout << "Before resolution, trail is: " << endl;
    print_trail();
    cout << "Conflicting clause: " << confl << endl;
    cout << "Fail bin lit: " << failBinLit << endl;
}

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
    Clause* last_resolved_long_cl = create_learnt_clause(confl);
    stats.litsRedNonMin += learnt_clause.size();
    minimize_learnt_clause();
    mimimize_learnt_clause_more_maybe();
    print_fully_minimized_learnt_clause();

    glue = calc_glue_using_seen2(learnt_clause);
    stats.litsRedFinal += learnt_clause.size();
    out_btlevel = find_backtrack_level_of_learnt();
    if (update_polarity_and_activity
        && params.rest_type == Restart::glue
        && conf.extra_bump_var_activities_based_on_glue
    ) {
        bump_var_activities_based_on_implied_by_learnts(glue);
    }
    implied_by_learnts.clear();

    return otf_subsume_last_resolved_clause(last_resolved_long_cl);

}

bool Searcher::litRedundant(const Lit p, uint32_t abstract_levels)
{
    #ifdef DEBUG_LITREDUNDANT
    cout << "Litredundant called" << endl;
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
                std::exit(-1);
                size = 0;
                break;
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
                    std::exit(-1);
                    break;
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
        const Var x = trail[i].var();
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

lbool Searcher::search()
{
    assert(ok);
    const double myTime = cpuTime();

    //Stats reset & update
    if (params.update)
        stats.numRestarts++;
    hist.clear();
    hist.reset_glue_hist_size(conf.shortTermHistorySize);

    assert(solver->prop_at_head());

    //Loop until restart or finish (SAT/UNSAT)
    last_decision_ended_in_conflict = false;
    blocked_restart = false;
    PropBy confl;

    while (
        (!params.needToStopSearch
            && !must_interrupt_asap()
        )
            || !confl.isNULL() //always finish the last conflict
    ) {
        if (!confl.isNULL()) {
            //TODO below is expensive
            if (((stats.conflStats.numConflicts % 5000) == 0)
                && var_decay < conf.var_decay_max
                && update_polarity_and_activity
            ) {
                var_decay += 0.01;
            }

            stats.conflStats.update(lastConflictCausedBy);
            print_restart_stat();
            #ifdef STATS_NEEDED
            hist.conflictAfterConflict.push(last_decision_ended_in_conflict);
            #endif
            last_decision_ended_in_conflict = true;
            if (params.update) {
                #ifdef STATS_NEEDED
                hist.trailDepthHist.push(trail.size()); //TODO  - trail_lim[0]
                #endif
                hist.trailDepthHistLonger.push(trail.size()); //TODO  - trail_lim[0]
            }
            check_blocking_restart();
            if (!handle_conflict(confl)) {
                dump_search_sql(myTime);
                return l_False;
            }
        } else {
            assert(ok);
            reduce_db_if_needed();
            check_need_restart();
            last_decision_ended_in_conflict = false;
            const lbool ret = new_decision();
            if (ret != l_Undef) {
                dump_search_sql(myTime);
                return ret;
            }
        }

        //Decision level is higher than 1, so must do normal propagation
        confl = propagate<false>(
            #ifdef STATS_NEEDED
            &hist.watchListSizeTraversed
            #endif
        );
    }

    cancelUntil(0);
    assert(solver->prop_at_head());
    if (!solver->datasync->syncData()) {
        return l_False;
    }
    dump_search_sql(myTime);

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

    return pow(y, seq);
}

void Searcher::check_need_restart()
{
    if (must_interrupt_asap())  {
        if (conf.verbosity >= 3)
            cout << "c must_interrupt_asap() is set, restartig as soon as possible!" << endl;
        params.needToStopSearch = true;
    }

    if ((stats.conflStats.numConflicts & 0xff) == 0xff) {
        //It's expensive to check time all the time
        if (cpuTime() > conf.maxTime) {
            params.needToStopSearch = true;
        }
    }

    switch (params.rest_type) {

        case Restart::never:
            //Just don't restart no matter what
            break;

        case Restart::geom:
        case Restart::luby:
            if (params.conflictsDoneThisRestart > max_conflicts_this_restart)
                params.needToStopSearch = true;

            break;

        case Restart::glue:
            if (hist.glueHist.isvalid()
                && conf.local_glue_multiplier * hist.glueHist.avg() > hist.glueHistLT.avg()
            ) {
                params.needToStopSearch = true;
            }

            break;

        default:
            assert(false && "This should not happen, auto decision is make before this point");
            break;
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

            //Drup
            if (decisionLevel() == 0) {
                *drup << cl[0] << fin;
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

            //Drup
            if (decisionLevel() == 0) {
                *drup << it->lits[0] << fin;
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

    hist.branchDepthHist.push(decisionLevel());
    hist.branchDepthDeltaHist.push(decisionLevel() - backtrack_level);

    hist.glueHist.push(glue);
    hist.glueHistLT.push(glue);

    hist.conflSizeHist.push(learnt_clause.size());
    hist.conflSizeHistLT.push(learnt_clause.size());

    hist.numResolutionsHist.push(resolutions.sum());
    hist.numResolutionsHistLT.push(resolutions.sum());

    hist.trailDepthDeltaHist.push(trail.size() - trail_lim[backtrack_level]);
}

void Searcher::attach_and_enqueue_learnt_clause(Clause* cl)
{
    switch (learnt_clause.size()) {
        case 0:
            assert(false);
        case 1:
            //Unitary learnt
            stats.learntUnits++;
            enqueue(learnt_clause[0]);
            assert(decisionLevel() == 0);

            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif

            break;
        case 2:
            //Binary learnt
            stats.learntBins++;
            solver->datasync->signalNewBinClause(learnt_clause);
            solver->attach_bin_clause(learnt_clause[0], learnt_clause[1], true);
            enqueue(learnt_clause[0], PropBy(learnt_clause[1], true));

            #ifdef STATS_NEEDED
            propStats.propsBinRed++;
            #endif
            break;

        case 3:
            //3-long learnt
            stats.learntTris++;
            solver->attach_tri_clause(learnt_clause[0], learnt_clause[1], learnt_clause[2], true);
            enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2], true));

            #ifdef STATS_NEEDED
            propStats.propsTriRed++;
            #endif
            break;

        default:
            //Long learnt
            cl->stats.resolutions = resolutions;
            stats.learntLongs++;
            solver->attachClause(*cl);
            enqueue(learnt_clause[0], PropBy(cl_alloc.get_offset(cl)));

            #ifdef STATS_NEEDED
            propStats.propsLongRed++;
            #endif

            break;
    }
}

void Searcher::print_learning_debug_info() const
{
    #ifndef VERBOSE_DEBUG
    return;
    #endif

    cout
    << "Learning:" << learnt_clause
    << endl
    << "reverting var " << learnt_clause[0].var()+1
    << " to " << !learnt_clause[0].sign()
    << endl;
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

Clause* Searcher::handle_last_confl_otf_subsumption(
    Clause* cl
    , const size_t glue
) {
    //No on-the-fly subsumption
    if (cl == NULL) {
        if (learnt_clause.size() > 3) {
            cl = cl_alloc.Clause_new(learnt_clause, Searcher::sumConflicts());
            cl->makeRed(glue);
            ClOffset offset = cl_alloc.get_offset(cl);
            solver->longRedCls.push_back(offset);
            return cl;
        }
        return NULL;
    }

    //Cannot make a non-implicit into an implicit
    if (learnt_clause.size() <= 3)
        return NULL;

    assert(cl->size() > 3);
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

bool Searcher::handle_conflict(const PropBy confl)
{
    uint32_t backtrack_level;
    uint32_t glue;
    resolutions.clear();
    stats.conflStats.numConflicts++;
    params.conflictsDoneThisRestart++;
    if (conf.doPrintConflDot)
        create_graphviz_confl_graph(confl);

    if (decisionLevel() == 0)
        return false;

    Clause* cl = analyze_conflict(
        confl
        , backtrack_level  //return backtrack level here
        , glue             //return glue here
    );
    print_learnt_clause();
    *drup << learnt_clause << fin;

    if (params.update) {
        update_history_stats(backtrack_level, glue);
    }
    cancelUntil(backtrack_level);

    add_otf_subsume_long_clauses();
    add_otf_subsume_implicit_clause();
    print_learning_debug_info();
    assert(value(learnt_clause[0]) == l_Undef);
    glue = std::min<uint32_t>(glue, std::numeric_limits<uint32_t>::max());
    cl = handle_last_confl_otf_subsumption(cl, glue);
    assert(learnt_clause.size() <= 3 || cl != NULL);
    attach_and_enqueue_learnt_clause(cl);

    varDecayActivity();
    decayClauseAct();

    return true;
}

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
    double backup_var_inc = var_inc;
    double backup_var_decay = var_decay;
    update_polarity_and_activity = false;

    //Set burst config
    conf.random_var_freq = 1;
    conf.polarity_mode = PolarityMode::polarmode_rnd;

    //Do burst
    params.clear();
    params.conflictsToDo = conf.burst_search_len;
    params.rest_type = Restart::never;
    lbool status = search();

    //Restore config
    conf.random_var_freq = backup_rand;
    conf.polarity_mode = backup_polar_mode;
    assert(var_decay == backup_var_decay);
    assert(var_inc == backup_var_inc);
    update_polarity_and_activity = true;

    //Print what has happened
    const double time_used = cpuTime() - myTime;
    if (conf.verbosity >= 2) {
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
    << " " << std::setw(7) << "freevar"
    << " " << std::setw(5) << "IrrL"
    << " " << std::setw(5) << "IrrT"
    << " " << std::setw(5) << "IrrB"
    << " " << std::setw(5) << "l/c"
    << " " << std::setw(5) << "RedL"
    << " " << std::setw(5) << "RedT"
    << " " << std::setw(5) << "RedB"
    << " " << std::setw(5) << "l/c"
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
void Searcher::dump_restart_sql()
{
    //Propagation stats
    PropStats thisPropStats = propStats - lastSQLPropStats;
    Stats thisStats = stats - lastSQLGlobalStats;

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

struct VarDumpOrder
{
    VarDumpOrder(size_t _var, size_t _polarSetSum) :
        var(_var)
        , polarSetSum(_polarSetSum)
    {}

    size_t var;
    size_t polarSetSum;

    bool operator<(const VarDumpOrder& other) const
    {
        //Order by largest polarSetSum first
        return polarSetSum > other.polarSetSum;
    }
};

void Searcher::print_restart_stat()
{
    //Print restart stat
    if (conf.verbosity >= 2
        && ((lastRestartPrint + conf.print_restart_line_every_n_confl)
          < stats.conflStats.numConflicts)
    ) {
        //Print restart output header
        if (lastRestartPrintHeader == 0
            ||(lastRestartPrintHeader + 20000) < stats.conflStats.numConflicts
        ) {
            print_restart_header();
            lastRestartPrintHeader = stats.conflStats.numConflicts;
        }
        print_restart_stat_line();
        lastRestartPrint = stats.conflStats.numConflicts;
    }
}

void Searcher::setup_restart_print()
{
    //Set up restart printing status
    lastRestartPrint = stats.conflStats.numConflicts;
    lastRestartPrintHeader = stats.conflStats.numConflicts;
    if (conf.verbosity >= 1) {
        print_restart_stat_line();
    }
}

void Searcher::restore_order_heap()
{
    order_heap.clear();
    for(size_t var = 0; var < nVars(); var++) {
        if (solver->varData[var].is_decision
            && value(var) == l_Undef
        ) {
            assert(varData[var].removed == Removed::none);
            insertVarOrder(var);
        }
    }
    assert(order_heap.heap_property());
}

void Searcher::reset_temp_cl_num()
{
    conf.cur_max_temp_red_cls = conf.max_temporary_learnt_clauses;
    num_red_cls_reducedb = count_num_red_cls_reducedb();
}

void Searcher::reduce_db_if_needed()
{
    //Check if we should do DBcleaning
    if (num_red_cls_reducedb > conf.cur_max_temp_red_cls) {
        if (conf.verbosity >= 3) {
            cout
            << "c "
            << " cleaning"
            << " num_irred_cls_reducedb: " << num_red_cls_reducedb
            << " numConflicts : " << stats.conflStats.numConflicts
            << " SumConfl: " << sumConflicts()
            << " max_confl_per_search_solve_call:" << max_confl_per_search_solve_call
            << " Trail size: " << trail.size() << endl;
        }
        solver->reduceDB->reduce_db_and_update_reset_stats();
        if (conf.verbosity >= 3) {
            watches.print_stat();
        }
        must_consolidate_mem = true;
        watches.consolidate();
        conf.cur_max_temp_red_cls *= conf.inc_max_temp_red_cls;

        num_red_cls_reducedb = count_num_red_cls_reducedb();
    }
}

void Searcher::clean_clauses_if_needed()
{
    const size_t newZeroDepthAss = trail.size() - lastCleanZeroDepthAssigns;
    if (newZeroDepthAss > ((double)solver->get_num_free_vars()*solver->conf.clean_after_perc_zero_depth_assigns))  {
        if (conf.verbosity >= 2) {
            cout << "c newZeroDepthAss : " << newZeroDepthAss  << endl;
        }

        lastCleanZeroDepthAssigns = trail.size();
        solver->clauseCleaner->remove_and_clean_all();
    }
}

lbool Searcher::perform_scc_and_varreplace_if_needed()
{
    if (conf.doFindAndReplaceEqLits
            && (solver->binTri.numNewBinsSinceSCC
                > ((double)solver->get_num_free_vars()*conf.sccFindPercent))
    ) {
        if (conf.verbosity >= 1) {
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
        if (!solver->varReplacer->replace_if_enough_is_found(floor((double)solver->get_num_free_vars()*0.001))) {
            return l_False;
        }
        #ifdef SLOW_DEBUG
        assert(solver->check_order_heap_sanity());
        #endif
    }

    return l_Undef;
}

void Searcher::save_search_loop_stats()
{
    #ifdef STATS_NEEDED
    if (solver->sqlStats) {
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
    check_no_removed_or_freed_cl_in_watch();

    if (solver->conf.verbosity >= 6) {
        cout
        << "c Searcher::solve() called"
        << endl;
    }

    resetStats();
    lbool status = l_Undef;
    if (conf.burst_search_len > 0
        && upper_level_iteration_num > 0
    ) {
        assert(solver->check_order_heap_sanity());
        setup_restart_print();
        status = burst_search();
        if (status != l_Undef)
            goto end;
    }

    params.rest_type = conf.restartType;
    if ((num_search_called == 1 && conf.do_calc_polarity_first_time)
        || conf.do_calc_polarity_every_time
    ) {
        calculate_and_set_polars();
    }

    setup_restart_print();
    max_conflicts_this_restart = conf.restart_first;
    assert(solver->check_order_heap_sanity());
    for(loop_num = 0
        ; !must_interrupt_asap()
          && stats.conflStats.numConflicts < max_confl_per_search_solve_call
          && !solver->must_interrupt_asap()
        ; loop_num ++
    ) {
        #ifdef SLOW_DEBUG
        assert(num_red_cls_reducedb == count_num_red_cls_reducedb());
        assert(order_heap.heap_property());
        assert(solver->check_order_heap_sanity());
        #endif

        //Only sort after a while
        //otherwise, we sort all the time for short queries
        if ((loop_num & 0x7ff) == 0x020
            && stats.conflStats.numConflicts > 4000
            && conf.doSortWatched
        ) {
            sortWatched();
            rearrange_clauses_watches();
        }

        assert(watches.get_smudged_list().empty());
        assert(update_polarity_and_activity);
        print_search_loop_num();

        lastRestartConfl = sumConflicts();
        params.clear();
        params.conflictsToDo = max_confl_per_search_solve_call-stats.conflStats.numConflicts;
        status = search();

        switch (params.rest_type) {
            case Restart::geom:
                max_conflicts_this_restart *= conf.restart_inc;
                break;

            case Restart::luby:
                max_conflicts_this_restart = luby(conf.restart_inc, loop_num) * (double)conf.restart_first;
                break;

            default:
                break;
        }

        if (must_abort(status)) {
            goto end;
        }

        clean_clauses_if_needed();
        if (!conf.never_stop_search) {
            status = perform_scc_and_varreplace_if_needed();
            if (status != l_Undef) {
                goto end;
            }
        }

        save_search_loop_stats();
        if (must_consolidate_mem) {
            cl_alloc.consolidate(solver);
            must_consolidate_mem = false;

            //TODO complete detach-reattacher cannot count num_red_cls_reducedb
            num_red_cls_reducedb = count_num_red_cls_reducedb();
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
        model = assigns;
        print_solution_varreplace_status();
    } else if (status == l_False) {
        if (conflict.size() == 0)
            ok = false;
    }
    cancelUntil(0);

    stats.cpu_time = cpuTime() - startTime;
    if (conf.verbosity >= 4) {
        cout << "c Searcher::solve() finished"
        << " status: " << status
        << " num_red_cls_reducedb: " << num_red_cls_reducedb
        << " numConflicts : " << stats.conflStats.numConflicts
        << " SumConfl: " << sumConflicts()
        << " max_confl_per_search_solve_call:" << max_confl_per_search_solve_call
        << endl;
    }

    #ifdef STATS_NEEDED
    if (solver->sqlStats) {
        dump_restart_sql();
    }
    #endif

    print_iteration_solving_stats();
}

void Searcher::print_iteration_solving_stats()
{
    if (conf.verbosity >= 3) {
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
        stats.print();
        propStats.print(stats.cpu_time);
        print_stats_line("c props/decision"
            , (double)propStats.propagations/(double)stats.decisions
        );
        print_stats_line("c props/conflict"
            , (double)propStats.propagations/(double)stats.conflStats.numConflicts
        );
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
    }
}

bool Searcher::pickPolarity(const Var var)
{
    switch(conf.polarity_mode) {
        case PolarityMode::polarmode_neg:
            return false;

        case PolarityMode::polarmode_pos:
            return true;

        case PolarityMode::polarmode_rnd:
            return mtrand.randInt(1);

        case PolarityMode::polarmode_automatic:
            return getStoredPolarity(var);
        default:
            assert(false);
    }

    return true;
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
        if (rand < frq && !order_heap.empty()) {
            const Var next_var = order_heap.random_element(mtrand);

            if (value(next_var) == l_Undef
                && solver->varData[next_var].is_decision
            ) {
                stats.decisionsRand++;
                next = Lit(next_var, !pickPolarity(next_var));
            }
        }
    }

    // Activity based decision:
    if (next == lit_Undef) {
        Var next_var = var_Undef;
        while (next_var == var_Undef
          || value(next_var) != l_Undef
          || !solver->varData[next_var].is_decision
        ) {
            //There is no more to branch on. Satisfying assignment found.
            if (order_heap.empty()) {
                next_var = var_Undef;
                break;
            }

            next_var = order_heap.remove_min();
        }

        if (next_var != var_Undef) {
            next = Lit(next_var, !pickPolarity(next_var));
        }
    }

    //Flip polaritiy if need be
    if (false
        && next != lit_Undef
    ) {
        next ^= true;
        stats.decisionFlippedPolar++;
    }

    //Try to update to dominator
    if (next != lit_Undef
        && conf.dominPickFreq > 0
        && (mtrand.randInt(conf.dominPickFreq) == 1)
    ) {
        Lit lit2 = lit_Undef;
        if (conf.doCache) {
            //Use cache
            lit2 = solver->litReachable[next.toInt()].lit;
        }

        //Update
        if (lit2 != lit_Undef
            && value(lit2.var()) == l_Undef
            && solver->varData[lit2.var()].is_decision
        ) {
            //Dominator may not actually dominate this variabe
            //So just to be sure, re-insert it
            insertVarOrder(next.var());

            //Save this literal & sign
            next = lit2;
        }
    }

    if (conf.verbosity >= 10) {
        if (next == lit_Undef) {
            cout << "SAT!" << endl;
        } else {
            cout << "decided on: " << next << endl;
        }
    }

    //No vars in heap: solution found
    if (next != lit_Undef) {
        assert(solver->varData[next.var()].is_decision);
        assert(solver->varData[next.var()].removed == Removed::none);
    }
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
        const TransCache& cache1 = solver->implCache[lit.toInt()];
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
        watch_subarray_const ws = watches[lit.toInt()];
        for (watch_subarray_const::const_iterator
            i = ws.begin()
            , end = ws.end()
            ; i != end && limit > 0
            ; i++
        ) {
            limit--;
            if (i->isBinary()) {
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
            }
        }
    }
}

void Searcher::minimise_redundant_more(vector<Lit>& cl)
{
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
    return (cc->value(var) == l_Undef && solver->varData[var].is_decision);
}

uint64_t Searcher::sumConflicts() const
{
    return solver->sumStats.conflStats.numConflicts + stats.conflStats.numConflicts;
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
            propStats.otfHyperTime += solver->watches[it->getLit1().toInt()].size()/2;
            propStats.otfHyperTime += solver->watches[it->getLit2().toInt()].size()/2;
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
            *drup << del << it->getLit1() << it->getLit2() << fin;

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

        //Update resolutions output
        if (p != lit_Undef) {
            resolutions_str << " | ";
        }
        resolutions_str << "{ " << confl << " | " << pathC << " -- ";

        for (uint32_t j = (p == lit_Undef) ? 0 : 1, size = confl.size(); j != size; j++) {
            Lit q = confl[j];
            const Var my_var = q.var();

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
    glue = calc_glue_using_seen2(learnt_clause);

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
PropBy Searcher::propagate(
    #ifdef STATS_NEEDED
    AvgCalc<size_t>* watchListSizeTraversed
    #endif
) {
    const size_t origTrailSize = trail.size();

    PropBy ret;
    if (conf.propBinFirst) {
        ret = propagate_strict_order(
            #ifdef STATS_NEEDED
            watchListSizeTraversed
            #endif
        );
    } else {
        ret = propagate_any_order<update_bogoprops>();
    }

    //Drup -- If declevel 0 propagation, we have to add the unitaries
    if (decisionLevel() == 0 && drup->enabled()) {
        for(size_t i = origTrailSize; i < trail.size(); i++) {
            #ifdef DEBUG_DRUP
            if (conf.verbosity >= 6) {
                cout
                << "c 0-level enqueue:"
                << trail[i]
                << endl;
            }
            #endif
            *drup << trail[i] << fin;
        }
        if (!ret.isNULL()) {
            *drup << fin;
        }
    }

    return ret;
}
template PropBy Searcher::propagate<true>(
    #ifdef STATS_NEEDED
    AvgCalc<size_t>* watchListSizeTraversed
    #endif
);
template PropBy Searcher::propagate<false>(
    #ifdef STATS_NEEDED
    AvgCalc<size_t>* watchListSizeTraversed
    #endif
);

size_t Searcher::mem_used() const
{
    size_t mem = HyperEngine::mem_used();
    mem += otf_subsuming_short_cls.capacity()*sizeof(OTFClause);
    mem += otf_subsuming_long_cls.capacity()*sizeof(ClOffset);
    mem += activities.capacity()*sizeof(uint32_t);
    mem += order_heap.mem_used();
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
        << "c activities bytes: "
        << activities.capacity()*sizeof(uint32_t)
        << endl;

        cout
        << "c order_heap bytes: "
        << order_heap.mem_used()
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

void Searcher::calculate_and_set_polars()
{
    CalcDefPolars calculator(solver);
    vector<unsigned char> calc_polars = calculator.calculate();
    assert(calc_polars.size() == nVars());
    for(size_t i = 0; i < calc_polars.size(); i++) {
        varData[i].polarity = calc_polars[i];
    }
}

void Searcher::fill_assumptions_set_from(const vector<AssumptionPair>& fill_from)
{
    if (fill_from.empty()) {
        return;
    }

    for(const AssumptionPair lit_pair: assumptions) {
        const Lit lit = lit_pair.lit_inter;
        if (lit.var() < assumptionsSet.size()) {
            if (assumptionsSet[lit.var()]) {
                //Yes, it can happen... due to variable value replacement
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
}

inline void Searcher::varDecayActivity()
{
    if (!update_polarity_and_activity) {
        return;
    }

    var_inc *= (1.0 / var_decay);
}

inline void Searcher::bump_var_activitiy(Var var)
{
    if (!update_polarity_and_activity) {
        return;
    }

    activities[var] += var_inc;

    #ifdef SLOW_DEBUG
    bool rescaled = false;
    #endif
    if (activities[var] > 1e100) {
        // Rescale:
        for (double& act : activities) {
            act *= 1e-100;
        }
        #ifdef SLOW_DEBUG
        rescaled = true;
        #endif

        //Reset var_inc
        var_inc *= 1e-100;

        //If var_inc is smaller than var_inc_start then this MUST be corrected
        //otherwise the 'varDecayActivity' may not decay anything in fact
        if (var_inc == 0.0) {
            var_inc = conf.var_inc_start;
        }
    }

    // Update order_heap with respect to new activity:
    if (order_heap.in_heap(var)) {
        order_heap.decrease(var);
    }

    #ifdef SLOW_DEBUG
    if (rescaled) {
        assert(order_heap.heap_property());
    }
    #endif
}

void Searcher::rearrange_clauses_watches()
{
    assert(decisionLevel() == 0);
    assert(ok);
    assert(qhead == trail.size());

    double myTime = cpuTime();
    for(watch_subarray ws: watches) {
        for(Watched& w: ws) {
            if (!w.isClause()) {
                continue;
            }
            Clause& cl = *cl_alloc.ptr(w.get_offset());
            w.setBlockedLit(find_good_blocked_lit(cl));
        }
    }

    const double time_used = cpuTime() - myTime;
    if (conf.verbosity >= 2) {
        cout
        << "c [blk-lit-opt] "
        << conf.print_times(time_used)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "blk-lit-opt"
            , time_used
        );
    }
}

inline Lit Searcher::find_good_blocked_lit(const Clause& c) const
{
    Lit lit = lit_Undef;
    uint32_t lowest_lev = std::numeric_limits<uint32_t>::max();
    for(size_t i = 1; i < c.size(); i++) {
        const Lit l = c[i];
        if (decisionLevel() > 0) {
            if (value(l) == l_True
                && varData[l.var()].level > 0
                && varData[l.var()].level < lowest_lev
            ) {
                lit = l;
                lowest_lev = varData[l.var()].level;
            }
        } else {
            if ((varData[l.var()].polarity ^ l.sign()) == true) {
                lit = l;
            }
        }
    }
    if (lit == lit_Undef) {
        lit = c[c.size()/2];
    }
    return lit;
}

Searcher::Stats& Searcher::Stats::operator+=(const Stats& other)
{
    numRestarts += other.numRestarts;
    blocked_restart += other.blocked_restart;
    blocked_restart_same += other.blocked_restart_same;

    //Decisions
    decisions += other.decisions;
    decisionsAssump += other.decisionsAssump;
    decisionsRand += other.decisionsRand;
    decisionFlippedPolar += other.decisionFlippedPolar;

    //Conflict minimisation stats
    litsRedNonMin += other.litsRedNonMin;
    litsRedFinal += other.litsRedFinal;
    recMinCl += other.recMinCl;
    recMinLitRem += other.recMinLitRem;

    furtherShrinkAttempt  += other.furtherShrinkAttempt;
    binTriShrinkedClause += other.binTriShrinkedClause;
    cacheShrinkedClause += other.cacheShrinkedClause;
    furtherShrinkedSuccess += other.furtherShrinkedSuccess;


    stampShrinkAttempt += other.stampShrinkAttempt;
    stampShrinkCl += other.stampShrinkCl;
    stampShrinkLit += other.stampShrinkLit;
    moreMinimLitsStart += other.moreMinimLitsStart;
    moreMinimLitsEnd += other.moreMinimLitsEnd;
    recMinimCost += other.recMinimCost;

    //Red stats
    learntUnits += other.learntUnits;
    learntBins += other.learntBins;
    learntTris += other.learntTris;
    learntLongs += other.learntLongs;
    otfSubsumed += other.otfSubsumed;
    otfSubsumedImplicit += other.otfSubsumedImplicit;
    otfSubsumedLong += other.otfSubsumedLong;
    otfSubsumedRed += other.otfSubsumedRed;
    otfSubsumedLitsGained += other.otfSubsumedLitsGained;

    //Hyper-bin & transitive reduction
    advancedPropCalled += other.advancedPropCalled;
    hyperBinAdded += other.hyperBinAdded;
    transReduRemIrred += other.transReduRemIrred;
    transReduRemRed += other.transReduRemRed;

    //Stat structs
    resolvs += other.resolvs;
    conflStats += other.conflStats;

    //Time
    cpu_time += other.cpu_time;

    return *this;
}

Searcher::Stats& Searcher::Stats::operator-=(const Stats& other)
{
    numRestarts -= other.numRestarts;
    blocked_restart -= other.blocked_restart;
    blocked_restart_same -= other.blocked_restart_same;

    //Decisions
    decisions -= other.decisions;
    decisionsAssump -= other.decisionsAssump;
    decisionsRand -= other.decisionsRand;
    decisionFlippedPolar -= other.decisionFlippedPolar;

    //Conflict minimisation stats
    litsRedNonMin -= other.litsRedNonMin;
    litsRedFinal -= other.litsRedFinal;
    recMinCl -= other.recMinCl;
    recMinLitRem -= other.recMinLitRem;

    furtherShrinkAttempt  -= other.furtherShrinkAttempt;
    binTriShrinkedClause -= other.binTriShrinkedClause;
    cacheShrinkedClause -= other.cacheShrinkedClause;
    furtherShrinkedSuccess -= other.furtherShrinkedSuccess;

    stampShrinkAttempt -= other.stampShrinkAttempt;
    stampShrinkCl -= other.stampShrinkCl;
    stampShrinkLit -= other.stampShrinkLit;
    moreMinimLitsStart -= other.moreMinimLitsStart;
    moreMinimLitsEnd -= other.moreMinimLitsEnd;
    recMinimCost -= other.recMinimCost;

    //Red stats
    learntUnits -= other.learntUnits;
    learntBins -= other.learntBins;
    learntTris -= other.learntTris;
    learntLongs -= other.learntLongs;
    otfSubsumed -= other.otfSubsumed;
    otfSubsumedImplicit -= other.otfSubsumedImplicit;
    otfSubsumedLong -= other.otfSubsumedLong;
    otfSubsumedRed -= other.otfSubsumedRed;
    otfSubsumedLitsGained -= other.otfSubsumedLitsGained;

    //Hyper-bin & transitive reduction
    advancedPropCalled -= other.advancedPropCalled;
    hyperBinAdded -= other.hyperBinAdded;
    transReduRemIrred -= other.transReduRemIrred;
    transReduRemRed -= other.transReduRemRed;

    //Stat structs
    resolvs -= other.resolvs;
    conflStats -= other.conflStats;

    //Time
    cpu_time -= other.cpu_time;

    return *this;
}

Searcher::Stats Searcher::Stats::operator-(const Stats& other) const
{
    Stats result = *this;
    result -= other;
    return result;
}

void Searcher::Stats::printCommon() const
{
    print_stats_line("c restarts"
        , numRestarts
        , (double)conflStats.numConflicts/(double)numRestarts
        , "confls per restart"

    );
    print_stats_line("c blocked restarts"
        , blocked_restart
        , (double)blocked_restart/(double)numRestarts
        , "per normal restart"

    );
    print_stats_line("c time", cpu_time);
    print_stats_line("c decisions", decisions
        , stats_line_percent(decisionsRand, decisions)
        , "% random"
    );

    print_stats_line("c decisions/conflicts"
        , (double)decisions/(double)conflStats.numConflicts
    );
}

void Searcher::Stats::print_short() const
{
    //Restarts stats
    printCommon();
    conflStats.print_short(cpu_time);

    print_stats_line("c conf lits non-minim"
        , litsRedNonMin
        , (double)litsRedNonMin/(double)conflStats.numConflicts
        , "lit/confl"
    );

    print_stats_line("c conf lits final"
        , (double)litsRedFinal/(double)conflStats.numConflicts
    );
}

void Searcher::Stats::print() const
{
    printCommon();
    conflStats.print(cpu_time);

    /*assert(numConflicts
        == conflsBin + conflsTri + conflsLongIrred + conflsLongRed);*/

    cout << "c LEARNT stats" << endl;
    print_stats_line("c units learnt"
        , learntUnits
        , stats_line_percent(learntUnits, conflStats.numConflicts)
        , "% of conflicts");

    print_stats_line("c bins learnt"
        , learntBins
        , stats_line_percent(learntBins, conflStats.numConflicts)
        , "% of conflicts");

    print_stats_line("c tris learnt"
        , learntTris
        , stats_line_percent(learntTris, conflStats.numConflicts)
        , "% of conflicts");

    print_stats_line("c long learnt"
        , learntLongs
        , stats_line_percent(learntLongs, conflStats.numConflicts)
        , "% of conflicts"
    );

    print_stats_line("c otf-subs"
        , otfSubsumed
        , ratio_for_stat(otfSubsumed, conflStats.numConflicts)
        , "/conflict"
    );

    print_stats_line("c otf-subs implicit"
        , otfSubsumedImplicit
        , stats_line_percent(otfSubsumedImplicit, otfSubsumed)
        , "%"
    );

    print_stats_line("c otf-subs long"
        , otfSubsumedLong
        , stats_line_percent(otfSubsumedLong, otfSubsumed)
        , "%"
    );

    print_stats_line("c otf-subs learnt"
        , otfSubsumedRed
        , stats_line_percent(otfSubsumedRed, otfSubsumed)
        , "% otf subsumptions"
    );

    print_stats_line("c otf-subs lits gained"
        , otfSubsumedLitsGained
        , ratio_for_stat(otfSubsumedLitsGained, otfSubsumed)
        , "lits/otf subsume"
    );

    cout << "c SEAMLESS HYPERBIN&TRANS-RED stats" << endl;
    print_stats_line("c advProp called"
        , advancedPropCalled
    );
    print_stats_line("c hyper-bin add bin"
        , hyperBinAdded
        , ratio_for_stat(hyperBinAdded, advancedPropCalled)
        , "bin/call"
    );
    print_stats_line("c trans-red rem irred bin"
        , transReduRemIrred
        , ratio_for_stat(transReduRemIrred, advancedPropCalled)
        , "bin/call"
    );
    print_stats_line("c trans-red rem red bin"
        , transReduRemRed
        , ratio_for_stat(transReduRemRed, advancedPropCalled)
        , "bin/call"
    );

    cout << "c CONFL LITS stats" << endl;
    print_stats_line("c orig "
        , litsRedNonMin
        , ratio_for_stat(litsRedNonMin, conflStats.numConflicts)
        , "lit/confl"
    );

    print_stats_line("c rec-min effective"
        , recMinCl
        , stats_line_percent(recMinCl, conflStats.numConflicts)
        , "% attempt successful"
    );

    print_stats_line("c rec-min lits"
        , recMinLitRem
        , stats_line_percent(recMinLitRem, litsRedNonMin)
        , "% less overall"
    );

    print_stats_line("c further-min call%"
        , stats_line_percent(furtherShrinkAttempt, conflStats.numConflicts)
        , stats_line_percent(furtherShrinkedSuccess, furtherShrinkAttempt)
        , "% attempt successful"
    );

    print_stats_line("c bintri-min lits"
        , binTriShrinkedClause
        , stats_line_percent(binTriShrinkedClause, litsRedNonMin)
        , "% less overall"
    );

    print_stats_line("c cache-min lits"
        , cacheShrinkedClause
        , stats_line_percent(cacheShrinkedClause, litsRedNonMin)
        , "% less overall"
    );

    print_stats_line("c stamp-min call%"
        , stats_line_percent(stampShrinkAttempt, conflStats.numConflicts)
        , stats_line_percent(stampShrinkCl, stampShrinkAttempt)
        , "% attempt successful"
    );

    print_stats_line("c stamp-min lits"
        , stampShrinkLit
        , stats_line_percent(stampShrinkLit, litsRedNonMin)
        , "% less overall"
    );

    print_stats_line("c final avg"
        , ratio_for_stat(litsRedFinal, conflStats.numConflicts)
    );

    //General stats
    //print_stats_line("c Memory used", (double)mem_used / 1048576.0, " MB");
    #if !defined(_MSC_VER) && defined(RUSAGE_THREAD)
    print_stats_line("c single-thread CPU time", cpu_time, " s");
    #else
    print_stats_line("c all-threads sum CPU time", cpu_time, " s");
    #endif
}

void Searcher::update_var_decay()
{
    var_decay = conf.var_decay_max;
}

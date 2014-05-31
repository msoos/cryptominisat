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

#include "searcher.h"
#include "simplifier.h"
#include "calcdefpolars.h"
#include "time_mem.h"
#include "solver.h"
#include <iomanip>
#include "sccfinder.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "propbyforgraph.h"
#include <algorithm>
#include <cstddef>
#include "sqlstats.h"
#include "datasync.h"
#include "reducedb.h"

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
Searcher::Searcher(const SolverConf& _conf, Solver* _solver, bool* _needToInterrupt) :
        HyperEngine(
            _conf
            , _needToInterrupt
        )

        //variables
        , solver(_solver)
        , var_inc(_conf.var_inc_start)
        , order_heap(VarOrderLt(activities))
        , clauseActivityIncrease(1)
{
    more_red_minim_limit_binary_actual = _conf.more_red_minim_limit_binary;
    more_red_minim_limit_cache_actual = _conf.more_red_minim_limit_cache;
    mtrand.seed(conf.origSeed);
    hist.setSize(conf.shortTermHistorySize, conf.blocking_restart_trail_hist_length);
}

Searcher::~Searcher()
{
}

void Searcher::new_var(const bool bva, const Var orig_outer)
{
    PropEngine::new_var(bva, orig_outer);
    activities.push_back(0);
    insertVarOrder(nVars()-1);
    assumptionsSet.push_back(false);

    act_polar_backup.activity.push_back(0);
    act_polar_backup.polarity.push_back(false);
}

void Searcher::new_vars(size_t n)
{
    PropEngine::new_vars(n);
    activities.resize(activities.size() + n, 0);
    for(int i = n-1; i >= 0; i--) {
        insertVarOrder((int)nVars()-i-1);
    }
    assumptionsSet.resize(assumptionsSet.size() + n, false);

    act_polar_backup.activity.resize(act_polar_backup.activity.size() + n, 0);
    act_polar_backup.polarity.resize(act_polar_backup.polarity.size() + n, false);
}

void Searcher::saveVarMem()
{
    PropEngine::saveVarMem();
    activities.resize(nVars());
    activities.shrink_to_fit();
    assumptionsSet.resize(nVars());
    assumptionsSet.shrink_to_fit();

    act_polar_backup.activity.resize(nVars());
    act_polar_backup.activity.shrink_to_fit();
    act_polar_backup.polarity.resize(nVars());
    act_polar_backup.polarity.shrink_to_fit();
}

void Searcher::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    if (act_polar_backup.saved) {
        updateArray(act_polar_backup.activity, interToOuter);
        updateArray(act_polar_backup.polarity, interToOuter);
    }
    //activities are not updated, they are taken from backup, which is updated
    renumber_assumptions(outerToInter);
    assert(longest_dec_trail.empty());
}

void Searcher::renumber_assumptions(const vector<Var>& outerToInter)
{
    for(const Lit lit: assumptions) {
        if (lit.var() < assumptionsSet.size()) {
            assumptionsSet[lit.var()] = false;
        } else {
            assert(solver->value(lit) != l_Undef);
        }
    }
    updateLitsMap(assumptions, outerToInter);
    for(const Lit lit: assumptions) {
        if (lit.var() < assumptionsSet.size()) {
            assumptionsSet[lit.var()] = true;
        } else {
            assert(solver->value(lit) != l_Undef);
        }
    }
}

/**
@brief Revert to the state at given level
*/
void Searcher::cancelUntil(uint32_t level)
{
    #ifdef VERBOSE_DEBUG
    cout << "Canceling until level " << level;
    if (level > 0) cout << " sublevel: " << trail_lim[level];
    cout << endl;
    #endif

    if (decisionLevel() > level) {

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

            const Var var = trail[sublevel].var();
            assert(value(var) != l_Undef);
            assigns[var] = l_Undef;
            insertVarOrder(var);
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

void Searcher::add_lit_to_learnt(
    const Lit lit
    , bool fromProber
) {
    const Var var = lit.var();
    assert(varData[var].removed == Removed::none
        || varData[var].removed == Removed::queued_replacer
    );

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
            if (!fromProber
                && params.rest_type != restart_type_geom
                && varData[var].reason != PropBy()
                && varData[var].reason.getType() == clause_t
            ) {
                Clause* cl = clAllocator.getPointer(varData[var].reason.getClause());
                if (cl->red()) {
                    lastDecisionLevel.push_back(std::make_pair(lit, cl->stats.glue));
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
        ; it++
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
    ClOffset offset = confl.getClause();
    Clause& cl = *clAllocator.getPointer(offset);

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
                cl = clAllocator.getPointer(reason.getClause());
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
                break;
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

                case null_clause_t:
                    release_assert(false);
                    std::exit(-1);
                    break;
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
            Clause* cl = clAllocator.getPointer(confl.getClause());
            cout << "resolv (long): " << *cl << endl;
            break;
        }

        case null_clause_t: {
            assert(false);
            break;
        }
    }
}

Clause* Searcher::add_literals_from_confl_to_learnt(
    const PropBy confl
    , const Lit p
    , bool fromProber
) {
    debug_print_resolving_clause(confl);

    Clause* cl = NULL;
    switch (confl.getType()) {
        case tertiary_t : {
            resolutions.tri++;
            stats.resolvs.tri++;
            add_lit_to_learnt(confl.lit3(), fromProber);

            if (p == lit_Undef) {
                add_lit_to_learnt(failBinLit, fromProber);
            }

            add_lit_to_learnt(confl.lit2(), fromProber);
            break;
        }

        case binary_t : {
            resolutions.bin++;
            stats.resolvs.bin++;
            if (p == lit_Undef) {
                add_lit_to_learnt(failBinLit, fromProber);
            }
            add_lit_to_learnt(confl.lit2(), fromProber);
            break;
        }

        case clause_t : {
            cl = clAllocator.getPointer(confl.getClause());
            if (cl->red()) {
                resolutions.redL++;
                stats.resolvs.redL++;
            } else {
                resolutions.irredL++;
                stats.resolvs.irredL++;
            }
            cl->stats.used_for_uip_creation++;
            if (cl->red() && !fromProber) {
                bumpClauseAct(cl);
            }

            for (size_t j = 0; j < cl->size(); j++) {
                //Will be resolved away, skip
                if (p != lit_Undef && j == 0)
                    continue;

                add_lit_to_learnt((*cl)[j], fromProber);
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

void Searcher::mimimize_learnt_clause_based_on_cache()
{
    if (conf.doMinimRedMore
        && learnt_clause.size() > 1
        && (conf.doAlwaysFMinim
            || calcGlue(learnt_clause) < 0.65*hist.glueHistLT.avg()
            || learnt_clause.size() < 0.65*hist.conflSizeHistLT.avg()
            || learnt_clause.size() < 10
            )
    ) {
        stats.moreMinimLitsStart += learnt_clause.size();

        //Binary&cache-based minim
        minimise_redundant_more(learnt_clause);

        //Stamp-based minimization
        if (conf.doStamp) {
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

Clause* Searcher::create_learnt_clause(PropBy confl, bool fromProber)
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

        last_resolved_long_cl = add_literals_from_confl_to_learnt(confl, p, fromProber);

        // Select next implication to look at
        while (!seen[trail[index--].var()]);

        p = trail[index+1];

        if (!fromProber
            && conf.doOTFSubsume
            //A long clause
            && last_resolved_long_cl != NULL
            //Must subsume, so must be smaller
            && last_resolved_long_cl->size() > tmp_learnt_clause_size
            //Everything in learnt_cl_2 seems to be also in cl
            && ((last_resolved_long_cl->abst & tmp_learnt_clause_abst) ==  tmp_learnt_clause_abst)
            && pathC > 1
        ) {
            check_otf_subsume(confl);
        }

        confl = varData[p.var()].reason;

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

void Searcher::bump_var_activities_based_on_last_decision_level(size_t glue)
{
    for (vector<pair<Lit, size_t> >::const_iterator
        it = lastDecisionLevel.begin(), end = lastDecisionLevel.end()
        ; it != end
        ; it++
    ) {
        if (it->second < glue) {
            bump_var_activitiy(it->first.var());
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
    PropBy confl
    , uint32_t& out_btlevel
    , uint32_t& glue
    , bool fromProber
) {
    //Set up environment
    learnt_clause.clear();
    assert(toClear.empty());
    lastDecisionLevel.clear();
    otf_subsuming_short_cls.clear();
    otf_subsuming_long_cls.clear();
    tmp_learnt_clause_size = 0;
    tmp_learnt_clause_abst = 0;
    assert(decisionLevel() > 0);

    print_debug_resolution_data(confl);
    Clause* last_resolved_long_cl = create_learnt_clause(confl, fromProber);
    stats.litsRedNonMin += learnt_clause.size();
    minimize_learnt_clause();
    mimimize_learnt_clause_based_on_cache();
    print_fully_minimized_learnt_clause();

    glue = calcGlue(learnt_clause);
    stats.litsRedFinal += learnt_clause.size();
    out_btlevel = find_backtrack_level_of_learnt();
    if (!fromProber && params.rest_type == restart_type_glue && conf.extra_bump_var_activities_based_on_glue) {
        bump_var_activities_based_on_last_decision_level(glue);
    }
    lastDecisionLevel.clear();

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
                cl = clAllocator.getPointer(reason.getClause());
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

void Searcher::analyzeFinal(const Lit p, vector<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push_back(p);

    if (decisionLevel() == 0)
        return;

    for (int32_t i = (int32_t)trail.size()-1; i >= (int32_t)trail_lim[0]; i--) {
        const Var x = trail[i].var();
        if (varData[x].reason.isNULL()) {
            assert(varData[x].level > 0);
            out_conflict.push_back(~trail[i]);
        }
    }
}

void Searcher::handle_longest_decision_trail()
{
    if (conf.doPrintLongestTrail == 0)
        return;

    //This comparision is NOT perfect, but it's fast. See below for exceptions
    if (decisionLevel() > longest_dec_trail.size()) {
        longest_dec_trail.clear();
        for(size_t i = 0; i < trail_lim.size(); i++) {

            //Avoid to print dummy decision levels' stuff
            if (trail_lim.size() > i+1
                && trail_lim[i+1] == trail_lim[i]
            ) {
                continue;
            }

            //Just in case there are some dummy decision levels, etc.
            if (trail.size() > i+1) {
                longest_dec_trail.push_back(trail[i+1]);
            }
        }
    }

    size_t diff = sumConflicts() - last_confl_longest_dec_trail_printed;
    if (diff >= conf.doPrintLongestTrail) {
        cout
        << "c [long-dec-trail] ";

        for(Lit lit: longest_dec_trail) {
            cout
            << solver->map_inter_to_outer(lit) << " ";
        }
        cout << endl;

        last_confl_longest_dec_trail_printed = sumConflicts();
    }
}

void Searcher::hyper_bin_update_cache(vector<Lit>& to_enqueue_toplevel)
{
    size_t numElems = trail.size() - trail_lim[0];
    if (conf.doCache
        && numElems <= conf.cacheUpdateCutoff
    ) {
        for (int64_t c = trail.size()-1; c > (int64_t)trail_lim[0]; c--) {
            const Lit thisLit = trail[c];
            const Lit ancestor = varData[thisLit.var()].reason.getAncestor();
            assert(thisLit != trail[trail_lim[0]]);
            const bool redStep = varData[thisLit.var()].reason.isRedStep();

            assert(ancestor != lit_Undef);
            bool taut = solver->implCache[(~ancestor).toInt()].merge(
                solver->implCache[(~thisLit).toInt()].lits
                , thisLit
                , redStep
                , ancestor.var()
                , solver->seen
            );

            //There is an ~ancestor V OTHER, ~ancestor V ~OTHER
            //So enqueue ~ancestor
            if (taut
                && (solver->varData[ancestor.var()].removed == Removed::none
                    || solver->varData[ancestor.var()].removed == Removed::queued_replacer)
            ) {
                to_enqueue_toplevel.push_back(~ancestor);
                *drup << (~ancestor) << fin;
            }
        }
    }
}

lbool Searcher::otf_hyper_prop_first_dec_level(bool& must_continue)
{
    assert(decisionLevel() == 1);

    must_continue = false;
    stats.advancedPropCalled++;
    solver->varData[trail.back().var()].depth = 0;
    Lit failed = propagateFullBFS();
    if (failed != lit_Undef) {
        *drup << ~failed << fin;

        //Update conflict stats
        stats.learntUnits++;
        stats.conflStats.numConflicts++;
        stats.conflStats.update(lastConflictCausedBy);
        #ifdef STATS_NEEDED
        hist.conflictAfterConflict.push(last_decision_ended_in_conflict);
        last_decision_ended_in_conflict = true;
        #endif

        cancelUntil(0);
        stats.litsRedNonMin += 1;
        stats.litsRedFinal += 1;
        #ifdef STATS_NEEDED
        propStats.propsUnit++;
        #endif
        stats.hyperBinAdded += hyperBinResAll();
        std::pair<size_t, size_t> tmp = removeUselessBins();
        stats.transReduRemIrred += tmp.first;
        stats.transReduRemRed += tmp.second;
        solver->enqueue(~failed);

        must_continue = true;
        return l_Undef;
    }

    vector<Lit> to_enqueue_toplevel;
    hyper_bin_update_cache(to_enqueue_toplevel);

    stats.hyperBinAdded += hyperBinResAll();
    std::pair<size_t, size_t> tmp = removeUselessBins();
    stats.transReduRemIrred += tmp.first;
    stats.transReduRemRed += tmp.second;

    //There are things to enqueue at top-level
    if (!to_enqueue_toplevel.empty()) {
        solver->cancelUntil(0);
        bool ret = solver->enqueueThese(to_enqueue_toplevel);
        if (!ret) {
            return l_False;
        }

        //Start from beginning
        must_continue = true;
        return l_Undef;
    }

    return l_Undef;
}

lbool Searcher::search()
{
    assert(ok);
    const double myTime = cpuTime();

    //Stats reset & update
    if (params.update)
        stats.numRestarts++;
    agility.reset(conf.agilityLimit);
    hist.clear();

    assert(solver->prop_at_head());

    //Loop until restart or finish (SAT/UNSAT)
    last_decision_ended_in_conflict = false;
    PropBy confl;

    while (
        (!params.needToStopSearch
            && sumConflicts() <= solver->getNextCleanLimit()
            && cpuTime() < conf.maxTime
            && !must_interrupt_asap()
        )
            || !confl.isNULL() //always finish the last conflict
    ) {
        if (!confl.isNULL()) {
            stats.conflStats.update(lastConflictCausedBy);
            checkNeedRestart();
            print_restart_stat();
            #ifdef STATS_NEEDED
            hist.conflictAfterConflict.push(last_decision_ended_in_conflict);
            #endif
            last_decision_ended_in_conflict = true;
            handle_longest_decision_trail();
            if (!handle_conflict(confl)) {
                dump_search_sql(myTime);
                return l_False;
            }
        } else {
            assert(ok);
            last_decision_ended_in_conflict = false;
            const lbool ret = new_decision();
            if (ret != l_Undef) {
                dump_search_sql(myTime);
                return ret;
            }
        }

        again:
        if (do_otf_this_round
            && decisionLevel() == 1
        ) {
            bool must_continue;
            lbool ret = otf_hyper_prop_first_dec_level(must_continue);
            if (ret != l_Undef) {
                dump_search_sql(myTime);
                return ret;
            }
            if (must_continue)
                goto again;
            confl = PropBy();
        } else {
            //Decision level is higher than 1, so must do normal propagation
            confl = propagate(
                #ifdef STATS_NEEDED
                &hist.watchListSizeTraversed
                #endif
            );
        }
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
    if (solver->conf.doSQL) {
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
        Lit p = assumptions[decisionLevel()];
        assert(varData[p.var()].removed == Removed::none
            || varData[p.var()].removed == Removed::queued_replacer
        );

        if (value(p) == l_True) {
            // Dummy decision level:
            newDecisionLevel();
        } else if (value(p) == l_False) {
            analyzeFinal(~p, conflict);
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
        #ifdef STATS_NEEDED_EXTRA
        if (next.sign()) {
            varData[next.var()].stats.negDecided++;
        } else {
            varData[next.var()].stats.posDecided++;
        }
        #endif
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    newDecisionLevel();
    enqueue(next);

    return l_Undef;
}

void Searcher::checkNeedRestart()
{
    if (must_interrupt_asap())  {
        if (conf.verbosity >= 3)
            cout << "c must_interrupt_asap() is set, restartig as soon as possible!" << endl;
        params.needToStopSearch = true;
    }

    switch (params.rest_type) {

        case restart_type_never:
            //Just don't restart no matter what
            break;

        case restart_type_geom:
            if (params.conflictsDoneThisRestart > max_conflicts_geometric)
                params.needToStopSearch = true;

            break;

        case restart_type_glue:
            if (conf.do_blocking_restart
                && hist.glueHist.isvalid()
                && hist.trailDepthHistLonger.isvalid()
                && decisionLevel() > 0
                && (trail.size()-trail_lim.at(0)) > hist.trailDepthHistLonger.avg()*conf.blocking_restart_multip
            ) {
                hist.glueHist.clear();
            }

            if (hist.glueHist.isvalid()
                && 0.95*hist.glueHist.avg() > hist.glueHistLT.avg()
            ) {
                params.needToStopSearch = true;
            }

            break;

        case restart_type_glue_agility:
            if (hist.glueHist.isvalid()
                && 0.95*hist.glueHist.avg() > hist.glueHistLT.avg()
                && agility.getAgility() < conf.agilityLimit
            ) {
                params.numAgilityNeedRestart++;
                if (params.numAgilityNeedRestart > conf.agilityViolationLimit) {
                    params.needToStopSearch = true;
                }
            } else {
                //Reset counter
                params.numAgilityNeedRestart = 0;
            }

            break;

        case restart_type_agility:
            if (agility.getAgility() < conf.agilityLimit) {
                params.numAgilityNeedRestart++;
                if (params.numAgilityNeedRestart > conf.agilityViolationLimit) {
                    params.needToStopSearch = true;
                }
            } else {
                //Reset counter
                params.numAgilityNeedRestart = 0;
            }

            break;
        default:
            assert(false && "This should not happen, auto decision is make before this point");
            break;
    }

    //If agility was used and it's too high, print it if need be
    if (conf.verbosity >= 4
        && params.needToStopSearch
        && (conf.restartType == restart_type_agility
            || conf.restartType == restart_type_glue_agility)
    ) {
        cout << "c Agility was too low, restarting asap";
        printAgilityStats();
        cout << endl;
    }

    //Conflict limit reached?
    if (params.conflictsDoneThisRestart > params.conflictsToDo) {
        if (conf.verbosity >= 3)
            cout
            << "c Over limit of conflicts for this restart"
            << " -- restarting as soon as possible!" << endl;
        params.needToStopSearch = true;
    }
}

void Searcher::add_otf_subsume_long_clauses()
{
    //Hande long OTF subsumption
    for(size_t i = 0; i < otf_subsuming_long_cls.size(); i++) {
        const ClOffset offset = otf_subsuming_long_cls[i];
        Clause& cl = *solver->clAllocator.getPointer(offset);
        cl.stats.conflicts_made += conf.rewardShortenedClauseWithConfl;

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

            if (do_otf_this_round && decisionLevel() == 1) {
                addHyperBin(cl[0], cl);
            } else {
                enqueue(cl[0], decisionLevel() == 0 ? PropBy() : PropBy(offset));

                //Drup
                if (decisionLevel() == 0) {
                    *drup << cl[0] << fin;
                }
            }
        } else {
            //We have a non-propagating clause

            std::swap(cl[at], cl[1]);
            assert(value(cl[1]) == l_Undef || value(cl[1]) == l_True);
        }
        solver->attachClause(cl, false);
        cl.reCalcAbstraction();
    }
    otf_subsuming_long_cls.clear();
}

void Searcher::add_otf_subsume_implicit_clause()
{
    //Handle implicit OTF subsumption
    for(vector<OTFClause>::iterator
        it = otf_subsuming_short_cls.begin(), end = otf_subsuming_short_cls.end()
        ; it != end
        ; it++
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
            if (do_otf_this_round && decisionLevel() == 1) {
                if (it->size == 2) {
                    enqueueComplex(it->lits[0], ~it->lits[1], true);
                } else {
                    addHyperBin(it->lits[0], it->lits[1], it->lits[2]);
                }
            } else {
                //Calculate reason
                PropBy by = PropBy();

                //if decision level is non-zero, we have to be more careful
                if (decisionLevel() != 0) {
                    if (it->size == 2) {
                        by = PropBy(it->lits[1]);
                    } else {
                        by = PropBy(it->lits[1], it->lits[2]);
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
                solver->attachBinClause(it->lits[0], it->lits[1], true);
            } else {
                solver->attachTriClause(it->lits[0], it->lits[1], it->lits[2], true);
            }
        }
    }
    otf_subsuming_short_cls.clear();
}

void Searcher::update_history_stats(size_t backtrack_level, size_t glue)
{
    assert(decisionLevel() > 0);
    hist.trailDepthHist.push(trail.size() - trail_lim[0]);
    hist.trailDepthHistLonger.push(trail.size() - trail_lim[0]);
    hist.trailDepthHistLT.push(trail.size() - trail_lim[0]);

    hist.branchDepthHist.push(decisionLevel());
    hist.branchDepthHistLT.push(decisionLevel());

    hist.branchDepthDeltaHist.push(decisionLevel() - backtrack_level);
    hist.branchDepthDeltaHistLT.push(decisionLevel() - backtrack_level);

    hist.glueHist.push(glue);
    hist.glueHistLT.push(glue);

    hist.conflSizeHist.push(learnt_clause.size());
    hist.conflSizeHistLT.push(learnt_clause.size());

    hist.agilityHist.push(agility.getAgility());
    hist.agilityHistLT.push(agility.getAgility());

    hist.numResolutionsHist.push(resolutions.sum());
    hist.numResolutionsHistLT.push(resolutions.sum());

    hist.trailDepthDeltaHist.push(trail.size() - trail_lim[backtrack_level]);
    hist.trailDepthDeltaHistLT.push(trail.size() - trail_lim[backtrack_level]);

    #ifdef STATS_NEEDED_EXTRA
    if (conf.doSQL && conf.dumpClauseDistribPer != 0) {
        if (sumConflicts() % conf.dumpClauseDistribPer == 0) {
            printClauseDistribSQL();

            //Clear distributions
            std::fill(clauseSizeDistrib.begin(), clauseSizeDistrib.end(), 0);
            std::fill(clauseGlueDistrib.begin(), clauseGlueDistrib.end(), 0);
            for(size_t i = 0; i < sizeAndGlue.shape()[0]; i++) {
                for(size_t i2 = 0; i2 < sizeAndGlue.shape()[1]; i2++) {
                    sizeAndGlue[i][i2] = 0;
                }
            }
        }

        //Add this new clause to distributions
        uint32_t truncSize = std::min<uint32_t>(learnt_clause.size(), conf.dumpClauseDistribMaxSize-1);
        uint32_t truncGlue = std::min<uint32_t>(glue, conf.dumpClauseDistribMaxGlue-1);
        clauseSizeDistrib[truncSize]++;
        clauseGlueDistrib[truncGlue]++;
        sizeAndGlue[truncSize][truncGlue]++;
    }
    #endif
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
            solver->attachBinClause(learnt_clause[0], learnt_clause[1], true);
            if (do_otf_this_round && decisionLevel() == 1)
                enqueueComplex(learnt_clause[0], ~learnt_clause[1], true);
            else
                enqueue(learnt_clause[0], PropBy(learnt_clause[1]));

            #ifdef STATS_NEEDED
            propStats.propsBinRed++;
            #endif
            break;

        case 3:
            //3-long learnt
            stats.learntTris++;
            std::sort((&learnt_clause[0])+1, (&learnt_clause[0])+3);
            solver->attachTriClause(learnt_clause[0], learnt_clause[1], learnt_clause[2], true);

            if (do_otf_this_round && decisionLevel() == 1)
                addHyperBin(learnt_clause[0], learnt_clause[1], learnt_clause[2]);
            else
                enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2]));

            #ifdef STATS_NEEDED
            propStats.propsTriRed++;
            #endif
            break;

        default:
            //Long learnt
            cl->stats.resolutions = resolutions;
            stats.learntLongs++;
            std::sort(learnt_clause.begin()+1, learnt_clause.end(), PolaritySorter(varData));
            solver->attachClause(*cl);
            if (do_otf_this_round && decisionLevel() == 1)
                addHyperBin(learnt_clause[0], *cl);
            else
                enqueue(learnt_clause[0], PropBy(clAllocator.getOffset(cl)));

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
            cl = clAllocator.Clause_new(learnt_clause, Searcher::sumConflicts());
            cl->makeRed(glue);
            ClOffset offset = clAllocator.getOffset(cl);
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
    cl->stats.conflicts_made += conf.rewardShortenedClauseWithConfl;

    return cl;
}

bool Searcher::handle_conflict(PropBy confl)
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
        , false
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

void Searcher::genRandomVarActMultDiv()
{
    uint32_t tosubstract = conf.var_inc_variability-mtrand.randInt(2*conf.var_inc_variability);
    var_inc_multiplier = conf.var_inc_multiplier - tosubstract;
    var_inc_divider = conf.var_inc_divider - tosubstract;

    if (conf.verbosity >= 1) {
        cout
        << "c Using var act-multip " << var_inc_multiplier
        << " instead of standard " << (conf.var_inc_multiplier)
        << " and act-divider " << var_inc_divider
        << " instead of standard " << (conf.var_inc_divider)
        << endl;
    }
}

void Searcher::resetStats()
{
    startTime = cpuTime();

    //About vars
    #ifdef STATS_NEEDED_EXTRA
    for(vector<VarData>::iterator
        it = varData.begin(), end = varData.end()
        ; it != end
        ; it++
    ) {
        it->stats.reset();
    }

    //Clause data
    if (conf.dumpClauseDistribPer != 0) {
        clauseSizeDistrib.resize(conf.dumpClauseDistribMaxSize, 0);
        clauseGlueDistrib.resize(conf.dumpClauseDistribMaxGlue, 0);
        sizeAndGlue.resize(boost::extents[conf.dumpClauseDistribMaxSize][conf.dumpClauseDistribMaxGlue]);
        for(size_t i = 0; i < sizeAndGlue.shape()[0]; i++) {
            for(size_t i2 = 0; i2 < sizeAndGlue.shape()[1]; i2++) {
                sizeAndGlue[i][i2] = 0;
            }
        }
    }
    #endif

    //Rest solving stats
    stats.clear();
    propStats.clear();
    #ifdef STATS_NEEDED
    lastSQLPropStats = propStats;
    lastSQLGlobalStats = stats;
    #endif

    lastCleanZeroDepthAssigns = trail.size();
}

lbool Searcher::burstSearch()
{
    const double myTime = cpuTime();
    if (conf.verbosity >= 2) {
        cout
        << "c Doing burst search for " << conf.burstSearchLen << " conflicts"
        << endl;
    }
    const size_t numUnitsUntilNow = stats.learntUnits;
    const size_t numBinsUntilNow = stats.learntBins;
    #ifdef STATS_NEEDED
    const size_t numTriLHBRUntilNow = propStats.triLHBR;
    const size_t numLongLHBRUntilNow = propStats.longLHBR;
    #endif

    //Save old config
    const double backup_rand = conf.random_var_freq;
    const PolarityMode backup_polar_mode = conf.polarity_mode;
    uint32_t backup_var_inc_divider = var_inc_divider;
    uint32_t backup_var_inc_multiplier = var_inc_multiplier;

    //Set burst config
    conf.random_var_freq = 1;
    conf.polarity_mode = polarmode_rnd;
    var_inc_divider = 1;
    var_inc_multiplier = 1;

    //Do burst
    params.clear();
    params.conflictsToDo = conf.burstSearchLen;
    params.rest_type = restart_type_never;
    lbool status = search();
    longest_dec_trail.clear();

    //Restore config
    conf.random_var_freq = backup_rand;
    conf.polarity_mode = backup_polar_mode;
    var_inc_divider = backup_var_inc_divider;
    var_inc_multiplier = backup_var_inc_multiplier;

    //Print what has happened
    const double time_used = cpuTime() - myTime;
    if (conf.verbosity >= 2) {
        cout
        << "c "
        << conf.burstSearchLen << "-long burst search "
        << " learnt units:" << (stats.learntUnits - numUnitsUntilNow)
        << " learnt bins: " << (stats.learntBins - numBinsUntilNow)
        << " T: " << std::setprecision(2) << std::fixed << time_used
        #ifdef STATS_NEEDED
        << " LHBR: "
        << (propStats.triLHBR + propStats.longLHBR - numLongLHBRUntilNow - numTriLHBRUntilNow)
        #endif
        << endl;
    }

    return status;
}

void Searcher::printRestartHeader() const
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

void Searcher::printRestartStats() const
{
    printBaseStats();
    if (conf.printFullStats) {
        solver->printClauseStats();
        hist.print();
    } else {
        solver->printClauseStats();
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
    << " " << std::setw(7) << solver->getNumFreeVars()
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

/*void Searcher::printVarStatsSQL()
{
    vector<MyPolarData> polarData;
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].posPolarSet == 0 && varData[i].negPolarSet == 0)
            continue;

        polarData.push_back(MyPolarData(
            varData[i].posPolarSet
            , varData[i].negPolarSet
            , varData[i].flippedPolarity
        ));
    }
    std::sort(polarData.begin(), polarData.end());

    for(size_t i = 0; i < polarData.size(); i++) {
        solver->sqlFile
        << "insert into `polarSet`"
        << "("
        << " `runID`, `simplifications`"
        << " , `order`, `pos`, `neg`, `total`, `flipped`"
        << ")"
        << " values ("
        //Position
        << "  " << solver->getSolveStats().runID
        << ", " << solver->getSolveStats().numSimplify
        //Data
        << ", " << i
        << ", " << polarData[i].pos
        << ", " << polarData[i].neg
        << ", " << polarData[i].pos + polarData[i].neg
        << ", " << polarData[i].flipped
        << " );" << endl;
    }
}*/

#ifdef STATS_NEEDED
#ifdef STATS_NEEDED_EXTRA
void Searcher::calcVariances(
    const vector<VarData>& data
    , double& avgDecLevelVar
    , double& avgTrailLevelVar
) {
    double sumVarDec = 0;
    double sumVarTrail = 0;
    size_t num = 0;
    size_t maxDecLevel = 0;
    for(size_t i = 0; i < nVars(); i++) {
        const VarData val = data[i];
        if (val.stats.posPolarSet || val.stats.negPolarSet) {
            sumVarDec += sqrt(val.stats.decLevelHist.var());
            sumVarTrail += sqrt(val.stats.decLevelHist.var());
            maxDecLevel = std::max<size_t>(val.stats.decLevelHist.getMax(), maxDecLevel);
            num++;
        }
    }

    avgDecLevelVar = sumVarDec/(double)num;
    avgTrailLevelVar = sumVarTrail/(double)num;
}
#endif

void Searcher::printRestartSQL()
{
    //Propagation stats
    PropStats thisPropStats = propStats - lastSQLPropStats;
    Stats thisStats = stats - lastSQLGlobalStats;

    //Print variance
    VariableVariance variableVarianceStat;
    #ifdef STATS_NEEDED_EXTRA
    if (conf.dump_tree_variance_stats) {
        calcVariances(varData, variableVarianceStat.avgDecLevelVar, variableVarianceStat.avgTrailLevelVar);
        calcVariances(varDataLT, variableVarianceStat.avgDecLevelVarLT, variableVarianceStat.avgTrailLevelVarLT);
    }
    #endif

    solver->sqlStats->restart(
        thisPropStats
        , thisStats
        , variableVarianceStat
        , solver
        , this
    );

    lastSQLPropStats = propStats;
    lastSQLGlobalStats = stats;

    //Variable stats
    #ifdef STATS_NEEDED_EXTRA
    if (conf.dumpTopNVars > 0) {
        solver->sqlStats->varDataDump(solver, this, calcVarsToDump(), varData);
    }
    #endif
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

#ifdef STATS_NEEDED_EXTRA
vector<Var> Searcher::calcVarsToDump() const
{
    //How much to dump per criteria
    const size_t numToDump = std::min(varData.size(), conf.dumpTopNVars);

    //Collect what needs to be dumped here
    set<Var> todump;

    //Top N vars polarity set
    vector<VarDumpOrder> order;
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].stats.posPolarSet + varData[i].stats.negPolarSet > 0) {
            order.push_back(
                VarDumpOrder(
                    i
                    , varData[i].stats.posPolarSet
                        + varData[i].stats.negPolarSet
                )
            );
        }
    }
    std::sort(order.begin(), order.end());

    //These vars need to be dumped according to above stat
    for(size_t i = 0; i < std::min(numToDump, order.size()); i++) {
        todump.insert(order[i].var);
    }

    //Top N vars, number of times decided on
    order.clear();
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].stats.posDecided + varData[i].stats.negDecided > 0) {
            order.push_back(
                VarDumpOrder(
                    i
                    , varData[i].stats.negDecided
                        + varData[i].stats.posDecided
                )
            );
        }
    }
    std::sort(order.begin(), order.end());

    //These vars need to be dumped according to above stat
    for(size_t i = 0; i < std::min(numToDump, order.size()); i++) {
        todump.insert(order[i].var);
    }

    vector<Var> toDumpVec;
    for(set<Var>::const_iterator
        it = todump.begin(), end = todump.end()
        ; it != end
        ; it++
    ) {
        toDumpVec.push_back(*it);
    }

    return toDumpVec;
}

void Searcher::printClauseDistribSQL()
{
    solver->sqlStats->clauseSizeDistrib(
        sumConflicts()
        , clauseSizeDistrib
    );
    solver->sqlStats->clauseGlueDistrib(
        sumConflicts()
        , clauseGlueDistrib
    );

    solver->sqlStats->clauseSizeGlueScatter(
        sumConflicts()
        , sizeAndGlue
    );
}
#endif

Restart Searcher::decide_restart_type() const
{
    Restart rest_type = conf.restartType;
    if (rest_type == restart_type_automatic) {
        if (solver->sumPropStats.propagations == 0) {

            //If no data yet, default to restart_type_glue
            rest_type = restart_type_glue;
        } else {
            //Otherwise, choose according to % of pos/neg polarities
            double total = solver->sumPropStats.varSetNeg + solver->sumPropStats.varSetPos;
            double percent = ((double)solver->sumPropStats.varSetNeg)/total*100.0;
            if (percent > 60.0
                || percent < 40.0
            ) {
                rest_type = restart_type_glue;
            } else {
                rest_type = restart_type_geom;
            }

            if (conf.verbosity >= 1) {
                cout
                << "c percent of negative polarities set: "
                << std::setprecision(2) << percent
                << " % (this is used to choose restart type)"
                << endl;
            }
        }

        if (conf.verbosity >= 2) {
            cout
            << "c Chose restart type "
            << restart_type_to_string(rest_type)
            << endl;
        }
    }

    return rest_type;
}

void Searcher::print_restart_stat()
{
    //Print restart stat
    if (conf.verbosity >= 2
        && ((lastRestartPrint + 2000) < stats.conflStats.numConflicts)
    ) {
        //Print restart output header
        if (lastRestartPrintHeader == 0
            ||(lastRestartPrintHeader + 20000) < stats.conflStats.numConflicts
        ) {
            printRestartHeader();
            lastRestartPrintHeader = stats.conflStats.numConflicts;
        }
        printRestartStats();
        lastRestartPrint = stats.conflStats.numConflicts;
    }
}

void Searcher::setup_restart_print()
{
    //Set up restart printing status
    lastRestartPrint = stats.conflStats.numConflicts;
    lastRestartPrintHeader = stats.conflStats.numConflicts;
    if (conf.verbosity >= 1) {
        printRestartStats();
    }
}

void Searcher::restore_order_heap()
{
    order_heap.clear();
    for(size_t var = 0; var < nVars(); var++) {
        if (solver->varData[var].is_decision
            && value(var) == l_Undef
        ) {
            assert(varData[var].removed == Removed::none
                || varData[var].removed == Removed::queued_replacer
            );
            insertVarOrder(var);
        }
    }
    assert(order_heap.heapProperty());
}

void Searcher::reduce_db_if_needed()
{
    //Check if we should do DBcleaning
    if (sumConflicts() > solver->getNextCleanLimit()) {
        if (conf.verbosity >= 3) {
            cout
            << "c "
            << " cleaning"
            << " getNextCleanLimit(): " << solver->getNextCleanLimit()
            << " numConflicts : " << stats.conflStats.numConflicts
            << " SumConfl: " << sumConflicts()
            << " maxConfls:" << max_conflicts
            << " Trail size: " << trail.size() << endl;
        }
        solver->reduceDB->reduce_db_and_update_reset_stats();

        genRandomVarActMultDiv();


        //watch consolidate
        if (conf.verbosity >= 2)
            watches.print_stat();

        watches.consolidate();
    }
}

void Searcher::clean_clauses_if_needed()
{
    const size_t newZeroDepthAss = trail.size() - lastCleanZeroDepthAssigns;
    if (newZeroDepthAss > ((double)solver->getNumFreeVars()*0.005))  {
        if (conf.verbosity >= 2) {
            cout << "c newZeroDepthAss : " << newZeroDepthAss  << endl;
        }

        lastCleanZeroDepthAssigns = trail.size();
        solver->clauseCleaner->removeAndCleanAll();
    }
}

lbool Searcher::perform_scc_and_varreplace_if_needed()
{
    if (conf.doFindAndReplaceEqLits
            && (solver->binTri.numNewBinsSinceSCC
                > ((double)solver->getNumFreeVars()*conf.sccFindPercent))
    ) {
        if (conf.verbosity >= 1) {
            cout
            << "c new bins since last SCC: "
            << std::setw(2)
            << solver->binTri.numNewBinsSinceSCC
            << " free vars %:"
            << std::fixed << std::setprecision(2) << std::setw(4)
            << stats_line_percent(solver->binTri.numNewBinsSinceSCC, solver->getNumFreeVars())
            << endl;
        }

        solver->clauseCleaner->removeAndCleanAll();

        //Find eq lits
        if (!solver->sCCFinder->performSCC()) {
            return l_False;
        }
        lastCleanZeroDepthAssigns = trail.size();

        //If enough new variables have been found to be replaced, replace them
        if (solver->varReplacer->getNewToReplaceVars() > ((double)solver->getNumFreeVars()*0.001)) {
            //Perform equivalent variable replacement
            if (!solver->varReplacer->performReplace()) {
                return l_False;
            }
        }
    }

    return l_Undef;
}

void Searcher::save_search_loop_stats()
{
    #ifdef STATS_NEEDED
    if (conf.doSQL) {
        printRestartSQL();
    }

    #ifdef STATS_NEEDED_EXTRA
    //Update varDataLT
    if (conf.dump_tree_variance_stats) {
        for(size_t i = 0; i < nVars(); i++) {
            varDataLT[i].stats.addData(varData[i].stats);
            varData[i].stats.reset();
        }
    }
    #endif
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

    if (stats.conflStats.numConflicts >= max_conflicts) {
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

lbool Searcher::solve(const uint64_t _maxConfls)
{
    assert(ok);
    assert(qhead == trail.size());
    max_conflicts = _maxConfls;
    num_search_called++;
    do_otf_this_round = nVars() < 500ULL*1000ULL && binTri.irredBins + binTri.redBins < 1500ULL*1000ULL && conf.otfHyperbin;

    if (solver->conf.verbosity >= 6) {
        cout
        << "c Searcher::solve() called"
        << endl;
    }

    resetStats();
    restore_activities_and_polarities();
    restore_order_heap();
    setup_restart_print();
    lbool status = l_Undef;
    status = burstSearch();
    if (status != l_Undef) {
        goto end;
    }

    restore_activities_and_polarities();
    restore_order_heap();
    params.rest_type = decide_restart_type();
    if ((num_search_called == 1 && conf.do_calc_polarity_first_time)
        || conf.do_calc_polarity_every_time
    ) {
        calculate_and_set_polars();
    }
    genRandomVarActMultDiv();
    setup_restart_print();
    max_conflicts_geometric = conf.restart_first;
    for(loop_num = 0
        ; !must_interrupt_asap()
          && stats.conflStats.numConflicts < max_conflicts
          && !solver->must_interrupt_asap()
        ; loop_num ++
    ) {
        print_search_loop_num();

        lastRestartConfl = sumConflicts();
        params.clear();
        params.conflictsToDo = max_conflicts-stats.conflStats.numConflicts;
        status = search();
        if (params.rest_type == restart_type_geom) {
            max_conflicts_geometric *= conf.restart_inc;
        }

        if (must_abort(status)) {
            goto end;
        }

        reduce_db_if_needed();
        clean_clauses_if_needed();
        status = perform_scc_and_varreplace_if_needed();
        if (status != l_Undef)
            goto end;

        save_search_loop_stats();
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
            && (varData[var].removed == Removed::replaced
                || varData[var].removed == Removed::queued_replacer)
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
        << " solver->getNextCleanLimit(): " << solver->getNextCleanLimit()
        << " numConflicts : " << stats.conflStats.numConflicts
        << " SumConfl: " << sumConflicts()
        << " maxConfls:" << max_conflicts
        << endl;
    }

    #ifdef STATS_NEEDED
    if (conf.doSQL) {
        printRestartSQL();
        //printVarStatsSQL();

        #ifdef STATS_NEEDED_EXTRA
        if (conf.dumpClauseDistribPer != 0) {
            printClauseDistribSQL();
            std::fill(clauseSizeDistrib.begin(), clauseSizeDistrib.end(), 0);
        }
        #endif
    }
    #endif

    print_iteration_solving_stats();
    backup_activities_and_polarities();
}

void Searcher::print_iteration_solving_stats()
{
    if (conf.verbosity >= 3) {
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
        stats.print();
        propStats.print(stats.cpu_time);
        printStatsLine("c props/decision"
            , (double)propStats.propagations/(double)stats.decisions
        );
        printStatsLine("c props/conflict"
            , (double)propStats.propagations/(double)stats.conflStats.numConflicts
        );
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
    }
}

inline int64_t abs64(int64_t a)
{
    if (a < 0) return -a;
    return a;
}

bool Searcher::pickPolarity(const Var var)
{
    switch(conf.polarity_mode) {
        case polarmode_neg:
            return false;

        case polarmode_pos:
            return true;

        case polarmode_rnd:
            return mtrand.randInt(1);

        case polarmode_automatic:
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
    double rand = mtrand.randDblExc();
    double frq = conf.random_var_freq;
    if (rand < frq && order_heap.size() > 0) {
        const Var next_var = order_heap[mtrand.randInt(order_heap.size()-1)];
        if (value(next_var) == l_Undef
            && solver->varData[next_var].is_decision
        ) {
            stats.decisionsRand++;
            next = Lit(next_var, !pickPolarity(next_var));
        }
    }

    // Activity based decision:
    while (next == lit_Undef
      || value(next.var()) != l_Undef
      || !solver->varData[next.var()].is_decision
    ) {
        //There is no more to branch on. Satisfying assignment found.
        if (order_heap.empty()) {
            next = lit_Undef;
            break;
        }

        const Var next_var = order_heap.removeMin();
        next = Lit(next_var, !pickPolarity(next_var));
    }

    //Flip polaritiy if need be
    if (next != lit_Undef
        && mtrand.randInt(conf.polarity_flip_frequency_multiplier) == 1
    ) {
        next ^= true;
        stats.decisionFlippedPolar++;
    }

    //Try to update to dominator
    if (next != lit_Undef
        && (mtrand.randInt(conf.dominPickFreq) == 1)
    ) {
        Lit lit2 = lit_Undef;
        /*if (conf.doStamp) {
            //Use timestamps
            lit2 = stamp.tstamp[next.toInt()].dominator[STAMP_RED];
        } else*/ if (conf.doCache) {
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

    //No vars in heap: solution found
    #ifdef VERBOSE_DEBUG
    if (next == lit_Undef) {
        cout << "SAT!" << endl;
    } else {
        cout << "decided on: " << next << endl;
    }
    #endif

    if (next != lit_Undef) {
        assert(solver->varData[next.var()].is_decision);
        assert(solver->varData[next.var()].removed == Removed::none
            || solver->varData[next.var()].removed == Removed::queued_replacer
        );
    }
    return next;
}

void Searcher::cache_based_more_minim(vector<Lit>& cl)
{
    int64_t limit = more_red_minim_limit_cache_actual;
    for (const Lit lit: cl) {
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
    for (const Lit lit: cl) {
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

    //Set all literals' seen[lit] = 1 in learnt clause
    //We will 'clean' the learnt clause by setting these to 0
    for (vector<Lit>::const_iterator
        it = cl.begin(), end = cl.end()
        ; it != end
        ; it++
    ) {
        seen[it->toInt()] = 1;
    }

    if (conf.doCache) {
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
        if (seen[i->toInt()])
            *j++ = *i;
        else
            changedClause = true;

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
        for(int i = ((int)cl.size())-1; i >= 1; i--) {
            cl[i] = cl[i-1];
        }
        cl[0] = firstLit;
    }

    stats.stampShrinkCl += ((origSize - cl.size()) > 0);
    stats.stampShrinkLit += origSize - cl.size();
}

void Searcher::insertVarOrder(const Var x)
{
    if (!order_heap.inHeap(x)
        && solver->varData[x].is_decision
    ) {
        order_heap.insert(x);
    }
}

bool Searcher::VarFilter::operator()(uint32_t var) const
{
    return (cc->value(var) == l_Undef && solver->varData[var].is_decision);
}

void Searcher::printAgilityStats()
{
    cout
    << " -- "
    << " confl:" << std::setw(6) << params.conflictsDoneThisRestart
    << ", rest:" << std::setw(3) << stats.numRestarts
    << ", ag:" << std::setw(4) << std::fixed << std::setprecision(2)
    << agility.getAgility()

    << ", agLim:" << std::setw(4) << std::fixed << std::setprecision(2)
    << conf.agilityLimit

    << ", agHist:" << std::setw(4) << std::fixed << std::setprecision(3)
    << hist.agilityHist.avg()

    /*<< ", agilityHistLong: " << std::setw(6) << std::fixed << std::setprecision(3)
    << agilityHist.avgLong()*/
    ;
}

uint64_t Searcher::sumConflicts() const
{
    return solver->sumStats.conflStats.numConflicts + stats.conflStats.numConflicts;
}

uint64_t Searcher::sumRestarts() const
{
    return stats.numRestarts + solver->getStats().numRestarts;
}

size_t Searcher::hyperBinResAll()
{
    size_t added = 0;

    for(std::set<BinaryClause>::const_iterator
        it = solver->needToAddBinClause.begin()
        , end = solver->needToAddBinClause.end()
        ; it != end
        ; it++
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
        if (val1 == l_True || val2 == l_True) {
            continue;
        }

        assert(val1 == l_Undef && val2 == l_Undef);
        solver->attachBinClause(it->getLit1(), it->getLit2(), true, false);
        added++;
    }
    solver->needToAddBinClause.clear();

    return added;
}

std::pair<size_t, size_t> Searcher::removeUselessBins()
{
    size_t removedIrred = 0;
    size_t removedRed = 0;

    if (conf.doTransRed) {
        for(std::set<BinaryClause>::iterator
            it = uselessBin.begin()
            , end = uselessBin.end()
            ; it != end
            ; it++
        ) {
            propStats.otfHyperTime += 2;
            //cout << "Removing binary clause: " << *it << endl;
            propStats.otfHyperTime += solver->watches[it->getLit1().toInt()].size()/2;
            propStats.otfHyperTime += solver->watches[it->getLit2().toInt()].size()/2;
            removeWBin(solver->watches, it->getLit1(), it->getLit2(), it->isRed());
            removeWBin(solver->watches, it->getLit2(), it->getLit1(), it->isRed());

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

    PropByForGraph confl(conflHalf, failBinLit, clAllocator);
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
        confl = PropByForGraph(varData[p.var()].reason, p, clAllocator);
        seen[p.var()] = 0; // this one is resolved
        pathC--;
    } while (pathC > 0); //UIP when eveything goes through this one
    assert(pathC == 0);
    learnt_clause[0] = ~p;

    // clear out seen
    for (uint32_t j = 0; j != learnt_clause.size(); j++)
        seen[learnt_clause[j].var()] = 0;    // ('seen[]' is now cleared)

    //Calculate glue
    glue = calcGlue(learnt_clause);

    return resolutions_str.str();
}

void Searcher::print_edges_for_graphviz_file(std::ofstream& file) const
{
    for (size_t i = 0; i < trail.size(); i++) {
        const Lit lit = trail[i];

        //0-decision level means it's pretty useless to put into the impl. graph
        if (varData[lit.var()].level == 0) continue;

        //Not directly connected with the conflict, drop
        if (!seen[lit.var()]) continue;

        PropBy reason = varData[lit.var()].reason;

        //A decision variable, it is not propagated by any clause
        if (reason.isNULL()) continue;

        PropByForGraph prop(reason, lit, clAllocator);
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

            PropByForGraph prop(reason, lits[i], clAllocator);
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
    PropByForGraph confl(conflPart, failBinLit, clAllocator);
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

void Searcher::decayClauseAct()
{
    clauseActivityIncrease *= conf.clauseDecayActivity;
}

void Searcher::bumpClauseAct(Clause* cl)
{
    cl->stats.activity += clauseActivityIncrease;
    if (cl->stats.activity > 1e20 ) {
        // Rescale
        for(vector<ClOffset>::iterator
            it = solver->longRedCls.begin(), end = solver->longRedCls.end()
            ; it != end
            ; it++
        ) {
            clAllocator.getPointer(*it)->stats.activity *= 1e-20;
        }
        clauseActivityIncrease *= 1e-20;
        clauseActivityIncrease = std::max(clauseActivityIncrease, 1.0);
    }
}

PropBy Searcher::propagate(
    #ifdef STATS_NEEDED
    AvgCalc<size_t>* watchListSizeTraversed
    #endif
) {
    const size_t origTrailSize = trail.size();

    PropBy ret;
    if (conf.propBinFirst) {
        ret = propagateBinFirst(
            #ifdef STATS_NEEDED
            watchListSizeTraversed
            #endif
        );
    } else {
        ret = propagateAnyOrder();
    }

    //Drup -- If declevel 0 propagation, we have to add the unitaries
    if (drup->enabled() && decisionLevel() == 0) {
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

size_t Searcher::memUsed() const
{
    size_t mem = HyperEngine::memUsed();
    mem += act_polar_backup.memUsed();
    mem += otf_subsuming_short_cls.capacity()*sizeof(OTFClause);
    mem += otf_subsuming_long_cls.capacity()*sizeof(ClOffset);
    mem += activities.capacity()*sizeof(uint32_t);
    mem += order_heap.memUsed();
    mem += learnt_clause.capacity()*sizeof(Lit);
    mem += hist.memUsed();
    mem += conflict.capacity()*sizeof(Lit);
    mem += model.capacity()*sizeof(lbool);

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
        << order_heap.memUsed()
        << endl;

        cout
        << "c learnt clause bytes: "
        << learnt_clause.capacity()*sizeof(Lit)
        << endl;

        cout
        << "c hist bytes: "
        << hist.memUsed()
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

void Searcher::backup_activities_and_polarities()
{
    act_polar_backup.activity.clear();
    act_polar_backup.activity.resize(nVars(), 0);
    act_polar_backup.polarity.clear();
    act_polar_backup.polarity.resize(nVars(), false);
    for (size_t i = 0; i < nVars(); i++) {
        act_polar_backup.polarity[i] = varData[i].polarity;
        act_polar_backup.activity[i] = activities[i];
    }
    act_polar_backup.var_inc = var_inc;
    act_polar_backup.saved = true;
}

void Searcher::restore_activities_and_polarities()
{
    if (!act_polar_backup.saved)
        return;

    for(size_t i = 0; i < nVars(); i++) {
        varData[i].polarity = act_polar_backup.polarity[i];
        activities[i] = act_polar_backup.activity[i];
    }
    var_inc = act_polar_backup.var_inc;
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

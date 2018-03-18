/******************************************
Copyright (c) 2016, Mate Soos

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
#include "solverconf.h"
#include "distillerlong.h"
//#define DEBUG_RESOLV

using namespace CMSat;
using std::cout;
using std::endl;

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
        , cla_inc(1)
{
    var_decay = conf.var_decay_start;
    step_size = solver->conf.orig_step_size;

    var_inc = conf.var_inc_start;
    mtrand.seed(conf.origSeed);
    hist.setSize(conf.shortTermHistorySize, conf.blocking_restart_trail_hist_length);
    cur_max_temp_red_lev2_cls = conf.max_temp_lev2_learnt_clauses;
}

Searcher::~Searcher()
{
    clear_gauss();
}

void Searcher::new_var(const bool bva, const uint32_t orig_outer)
{
    PropEngine::new_var(bva, orig_outer);

    var_act_vsids.push_back(0);
    var_act_maple.push_back(0);
    insert_var_order_all((int)nVars()-1);
}

void Searcher::new_vars(size_t n)
{
    PropEngine::new_vars(n);

    var_act_vsids.insert(var_act_vsids.end(), n, 0);
    var_act_maple.insert(var_act_maple.end(), n, 0);
    for(int i = n-1; i >= 0; i--) {
        insert_var_order_all((int)nVars()-i-1);
    }
}

void Searcher::save_on_var_memory()
{
    PropEngine::save_on_var_memory();

    var_act_vsids.resize(nVars());
    var_act_maple.resize(nVars());

    var_act_vsids.shrink_to_fit();
    var_act_maple.shrink_to_fit();

}

void Searcher::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    updateArray(var_act_vsids, interToOuter);
    updateArray(var_act_maple, interToOuter);

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
    #ifdef STATS_NEEDED
    antec_data.vsids_all_incoming_vars.push(var_act_vsids[lit.var()]/var_inc);
    #endif
    const uint32_t var = lit.var();
    assert(varData[var].removed == Removed::none);

    //If var is at level 0, don't do anything with it, just skip
    if (seen[var] || varData[var].level == 0) {
        return;
    }
    seen[var] = 1;

    if (!update_bogoprops) {
        if (VSIDS) {
            bump_vsids_var_act<update_bogoprops>(var, 0.5);
            implied_by_learnts.push_back(var);
        } else {
            varData[var].conflicted++;
        }

        if (conf.doOTFSubsume) {
            tmp_learnt_clause_size++;
            seen2[lit.toInt()] = 1;
            tmp_learnt_clause_abst |= abst_var(lit.var());
        }
    }

    if (varData[var].level >= decisionLevel()) {
        pathC++;
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
#ifdef STATS_NEEDED
    cl.stats.ID = clauseID;
    clauseID++;
#endif
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

    if (num_lits_from_cl <= 2) {
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

    if (new_glue < cl->stats.glue) {
        if (cl->stats.glue <= conf.protect_cl_if_improved_glue_below_this_glue_for_one_turn) {
            cl->stats.ttl = 1;
        }
        cl->stats.glue = new_glue;

        //move to lev0 if very low glue
        if (new_glue <= conf.glue_put_lev0_if_below_or_eq
            && cl->stats.which_red_array >= 1
        ) {
            cl->stats.which_red_array = 0;
        }

        //move to lev1 if low glue
        if (new_glue <= conf.glue_put_lev1_if_below_or_eq
            && solver->conf.glue_put_lev1_if_below_or_eq != 0
            && cl->stats.which_red_array == 2
        ) {
            cl->stats.which_red_array = 1;
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
        case binary_t : {
            if (confl.isRedStep()) {
                #ifdef STATS_NEEDED
                antec_data.binRed++;
                #endif
                stats.resolvs.binRed++;
            } else {
                #ifdef STATS_NEEDED
                antec_data.binIrred++;
                #endif
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
                antec_data.age_long_reds.push(sumConflicts - cl->stats.introduced_at_conflict);
                antec_data.glue_long_reds.push(cl->stats.glue);
                #endif
            } else {
                #ifdef STATS_NEEDED
                antec_data.longIrred++;
                #endif
                stats.resolvs.longRed++;
            }
            #ifdef STATS_NEEDED
            antec_data.size_longs.push(cl->size());
            cl->stats.used_for_uip_creation++;
            #endif

            if (!update_bogoprops
                && cl->red()
                && cl->stats.which_red_array != 0
            ) {
                if (conf.update_glues_on_analyze) {
                    update_clause_glue_from_analysis(cl);
                }

                if (cl->stats.which_red_array == 1) {
                    cl->stats.last_touched = sumConflicts;
                } else if (cl->stats.which_red_array == 2) {
                    bumpClauseAct(cl);
                }
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
        stats.permDiff_attempt++;
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
        stats.permDiff_success++;
        stats.permDiff_rem_lits+=nb;
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
            #ifdef STATS_NEEDED
            antec_data.vsids_of_resolving_literals.push(var_act_vsids[p.var()]/var_inc);
            #endif
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

void Searcher::simple_create_learnt_clause(
    PropBy confl,
    vector<Lit>& out_learnt,
    bool True_confl
) {
    int until = -1;
    int mypathC = 0;
    Lit p = lit_Undef;
    int index = trail.size() - 1;
    assert(decisionLevel() == 1);

    do {
        if (!confl.isNULL()) {
            if (confl.getType() == binary_t) {
                if (p == lit_Undef && True_confl == false) {
                    Lit q = failBinLit;
                    if (!seen[q.var()]) {
                        seen[q.var()] = 1;
                        mypathC++;
                    }
                }
                Lit q = confl.lit2();
                if (!seen[q.var()]) {
                    seen[q.var()] = 1;
                    mypathC++;
                }
            } else {
                const Clause& c = *solver->cl_alloc.ptr(confl.get_offset());

                // if True_confl==true, then choose p begin with the 1st index of c
                for (uint32_t j = (p == lit_Undef && True_confl == false) ? 0 : 1
                    ; j < c.size()
                    ; j++
                ) {
                    Lit q = c[j];
                    assert(q.var() < seen.size());
                    if (!seen[q.var()]) {
                        seen[q.var()] = 1;
                        mypathC++;
                    }
                }
            }
        } else {
            assert(confl.isNULL());
            out_learnt.push_back(~p);
        }
        // if not break, while() will come to the index of trail blow 0, and fatal error occur;
        if (mypathC == 0) {
            break;
        }
        // Select next clause to look at:
        while (!seen[trail[index--].var()]);
        // if the reason cr from the 0-level assigned var, we must break avoid move forth further;
        // but attention that maybe seen[x]=1 and never be clear. However makes no matter;
        if ((int)trail_lim[0] > index + 1
            && until == -1
        ) {
            until = out_learnt.size();
        }
        p = trail[index + 1];
        confl = varData[p.var()].reason;

        //under normal circumstances this does not happen, but here, it can
        //reason is undefined for level 0
        if (varData[p.var()].level == 0) {
            confl = PropBy();
        }
        seen[p.var()] = 0;
        mypathC--;
    } while (mypathC >= 0);

    if (until != -1)
        out_learnt.resize(until);
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
        || learnt_clause.size() <= 2
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
    #ifdef STATS_NEEDED
    antec_data.clear();
    #endif
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

    //further minimisation 1 -- short, small glue clauses
    glue = std::numeric_limits<uint32_t>::max();
    if (learnt_clause.size() <= conf.max_size_more_minim) {
        glue = calc_glue(learnt_clause);
        if (glue <= conf.max_glue_more_minim) {
            minimize_using_permdiff();
        }
    }
    if (glue == std::numeric_limits<uint32_t>::max()) {
        glue = calc_glue(learnt_clause);
    }
    print_fully_minimized_learnt_clause();

    out_btlevel = find_backtrack_level_of_learnt();
    if (!update_bogoprops) {
        if (VSIDS) {
            bump_var_activities_based_on_implied_by_learnts<update_bogoprops>(out_btlevel);
        } else {
            assert(toClear.empty());
            const Lit p = learnt_clause[0];
            seen[p.var()] = true;
            toClear.push_back(p);
            for (int i = learnt_clause.size() - 1; i >= 0; i--) {
                const uint32_t v = learnt_clause[i].var();
                if (varData[v].reason.isClause()) {
                    ClOffset offs = varData[v].reason.get_offset();
                    Clause* cl = cl_alloc.ptr(offs);
                    for (const Lit l: *cl) {
                        if (!seen[l.var()]) {
                            seen[l.var()] = true;
                            varData[l.var()].almost_conflicted++;
                            toClear.push_back(l);
                        }
                    }
                } else if (varData[v].reason.getType() == binary_t) {
                    Lit l = varData[v].reason.lit2();
                    if (!seen[l.var()]) {
                        seen[l.var()] = true;
                        varData[l.var()].almost_conflicted++;
                        toClear.push_back(l);
                    }
                    l = Lit(v, false);
                    if (!seen[l.var()]) {
                        seen[l.var()] = true;
                        varData[l.var()].almost_conflicted++;
                        toClear.push_back(l);
                    }
                }
            }
            for (Lit l: toClear) {
                seen[l.var()] = 0;
            }
            toClear.clear();
        }
    }

    #ifdef STATS_NEEDED
    for(const Lit l: learnt_clause) {
        antec_data.vsids_vars.push(var_act_vsids[l.var()]/var_inc);
    }
    #endif

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
        && sumConflicts > conf.lower_bound_for_blocking_restart
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
            //manipulate startup parameters
            if (!update_bogoprops) {
                if (VSIDS &&
                    ((stats.conflStats.numConflicts & 0xfff) == 0xfff) &&
                    var_decay < conf.var_decay_max
                ) {
                    var_decay += 0.01;
                }
                if (!VSIDS && step_size > solver->conf.min_step_size) {
                    step_size -= solver->conf.step_size_dec;
                }
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
            }
            if (!handle_conflict<update_bogoprops>(confl)) {
                dump_search_loop_stats(myTime);
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
                dump_search_loop_stats(myTime);
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

    cancelUntil<true, update_bogoprops>(0);
    assert(solver->prop_at_head());
    if (!solver->datasync->syncData()) {
        return l_False;
    }
    dump_search_loop_stats(myTime);

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
    int size = 1;
    int seq;
    for (seq = 0
        ; size < x + 1
        ; seq++
    ) {
        size = 2 * size + 1;
    }

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

    assert(params.rest_type != Restart::glue_geom);

    if (VSIDS
        && params.rest_type == Restart::glue
    ) {
        check_blocking_restart();
        if (hist.glueHist.isvalid()
            && conf.local_glue_multiplier * hist.glueHist.avg() > hist.glueHistLTLimited.avg()
        ) {
            params.needToStopSearch = true;
        }
    }
    if (   (!VSIDS
            || conf.restartType == Restart::glue_geom
            || params.rest_type == Restart::geom
            || params.rest_type == Restart::luby)
        && (int64_t)params.conflictsDoneThisRestart > max_confl_this_phase
    ) {
        params.needToStopSearch = true;
    }

    //Conflict limit reached?
    if (params.conflictsDoneThisRestart > params.max_confl_to_do) {
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
        assert(cl.size() > 2);

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
                assert(it->size == 2);
                by = PropBy(it->lits[1], true);
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
            }
        }
    }
    otf_subsuming_short_cls.clear();
}

void Searcher::update_history_stats(size_t backtrack_level, uint32_t glue)
{
    assert(decisionLevel() > 0);

    //short-term averages
    hist.branchDepthHist.push(decisionLevel());
    #ifdef STATS_NEEDED
    hist.branchDepthHistQueue.push(decisionLevel());
    hist.numResolutionsHist.push(antec_data.num());
    #endif
    hist.branchDepthDeltaHist.push(decisionLevel() - backtrack_level);
    hist.conflSizeHist.push(learnt_clause.size());
    hist.trailDepthDeltaHist.push(trail.size() - trail_lim[backtrack_level]);

    //long-term averages
    hist.decisionLevelHistLT.push(decisionLevel());
    hist.backtrackLevelHistLT.push(backtrack_level);

    #ifdef STATS_NEEDED
    hist.vsidsVarsAvgLT.push(antec_data.vsids_vars.avg());
    hist.numResolutionsHistLT.push(antec_data.num());
    #endif
    hist.conflSizeHistLT.push(learnt_clause.size());

    hist.trailDepthHistLT.push(trail.size());
    if (params.rest_type == Restart::glue
        && VSIDS
    ) {
        hist.glueHistLTLimited.push(std::min<size_t>(glue, 50));
    }
    hist.glueHistLTAll.push(glue);
    hist.glueHist.push(glue);
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
    , const uint32_t old_decision_level
) {
    vector<double> last_dec_var_act;
    for(int i = (int)decisionLevel()-1; i >= 0; i--) {
        uint32_t at = trail_lim[i];
        if (at < trail.size()) {
            uint32_t v = trail[at].var();
            double act_rel = var_act_vsids[v]/var_inc;
            last_dec_var_act.push_back(act_rel);
            if (last_dec_var_act.size() >= 2)
                break;
        }
    }

    while(last_dec_var_act.size() < 2)
        last_dec_var_act.push_back(0);

    vector<double> first_dec_var_act;
    for(size_t i = 0; i < decisionLevel(); i++) {
        uint32_t at = trail_lim[i];
        if (at < trail.size()) {
            uint32_t v = trail[at].var();
            double act_rel = var_act_vsids[v]/var_inc;
            first_dec_var_act.push_back(act_rel);
            if (first_dec_var_act.size() >= 2)
                break;
        }
    }

    while(first_dec_var_act.size() < 2)
        first_dec_var_act.push_back(0);

    solver->sqlStats->dump_clause_stats(
        solver
        , clauseID
        , glue
        , decisionLevel()
        , learnt_clause.size()
        , antec_data
        , old_decision_level
        , trail.size()
        , params.conflictsDoneThisRestart
        , restart_type_to_short_string(params.rest_type)
        , hist
        , last_dec_var_act[0]
        , last_dec_var_act[1]
        , first_dec_var_act[0]
        , first_dec_var_act[1]
    );
}
#endif

Clause* Searcher::handle_last_confl_otf_subsumption(
    Clause* cl
    , const uint32_t glue
    , const uint32_t old_decision_level
) {
    //Cannot make a non-implicit into an implicit
    if (learnt_clause.size() <= 2) {
        *drat << learnt_clause << fin;
        return NULL;
    }

    //No on-the-fly subsumption
    if (cl == NULL || cl->gauss_temp_cl() || !conf.doOTFSubsume) {
        cl = cl_alloc.Clause_new(learnt_clause
        , sumConflicts
        #ifdef STATS_NEEDED
        , clauseID
        #endif
        );
        cl->makeRed(glue);
        ClOffset offset = cl_alloc.get_offset(cl);
        unsigned which_arr = 2;

        //double glue_rel = ((double)cl->stats.glue) / hist.glueHistLTAll.avg();
        if (glue <= conf.glue_put_lev0_if_below_or_eq) {
            which_arr = 0;
        } else if (
            glue <= conf.glue_put_lev1_if_below_or_eq
            && conf.glue_put_lev1_if_below_or_eq != 0
        ) {
            which_arr = 1;
        } else {
            which_arr = 2;
        }

        if (which_arr == 0) {
            stats.red_cl_in_which0++;
        }

        /*if (conf.guess_cl_effectiveness) {
            unsigned lower_it = guess_clause_array(cl->stats, decisionLevel());
            if (lower_it) {
                stats.guess_different++;
                cl->stats.ttl = 1;
            }
        }*/

        cl->stats.which_red_array = which_arr;
        solver->longRedCls[cl->stats.which_red_array].push_back(offset);
        *drat << *cl << fin;

        #ifdef STATS_NEEDED
        if (solver->sqlStats
            && drat
            && conf.dump_individual_restarts_and_clauses
            && learnt_clause.size() > 2
        ) {
            dump_sql_clause_data(
                glue
                , old_decision_level
            );
        }
        #endif
        clauseID++;

        return cl;
    }

    assert(cl->size() > 2);
#ifdef VERBOSE_DEBUG
    cout << "Detaching OTF subsumed (LAST) clause:" << *cl << endl;
#endif
    *(solver->drat) << deldelay << *cl << fin;
    solver->detachClause(*cl, false);

    //Shrink clause
    assert(cl->size() > learnt_clause.size());
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
        cl->stats.ID = clauseID;
        clauseID++;
    #endif
    *(solver->drat) << *cl << fin << findelay;

    #ifdef STATS_NEEDED
    cl->stats.conflicts_made += conf.rewardShortenedClauseWithConfl;
    #endif

    return cl;
}

template<bool update_bogoprops>
bool Searcher::handle_conflict(const PropBy confl)
{
    stats.conflStats.numConflicts++;
    sumConflicts++;
    if (sumConflicts == 100000 && //TODO magic constant
        longRedCls[0].size() < 100 &&
        //so that in case of some "standard-minisat behavriour" config
        //we don't override it
        conf.glue_put_lev0_if_below_or_eq != 0
    ) {
        conf.glue_put_lev0_if_below_or_eq += 2; //TODO magic constant
    }

    /*if (sumConflicts > 50000) {
        DISTANCE = 0;
    }
    if (VSIDS && DISTANCE) {
        collectFirstUIP(confl);
    }*/

    params.conflictsDoneThisRestart++;
    if (conf.doPrintConflDot)
        create_graphviz_confl_graph(confl);

    if (decisionLevel() == 0)
        return false;

    uint32_t backtrack_level;
    uint32_t glue;
    Clause* subsumed_cl = analyze_conflict<update_bogoprops>(
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
    uint32_t old_decision_level = decisionLevel();
    cancelUntil<true, update_bogoprops>(backtrack_level);

    add_otf_subsume_long_clauses();
    add_otf_subsume_implicit_clause();
    print_learning_debug_info();
    assert(value(learnt_clause[0]) == l_Undef);
    glue = std::min<uint32_t>(glue, std::numeric_limits<uint32_t>::max());
    Clause* cl = handle_last_confl_otf_subsumption(subsumed_cl, glue, old_decision_level);
    assert(learnt_clause.size() <= 2 || cl != NULL);
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
        if (VSIDS) {
            varDecayActivity();
        }
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
    params.max_confl_to_do = conf.burst_search_len;
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

void Searcher::check_calc_features()
{
    if (last_feature_calc_confl == 0 || (last_feature_calc_confl + 100000) < sumConflicts) {
        last_feature_calc_confl = sumConflicts+1;
        if (nVars() > 2
            && longIrredCls.size() > 1
            && (binTri.irredBins + binTri.redBins) > 1
        ) {
            solver->last_solve_feature = solver->calculate_features();
        }
    }
 }

void Searcher::print_restart_header()
{
    //Print restart output header
    if ((lastRestartPrintHeader == 0 || (lastRestartPrintHeader + 1600000) < sumConflicts)
        && conf.verbosity
    ) {
        cout
        << "c"
        << " " << std::setw(6) << "type"
        << " " << std::setw(5) << "VSIDS"
        << " " << std::setw(5) << "rest"
        << " " << std::setw(5) << "conf"
        << " " << std::setw(5) << "freevar"
        << " " << std::setw(5) << "IrrL"
        << " " << std::setw(5) << "IrrB"
        << " " << std::setw(7) << "l/longC"
        << " " << std::setw(7) << "l/allC";

        for(size_t i = 0; i < longRedCls.size(); i++) {
            cout << " " << std::setw(4) << "RedL" << i;
        }

        cout
        << " " << std::setw(5) << "RedB"
        << " " << std::setw(7) << "l/longC"
        << " " << std::setw(7) << "l/allC"
        << endl;
        lastRestartPrintHeader = sumConflicts;
    }
}

void Searcher::print_restart_stat_line() const
{
    print_restart_stats_base();
    if (conf.print_full_restart_stat) {
        solver->print_clause_stats();
        hist.print();
    } else {
        solver->print_clause_stats();
    }

    cout << endl;
}

void Searcher::print_restart_stats_base() const
{
    cout << "c"
         << " " << std::setw(6) << restart_type_to_short_string(params.rest_type);
    cout << " " << std::setw(5) << (int)VSIDS;
    cout << " " << std::setw(5) << sumRestarts();

    if (sumConflicts >  20000) {
        cout << " " << std::setw(4) << sumConflicts/1000 << "K";
    } else {
        cout << " " << std::setw(5) << sumConflicts;
    }

    cout << " " << std::setw(7) << solver->get_num_free_vars();
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
        restart_type_to_short_string(params.rest_type)
        , thisPropStats
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
        && !conf.print_all_restarts
        && ((lastRestartPrint + conf.print_restart_line_every_n_confl)
          < sumConflicts)
    ) {
        print_restart_stat_line();
        lastRestartPrint = sumConflicts;
    }
}

void Searcher::reset_temp_cl_num()
{
    cur_max_temp_red_lev2_cls = conf.max_temp_lev2_learnt_clauses;
}

void Searcher::reduce_db_if_needed()
{
    if (conf.every_lev1_reduce != 0
        && sumConflicts >= next_lev1_reduce
    ) {
        solver->reduceDB->handle_lev1();
        next_lev1_reduce = sumConflicts + conf.every_lev1_reduce;
    }

    if (conf.every_lev2_reduce != 0) {
        if (sumConflicts >= next_lev2_reduce) {
            solver->reduceDB->handle_lev2();
            cl_alloc.consolidate(solver);
            next_lev2_reduce = sumConflicts + conf.every_lev2_reduce;
        }
    } else {
        if (longRedCls[2].size() > cur_max_temp_red_lev2_cls) {
            solver->reduceDB->handle_lev2();
            cur_max_temp_red_lev2_cls *= conf.inc_max_temp_lev2_red_cls;
            cl_alloc.consolidate(solver);
        }
    }
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
        && newZeroDepthAss > ((double)nVars()*0.05)
    ) {
        if (conf.verbosity >= 2) {
            cout << "c newZeroDepthAss : " << newZeroDepthAss
            << " -- "
            << (double)newZeroDepthAss/(double)nVars()*100.0
            << " % of active vars"
            << endl;
        }
        lastCleanZeroDepthAssigns = trail.size();
        solver->clauseCleaner->remove_and_clean_all();

        cl_alloc.consolidate(solver);
        rebuildOrderHeap(); //TODO only filter is needed!
        simpDB_props = (litStats.redLits + litStats.irredLits)<<5;
    }

    return true;
}

void Searcher::rebuildOrderHeap()
{
    vector<uint32_t> vs;
    for (uint32_t v = 0; v < nVars(); v++) {
        if (varData[v].removed == Removed::none
            && value(v) == l_Undef
        ) {
            vs.push_back(v);
        }
    }
    order_heap_vsids.build(vs);
    order_heap_maple.build(vs);
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

inline void Searcher::dump_search_loop_stats(double myTime)
{
    check_calc_features();
    print_restart_header();
    dump_search_sql(myTime);
    if (conf.verbosity && conf.print_all_restarts)
        print_restart_stat_line();
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
            << sumConflicts
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
    const uint64_t _max_confls
    , const unsigned upper_level_iteration_num
) {
    assert(ok);
    assert(qhead == trail.size());
    max_confl_per_search_solve_call = _max_confls;
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

    if (VSIDS) {
        if (conf.restartType == Restart::geom) {
            max_confl_phase = conf.restart_first;
            max_confl_this_phase = conf.restart_first;
            params.rest_type = Restart::geom;
        }

        if (conf.restartType == Restart::luby) {
            max_confl_this_phase = conf.restart_first;
            params.rest_type = Restart::luby;
        }
    } else {
        max_confl_this_phase = conf.restart_first;
        params.rest_type = Restart::luby;
    }

    assert(solver->check_order_heap_sanity());
    for(loop_num = 0
        ; stats.conflStats.numConflicts < max_confl_per_search_solve_call
        ; loop_num ++
    ) {
        #ifdef SLOW_DEBUG
        assert(solver->check_order_heap_sanity());
        #endif

        assert(watches.get_smudged_list().empty());
        print_search_loop_num();

        lastRestartConfl = sumConflicts;
        params.clear();
        params.max_confl_to_do = max_confl_per_search_solve_call-stats.conflStats.numConflicts;
        status = search<false>();
        if (status == l_Undef) {
            adjust_phases_restarts();
        }

        if (must_abort(status)) {
            goto end;
        }

        if (status == l_Undef &&
            solver->conf.do_distill_clauses &&
            sumConflicts > next_distill
        ) {
            if (!solver->distill_long_cls->distill(true)) {
                status = l_False;
                goto end;
            }
            next_distill = std::min<double>(sumConflicts * 0.2 + sumConflicts + 3000,
                                    sumConflicts + 50000);
        }
    }

    end:
    finish_up_solve(status);

    return status;
}

void Searcher::adjust_phases_restarts()
{
    //Haven't finished the phase. Keep rolling.
    if (max_confl_this_phase > 0)
        return;

    if (conf.maple) {
        VSIDS = false;
        params.rest_type = Restart::luby;
        max_confl_this_phase = luby(2, loop_num) * (double)conf.restart_first;
    } else {
        VSIDS = true;
        if (conf.verbosity >= 3) {
            cout << "c doing VSIDS" << endl;
        }
        switch(conf.restartType) {
        case Restart::never:
        case Restart::glue:
            //nothing special
            break;
        case Restart::geom:
            max_confl_phase *= conf.restart_inc;
            max_confl_this_phase = max_confl_phase;
            break;

        case Restart::luby:
            max_confl_this_phase = luby(conf.restart_inc*1.5, loop_num)
                                    * (double)conf.restart_first/2.0;
            break;

        case Restart::glue_geom:
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
                    max_confl_this_phase = conf.ratio_glue_geom *max_confl_phase;
                    break;

                default:
                    release_assert(false);
            }
            if (conf.verbosity >= 3) {
                cout << "Phase is now "
                << std::setw(10) << getNameOfRestartType(params.rest_type)
                << " this phase size: " << max_confl_this_phase
                << " global phase size: " << max_confl_phase << endl;
            }
            break;
        }
    }
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
        << " SumConfl: " << sumConflicts
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
    Heap<VarOrderLt> &order_heap = VSIDS ? order_heap_vsids : order_heap_maple;
    if (conf.random_var_freq > 0) {
        double rand = mtrand.randDblExc();
        double frq = conf.random_var_freq;
        if (rand < frq && !order_heap.empty()) {
            const uint32_t next_var = order_heap.random_element(mtrand);

            if (value(next_var) == l_Undef
                && solver->varData[next_var].removed == Removed::none
            ) {
                stats.decisionsRand++;
                next = Lit(next_var, !pickPolarity(next_var));
            }
        }
    }

    if (next == lit_Undef) {
        uint32_t v = var_Undef;
        while (v == var_Undef || value(v) != l_Undef) {
            //There is no more to branch on. Satisfying assignment found.
            if (order_heap.empty()) {
                return lit_Undef;
            }

            if (!VSIDS) {
                uint32_t v2 = order_heap_maple[0];
                uint32_t age = sumConflicts - varData[v2].cancelled;
                while (age > 0) {
                    double decay = pow(0.95, age);
                    var_act_maple[v2] *= decay;
                    if (order_heap_maple.inHeap(v2)) {
                        order_heap_maple.increase(v2);
                    }

                    varData[v2].cancelled = sumConflicts;
                    v2 = order_heap_maple[0];
                    age = sumConflicts - varData[v2].cancelled;
                }
            }
            v = order_heap.removeMin();
        }
        next = Lit(v, !pickPolarity(v));
    }

    //No vars in heap: solution found
    #ifdef SLOW_DEBUG
    if (next != lit_Undef) {
        assert(solver->varData[next.var()].removed == Removed::none);
    }
    #endif
    return next;
}

bool Searcher::VarFilter::operator()(uint32_t var) const
{
    return (cc->value(var) == l_Undef && solver->varData[var].removed == Removed::none);
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
    s << "confls/" << "confl" << solver->sumConflicts << ".dot";
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
    ret = propagate_any_order<update_bogoprops>();

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
    mem += var_act_vsids.capacity()*sizeof(uint32_t);
    mem += var_act_maple.capacity()*sizeof(uint32_t);
    mem += order_heap_vsids.mem_used();
    mem += order_heap_maple.mem_used();
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
        << "c order_heap_vsids bytes: "
        << order_heap_vsids.mem_used()
        << endl;

        cout
        << "c order_heap_maple bytes: "
        << order_heap_maple.mem_used()
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
        goto end;
    }

    //First check -- can't unset at the same time since the same
    //internal variable may be inside 'assumptions' -- in case the variables
    //have been replaced with each other.
    for(const AssumptionPair lit_pair: unfill_from) {
        const Lit lit = lit_pair.lit_inter;
        if (lit.var() < assumptionsSet.size()) {
            #ifdef SLOW_DEBUG
            if (!assumptionsSet[lit.var()]) {
                cout << "ERROR: var " << lit.var() + 1 << " is in assumptions but not in assumptionsSet" << endl;
            }
            #endif
            assert(assumptionsSet[lit.var()]);
        }
    }

    //Then unset
    for(const AssumptionPair lit_pair: unfill_from) {
        const Lit lit = lit_pair.lit_inter;
        if (lit.var() < assumptionsSet.size()) {
            assumptionsSet[lit.var()] = false;
        }
    }

    end:;
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
        assert(cl.size() > 2);
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
        for(size_t j = 0; j < sz; j++)
        {
            tmp_cl.push_back(f.get_lit());
        }
        ClauseStats cl_stats;
        if (red) {
            f.get_struct(cl_stats);
        }

        Clause* cl = cl_alloc.Clause_new(tmp_cl
        , cl_stats.last_touched
        #ifdef STATS_NEEDED
        , cl_stats.ID
        #endif
        );
        if (red) {
            cl->makeRed(cl_stats.glue, cla_inc);
        }
        cl->stats = cl_stats;
        attachClause(*cl);
        const ClOffset offs = cl_alloc.get_offset(cl);
        if (red) {
            cl->stats.which_red_array = 2;
            if (cl->stats.glue <= conf.glue_put_lev0_if_below_or_eq) {
                cl->stats.which_red_array = 0;
            } else if (cl->stats.glue <= conf.glue_put_lev1_if_below_or_eq
                && conf.glue_put_lev1_if_below_or_eq != 0
            ) {
                cl->stats.which_red_array = 1;
            }

            longRedCls[0].push_back(cl->stats.which_red_array);
            litStats.redLits += cl->size();
        } else {
            longIrredCls.push_back(offs);
            litStats.irredLits += cl->size();
        }
    }
}

unsigned Searcher::guess_clause_array(
    const ClauseStats& cl_stats
    , uint32_t backtrack_lev
) const {
    /*
    uint32_t votes = 0;
    //double trail_depth_rel = (double)trail.size()/hist.trailDepthHistLT.avg();
    double dec_lev_rel = (double)decisionLevel()/hist.decisionLevelHistLT.avg();
    if (dec_lev_rel < 0.10) {
        votes++;
    }

    double backtrack_lev_rel = (double)backtrack_lev/hist.decisionLevelHistLT.avg();
    if (backtrack_lev_rel < 0.10) {
        votes++;
    }

    if (antec_data.glue_long_reds.avg() > 12) {
        votes += 1;
    }

    if (votes > 2) {
        return true;
    }*/
    return false;
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

uint64_t Searcher::read_binary_cls(
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
    return num;
}

void Searcher::save_state(SimpleOutFile& f, const lbool status) const
{
    assert(decisionLevel() == 0);
    PropEngine::save_state(f);

    f.put_vector(var_act_vsids);
    f.put_vector(var_act_maple);
    f.put_vector(model);
    f.put_vector(full_model);
    f.put_vector(conflict);

    //Clauses
    if (status == l_Undef) {
        write_binary_cls(f, false);
        write_binary_cls(f, true);
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

    f.get_vector(var_act_vsids);
    f.get_vector(var_act_maple);
    for(size_t i = 0; i < nVars(); i++) {
        if (varData[i].removed == Removed::none
            && value(i) == l_Undef
        ) {
            insert_var_order_all(i);
        }
    }
    f.get_vector(model);
    f.get_vector(full_model);
    f.get_vector(conflict);

    //Clauses
    if (status == l_Undef) {
        binTri.irredBins = read_binary_cls(f, false);
        binTri.redBins =read_binary_cls(f, true);
        read_long_cls(f, false);
        for(size_t i = 0; i < longRedCls.size(); i++) {
            read_long_cls(f, true);
        }
    }
}


//Normal running
template
void Searcher::cancelUntil<true, false>(uint32_t level);

//During inprocessing, dont update anyting really (probing, distilling)
template
void Searcher::cancelUntil<false, true>(uint32_t level);

//During bursting, we need to insert var_order but not update other stats
template
void Searcher::cancelUntil<true, true>(uint32_t level);

template<bool do_insert_var_order, bool update_bogoprops>
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

             if (!update_bogoprops && !VSIDS) {
                assert(sumConflicts >= varData[var].picked);
                uint32_t age = sumConflicts - varData[var].picked;
                if (age > 0) {
                    double adjusted_reward = ((double)(varData[var].conflicted + varData[var].almost_conflicted)) / ((double)age);
                    double old_activity = var_act_maple[var];
                    var_act_maple[var] = step_size * adjusted_reward + ((1.0 - step_size) * old_activity);
                    if (order_heap_maple.inHeap(var)) {
                        if (var_act_maple[var] > old_activity)
                            order_heap_maple.decrease(var);
                        else
                            order_heap_maple.increase(var);
                    }
                }
                varData[var].cancelled = sumConflicts;
            }

            assigns[var] = l_Undef;
            if (do_insert_var_order) {
                insert_var_order(var);
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

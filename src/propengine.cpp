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

#include "propengine.h"
#include <cmath>
#include <string.h>
#include <algorithm>
#include <limits.h>
#include <vector>
#include <iomanip>
#include <algorithm>

#include "solver.h"
#include "clauseallocator.h"
#include "clause.h"
#include "time_mem.h"
#include "varupdatehelper.h"
#include "watchalgos.h"
#include "sqlstats.h"
#ifdef USE_GAUSS
#include "gaussian.h"
#endif

using namespace CMSat;
using std::cout;
using std::endl;

//#define DEBUG_ENQUEUE_LEVEL0
//#define VERBOSE_DEBUG_POLARITIES
//#define DEBUG_DYNAMIC_RESTART

/**
@brief Sets a sane default config and allocates handler classes
*/
PropEngine::PropEngine(
    const SolverConf* _conf
    , Solver* _solver
    , std::atomic<bool>* _must_interrupt_inter
) :
        CNF(_conf, _must_interrupt_inter)
        , order_heap_vsids(VarOrderLt(var_act_vsids))
        , order_heap_maple(VarOrderLt(var_act_maple))
        , qhead(0)
        , solver(_solver)
{
}

PropEngine::~PropEngine()
{
}

void PropEngine::new_var(
    const bool bva,
    uint32_t orig_outer,
    const bool insert_varorder)
{
    CNF::new_var(bva, orig_outer, insert_varorder);

    var_act_vsids.insert(var_act_vsids.end(), 1, ActAndOffset());
    var_act_maple.insert(var_act_maple.end(), 1, ActAndOffset());
    #ifdef VMTF_NEEDED
    vmtf_btab.insert(vmtf_btab.end(), 1, 0);
    vmtf_links.insert(vmtf_links.end(), 1, Link());
    #endif

    //TODO
    //trail... update x->whatever
}

void PropEngine::new_vars(size_t n)
{
    CNF::new_vars(n);

    var_act_vsids.insert(var_act_vsids.end(), n, ActAndOffset());
    var_act_maple.insert(var_act_maple.end(), n, ActAndOffset());
    #ifdef VMTF_NEEDED
    vmtf_btab.insert(vmtf_btab.end(), n, 0);
    vmtf_links.insert(vmtf_links.end(), n, Link());
    #endif

    //TODO
    //trail... update x->whatever
}

void PropEngine::save_on_var_memory()
{
    CNF::save_on_var_memory();

    var_act_vsids.resize(nVars());
    var_act_maple.resize(nVars());

    var_act_vsids.shrink_to_fit();
    var_act_maple.shrink_to_fit();

}

/**
 @ *brief Attach normal a clause to the watchlists

 Handles 2, 3 and >3 clause sizes differently and specially
 */

void PropEngine::attachClause(
    const Clause& c
    , const bool checkAttach
) {
    const ClOffset offset = cl_alloc.get_offset(&c);

    assert(c.size() > 2);
    if (checkAttach) {
        assert(value(c[0]) == l_Undef);
        assert(value(c[1]) == l_Undef || value(c[1]) == l_False);
    }

    #ifdef DEBUG_ATTACH
    for (uint32_t i = 0; i < c.size(); i++) {
        assert(varData[c[i].var()].removed == Removed::none);
    }
    #endif //DEBUG_ATTACH

    const Lit blocked_lit = c[2];
    watches[c[0]].push(Watched(offset, blocked_lit));
    watches[c[1]].push(Watched(offset, blocked_lit));
}

/**
@brief Detaches a (potentially) modified clause

The first two literals might have chaned through modification, so they are
passed along as arguments -- they are needed to find the correct place where
the clause is
*/
void PropEngine::detach_modified_clause(
    const Lit lit1
    , const Lit lit2
    , const Clause* address
) {
    ClOffset offset = cl_alloc.get_offset(address);
    removeWCl(watches[lit1], offset);
    removeWCl(watches[lit2], offset);
}



#ifdef USE_GAUSS
PropBy PropEngine::gauss_jordan_elim(const Lit p, const uint32_t currLevel)
{
    if (gmatrices.empty()) {
        return PropBy();
    }

    #ifdef VERBOSE_DEBUG
    cout << "Gauss searcher::Gauss_elimination called, declevel: " << decisionLevel() << endl;
    #endif

    for(uint32_t i = 0; i < gqueuedata.size(); i++) {
        if (gqueuedata[i].engaus_disable) {
            continue;
        }
        gqueuedata[i].reset();
        gmatrices[i]->update_cols_vals_set();
    }

    bool confl_in_gauss = false;

    assert(gwatches.size() > p.var());
    vec<GaussWatched>& ws = gwatches[p.var()];
    GaussWatched* i = ws.begin();
    GaussWatched* j = i;
    const GaussWatched* end = ws.end();
    #ifdef VERBOSE_DEBUG
    cout << "New GQHEAD: " << p << endl;
    #endif

    for (; i != end; i++) {
        if (gqueuedata[i->matrix_num].engaus_disable) {
            //remove watch and continue
            continue;
        }

        gqueuedata[i->matrix_num].new_resp_var = std::numeric_limits<uint32_t>::max();
        gqueuedata[i->matrix_num].new_resp_row = std::numeric_limits<uint32_t>::max();
        gqueuedata[i->matrix_num].do_eliminate = false;
        gqueuedata[i->matrix_num].currLevel = currLevel;

        if (gmatrices[i->matrix_num]->find_truths(
            i, j, p.var(), i->row_n, gqueuedata[i->matrix_num])
        ) {
            continue;
        } else {
            confl_in_gauss = true;
            i++;
            break;
        }
    }

    for (; i != end; i++) {
        *j++ = *i;
    }
    ws.shrink(i-j);

    for (size_t g = 0; g < gqueuedata.size(); g++) {
        if (gqueuedata[g].engaus_disable)
            continue;

        if (gqueuedata[g].do_eliminate) {
            gmatrices[g]->eliminate_col(p.var(), gqueuedata[g]);
            confl_in_gauss |= (gqueuedata[g].ret == gauss_res::confl);
        }
    }

    #ifdef SLOW_DEBUG
    if (!confl_in_gauss) {
        for (size_t g = 0; g < gqueuedata.size(); g++) {
            if (gqueuedata[g].engaus_disable)
                continue;

            assert(solver->gqhead == solver->trail.size());
            gmatrices[g]->check_invariants();
        }
    }
    #endif

    for (GaussQData& gqd: gqueuedata) {
        if (gqd.engaus_disable)
            continue;

        //There was a conflict but this is not that matrix.
        //Just skip.
        if (confl_in_gauss && gqd.ret != gauss_res::confl) {
            continue;
        }

        switch (gqd.ret) {
            case gauss_res::confl :{
                gqd.num_conflicts++;
                qhead = trail.size();
                return gqd.confl;
//                 bool ret = handle_conflict(gqd.confl);
//                 if (!ret) return gauss_ret::g_false;
//                 return gauss_ret::g_cont;
            }

            case gauss_res::prop:
                gqd.num_props++;
                break;

            case gauss_res::none:
                //nothing
                break;

            default:
                assert(false);
                return PropBy();
        }
    }
    return PropBy();
}
#endif //USE_GAUSS

lbool PropEngine::bnn_prop(const uint32_t bnn_idx, uint32_t level)
{
    BNN* bnn = bnns[bnn_idx];

    int32_t val = 0;
    int32_t undefs = 0;
    for(const auto& p: bnn->in) {
        assert(p.w >= 0);
        if (value(p.lit) == l_Undef) {
            undefs += p.w;
        }
        if (value(p.lit) == l_True) {
            val += p.w;
        }
    }

    // we are over the cutoff no matter what undefs is
    if (val > bnn->cutoff) {
        if (value(bnn->out) == l_False)
            return l_False;
        if (value(bnn->out) == l_True)
            return l_True;

        assert(value(bnn->out) == l_Undef);
        enqueue<false>(bnn->out, level, PropBy(bnn_idx, nullptr));
//         cout << "BNN prop set BNN out " << bnn->out << " due to being over for sure" << endl;
        return l_Undef;
    }

    // we are under the cutoff no matter what undefs is
    if (val <= bnn->cutoff && val+undefs <= bnn->cutoff) {
        if (value(bnn->out) == l_True)
            return l_False;
        if (value(bnn->out) == l_False)
            return l_True;

        assert(value(bnn->out) == l_Undef);
        enqueue<false>(~bnn->out, level, PropBy(bnn_idx, nullptr));
//         cout << "BNN prop set BNN out " << ~bnn->out << " due to being under for sure" << endl;
    }

    //Stronger propagation
    if (value(bnn->out) == l_True) {
        // check if anything MUST be set for TRUE output
        for(const auto& l: bnn->in) {
            if (value(l.lit) == l_Undef) {
                int32_t needed = bnn->cutoff-val+1;
                int32_t tot_remain = undefs - l.w;
                if (tot_remain < needed) {
                    enqueue<false>(l.lit, level, PropBy(bnn_idx, nullptr));
//                     cout << "BNN prop set " << l.lit << " due to positive slack" << endl;
                    undefs -= l.w;
                    val += l.w;
                }
            }
        }
    } else if (value(bnn->out) == l_False) {
        // check if anything MUST be unset for FALSE output
        for(const auto& l: bnn->in) {
            if (value(l.lit) == l_Undef) {
                int32_t toomuch = bnn->cutoff-val+1;
                if (l.w >= toomuch) {
                    enqueue<false>(~l.lit, level, PropBy(bnn_idx, nullptr));
//                     cout << "BNN prop set " << ~l.lit << " due to negative slack" << endl;
                    undefs -= l.w;
                    val += l.w;
                }
            }
        }
    }

    return l_Undef;
}

vector<Lit>* PropEngine::get_bnn_reason(BNN* bnn, Lit lit)
{
    if (lit == lit_Undef) {
        get_bnn_confl_reason(bnn, &bnn_confl_reason);
        return &bnn_confl_reason;
    }

    auto& reason = varData[lit.var()].reason;
    assert(reason.isBNN());
    if (reason.bnn_reason_set()) {
        return &bnn_reasons[reason.get_bnn_reason()];
    }

    vector<Lit>* ret;

    //Get an empty slot
    uint32_t empty_slot;
    if (bnn_reasons_empty_slots.empty()) {
        bnn_reasons.push_back(vector<Lit>());
        empty_slot = bnn_reasons.size()-1;
    } else {
        empty_slot = bnn_reasons_empty_slots.back();
        bnn_reasons_empty_slots.pop_back();
    }
    ret = &bnn_reasons[empty_slot];
    reason.set_bnn_reason(empty_slot);


    //lbool eval = bnn_eval(*bnn);
    get_bnn_prop_reason(bnn, lit, ret);
//     cout << "get_bnn_reason (" << lit << ") returning: ";
//     for(const auto& l: *ret) {
//         cout << l << " ";
//     }
//     cout << "0" << endl;

    return ret;
}

void PropEngine::get_bnn_confl_reason(BNN* bnn, vector<Lit>* ret)
{
    assert(value(bnn->out) != l_Undef);

    //It's set to TRUE, but it's actually LESS than cutoff
    if (value(bnn->out) == l_True) {
        ret->clear();
        ret->push_back(~bnn->out);

        //take all FALSE ones, they are causing to go under.
        for(const auto& l: bnn->in) {
            if (value(l.lit) == l_False) {
                ret->push_back(l.lit);
            }
        }
        return;
    }

    //it's set to FALSE but it's actually MORE than cutoff.
    if (value(bnn->out) == l_False) {
        ret->clear();
        ret->push_back(bnn->out);

        //take all TRUE ones, they are causing to go over.
        for(const auto& l: bnn->in) {
            if (value(l.lit) == l_True) {
                ret->push_back(l.lit);
            }
        }
        return;
    }
    assert(false);
}

void PropEngine::get_bnn_prop_reason(
    BNN* bnn, Lit lit, vector<Lit>* ret)
{
    assert(value(bnn->out) != l_Undef);

    //It's set to TRUE
    if (value(bnn->out) == l_True) {
        ret->clear();
        ret->push_back(lit); //this is what's propagated, must be 1st

        //take all TRUE ones at or below level, they caused it to meet cutoff
        for(const auto& l: bnn->in) {
            if (varData[l.lit.var()].sublevel <= varData[lit.var()].sublevel)
            {
                if (lit == bnn->out) {
                    //for the output, only take the TRUE ones
                    if (value(l.lit) == l_True) {
                        ret->push_back(~l.lit);
                    }
                } else {
                    //for intermediate, take all
                    if (l.lit.var() != lit.var()) {
                        ret->push_back(l.lit ^
                            (value(l.lit) == l_True));
                    }
                }
            }
        }

        //Make sure bnn->out is inside
        if (lit != bnn->out) {
            ret->push_back(~bnn->out);
        }
        return;
    }

    //it's set to FALSE
    if (value(bnn->out) == l_False) {
        ret->clear();
        ret->push_back(lit); //this is what's propagated, must be 1st

        //take all FALSE ones at or below level, they caused it to go below cutoff
        for(const auto& l: bnn->in) {
            if (varData[l.lit.var()].sublevel <= varData[lit.var()].sublevel)
            {
                if (lit == ~bnn->out) {
                    //for the output, only take the FALSE ones
                    if (value(l.lit) == l_False) {
                        ret->push_back(l.lit);
                    }
                } else {
                    //for intermediate, take all
                    if (l.lit.var() != lit.var()) {
                        ret->push_back(l.lit ^
                            (value(l.lit) == l_True));
                    }
                }
            }
        }

        //Make sure bnn->out is inside
        if (lit != ~bnn->out) {
            ret->push_back(bnn->out);
        }

        return;
    }
    assert(false);
}


/**
@brief Propagates a binary clause

Need to be somewhat tricky if the clause indicates that current assignment
is incorrect (i.e. both literals evaluate to FALSE). If conflict if found,
sets failBinLit
*/
template<bool update_bogoprops>
inline bool PropEngine::prop_bin_cl(
    const Watched* i
    , const Lit p
    , PropBy& confl
    , uint32_t currLevel
) {
    const lbool val = value(i->lit2());
    if (val == l_Undef) {
        #ifdef STATS_NEEDED
        if (i->red())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;
        #endif

        enqueue<update_bogoprops>(i->lit2(), currLevel, PropBy(~p, i->red()));
    } else if (val == l_False) {
        #ifdef STATS_NEEDED
        if (i->red())
            lastConflictCausedBy = ConflCausedBy::binred;
        else
            lastConflictCausedBy = ConflCausedBy::binirred;
        #endif

        confl = PropBy(~p, i->red());
        failBinLit = i->lit2();
        qhead = trail.size();
        return false;
    }

    return true;
}

template<bool update_bogoprops, bool red_also, bool use_disable>
bool PropEngine::prop_long_cl_any_order(
    Watched* i
    , Watched*& j
    , const Lit p
    , PropBy& confl
    , uint32_t currLevel
) {
    //Blocked literal is satisfied, so clause is satisfied
    if (value(i->getBlockedLit()) == l_True) {
        *j++ = *i;
        return true;
    }
    if (update_bogoprops) {
        propStats.bogoProps += 4;
    }
    const ClOffset offset = i->get_offset();
    Clause& c = *cl_alloc.ptr(offset);

    #ifdef SLOW_DEBUG
    assert(!c.getRemoved());
    assert(!c.freed());
    if (!use_disable) {
        assert(!c.disabled);
    }
    #endif

    if (!red_also && c.red()) {
        *j++ = *i;
        return true;
    }

    if (use_disable && c.disabled) {
        *j++ = *i;
        return true;
    }

    if (prop_normal_helper<update_bogoprops>(c, offset, j, p) == PROP_NOTHING) {
        return true;
    }

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        handle_normal_prop_fail<update_bogoprops>(c, offset, confl);
        return false;
    } else {
        if (!update_bogoprops) {
            #if defined(NORMAL_CL_USE_STATS)
            c.stats.props_made++;
            #endif
            #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
            c.stats.props_made++;
            #endif
            #ifdef STATS_NEEDED
            if (c.red())
                propStats.propsLongRed++;
            else
                propStats.propsLongIrred++;
            #endif
        }

        if (currLevel == decisionLevel()) {
            enqueue<update_bogoprops>(c[0], currLevel, PropBy(offset));
        } else {
            uint32_t nMaxLevel = currLevel;
            uint32_t nMaxInd = 1;
            // pass over all the literals in the clause and find the one with the biggest level
            for (uint32_t nInd = 2; nInd < c.size(); ++nInd) {
                uint32_t nLevel = varData[c[nInd].var()].level;
                if (nLevel > nMaxLevel) {
                    nMaxLevel = nLevel;
                    nMaxInd = nInd;
                }
            }

            if (nMaxInd != 1) {
                std::swap(c[1], c[nMaxInd]);
                j--; // undo last watch
                watches[c[1]].push(*i);
            }

            enqueue<update_bogoprops>(c[0], nMaxLevel, PropBy(offset));
        }
    }

    return true;
}

PropBy PropEngine::propagate_any_order_fast()
{
    PropBy confl;

    #ifdef VERBOSE_DEBUG_PROP
    cout << "Fast Propagation started" << endl;
    #endif

    //so we don't re-calculate it all the time
    uint32_t declevel = decisionLevel();

    const bool fast_confl_break = solver->conf.fast_confl_break;

    int64_t num_props = 0;
    while (qhead < trail.size() && (!fast_confl_break || confl.isNULL())) {
        const Lit p = trail[qhead].lit;     // 'p' is enqueued fact to propagate.
        const uint32_t currLevel = trail[qhead].lev;
        qhead++;
        watch_subarray ws = watches[~p];
        Watched* i;
        Watched* j;
        Watched* end;
        num_props++;

        for (i = j = ws.begin(), end = ws.end(); unlikely(i != end);) {
            //Prop bin clause
            if (i->isBin()) {
                assert(j < end);
                *j++ = *i;
                const lbool val = value(i->lit2());
                if (val == l_Undef) {
                    #ifdef STATS_NEEDED
                    if (i->red())
                        propStats.propsBinRed++;
                    else
                        propStats.propsBinIrred++;
                    #endif
                    enqueue<false>(i->lit2(), currLevel, PropBy(~p, i->red()));
                    i++;
                } else if (val == l_False) {
                    confl = PropBy(~p, i->red());
                    failBinLit = i->lit2();
                    #ifdef STATS_NEEDED
                    if (i->red())
                        lastConflictCausedBy = ConflCausedBy::binred;
                    else
                        lastConflictCausedBy = ConflCausedBy::binirred;
                    #endif
                    i++;
                    while (i < end) {
                        *j++ = *i++;
                    }
                    qhead = trail.size();
                } else {
                    i++;
                }
                continue;
            }

            // propagate BNN constraint
            if (i->isBNN()) {
                *j++ = *i;
                const lbool val = bnn_prop(i->get_bnn(), currLevel);
                if (val == l_False) {
                    confl = PropBy(i->get_bnn(), nullptr);
                    i++;
                    while (i < end) {
                        *j++ = *i++;
                    }
                    qhead = trail.size();
                } else {
                    i++;
                }
                continue;
            }

            //propagate normal clause
            assert(i->isClause());
            Lit blocked = i->getBlockedLit();
            if (likely(value(blocked) == l_True)) {
                *j++ = *i++;
                continue;
            }

            const ClOffset offset = i->get_offset();
            Clause& c = *cl_alloc.ptr(offset);
            Lit      false_lit = ~p;
            if (c[0] == false_lit) {
                c[0] = c[1], c[1] = false_lit;
            }
            assert(c[1] == false_lit);
            i++;

            Lit     first = c[0];
            Watched w     = Watched(offset, first);
            if (first != blocked && value(first) == l_True) {
                *j++ = w;
                continue;
            }

            // Look for new watch:
            for (uint32_t k = 2; k < c.size(); k++) {
                //Literal is either unset or satisfied, attach to other watchlist
                if (likely(value(c[k]) != l_False)) {
                    c[1] = c[k];
                    c[k] = false_lit;
                    watches[c[1]].push(w);
                    goto nextClause;
                }
            }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(c[0]) == l_False) {
                confl = PropBy(offset);
                #ifdef STATS_NEEDED
                if (c.red()) {
                    red_stats_extra[c.stats.extra_pos].conflicts_made++;
                    lastConflictCausedBy = ConflCausedBy::longred;
                } else {
                    lastConflictCausedBy = ConflCausedBy::longirred;
                }
                #endif
                #ifdef VERBOSE_DEBUG
                cout << "Conflicting on clause:" << c << endl;
                #endif
                while (i < end) {
                    *j++ = *i++;
                }
                assert(j <= end);
                qhead = trail.size();
            } else {
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR) || defined(NORMAL_CL_USE_STATS)
                c.stats.props_made++;
                #endif
                #ifdef STATS_NEEDED
                if (c.red())
                    propStats.propsLongRed++;
                else
                    propStats.propsLongIrred++;
                #endif
                if (currLevel == declevel) {
                    enqueue<false>(c[0], currLevel, PropBy(offset));
                } else {
                    uint32_t nMaxLevel = currLevel;
                    uint32_t nMaxInd = 1;
                    // pass over all the literals in the clause and find the one with the biggest level
                    for (uint32_t nInd = 2; nInd < c.size(); ++nInd) {
                        uint32_t nLevel = varData[c[nInd].var()].level;
                        if (nLevel > nMaxLevel) {
                            nMaxLevel = nLevel;
                            nMaxInd = nInd;
                        }
                    }

                    if (nMaxInd != 1) {
                        std::swap(c[1], c[nMaxInd]);
                        j--; // undo last watch
                        watches[c[1]].push(w);
                    }

                    enqueue<false>(c[0], nMaxLevel, PropBy(offset));
                }
            }

            nextClause:;
        }
        ws.shrink_(i-j);

        #ifdef USE_GAUSS
        if (!all_matrices_disabled) {
            PropBy ret = gauss_jordan_elim(p, currLevel);
            //cout << "ret: " << ret << " -- " << endl;
            if (!ret.isNULL()) {
                confl = ret;
                break;
            }
        }
        #endif //USE_GAUSS
    }
    qhead = trail.size();
    simpDB_props -= num_props;
    propStats.propagations += (uint64_t)num_props;

    #ifdef VERBOSE_DEBUG
    cout << "Propagation (propagate_any_order_fast) ended." << endl;
    #endif

    return confl;
}

template<bool update_bogoprops, bool red_also, bool use_disable>
PropBy PropEngine::propagate_any_order()
{
    PropBy confl;

    #ifdef VERBOSE_DEBUG_PROP
    cout << "Fast Propagation started" << endl;
    #endif

    const bool fast_confl_break = solver->conf.fast_confl_break;

    while (qhead < trail.size() && (!fast_confl_break || confl.isNULL())) {
        const Lit p = trail[qhead].lit;     // 'p' is enqueued fact to propagate.
        watch_subarray ws = watches[~p];
        uint32_t currLevel = trail[qhead].lev;

        Watched* i = ws.begin();
        Watched* j = i;
        Watched* end = ws.end();
        if (update_bogoprops) {
            propStats.bogoProps += ws.size()/4 + 1;
        }
        propStats.propagations++;
        for (; i != end; i++) {
            // propagate binary clause
            if (likely(i->isBin())) {
                *j++ = *i;
                if (!red_also && i->red()) {
                    continue;
                }
                if (use_disable && i->bin_cl_marked()) {
                    continue;
                }
                if (!prop_bin_cl<update_bogoprops>(i, p, confl, currLevel)) {
                    i++;
                    break;
                }
                continue;
            }

            // propagate BNN constraint
            if (i->isBNN()) {
                *j++ = *i;
                const lbool val = bnn_prop(i->get_bnn(), currLevel);
                if (val == l_False) {
                    confl = PropBy(i->get_bnn(), nullptr);
                    i++;
                    break;
                }
                continue;
            }

            //propagate normal clause
            assert(i->isClause());
            if (!prop_long_cl_any_order<update_bogoprops, red_also, use_disable>(i, j, p, confl, currLevel)) {
                i++;
                break;
            }
            continue;
        }
        while (i != end) {
            *j++ = *i++;
        }
        ws.shrink_(end-j);
        qhead++;
    }

    #ifdef VERBOSE_DEBUG
    cout << "Propagation (propagate_any_order) ended." << endl;
    #endif

    return confl;
}
template PropBy PropEngine::propagate_any_order<false>();
template PropBy PropEngine::propagate_any_order<true>();
template PropBy PropEngine::propagate_any_order<true, false, true>();
template PropBy PropEngine::propagate_any_order<true, true,  true>();


void PropEngine::printWatchList(const Lit lit) const
{
    watch_subarray_const ws = watches[lit];
    for (const Watched *it2 = ws.begin(), *end2 = ws.end()
        ; it2 != end2
        ; it2++
    ) {
        if (it2->isBin()) {
            cout << "bin: " << lit << " , " << it2->lit2() << " red : " <<  (it2->red()) << endl;
        } else if (it2->isClause()) {
            cout << "cla:" << it2->get_offset() << endl;
        } else {
            assert(false);
        }
    }
}

void PropEngine::updateVars(
    [[maybe_unused]] const vector<uint32_t>& outerToInter,
    [[maybe_unused]] const vector<uint32_t>& interToOuter
) {
    //Trail is NOT correct, only its length is correct
    for(Trail& t: trail) {
        t.lit = lit_Undef;
    }
}

void PropEngine::print_trail()
{
    for(size_t i = trail_lim[0]; i < trail.size(); i++) {
        assert(varData[trail[i].lit.var()].level == trail[i].lev);
        cout
        << "trail " << i << ":" << trail[i].lit
        << " lev: " << trail[i].lev
        << " reason: " << varData[trail[i].lit.var()].reason
        << endl;
    }
}


template<bool update_bogoprops>
bool PropEngine::propagate_occur()
{
    assert(ok);
    while (qhead < trail_size()) {
        const Lit p = trail[qhead].lit;
        qhead++;
        watch_subarray ws = watches[~p];

        //Go through each occur
        for (const Watched* it = ws.begin(), *end = ws.end()
            ; it != end
            ; ++it
        ) {
            if (it->isClause()) {
                if (!propagate_long_clause_occur<update_bogoprops>(it->get_offset()))
                    return false;
            }

            if (it->isBin()) {
                if (!propagate_binary_clause_occur<update_bogoprops>(*it))
                    return false;
            }
        }
    }

    return true;
}

template bool PropEngine::propagate_occur<true>();
template bool PropEngine::propagate_occur<false>();

template<bool update_bogoprops>
inline bool PropEngine::propagate_binary_clause_occur(
    const Watched& ws)
{
    const lbool val = value(ws.lit2());
    if (val == l_False) {
        return false;
    }

    if (val == l_Undef) {
        enqueue<update_bogoprops>(ws.lit2());
        #ifdef STATS_NEEDED
        if (ws.red())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;
        #endif
    }

    return true;
}

template<bool update_bogoprops>
inline bool PropEngine::propagate_long_clause_occur(
    const ClOffset offset)
{
    const Clause& cl = *cl_alloc.ptr(offset);
    assert(!cl.freed() && "Cannot be already freed in occur");
    if (cl.getRemoved())
        return true;

    Lit lastUndef = lit_Undef;
    uint32_t numUndef = 0;
    bool satcl = false;
    for (const Lit lit: cl) {
        const lbool val = value(lit);
        if (val == l_True) {
            satcl = true;
            break;
        }
        if (val == l_Undef) {
            numUndef++;
            if (numUndef > 1) break;
            lastUndef = lit;
        }
    }
    if (satcl)
        return true;

    //Problem is UNSAT
    if (numUndef == 0) {
        return false;
    }

    if (numUndef > 1)
        return true;

    enqueue<update_bogoprops>(lastUndef);
    #ifdef STATS_NEEDED
    if (cl.red())
        propStats.propsLongRed++;
    else
        propStats.propsLongIrred++;
    #endif

    return true;
}

#ifdef STATS_NEEDED_BRANCH
void PropEngine::sql_dump_vardata_picktime(uint32_t v, PropBy from)
{
    if (!solver->sqlStats)
        return;

    bool dump = false;
    double rnd_num = solver->mtrand.randDblExc();
    if (rnd_num <= conf.dump_individual_cldata_ratio*0.1) {
        dump = true;
    }
    varData[v].dump = dump;
    if (!dump)
        return;

    solver->dump_restart_sql(rst_dat_type::var);

    uint64_t outer_var = map_inter_to_outer(v);

    varData[v].sumDecisions_at_picktime = sumDecisions;
    varData[v].sumConflicts_at_picktime = sumConflicts;
    varData[v].sumAntecedents_at_picktime = sumAntecedents;
    varData[v].sumAntecedentsLits_at_picktime = sumAntecedentsLits;
    varData[v].sumConflictClauseLits_at_picktime = sumConflictClauseLits;
    varData[v].sumPropagations_at_picktime = sumPropagations;
    varData[v].sumDecisionBasedCl_at_picktime = sumDecisionBasedCl;
    varData[v].sumClLBD_at_picktime = sumClLBD;
    varData[v].sumClSize_at_picktime = sumClSize;
    double rel_activity_at_picktime =
        std::log2(var_act_vsids[v]+10e-300)/std::log2(max_vsids_act+10e-300);

    varData[v].last_time_set_was_dec = (from == PropBy());

    //inside data
    varData[v].inside_conflict_clause_glue_at_picktime = varData[v].inside_conflict_clause_glue;
    varData[v].inside_conflict_clause_at_picktime = varData[v].inside_conflict_clause;
    varData[v].inside_conflict_clause_antecedents_at_picktime = varData[v].inside_conflict_clause_antecedents;

    solver->sqlStats->var_data_picktime(
        solver
        , outer_var
        , varData[v]
        , rel_activity_at_picktime
    );
}
#endif

#ifdef VMTF_NEEDED
// Update queue to point to last potentially still unassigned variable.
// All variables after 'queue.unassigned' in bump order are assumed to be
// assigned.  Then update the 'queue.vmtf_bumped' field and log it.  This is
// inlined here since it occurs in several inner loops.
//
void PropEngine::vmtf_update_queue_unassigned (uint32_t idx) {
    assert(idx != std::numeric_limits<uint32_t>::max());
    vmtf_queue.unassigned = idx;
    vmtf_queue.vmtf_bumped = vmtf_btab[idx];
}

void PropEngine::vmtf_init_enqueue (uint32_t var) {
    Link & l = vmtf_links[var];
    l.next = std::numeric_limits<uint32_t>::max();
    if (vmtf_queue.last != std::numeric_limits<uint32_t>::max()) {
        assert(vmtf_links[vmtf_queue.last].next == std::numeric_limits<uint32_t>::max());
        vmtf_links[vmtf_queue.last].next = var;
    } else {
        assert(vmtf_queue.first == std::numeric_limits<uint32_t>::max());
        vmtf_queue.first = var;
    }
    vmtf_btab[var] = ++vmtf_queue.vmtf_bumped;
    l.prev = vmtf_queue.last;
    vmtf_queue.last = var;
    vmtf_update_queue_unassigned(vmtf_queue.last);
}

// Move vmtf_bumped variables to the front of the (VMTF) decision queue.  The
// 'vmtf_bumped' time stamp is updated accordingly.  It is used to determine
// whether the 'queue.assigned' pointer has to be moved in 'unassign'.

void PropEngine::vmtf_bump_queue (uint32_t var) {
    if (vmtf_links[var].next == std::numeric_limits<uint32_t>::max()) {
        return;
    }
    //Remove from wherever it is, put to the top
    vmtf_queue.dequeue (vmtf_links, var);
    vmtf_queue.enqueue (vmtf_links, var);

    assert (vmtf_queue.vmtf_bumped != std::numeric_limits<uint32_t>::max());
    vmtf_btab[var] = ++vmtf_queue.vmtf_bumped;
    //LOG ("moved to front variable %d and vmtf_bumped to %" PRId64 "", idx, vmtf_btab[idx]);
    if (value(var) == l_Undef) {
        vmtf_update_queue_unassigned(var);
    }
}
#endif

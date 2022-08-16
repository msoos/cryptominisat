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
#include "gaussian.h"

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

    var_act_vsids.insert(var_act_vsids.end(), 1, 0);
    vmtf_btab.insert(vmtf_btab.end(), 1, 0);
    vmtf_links.insert(vmtf_links.end(), 1, Link());
}

void PropEngine::new_vars(size_t n)
{
    CNF::new_vars(n);

    var_act_vsids.insert(var_act_vsids.end(), n, 0);
    vmtf_btab.insert(vmtf_btab.end(), n, 0);
    vmtf_links.insert(vmtf_links.end(), n, Link());
}

void PropEngine::save_on_var_memory()
{
    CNF::save_on_var_memory();

    var_act_vsids.resize(nVars());
    var_act_vsids.shrink_to_fit();
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

PropBy PropEngine::gauss_jordan_elim(const Lit p, const uint32_t currLevel)
{
    VERBOSE_PRINT("PropEngine::gauss_jordan_elim called, declev: "
        << decisionLevel() << " lit to prop: " << p);

    if (gmatrices.empty()) return PropBy();
    for(uint32_t i = 0; i < gqueuedata.size(); i++) {
        if (gqueuedata[i].disabled || !gmatrices[i]->is_initialized()) continue;
        gqueuedata[i].reset();
        gmatrices[i]->update_cols_vals_set();
    }

    bool confl_in_gauss = false;
    assert(gwatches.size() > p.var());
    vec<GaussWatched>& ws = gwatches[p.var()];
    GaussWatched* i = ws.begin();
    GaussWatched* j = i;
    const GaussWatched* end = ws.end();

    for (; i != end; i++) {
        if (gqueuedata[i->matrix_num].disabled || !gmatrices[i->matrix_num]->is_initialized())
            continue; //remove watch and continue

        gqueuedata[i->matrix_num].new_resp_var = numeric_limits<uint32_t>::max();
        gqueuedata[i->matrix_num].new_resp_row = numeric_limits<uint32_t>::max();
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

    for (; i != end; i++) *j++ = *i;
    ws.shrink(i-j);

    for (size_t g = 0; g < gqueuedata.size(); g++) {
        if (gqueuedata[g].disabled || !gmatrices[g]->is_initialized())
            continue;

        if (gqueuedata[g].do_eliminate) {
            gmatrices[g]->eliminate_col(p.var(), gqueuedata[g]);
            confl_in_gauss |= (gqueuedata[g].ret == gauss_res::confl);
        }
    }

    for (GaussQData& gqd: gqueuedata) {
        if (gqd.disabled) continue;

        //There was a conflict but this is not that matrix.
        //Just skip.
        if (confl_in_gauss && gqd.ret != gauss_res::confl) continue;

        switch (gqd.ret) {
            case gauss_res::confl :{
                gqd.num_conflicts++;
                qhead = trail.size();
                return gqd.confl;
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

lbool PropEngine::bnn_prop(
    const uint32_t bnn_idx, uint32_t level, Lit l, BNNPropType prop_t)
{
    BNN* bnn = bnns[bnn_idx];
    switch(prop_t) {
        case bnn_neg_t:
            bnn->ts++;
            bnn->undefs--;
            break;
        case bnn_pos_t:
            bnn->undefs--;
            break;
        case bnn_out_t:
            break;
    }
    #ifdef SLOW_DEBUG
    assert (bnn->ts >= 0);
    assert (bnn->undefs >= 0);
    assert (bnn->ts <= (int32_t)bnn->size());
    assert (bnn->undefs <= (int32_t)bnn->size());
    #endif

    const int32_t ts = bnn->ts;
    const int32_t undefs = bnn->undefs;

    if (ts+undefs < bnn->cutoff) {
        // we are under the cutoff no matter what undef+unknowns is
        if (bnn->set) {
//                 cout << "returning l_False from bnn_prop" <<  "declev: " << decisionLevel() << endl;
            return l_False;
        }

        if (value(bnn->out) == l_False)
            return l_True;
        if (value(bnn->out) == l_True)
            return l_False;

        assert(value(bnn->out) == l_Undef);
        enqueue<false>(~bnn->out, level, PropBy(bnn_idx, nullptr));
//         cout << "BNN prop set BNN out " << ~bnn->out << " due to being under for sure" << endl;
        return l_True;
    }

    if (ts >= bnn->cutoff) {
        // we are over the cutoff
        if (bnn->set) {
            return l_True;
        }

        // we are at the cutoff no matter what undefs is
        if (value(bnn->out) == l_True)
            return l_True;
        if (value(bnn->out) == l_False)
            return l_False;

        assert(value(bnn->out) == l_Undef);
        enqueue<false>(bnn->out, level, PropBy(bnn_idx, nullptr));
        //         cout << "BNN prop set BNN out " << bnn->out << " due to being over for sure" << endl;        }
        return l_True;
    }

    if (
        ((!bnn->set && value(bnn->out) == l_True) || bnn->set) &&
            bnn->cutoff - ts == undefs)
    {
        //it's TRUE and UNDEF is exactly what's missing

        for(const auto& p: *bnn) {
            if (value(p) == l_Undef) {
                enqueue<false>(p, level, PropBy(bnn_idx, nullptr));
            }
        }
        return l_True;
    }

    if (
        ((!bnn->set && value(bnn->out) == l_False) &&
            bnn->cutoff == ts + 1))
    {
        //it's FALSE and UNDEF must ALL be set to 0

        for(const auto& p: *bnn) {
            if (value(p) == l_Undef) {
                enqueue<false>(~p, level, PropBy(bnn_idx, nullptr));
            }
        }
        return l_True;
    }

    return l_Undef;
}

vector<Lit>* PropEngine::get_bnn_reason(BNN* bnn, Lit lit)
{
//     cout << "Getting BNN reason, lit: " << lit << " bnn: " << *bnn << endl;
//     cout << "values: ";
//     for(const auto& l: bnn->in) {
//         cout << l << " val: " << value(l) << " , ";
//     }
//     if (!bnn->set) {
//         cout << " -- out : " << value(bnn->out);
//     }
//     cout << endl;

    if (lit == lit_Undef) {
        get_bnn_confl_reason(bnn, &bnn_confl_reason);
        return &bnn_confl_reason;
    }

    auto& reason = varData[lit.var()].reason;
//     cout
//     << " reason lev: " << varData[lit.var()].level
//     << " sublev: " << varData[lit.var()].sublevel
//     << " reason type: " << varData[lit.var()].reason.getType()
//     << endl;
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

    get_bnn_prop_reason(bnn, lit, ret);
//     cout << "get_bnn_reason (" << lit << ") returning: ";
//     for(const auto& l: *ret) {
//         cout << l << " val(" << value(l) << ") ";
//     }
//     cout << "0" << endl;

    return ret;
}

void PropEngine::get_bnn_confl_reason(BNN* bnn, vector<Lit>* ret)
{
    assert(bnn->set || value(bnn->out) != l_Undef);

    //It's set to TRUE, but it's actually LESS than cutoff
    if (bnn->set||
        (!bnn->set && value(bnn->out) == l_True))
    {
        ret->clear();
        if (!bnn->set)
            ret->push_back(~bnn->out);

        int32_t need = bnn->size()-bnn->cutoff+1;
        for(const auto& l: *bnn) {
            if (value(l) == l_False) {
               ret->push_back(l);
               need--;
            }
            if (need == 0) break;
        }
    }

    //it's set to FALSE but it's actually MORE than cutoff.
    if ((!bnn->set && value(bnn->out) == l_False))
    {
        ret->clear();
        if (!bnn->set)
            ret->push_back(bnn->out);

        int32_t need = bnn->cutoff;
        for(const auto& l: *bnn) {
            if (value(l) == l_True) {
                ret->push_back(~l);
                need--;
            }
            if (need == 0) break;
        }
    }

    uint32_t maxsublevel = 0;
    uint32_t at = 0;
    for(uint32_t i = 0; i < ret->size(); i ++) {
        Lit l = (*ret)[i];
        if (varData[l.var()].sublevel >= maxsublevel) {
            maxsublevel = varData[l.var()].sublevel;
            at = i;
        }
    }
    std::swap((*ret)[0], (*ret)[at]);
}

void PropEngine::get_bnn_prop_reason(
    BNN* bnn, Lit lit, vector<Lit>* ret)
{
    assert(bnn->set|| value(bnn->out) != l_Undef);
    assert(value(lit) == l_True); //it's being propagated

    if (lit.var() == bnn->out.var()) {
        // bnn->out got set

        //It's set to TRUE
        if (value(bnn->out) == l_True) {
            ret->clear();
            ret->push_back(lit); //this is what's propagated, must be 1st

            //Caused it to meet cutoff
            int32_t need = bnn->cutoff;
            for(const auto& l: *bnn) {
                if (varData[l.var()].sublevel <= varData[lit.var()].sublevel
                    && value(l) == l_True)
                {
                    need--;
                    ret->push_back(~l);
                }
                if (need == 0) break;
            }
        }

        //it's set to FALSE
        if (value(bnn->out) == l_False) {
            ret->clear();
            ret->push_back(lit); //this is what's propagated, must be 1st

            //Caused it to meet cutoff
            int32_t need = bnn->size()-bnn->cutoff+1;
            for(const auto& l: *bnn) {
                if (varData[l.var()].sublevel <= varData[lit.var()].sublevel
                    && value(l) == l_False)
                {
                    need--;
                    ret->push_back(l);
                }
                if (need == 0) break;
            }
        }
        return;
    } else {
        // bnn->in got set

        ret->clear();
        ret->push_back(lit); //this is what's propagated, must be 1st
        if (!bnn->set) {
            ret->push_back(bnn->out ^ (value(bnn->out) == l_True));
        }
        for(const auto& l: *bnn) {
            if (varData[l.var()].sublevel < varData[lit.var()].sublevel) {
                if (bnn->set ||
                    (!bnn->set && value(bnn->out) == l_True))
                {
                    if (value(l) == l_False) {
                        ret->push_back(l);
                    }
                }
                if (!bnn->set && value(bnn->out) == l_False)
                {
                    if (value(l) == l_True) {
                        ret->push_back(~l);
                    }
                }
            }
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
template<bool inprocess>
inline bool PropEngine::prop_bin_cl(
    const Watched* i
    , const Lit p
    , PropBy& confl
    , uint32_t currLevel
) {
    const lbool val = value(i->lit2());
    if (val == l_Undef) {
        enqueue<inprocess>(i->lit2(), currLevel, PropBy(~p, i->red(), i->get_ID()));
    } else if (val == l_False) {
        confl = PropBy(~p, i->red(), i->get_ID());
        failBinLit = i->lit2();
        qhead = trail.size();
        return false;
    }

    return true;
}

template<bool inprocess, bool red_also, bool use_disable>
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
    if (inprocess) {
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

    if (prop_normal_helper<inprocess>(c, offset, j, p) == PROP_NOTHING) {
        return true;
    }

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        handle_normal_prop_fail<inprocess>(c, offset, confl);
        return false;
    } else {
        if (!inprocess) {
            #if defined(NORMAL_CL_USE_STATS)
            c.stats.props_made++;
            #endif
            #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
            c.stats.props_made++;
            c.stats.last_touched_any = sumConflicts;
            #endif
        }

        if (currLevel == decisionLevel()) {
            enqueue<inprocess>(c[0], currLevel, PropBy(offset));
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

            enqueue<inprocess>(c[0], nMaxLevel, PropBy(offset));
        }
    }

    return true;
}

void CMSat::PropEngine::reverse_one_bnn(uint32_t idx, BNNPropType t) {
    BNN* const bnn= bnns[idx];
    SLOW_DEBUG_DO(assert(bnn != NULL));
    switch(t) {
        case bnn_neg_t:
            bnn->ts--;
            bnn->undefs++;
            break;
        case bnn_pos_t:
            bnn->undefs++;
            break;
        case bnn_out_t:
            break;
    }
    VERBOSE_PRINT("reverse bnn idx: " << idx
        << " bnn->undefs: " << bnn->undefs
        << " bnn->ts: " << bnn->ts
        << " bnn->sz: " << bnn->size()
        << " BNN: " << *bnn);

    SLOW_DEBUG_DO(assert(bnn->ts >= 0));
    SLOW_DEBUG_DO(assert(bnn->undefs >= 0));
    SLOW_DEBUG_DO(assert(bnn->ts <= (int32_t)bnn->size()));
    SLOW_DEBUG_DO(assert(bnn->undefs <= (int32_t)bnn->size()));
}

void CMSat::PropEngine::reverse_prop(const CMSat::Lit l)
{
    if (!varData[l.var()].propagated) return;
    watch_subarray ws = watches[~l];
    for (const auto& i: ws) {
        if (i.isBNN()) {
            reverse_one_bnn(i.get_bnn(), i.get_bnn_prop_t());
        }
    }
    varData[l.var()].propagated = false;
}

template<bool inprocess, bool red_also, bool distill_use>
PropBy PropEngine::propagate_any_order()
{
    PropBy confl;
    VERBOSE_PRINT("propagate_any_order started");

    while (qhead < trail.size() && confl.isNULL()) {
        const Lit p = trail[qhead].lit;     // 'p' is enqueued fact to propagate.
        varData[p.var()].propagated = true;
        watch_subarray ws = watches[~p];
        uint32_t currLevel = trail[qhead].lev;

        Watched* i = ws.begin();
        Watched* j = i;
        Watched* end = ws.end();
        if (inprocess) {
            propStats.bogoProps += ws.size()/4 + 1;
        }
        propStats.propagations++;
        simpDB_props--;
        for (; i != end; i++) {
            // propagate binary clause
            if (likely(i->isBin())) {
                *j++ = *i;
                if (!red_also && i->red()) {
                    continue;
                }
                if (distill_use && i->bin_cl_marked()) {
                    continue;
                }
                prop_bin_cl<inprocess>(i, p, confl, currLevel);
                continue;
            }

            // propagate BNN constraint
            if (i->isBNN()) {
                *j++ = *i;
                const lbool val = bnn_prop(
                    i->get_bnn(), currLevel, p, i->get_bnn_prop_t());
                if (val == l_False) confl = PropBy(i->get_bnn(), nullptr);
                continue;
            }

            //propagate normal clause
            assert(i->isClause());
            prop_long_cl_any_order<inprocess, red_also, distill_use>(i, j, p, confl, currLevel);
            continue;
        }
        while (i != end) {
            *j++ = *i++;
        }
        ws.shrink_(end-j);
        VERBOSE_PRINT("prop went through watchlist of " << p);

        //distillation would need to generate TBDD proofs to simplify clauses with GJ
        if (confl.isNULL() && !distill_use) {
            confl = gauss_jordan_elim(p, currLevel);
        }

        qhead++;
    }

    #ifdef SLOW_DEBUG
    if (confl.isNULL() && !distill_use) {
        for (size_t g = 0; g < gqueuedata.size(); g++) {
            if (gqueuedata[g].disabled) continue;
            gmatrices[g]->check_invariants();
        }
    }
    #endif

// For BNN debugging
//     if (confl.isNULL()) {
//         for(uint32_t idx = 0; idx < bnns.size(); idx++) {
//             auto& bnn = bnns[idx];
//             if (!bnn) continue;
//             int32_t undefs = 0;
//             int32_t ts = 0;
//             for(const auto& l: *bnn) {
//                 if (value(l) == l_True) {
//                     ts++;
//                 }
//                 if (value(l) == l_Undef) {
//                     undefs++;
//                 }
//             }
//             cout << "u: " << undefs << " my u: " << bnn->undefs << " -- ";
//             cout << "t: " << ts << " my t: " << bnn->ts << " idx: " << idx
//             << " sz :" << bnn->size() << endl;
//             assert(undefs == bnn->undefs);
//             assert(ts == bnn->ts);
//         }
//         cout << "ALL BNNS CHECKED========" << endl;
//     }


    VERBOSE_PRINT("Propagation (propagate_any_order) ended.");

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

template<bool inprocess>
bool PropEngine::propagate_occur(int64_t* limit_to_decrease)
{
    assert(ok);
    bool ret = true;

    while (qhead < trail.size()) {
        const Lit p = trail[qhead].lit;
        qhead++;
        watch_subarray ws = watches[~p];

        //Go through each occur
        *limit_to_decrease -= 1;
        for (const Watched* it = ws.begin(), *end = ws.end()
            ; it != end
            ; ++it
        ) {
            if (it->isClause()) {
                *limit_to_decrease -= 1;
                if (!prop_long_cl_occur<inprocess>(it->get_offset())) ret = false;
            }
            if (it->isBin()) {
                if (!prop_bin_cl_occur<inprocess>(*it)) ret = false;
            }
            assert(!it->isBNN());
        }
    }
    assert(gmatrices.empty());

    if (decisionLevel() == 0 && !ret) {
        *frat << add << ++clauseID << fin;
        assert(unsat_cl_ID == 0);
        unsat_cl_ID = clauseID;
    }

    return ret;
}

template bool PropEngine::propagate_occur<true>(int64_t*);
template bool PropEngine::propagate_occur<false>(int64_t*);

template<bool inprocess>
inline bool PropEngine::prop_bin_cl_occur(
    const Watched& ws)
{
    const lbool val = value(ws.lit2());
    if (val == l_False) return false;
    if (val == l_Undef) enqueue<inprocess>(ws.lit2());
    return true;
}

template<bool inprocess>
inline bool PropEngine::prop_long_cl_occur(
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

    enqueue<inprocess>(lastUndef);

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

///// VMTF ////

void PropEngine::vmtf_check_unassigned()
{
    uint32_t at = vmtf_queue.unassigned;
    uint32_t unassigned = 0;
    while (at  != numeric_limits<uint32_t>::max()) {
        at = vmtf_links[at].next;
        if (at != numeric_limits<uint32_t>::max()) {
            if (value(at) == l_Undef && varData[at].removed == Removed::none) {
                cout << "vmtf OOOPS, var " << at+1 << " would have been unassigned. btab[var]: " << vmtf_btab[at] << endl;
                unassigned++;
            }
        }
    }
    if (unassigned) {
        cout << "unassigned total: " << unassigned << endl;
        assert(unassigned == 0);
    }
}

uint32_t PropEngine::vmtf_pick_var()
{
    uint64_t searched = 0;
    uint32_t res = vmtf_queue.unassigned;
    VERBOSE_PRINT("vmtf start unassigned: " << res);

    SLOW_DEBUG_DO(vmtf_check_unassigned());
    while (res != numeric_limits<uint32_t>::max()
        && value(res) != l_Undef
    ) {
        res = vmtf_links[res].prev;
        searched++;
    }

    if (res == numeric_limits<uint32_t>::max()) {
        vmtf_check_unassigned();
        return var_Undef;
    }
    if (searched) vmtf_update_queue_unassigned(res);
    VERBOSE_PRINT("vmtf next queue decision variable " << res << " btab value: " << vmtf_btab[res]);
    return res;
}

// Update queue to point to last potentially still unassigned variable.
// All variables after 'queue.unassigned' in bump order are assumed to be
// assigned.  Then update the 'queue.vmtf_bumped' field and log it.
void PropEngine::vmtf_update_queue_unassigned (const uint32_t var) {
    assert(var != numeric_limits<uint32_t>::max());
    assert(var < nVars());
    VERBOSE_PRINT("vmtf_queue.unassigned set to: " << var+1
        << " vmtf_queue.vmtf_bumped set to: " << vmtf_btab[var]);
    vmtf_queue.unassigned = var;
    vmtf_queue.vmtf_bumped = vmtf_btab[var];
}

void PropEngine::vmtf_init_enqueue (const uint32_t var) {
    assert(var < nVars());
    assert(var < vmtf_links.size());
    Link & l = vmtf_links[var];

    //Put at the end of the queue
    l.next = numeric_limits<uint32_t>::max();
    if (vmtf_queue.last != numeric_limits<uint32_t>::max()) {
        // Not empty queue
        assert(vmtf_links[vmtf_queue.last].next == numeric_limits<uint32_t>::max());
        vmtf_links[vmtf_queue.last].next = var;
    } else {
        // Empty queue
        assert(vmtf_queue.first == numeric_limits<uint32_t>::max());
        vmtf_queue.first = var;
    }
    l.prev = vmtf_queue.last;
    vmtf_queue.last = var;

    vmtf_btab[var] = ++stats_bumped; // set timestamp of enqueue
    vmtf_update_queue_unassigned(vmtf_queue.last);
}

void PropEngine::vmtf_dequeue (const uint32_t var) {
    Link & l = vmtf_links[var];
    if (vmtf_queue.unassigned == var) {
        vmtf_queue.unassigned = l.prev;
        if (vmtf_queue.unassigned != numeric_limits<uint32_t>::max()) {
            vmtf_update_queue_unassigned(vmtf_queue.unassigned);
        }
    }
    //vmtf_queue.dequeue (vmtf_links, var);
}

// Move vmtf_bumped variables to the front of the (VMTF) decision queue.  The
// 'vmtf_bumped' time stamp is updated accordingly.  It is used to determine
// whether the 'queue.assigned' pointer has to be moved in 'unassign'.

void PropEngine::vmtf_bump_queue (const uint32_t var) {
    if (vmtf_links[var].next == numeric_limits<uint32_t>::max()) {
        return;
    }
    //Remove from wherever it is, put to the end
    vmtf_queue.dequeue (vmtf_links, var);
    vmtf_queue.enqueue (vmtf_links, var);

    assert (stats_bumped != numeric_limits<uint64_t>::max());
    vmtf_btab[var] = ++stats_bumped;
    VERBOSE_PRINT("vmtf moved to last element in queue the variable " << var+1 << " and vmtf_bumped to " << vmtf_btab[var]);
    if (value(var) == l_Undef) vmtf_update_queue_unassigned(var);
}

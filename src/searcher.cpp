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

#include "searcher.h"
#include "constants.h"
#include "frat.h"
#include "occsimplifier.h"
#include "propby.h"
#include "solvertypesmini.h"
#include "time_mem.h"
#include "solver.h"
#include <iomanip>
#include "varreplacer.h"
#include "clausecleaner.h"
#include "propbyforgraph.h"
#include <algorithm>
#include <sstream>
#include <cstddef>
#include <cmath>
#include <utility>

#include "sqlstats.h"
#include "datasync.h"
#include "reducedb.h"
#include "watchalgos.h"
#include "hasher.h"
#include "solverconf.h"
#include "distillerlong.h"
#include "solvertypes.h"
#include "gaussian.h"
#include "distillerbin.h"
#include "distillerlongwithimpl.h"
#include "intree.h"
#include "str_impl_w_impl.h"
#include "subsumeimplicit.h"
#include "sls.h"
#ifdef USE_VALGRIND
#include "valgrind/valgrind.h"
#include "valgrind/memcheck.h"
#endif


using namespace CMSat;
using std::cout;
using std::endl;

/**
@brief Sets a sane default config and allocates handler classes
*/
Searcher::Searcher(const SolverConf *_conf, Solver* _solver, std::atomic<bool>* _must_interrupt_inter) :
        HyperEngine(
            _conf
            , _solver
            , _must_interrupt_inter
        )
        , solver(_solver)
        , cla_inc(1)
{
    var_inc_vsids = 1;

    more_red_minim_limit_binary_actual = conf.more_red_minim_limit_binary;
    hist.setSize(conf.shortTermHistorySize);

    tier1_glue = conf.reducetier1glue;
    tier2_glue = conf.reducetier2glue;
    next_cls_distill = 5000.0*conf.global_next_multiplier;
    next_bins_distill = 12000.0*conf.global_next_multiplier;
    next_full_probe = 20000.0*conf.global_next_multiplier;
    next_sub_str_with_bin = 25000.0*conf.global_next_multiplier;
    next_intree = 50000.0*conf.global_next_multiplier;
    next_str_impl_with_impl = 40000.0*conf.global_next_multiplier;
}

Searcher::~Searcher() { clear_gauss_matrices(true); }

void Searcher::new_var(
    const bool bva,
    const uint32_t orig_outer,
    bool insert_varorder)
{
    PropEngine::new_var(bva, orig_outer, insert_varorder);

    if (insert_varorder) {
        insert_var_order_all((int)nVars()-1);
    }
}

void Searcher::new_vars(size_t n)
{
    PropEngine::new_vars(n);

    for(int i = n-1; i >= 0; i--) {
        insert_var_order_all((int)nVars()-i-1);
    }

}

void Searcher::save_on_var_memory()
{
    PropEngine::save_on_var_memory();

}

void Searcher::updateVars(
    [[maybe_unused]] const vector<uint32_t>& outer_to_inter
    , const vector<uint32_t>& inter_to_outer
) {
    updateArray(var_act_vsids, inter_to_outer);
    updateArray(vmtf_btab, inter_to_outer);
    updateArray(vmtf_links, inter_to_outer);

    auto upd = [&](uint32_t v) {
        if (v != numeric_limits<uint32_t>::max())
            v = inter_to_outer[v];
    };

    for(auto& l: vmtf_links) {
        upd(l.next);
        upd(l.prev);
    }
    upd(vmtf_queue.first);
    upd(vmtf_queue.last);
    upd(vmtf_queue.unassigned);
}

//TODO add hint here for FRAT
template<bool inprocess>
inline void Searcher::add_lit_to_learnt(
    const Lit lit
    , const uint32_t nDecisionLevel
) {
    const uint32_t var = lit.var();
    assert(var_data[var].removed == Removed::none);

    if (var_data[var].level == 0) {
        if (frat->enabled()) {
            assert(unit_cl_IDs[var] != 0);
            lrat.units.push_back(unit_cl_IDs[var]);
        }
        return;
    }
    otfs_antec_nonzero++;

    if (seen[var]) return;
    seen[var] = 1;

    if (!inprocess) {

        switch(branch_strategy) {
            case branch::vsids:
                vsids_bump_var_act<inprocess>(var);
                break;

            case branch::rand:
                break;

            case branch::vmtf:
                implied_by_learnts.push_back(var);
                break;
        }
    }

    if (var_data[var].level >= nDecisionLevel) {
        pathC++;
        otfs_cur_lev_seen.push_back(var);
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
        if (var_data[learnt_clause[i].var()].reason.isnullptr()
            || !litRedundant(learnt_clause[i], abstract_level)
        ) {
            learnt_clause[j++] = learnt_clause[i];
        }
    }
    learnt_clause.resize(j);
}


void Searcher::normalClMinim()
{
    size_t i,j;
    for (i = j = 1; i < learnt_clause.size(); i++) {
        const PropBy& reason = var_data[learnt_clause[i].var()].reason;
        size_t size;
        Lit *lits = nullptr;
        int32_t id = 0;
        PropByType type = reason.getType();
        if (type == null_clause_t) {
            //decision clause
            learnt_clause[j++] = learnt_clause[i];
            continue;
        }

        switch (type) {
            case binary_t:
                size = 1;
                id = reason.get_id();
                break;

            case clause_t: {
                Clause* cl2 = cl_alloc.ptr(reason.get_offset());
                lits = cl2->begin();
                size = cl2->size()-1;
                id = cl2->stats.id;
                break;
            }

            case xor_t: {
                auto cl = get_xor_reason(reason, id);
                lits = cl->data();
                size = cl->size()-1;
                sumAntecedentsLits += size;
                break;
            }

            case bnn_t: {
                assert(!frat->enabled());
                auto bnn_reason = get_bnn_reason(bnns[reason.getBNNidx()], learnt_clause[i]);
                lits = bnn_reason->data();
                size = bnn_reason->size()-1;
                sumAntecedentsLits += size;
                break;
            }

            default: release_assert(false);
        }

        bool remove = true;
        for (size_t k = 0; k < size; k++) {
            const Lit p = (type == binary_t) ? reason.lit2() : lits[k+1];
            if (!seen[p.var()] && var_data[p.var()].level > 0) {
                remove = false;
                break;
            }
        }
        if (!remove) {
            learnt_clause[j++] = learnt_clause[i];
            continue;
        }
        if (frat->enabled()) {
            lrat.reasons.push_back({var_data[learnt_clause[i].var()].sublevel, id});
            for (size_t k = 0; k < size; k++) {
                const Lit p = (type == binary_t) ? reason.lit2() : lits[k+1];
                if (var_data[p.var()].level == 0) {
                    assert(unit_cl_IDs[p.var()] != 0);
                    lrat.units.push_back(unit_cl_IDs[p.var()]);
                }
            }
        }
    }
    learnt_clause.resize(j);
}

void Searcher::LratChain::clear()
{
    reasons.clear();
    units.clear();
    binmin.clear();
    confl_id = 0;
}

void Searcher::LratChain::to_hints(vector<int32_t>& out)
{
    out = units;
    for (auto it = binmin.rbegin(); it != binmin.rend(); ++it)
        out.push_back(*it);
    std::sort(reasons.begin(), reasons.end());
    for (size_t i = 0; i < reasons.size(); i++) {
        if (i > 0 && reasons[i] == reasons[i-1]) continue;
        out.push_back(reasons[i].second);
    }
    assert(confl_id != 0);
    out.push_back(confl_id);
}

//Chain for a conflict where every literal is at level 0: unit IDs first,
//then the conflicting clause's ID
void Searcher::build_level0_confl_chain(const PropBy confl)
{
    assert(frat->enabled());
    chain.clear();
    int32_t id;
    switch (confl.getType()) {
        case binary_t:
            id = confl.get_id();
            chain.push_back(unit_cl_IDs[failBinLit.var()]);
            chain.push_back(unit_cl_IDs[confl.lit2().var()]);
            break;
        case clause_t: {
            Clause* cl = cl_alloc.ptr(confl.get_offset());
            id = cl->stats.id;
            for (const Lit l: *cl) chain.push_back(unit_cl_IDs[l.var()]);
            break;
        }
        case xor_t: {
            auto cl = get_xor_reason(confl, id);
            for (const Lit l: *cl) chain.push_back(unit_cl_IDs[l.var()]);
            break;
        }
        default: return; //no hints for BNN & co.
    }
    for (const auto& x: chain) assert(x != 0);
    chain.push_back(id);
}

//Improve glue, as kissat's promote: the tier follows from the glue
void Searcher::promote_clause(Clause* cl, const uint32_t new_glue)
{
    assert(cl->red());
    if (cl->stats.is_ternary_resolvent) return;
    if (new_glue >= cl->stats.glue) return;
    cl->stats.glue = new_glue;
}

//Kissat's mark_clause_as_used: refresh 'used', recompute glue and promote
void Searcher::bump_clause(Clause* cl)
{
    if (!cl->red()) return;
    cl->stats.used = CL_MAX_USED;
    if (!cl->stats.is_ternary_resolvent) {
        const uint32_t new_glue = calc_glue(*cl);
        if (new_glue < cl->stats.glue) promote_clause(cl, new_glue);
    }
    glue_used_hist[rst.stable][std::min<uint32_t>(cl->stats.glue, 64)]++;
}

template<bool inprocess>
void Searcher::add_lits_to_learnt(
    const PropBy confl
    , const Lit p
    , uint32_t nDecisionLevel
) {
    sumAntecedents++;

    Lit* lits = nullptr;
    size_t size = 0;
    int32_t id;
    switch (confl.getType()) {
        case binary_t : {
            id = confl.get_id();
            sumAntecedentsLits += 2;

            if (confl.isRedStep()) {
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                antec_data.binRed++;
                #endif
                stats.resolvs.binRed++;
                if (!hyper_bin_ranges.empty()) mark_hyper_bin_used(id);
            } else {
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                antec_data.binIrred++;
                #endif
                stats.resolvs.binIrred++;
            }
            break;
        }

        case clause_t : {
            Clause* cl = cl_alloc.ptr(confl.get_offset());
            id = cl->stats.id;
            assert(!cl->get_removed());
            lits = cl->begin();
            size = cl->size();
            sumAntecedentsLits += cl->size();

            if (cl->red()) {
                stats.resolvs.longRed++;
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                antec_data.longRed++;
                antec_data.glue_long_reds.push(cl->stats.glue);
                #endif
            } else {
                stats.resolvs.longIrred++;
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                antec_data.longIrred++;
                #endif
            }
            #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
            antec_data.size_longs.push(cl->size());
            if (!inprocess) cl->stats.uip1_used++;
            #endif

            if (!inprocess && cl->red()) {
                bump_clause(cl);
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                cl->stats.last_touched_any = sum_conflicts;
                bump_cl_act<inprocess>(cl);
                #endif
            }
            break;
        }

        case xor_t: {
            auto cl = get_xor_reason(confl, id);
            lits = cl->data();
            size = cl->size();
            sumAntecedentsLits += size;
            break;
        }

        case bnn_t: {
            auto bnn_reason = get_bnn_reason(bnns[confl.getBNNidx()], p);
            lits = bnn_reason->data();
            size = bnn_reason->size();
            sumAntecedentsLits += size;
            id = 0; // so we don't get a warning, assert below
            assert(!frat->enabled());
            break;
        }

        case null_clause_t:
        default: release_assert(false && "Error in conflict analysis (otherwise should be UIP)");
    }
    if (frat->enabled()) {
        if (p == lit_Undef) lrat.confl_id = id;
        else lrat.reasons.push_back({var_data[p.var()].sublevel, id});
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

            case bnn_t:
            case clause_t:
            case xor_t:
                x = lits[i];
                if (i == size-1) {
                    cont = false;
                }
                break;

            case null_clause_t:
                assert(false);
                break;
        }
        //skip the pivot by value: after OTFS a stale reason may not contain p
        if (p == lit_Undef
            || (confl.getType() == binary_t ? i > 0 : x != p)
        ) {
            add_lit_to_learnt<inprocess>(x, nDecisionLevel);
        }
        i++;
    }
}

template<bool inprocess>
void Searcher::minimize_learnt_clause()
{
    const size_t origSize = learnt_clause.size();

    to_clear = learnt_clause;
    if (conf.doRecursiveMinim) {
        recursiveConfClauseMin();
    } else {
        normalClMinim();
    }
    stats.recMinCl += ((origSize - learnt_clause.size()) > 0);
    stats.recMinLitRem += origSize - learnt_clause.size();

    if (conf.do_shrink_uip) shrink_learnt_clause<inprocess>();

    for (const Lit lit: to_clear) {
        seen[lit.var()] = 0;
    }
    to_clear.clear();
}

//Try to replace all clause lits of one level with that level's UIP.
//seen = in clause or proven redundant, seen2 = shrinkable (this block)
template<bool inprocess>
bool Searcher::try_shrink_block(
    const uint32_t block_begin
    , const uint32_t block_end
    , const uint32_t blevel
    , const uint32_t max_trail
    , const uint32_t abstract_levels
    , Lit& replace_with
) {
    assert(shrink_seen2_clear.empty());
    uint32_t open = 0;
    for (uint32_t k = block_begin; k < block_end; k++) {
        const uint32_t v = learnt_clause[k].var();
        seen2[v] = 1;
        shrink_seen2_clear.push_back(v);
        open++;
    }

    tmp_block_reasons.clear();
    tmp_block_units.clear();
    uint32_t pos = std::min<uint32_t>(max_trail, trail.size()-1);
    Lit uip = lit_Undef;
    bool ok = true;

    while (true) {
        while (!seen2[trail[pos].lit.var()]) {
            if (pos == 0) { ok = false; break; }
            pos--;
        }
        if (!ok) break;
        uip = trail[pos].lit;
        open--;
        if (open == 0) break;

        const PropBy reason = var_data[uip.var()].reason;
        const PropByType type = reason.getType();
        if (type == null_clause_t) { ok = false; break; }

        Lit* lits = nullptr;
        size_t size = 0;
        int32_t id = 0;
        switch (type) {
            case binary_t:
                size = 1;
                id = reason.get_id();
                break;

            case clause_t: {
                Clause* cl = cl_alloc.ptr(reason.get_offset());
                lits = cl->begin();
                size = cl->size()-1;
                id = cl->stats.id;
                break;
            }

            case xor_t: {
                auto cl = get_xor_reason(reason, id);
                lits = cl->data();
                size = cl->size()-1;
                break;
            }

            case bnn_t: {
                assert(!frat->enabled());
                auto cl = get_bnn_reason(bnns[reason.getBNNidx()], uip);
                lits = cl->data();
                size = cl->size()-1;
                break;
            }

            default: release_assert(false);
        }
        if (frat->enabled()) tmp_block_reasons.push_back({pos, id});

        for (size_t k2 = 0; k2 < size; k2++) {
            const Lit q = (type == binary_t) ? reason.lit2() : lits[k2+1];
            const uint32_t qv = q.var();
            const uint32_t qlev = var_data[qv].level;
            if (qlev == 0) {
                if (frat->enabled()) {
                    assert(unit_cl_IDs[qv] != 0);
                    tmp_block_units.push_back(unit_cl_IDs[qv]);
                }
                continue;
            }
            if (seen2[qv]) continue;
            if (qlev == blevel) {
                seen2[qv] = 1;
                shrink_seen2_clear.push_back(qv);
                open++;
                continue;
            }
            if (qlev > blevel) { ok = false; break; }

            //lower level: in clause/proven redundant, or provably redundant
            if (seen[qv]) continue;
            if (var_data[qv].reason.isnullptr()
                || !litRedundant(q, abstract_levels)
            ) {
                ok = false;
                break;
            }
            seen[qv] = 1;
            to_clear.push_back(q);
        }
        if (!ok) break;
        assert(pos > 0);
        pos--;
    }

    for (const uint32_t v: shrink_seen2_clear) {
        seen2[v] = 0;
        //shrunken-away lits are implied by the block UIP, keep as redundant
        if (ok && !seen[v]) {
            seen[v] = 1;
            to_clear.push_back(Lit(v, false));
        }
    }
    shrink_seen2_clear.clear();
    if (!ok) return false;

    replace_with = ~uip;
    if (!seen[uip.var()]) {
        seen[uip.var()] = 1;
        to_clear.push_back(uip);
    }
    if (frat->enabled()) {
        for (const auto& r: tmp_block_reasons) lrat.reasons.push_back(r);
        for (const auto& u: tmp_block_units) lrat.units.push_back(u);
    }
    if (!inprocess) {
        switch (branch_strategy) {
            case branch::vsids:
                vsids_bump_var_act<inprocess>(uip.var());
                break;
            case branch::vmtf:
                implied_by_learnts.push_back(uip.var());
                break;
            default:
                break;
        }
    }
    return true;
}

//All-UIP shrinking [FengBacchus-SAT'20], as in CaDiCaL's shrink.cpp
template<bool inprocess>
void Searcher::shrink_learnt_clause()
{
    SLOW_DEBUG_DO(check_seen2_clean());
    if (learnt_clause.size() < 3) return;

    //[0] is the 1UIP, sort the rest by (level, trail pos) descending
    std::sort(learnt_clause.begin()+1, learnt_clause.end(),
        [this](const Lit a, const Lit b) {
            const auto& va = var_data[a.var()];
            const auto& vb = var_data[b.var()];
            if (va.level != vb.level) return va.level > vb.level;
            return va.sublevel > vb.sublevel;
        });

    uint32_t abstract_levels = 0;
    for (size_t i = 1; i < learnt_clause.size(); i++) {
        abstract_levels |= abstractLevel(learnt_clause[i].var());
    }

    uint32_t removed = 0;
    uint32_t j = 1;
    uint32_t i = 1;
    while (i < learnt_clause.size()) {
        const uint32_t blevel = var_data[learnt_clause[i].var()].level;
        const uint32_t max_trail = var_data[learnt_clause[i].var()].sublevel;
        uint32_t block_end = i+1;
        while (block_end < learnt_clause.size()
            && var_data[learnt_clause[block_end].var()].level == blevel
        ) {
            block_end++;
        }

        Lit replace_with = lit_Undef;
        if (block_end - i >= 2
            && try_shrink_block<inprocess>(
                i, block_end, blevel, max_trail, abstract_levels, replace_with)
        ) {
            learnt_clause[j++] = replace_with;
            removed += block_end - i - 1;
        } else {
            for (uint32_t k = i; k < block_end; k++) {
                learnt_clause[j++] = learnt_clause[k];
            }
        }
        i = block_end;
    }
    learnt_clause.resize(j);

    stats.shrinkCl += (removed > 0);
    stats.shrinkLitRem += removed;
}

void Searcher::print_fully_minimized_learnt_clause() const
{
}

size_t Searcher::find_backtrack_level_of_learnt()
{
    if (learnt_clause.size() <= 1)
        return 0;
    else {
        uint32_t max_i = 1;
        for (uint32_t i = 2; i < learnt_clause.size(); i++) {
            if (level(learnt_clause[i]) > level(learnt_clause[max_i]))
                max_i = i;
        }
        std::swap(learnt_clause[max_i], learnt_clause[1]);
        return var_data[learnt_clause[1].var()].level;
    }
}

template<bool inprocess>
void Searcher::create_learnt_clause(PropBy confl)
{
    pathC = 0;
    int index = trail.size() - 1;
    Lit p = lit_Undef;
    implied_by_learnts.clear();
    otfs_cur_lev_seen.clear();
    otfs_driving = false;

    // Get decision level to go back to
    Lit lit0 = lit_Error;
    switch (confl.getType()) {
        case binary_t : {
            lit0 = failBinLit;
            break;
        }
        case xor_t: {
            int32_t ID;
            auto cl = get_xor_reason(confl, ID);
            lit0 = (*cl)[0];
            break;
        }
        case bnn_t : {
            auto cl = get_bnn_reason(bnns[confl.getBNNidx()], lit_Undef);
            lit0 = (*cl)[0];
            break;
        }
        case clause_t : {
            Clause* cl = cl_alloc.ptr(confl.get_offset());
            lit0 = (*cl)[0];
            break;
        }
        default: release_assert(false);
    }
    uint32_t nDecisionLevel = var_data[lit0.var()].level;

    // 1st UIP clause generation
    learnt_clause.push_back(lit_Undef); //make space for ~p
    for (;;) {
        otfs_antec_nonzero = (p == lit_Undef) ? 0 : 1;
        add_lits_to_learnt<inprocess>(confl, p, nDecisionLevel);

        //OTFS: resolvent equals antecedent minus p, strengthen the antecedent
        if (!inprocess && conf.do_otfs
            && p != lit_Undef
            && confl.getType() == clause_t
            && otfs_antec_nonzero > 2
            && pathC + learnt_clause.size() - 1 < otfs_antec_nonzero
            && pathC + learnt_clause.size() - 1 >= 3
        ) {
            Clause* cl = cl_alloc.ptr(confl.get_offset());
            bool otfs_ok = (*cl)[0] == p;
            #ifdef STATS_NEEDED
            otfs_ok = otfs_ok && !cl->stats.locked_for_data_gen;
            #endif
            if (otfs_ok) {
                otfs_strengthen(confl.get_offset(), p);
                for (const uint32_t v: otfs_cur_lev_seen) seen[v] = 0;
                otfs_cur_lev_seen.clear();
                for (const Lit l: learnt_clause) {
                    if (l != lit_Undef) seen[l.var()] = 0;
                }
                if (pathC == 1) {
                    //strengthened clause is asserting, it drives, learn nothing
                    learnt_clause.clear();
                    otfs_driving = true;
                    otfs_driving_cl = confl.get_offset();
                    stats.otfsDriving++;
                    return;
                }
                //restart analysis with the strengthened clause as conflict
                learnt_clause.clear();
                learnt_clause.push_back(lit_Undef);
                lrat.clear();
                pathC = 0;
                p = lit_Undef;
                continue;
            }
        }

        // Select next implication to look at
        do {
            while (!seen[trail[index--].lit.var()]);
            p = trail[index+1].lit;
            assert(p != lit_Undef);
        } while(trail[index+1].lev < nDecisionLevel);

        confl = var_data[p.var()].reason;
        assert(var_data[p.var()].level > 0);

        //This clears out vars that haven't been added to learnt_clause,
        //but their 'seen' has been set
        seen[p.var()] = 0;

        //Okay, one more path done
        pathC--;
        if (pathC == 0) break;
    }
    learnt_clause[0] = ~p;
}

//OTFS: remove p and level-0 lits from an attached clause, in place
Clause* Searcher::otfs_strengthen(const ClOffset offset, const Lit p)
{
    Clause& cl = *cl_alloc.ptr(offset);
    assert(cl.size() > 2);
    assert(!cl.freed() && !cl.get_removed());
    const Lit ow0 = cl[0];
    const Lit ow1 = cl[1];

    if (frat->enabled()) *frat << deldelay << cl << fin;

    otfs_tmp_lits.clear();
    for (const Lit l: cl) {
        if (l == p) continue;
        if (var_data[l.var()].level == 0) {
            assert(value(l) == l_False);
            continue;
        }
        otfs_tmp_lits.push_back(l);
    }
    assert(otfs_tmp_lits.size() >= 3);

    //watch the two highest (level, trail pos) lits, [0] is the highest
    for (uint32_t w = 0; w < 2; w++) {
        uint32_t best = w;
        for (uint32_t i2 = w+1; i2 < otfs_tmp_lits.size(); i2++) {
            const auto& va = var_data[otfs_tmp_lits[i2].var()];
            const auto& vb = var_data[otfs_tmp_lits[best].var()];
            if (va.level > vb.level
                || (va.level == vb.level && va.sublevel > vb.sublevel)
            ) {
                best = i2;
            }
        }
        std::swap(otfs_tmp_lits[w], otfs_tmp_lits[best]);
    }

    const uint32_t removed_num = cl.size() - otfs_tmp_lits.size();
    for (uint32_t i2 = 0; i2 < otfs_tmp_lits.size(); i2++) cl[i2] = otfs_tmp_lits[i2];
    cl.resize(otfs_tmp_lits.size());
    if (cl.red()) lit_stats.red_lits -= removed_num;
    else lit_stats.irred_lits -= removed_num;

    cl.stats.id = ++clause_id;
    if (frat->enabled()) {
        //the strengthened clause equals the current resolvent, so the
        //chain collected so far is exactly its derivation
        lrat.to_hints(chain);
        *frat << add << cl.stats.id << otfs_tmp_lits;
        add_chain();
        *frat << fin << findelay;
    }

    //re-watch: blocked lits may be stale, so re-add both
    removeWCl(watches[ow0], offset);
    removeWCl(watches[ow1], offset);
    watches[cl[0]].push(Watched(offset, cl[1]));
    watches[cl[1]].push(Watched(offset, cl[0]));

    stats.otfsStr++;
    stats.otfsLitsRem += removed_num;
    return &cl;
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
    assert(decision_level() == 1);

    do {
        switch (confl.getType()) {
            case binary_t: {
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
                break;
            }

            case bnn_t:
            case xor_t:
            case clause_t: {
                Lit* lits;
                uint32_t size;
                if (confl.getType() == clause_t) {
                    auto cl = solver->cl_alloc.ptr(confl.get_offset());
                    lits = cl->getData();
                    size = cl->size();
                } else if (confl.getType() == bnn_t) {
                    auto cl = get_bnn_reason(bnns[confl.getBNNidx()], p);
                    lits = cl->data();
                    size = cl->size();
                } else {
                    int32_t ID;
                    assert(confl.getType() == xor_t);
                    auto cl = get_xor_reason(confl, ID);
                    lits = cl->data();
                    size = cl->size();
                }

                // if True_confl==true, then choose p begin with the 1st index of lits
                for (uint32_t j = (p == lit_Undef && True_confl == false) ? 0 : 1 ; j < size ; j++) {
                    Lit q = lits[j];
                    assert(q.var() < seen.size());
                    if (!seen[q.var()]) {
                        seen[q.var()] = 1;
                        mypathC++;
                    }
                }
                break;
            }

            case null_clause_t:
                assert(confl.isnullptr());
                out_learnt.push_back(~p);
                break;
        }
        // if not break, while() will come to the index of trail blow 0, and fatal error occur;
        if (mypathC == 0) {
            break;
        }

        // Select next clause to look at:
        while (!seen[trail[index--].lit.var()]);
        // if the reason cr from the 0-level assigned var, we must break avoid move forth further;
        // but attention that maybe seen[x]=1 and never be clear. However makes no matter;
        if ((int)trail_lim[0] > index + 1
            && until == -1
        ) {
            until = out_learnt.size();
        }
        p = trail[index + 1].lit;
        confl = var_data[p.var()].reason;

        //under normal circumstances this does not happen, but here, it can
        //reason is undefined for level 0
        if (var_data[p.var()].level == 0) {
            confl = PropBy();
        }
        seen[p.var()] = 0;
        mypathC--;
    } while (mypathC >= 0);

    if (until != -1)
        out_learnt.resize(until);
}

struct vmtf_bump_sort {
    vmtf_bump_sort (const vector<uint64_t>& _vmtf_btab):
        vmtf_btab(_vmtf_btab)
    {}

    bool operator () (const uint32_t & a, const uint32_t & b) const
    {
        return vmtf_btab[a] < vmtf_btab[b];
    }

    const vector<uint64_t>& vmtf_btab;
};

//Bump vars in the reasons of the learnt clause, as in CaDiCaL
void Searcher::bump_reason_side_lit(const Lit lit, const uint32_t depth)
{
    if (var_data[lit.var()].level == 0) return;
    const PropBy& reason = var_data[lit.var()].reason;
    const PropByType type = reason.getType();

    Lit* lits = nullptr;
    size_t size = 0;
    switch (type) {
        case binary_t:
            size = 1;
            break;

        case clause_t: {
            Clause* cl = cl_alloc.ptr(reason.get_offset());
            lits = cl->begin();
            size = cl->size()-1;
            break;
        }

        //XOR/BNN reasons are expensive to recover, skip them
        default:
            return;
    }

    for (size_t i = 0; i < size; i++) {
        const Lit q = (type == binary_t) ? reason.lit2() : lits[i+1];
        const uint32_t var = q.var();
        if (seen[var] || var_data[var].level == 0) continue;
        seen[var] = 1;
        to_clear.push_back(q);

        switch (branch_strategy) {
            case branch::vsids:
                vsids_bump_var_act<false>(var);
                break;
            case branch::vmtf:
                implied_by_learnts.push_back(var);
                break;
            default:
                break;
        }
        if (depth >= 2) bump_reason_side_lit(q, depth-1);
    }
}

void Searcher::bump_reason_side_lits()
{
    assert(to_clear.empty());
    for (const Lit l: learnt_clause) {
        seen[l.var()] = 1;
        to_clear.push_back(l);
    }
    const uint32_t depth = conf.bump_reason_depth
        + rst.stable;
    for (const Lit l: learnt_clause) bump_reason_side_lit(l, depth);
    for (const Lit l: to_clear) seen[l.var()] = 0;
    to_clear.clear();
}

template<bool inprocess>
void Searcher::analyze_conflict(
    const PropBy confl,
    uint32_t& out_btlevel,
    uint32_t& glue,
    [[maybe_unused]] uint32_t& glue_before_minim,
    [[maybe_unused]] uint32_t& size_before_minim
) {
    //Set up environment

    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    antec_data.clear();
    #endif

    learnt_clause.clear();
    chain.clear();
    lrat.clear();
    assert(to_clear.empty());
    implied_by_learnts.clear();
    assert(decision_level() > 0);

    create_learnt_clause<inprocess>(confl);
    if (otfs_driving) {
        //no clause was learnt, the strengthened clause drives
        if (!inprocess && branch_strategy == branch::vmtf) {
            std::sort(implied_by_learnts.begin(),
                      implied_by_learnts.end(),
                      vmtf_bump_sort(vmtf_btab));
            for (const auto& v: implied_by_learnts) vmtf_bump_queue(v);
        }
        implied_by_learnts.clear();
        out_btlevel = 0;
        glue = 0;
        glue_before_minim = 0;
        size_before_minim = 0;
        return;
    }
    stats.litsRedNonMin += learnt_clause.size();
#if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    glue_before_minim = calc_glue(learnt_clause);
    size_before_minim = learnt_clause.size();
#else
    glue_before_minim = 0; //to silence warnings
    size_before_minim = 0; //to silence warnings

#endif
    minimize_learnt_clause<inprocess>();
    stats.litsRedFinal += learnt_clause.size();

    glue = calc_glue(learnt_clause);
    print_fully_minimized_learnt_clause();

    if (glue <= conf.max_glue_more_minim) {
        bool doit = false;
        if (conf.doMinimRedMoreMore == 1 && learnt_clause.size() <= conf.max_size_more_minim) {
            doit = true;
        }
        if (conf.doMinimRedMoreMore == 2 && learnt_clause.size() > conf.max_size_more_minim) {
            doit = true;
        }
        if (conf.doMinimRedMoreMore == 3) {
            doit = true;
        }
        if (doit) {
            minimise_redundant_more_more(learnt_clause);
            glue = calc_glue(learnt_clause);
        }
    }


    if (!inprocess && conf.bump_reason_depth > 0 && branch_strategy != branch::rand) {
        bump_reason_side_lits();
    }

    out_btlevel = find_backtrack_level_of_learnt();
    if (!inprocess) {
        switch(branch_strategy) {
            case branch::vsids:
                break;

            case branch::vmtf:
                std::sort(implied_by_learnts.begin(),
                          implied_by_learnts.end(),
                          vmtf_bump_sort(vmtf_btab));

                for (const auto& v: implied_by_learnts) vmtf_bump_queue(v);
                implied_by_learnts.clear();
                break;

            default:
                break;
        }
    }
    sumConflictClauseLits += learnt_clause.size();
}

bool Searcher::litRedundant(const Lit p, uint32_t abstract_levels)
{
    #ifdef DEBUG_LITREDUNDANT
    cout << "c " << __func__ << " called" << endl;
    #endif

    analyze_stack.clear();
    analyze_stack.push(p);

    size_t top = to_clear.size();
    const size_t chain_rsn_top = lrat.reasons.size();
    const size_t chain_unit_top = lrat.units.size();
    while (!analyze_stack.empty()) {
        #ifdef DEBUG_LITREDUNDANT
        cout << "At point in litRedundant: " << analyze_stack.top() << endl;
        #endif

        Lit p_analyze = analyze_stack.top();
        const PropBy reason = var_data[analyze_stack.top().var()].reason;
        PropByType type = reason.getType();
        analyze_stack.pop();

        //Must have a reason
        assert(!reason.isnullptr());

        size_t size;
        Lit* lits = nullptr;
        int32_t ID = 0;
        switch (type) {
            case clause_t: {
                Clause* cl = cl_alloc.ptr(reason.get_offset());
                lits = cl->begin();
                size = cl->size()-1;
                ID = cl->stats.id;
                break;
            }

            case xor_t: {
                auto cl = get_xor_reason(reason, ID);
                lits = cl->data();
                size = cl->size()-1;
                break;
            }

            case bnn_t: {
                assert(!frat->enabled());
                vector<Lit>* cl = get_bnn_reason(bnns[reason.getBNNidx()],
                    Lit(p_analyze.var(), value(p_analyze.var()) == l_False));
                lits = cl->data();
                size = cl->size()-1;
                break;
            }

            case binary_t:
                size = 1;
                ID = reason.get_id();
                break;

            case null_clause_t:
            default: release_assert(false);
        }
        if (frat->enabled()) {
            lrat.reasons.push_back({var_data[p_analyze.var()].sublevel, ID});
        }

        for (size_t i = 0
            ; i < size
            ; i++
        ) {
            Lit p2;
            switch (type) {
                case xor_t:
                case bnn_t:
                case clause_t:
                    p2 = lits[i+1];
                    break;

                case binary_t:
                    p2 = reason.lit2();
                    break;

                case null_clause_t:
                default:
                    release_assert(false);
            }
            stats.recMinimCost++;

            if (var_data[p2.var()].level == 0) {
                if (frat->enabled()) {
                    assert(unit_cl_IDs[p2.var()] != 0);
                    lrat.units.push_back(unit_cl_IDs[p2.var()]);
                }
                continue;
            }

            if (!seen[p2.var()]) {
                if (!var_data[p2.var()].reason.isnullptr()
                    && (abstractLevel(p2.var()) & abstract_levels) != 0
                ) {
                    seen[p2.var()] = 1;
                    analyze_stack.push(p2);
                    to_clear.push_back(p2);
                } else {
                    //Return to where we started before function executed
                    for (size_t j = top; j < to_clear.size(); j++) {
                        seen[to_clear[j].var()] = 0;
                    }
                    to_clear.resize(top);
                    lrat.reasons.resize(chain_rsn_top);
                    lrat.units.resize(chain_unit_top);
                    return false;
                }
            }
        }
    }
    return true;
}
template void Searcher::analyze_conflict<true>(const PropBy confl
    , uint32_t& out_btlevel
    , uint32_t& glue
    , uint32_t& glue_before_minim
    , uint32_t& size_before_minim
);
template void Searcher::analyze_conflict<false>(const PropBy confl
    , uint32_t& out_btlevel
    , uint32_t& glue
    , uint32_t& glue_before_minim
    , uint32_t& size_before_minim
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
    for (uint32_t i = 0; i != B.size(); i++) seen[B[i].toInt()] = 0;

    return ret;
}

void Searcher::analyze_final_confl_with_assumptions(const Lit p, vector<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push_back(p);

    if (decision_level() == 0) {
        return;
    }

    //It's been set at level 0. The seen[] may not be large enough to do
    //seen[p.var()] -- we might have mem-saved that
    if (var_data[p.var()].level == 0) {
        return;
    }

    seen[p.var()] = 1;

    assert(!trail_lim.empty());
    for (int64_t i = (int64_t)trail.size() - 1; i >= (int64_t)trail_lim[0]; i--) {
        const uint32_t x = trail[i].lit.var();
        if (seen[x]) {
            const PropBy reason = var_data[x].reason;
            if (reason.isnullptr()) {
                assert(var_data[x].level > 0);
                out_conflict.push_back(~trail[i].lit);
            } else {
                int32_t ID;
                switch(reason.getType()) {
                    case clause_t : {
                        const Clause& cl = *cl_alloc.ptr(reason.get_offset());
                        ID = cl.stats.id;
                        assert(value(cl[0]) == l_True);
                        for(const Lit lit: cl) {
                            if (var_data[lit.var()].level > 0) {
                                seen[lit.var()] = 1;
                            }
                        }
                        break;
                    }

                    case bnn_t : {
                        vector<Lit>* cl = get_bnn_reason(bnns[reason.getBNNidx()], lit_Undef);
                        for(const Lit lit: *cl) {
                            if (var_data[lit.var()].level > 0)seen[lit.var()] = 1;
                        }
                        break;
                    }

                    case binary_t: {
                        const Lit lit = reason.lit2();
                        if (var_data[lit.var()].level > 0) seen[lit.var()] = 1;
                        ID = reason.get_id();
                        break;
                    }

                    case xor_t: {
                        auto cl = get_xor_reason(reason, ID);
                        assert(value((*cl)[0]) == l_True);
                        for(const Lit lit: *cl) {
                            if (var_data[lit.var()].level > 0) seen[lit.var()] = 1;
                        }
                        break;
                    }

                    case null_clause_t: release_assert(false);
                }
            }
            seen[x] = 0;
        }
    }
    seen[p.var()] = 0;

}

void Searcher::update_assump_conflict_to_orig_outer(vector<Lit>& out_conflict) {
    if (assumptions.empty()) return;

    vector<pair<Lit,Lit>> inter_assumptions;
    for(const Lit p: assumptions) {
        Lit p2 = solver->var_replacer->get_lit_replaced_with_outer(p);
        p2 = solver->map_outer_to_inter(p2);
        inter_assumptions.push_back(std::make_pair(p, p2));
    }

    // Notice that we need to sort by the internal lit but since we are looking for the
    // opposite of the lit in out_conflict, we need to sort by the negated literal for things
    // to line up
    std::sort(inter_assumptions.begin(), inter_assumptions.end(),
            [](const pair<Lit,Lit>& a, const pair<Lit,Lit>& b){return (~a.second)<(~b.second);});

    std::sort(out_conflict.begin(), out_conflict.end());
    assert(out_conflict.size() <= assumptions.size());
    //They now are in the order where we can go through them linearly

    uint32_t at_assump = 0;
    uint32_t j = 0;
    for(size_t i = 0; i < out_conflict.size(); i++) {
        Lit lit = out_conflict[i];
        while(lit != ~inter_assumptions[at_assump].second) {
            at_assump++;
            assert(at_assump < inter_assumptions.size()
                    && "final conflict contains literals that are not from the assumptions!");
        }
        assert(lit == ~inter_assumptions[at_assump].second);

        //in case of symmetry breaking, we can be in trouble
        //then, the orig_outside is actually lit_Undef
        //in these cases, the symmetry breaking literal needs to be taken out
        if (!var_data[inter_assumptions[at_assump].second.var()].is_bva) {
            //Update to correct outside lit
            out_conflict[j++] = ~inter_assumptions[at_assump].first;
        }
    }
    out_conflict.resize(j);
}

void Searcher::print_order_heap()
{
    switch(branch_strategy) {
        case branch::vsids:
            cout << "vsids heap size: " << order_heap_vsids.size() << endl;
            cout << "vsids acts: ";
            for(auto x: var_act_vsids) {
                cout << std::setprecision(12) << x << " ";
            }
            cout << endl;
            cout << "VSIDS order heap: " << endl;
            order_heap_vsids.print_heap();
            break;

        case branch::rand:
            cout << "rand heap size: " << order_heap_rand.size() << endl;
            cout << "rand order heap: " << endl;
            order_heap_rand.print_heap();
            break;

        case branch::vmtf:
            cout << "vmtf order printing not implemented yet." << endl;
            break;
    }
}

// Disables a single matrix at dec level 0, re-attaching its XORs as
// plain XOR clauses. The matrix object is kept so its watches can be
// removed lazily by gauss_jordan_elim().
bool Searcher::disable_gauss_matrix(const uint32_t i)
{
    assert(decision_level() == 0);
    auto& gqd = gqueuedata[i];
    assert(!gqd.disabled);
    gmatrices[i]->delete_reasons();
    gqd.disabled = true;
    gauss_disabled_this_solve++;
    gmatrices[i]->move_back_xor_clauses();
    return okay();
}

void Searcher::check_need_gauss_jordan_disable()
{
    for(uint32_t i = 0; i < gqueuedata.size(); i++) {
        auto& gqd = gqueuedata[i];
        if (gqd.disabled) continue;

        if (conf.gaussconf.autodisable && gmatrices[i]->must_disable(gqd)) {
            //XORs are detached into the matrix, so disable at dec level 0
            gqd.disable_pending = true;
            gauss_disable_pending = true;
            params.must_stop = true;
        }
    }
}

//Level-0 work is pending, restart must go to level 0
bool Searcher::must_do_level0_work() const
{
    if (fast_backw.fast_backw_on || !assumptions.empty()) return true;
    if (!xorclauses.empty() || !gmatrices.empty() || !bnns.empty()) return true;
    if (gauss_disable_pending) return true;
    if (solver->datasync->enabled()) return true;
    //branch strategy switch needs an empty trail, heaps are not rebuilt
    if (pick_branch_strategy() != branch_strategy) return true;
    if (!conf.never_stop_search) {
        if (conf.do_distill_clauses && sum_conflicts > next_cls_distill) return true;
        if (conf.do_full_probe && sum_conflicts > next_full_probe) return true;
        if (conf.do_distill_bin_clauses && sum_conflicts > next_bins_distill) return true;
        if (conf.do_distill_clauses && sum_conflicts > next_sub_str_with_bin) return true;
        if (conf.doStrSubImplicit && sum_conflicts > next_str_impl_with_impl) return true;
        if (conf.doIntreeProbe && conf.doFindAndReplaceEqLits && sum_conflicts > next_intree)
            return true;
    }
    if (rephasing()) return true;
    return false;
}

//Marijn Heule's reuse trail on restart, as in CaDiCaL
uint32_t Searcher::reuse_trail_level()
{
    if (decision_level() == 0) return 0;
    const bool use_vsids = branch_strategy == branch::vsids;

    uint32_t next = var_Undef;
    if (use_vsids) {
        while (!order_heap_vsids.empty()) {
            const uint32_t v = order_heap_vsids[0];
            if (value(v) == l_Undef) { next = v; break; }
            order_heap_vsids.removeMin();
        }
    } else if (branch_strategy == branch::vmtf) {
        next = vmtf_pick_var();
    } else {
        return 0;
    }
    if (next == var_Undef) return 0;

    uint32_t res = 0;
    while (res < decision_level()) {
        const uint32_t v = trail[trail_lim[res]].lit.var();
        //with chrono BT this slot may not be a real decision
        if (var_data[v].level != res+1 || !var_data[v].reason.isnullptr()) break;
        const bool keep = use_vsids
            ? var_act_vsids[v] >= var_act_vsids[next]
            : vmtf_btab[v] >= vmtf_btab[next];
        if (!keep) break;
        res++;
    }
    return res;
}

lbool Searcher::search()
{
    assert(ok);
    #ifdef SLOW_DEBUG
    check_no_zero_ID_bins();
    check_no_duplicate_lits_anywhere();
    check_order_heap_sanity();
    #endif
    const double my_time = cpu_time();

    //Stats reset & update
    stats.numRestarts++;
    restarts_in_mode[rst.stable]++;
    hist.clear();
    hist.reset_glueHist_size(conf.shortTermHistorySize);

    assert(solver->prop_at_head());

    //Loop until restart or finish (SAT/UNSAT)
    PropBy confl;
    lbool search_ret = l_Undef;

    while (!params.must_stop
        || !confl.isnullptr() //always finish the last conflict
    ) {
        confl = PropBy();
        if (!solver->okay()) {
            assert(!frat->enabled() || unsat_cl_ID != 0);
            search_ret = l_False;
            goto end;
        }
        confl = propagate<false>();
        no_conflict_until = confl.isnullptr() ? trail.size() :
            (decision_level() == 0 ? 0 : trail_lim[decision_level()-1]);
        if (!confl.isnullptr()) {
            #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
            hist.trailDepthHist.push(trail.size());
            #endif
            hist.trailDepthHistLonger.push(trail.size());
            if (!handle_conflict(confl)) {
                assert(!frat->enabled() || unsat_cl_ID != 0);
                search_ret = l_False;
                goto end;
            }
            check_need_restart();
            check_need_gauss_jordan_disable();
        } else {
            assert(ok);
            if (decision_level() == 0) {
                SLOW_DEBUG_DO(for(const auto& bnn: bnns) if (bnn) assert(solver->check_bnn_sane(*bnn)););
                if (!clean_clauses_if_needed()) {
                    assert(!frat->enabled() || unsat_cl_ID != 0);
                    search_ret = l_False;
                    goto end;
                }
            }
            reduce_db_if_needed();
            lbool dec_ret;
            if (fast_backw.fast_backw_on) dec_ret = new_decision_fast_backw();
            else dec_ret = new_decision<false>();
            if (dec_ret != l_Undef) {
                search_ret = dec_ret;
                goto end;
            }
        }
    }
    rst.lim_restart = sum_conflicts + conf.restartint;

    {
        uint32_t reuse_lev = 0;
        if (conf.do_restart_reuse_trail && !must_do_level0_work()) {
            reuse_lev = reuse_trail_level();
        }
        cancel_until(reuse_lev);
    }
    confl = propagate<false>();
    if (!confl.isnullptr() && decision_level() > 0) {
        //reused trail turned out bad, do a full restart
        cancel_until(0);
        confl = propagate<false>();
    }
    if (!confl.isnullptr() || !solver->datasync->syncData()) {
        assert(!frat->enabled() || unsat_cl_ID != 0);
        ok = false;
        search_ret = l_False;
        goto end;
    }
    assert(solver->prop_at_head());
    assert(search_ret == l_Undef);
    if (gauss_disable_pending) {
        gauss_disable_pending = false;
        for(uint32_t i = 0; i < gqueuedata.size(); i++) {
            if (!gqueuedata[i].disable_pending) continue;
            gqueuedata[i].disable_pending = false;
            if (!disable_gauss_matrix(i)) {
                search_ret = l_False;
                goto end;
            }
        }
    }
    SLOW_DEBUG_DO(check_no_zero_ID_bins());
    SLOW_DEBUG_DO(check_no_duplicate_lits_anywhere());
    SLOW_DEBUG_DO(assert(check_order_heap_sanity()));

    end:
    print_restart_stat();
    dump_search_loop_stats(my_time);
    return search_ret;
}

void Searcher::dump_search_sql(const double my_time)
{
    if (solver->sql_stats) {
        solver->sql_stats->time_passed_min(
            solver
            , "search"
            , cpu_time()-my_time
        );
    }
}

/**
@brief Picks a new decision variable to branch on

@returns l_Undef if it should restart instead. l_False if it reached UNSAT
         (through simplification)
*/
template<bool inprocess>
lbool Searcher::new_decision() {
    SLOW_DEBUG_DO(assert(solver->prop_at_head()));
    Lit next = lit_Undef;
    while (decision_level() < assumptions.size()) {
        Lit p = solver->assumptions[solver->decision_level()];
        p = solver->var_replacer->get_lit_replaced_with_outer(p);
        p = solver->map_outer_to_inter(p);
        SLOW_DEBUG_DO(assert(var_data[p.var()].removed == Removed::none));

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
        sumDecisions++;
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    new_decision_level();
    enqueue<inprocess>(next);

    return l_Undef;
}

void Searcher::update_history_stats(
    size_t backtrack_level,
    uint32_t glue
) {
    assert(decision_level() > 0);

    //short-term averages
    hist.branchDepthHist.push(decision_level());
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    hist.backtrackLevelHist.push(backtrack_level);
    hist.branchDepthHistQueue.push(decision_level());
    hist.numResolutionsHist.push(antec_data.num());
    #endif
    hist.branchDepthDeltaHist.push(decision_level() - backtrack_level);
    hist.conflSizeHist.push(learnt_clause.size());
    hist.trailDepthDeltaHist.push(trail.size() - trail_lim[backtrack_level]);

    //long-term averages
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    hist.numResolutionsHistLT.push(antec_data.num());
    const uint32_t overlap = antec_data.sum_size()-(antec_data.num()-1)-learnt_clause.size();
    hist.antec_data_sum_sizeHistLT.push(antec_data.sum_size());
    hist.overlapHistLT.push(overlap);
    #endif
    hist.backtrackLevelHistLT.push(backtrack_level);
    hist.conflSizeHistLT.push(learnt_clause.size());
    hist.trailDepthHistLT.push(trail.size());
    hist.glueHistLT.push(glue);
    hist.glueHist.push(glue);

    //restart scheduling
    rst.cur.fast.update(glue);
    rst.cur.slow.update(glue);

    //Global stats from cnf.h
    sumClLBD += glue;
    sumClSize += learnt_clause.size();
}

template<bool inprocess>
void Searcher::attach_and_enqueue_learnt_clause(
    Clause* cl, const uint32_t level, const bool enq,
    const uint64_t ID)
{
    switch (learnt_clause.size()) {
        case 0: release_assert(false);
        case 1:
            //Unit learnt
            stats.learntUnits++;
            if (enq) {
                assert(level == 0);
                uint32_t v = learnt_clause[0].var();
                if (frat->enabled()) {
                    assert(unit_cl_IDs[v] == 0);
                    assert(unit_cl_XIDs[v] == 0);
                    assert(ID != 0);
                    unit_cl_IDs[v] = ID;
                    const auto xid = ++clauseXID;
                    if (!frat->incremental())
                      *frat << implyxfromcls << xid << learnt_clause[0] << fratchain << ID << fin;
                    unit_cl_XIDs[v] = xid;
                }
                enqueue<false>(learnt_clause[0], level, PropBy(), false);
            }
            break;
        case 2:
            //Binary learnt
            stats.learntBins++;
            //solver->datasync->signalNewBinClause(learnt_clause);
            solver->attach_bin_clause(learnt_clause[0], learnt_clause[1], true, ID, enq);
            if (enq) enqueue<false>(learnt_clause[0], level, PropBy(learnt_clause[1], true, ID));
            break;

        default:
            //Long learnt
            stats.learntLongs++;
            solver->attachClause(*cl, enq);
            if (enq) enqueue<false>(learnt_clause[0], level, PropBy(cl_alloc.get_offset(cl)));
            #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
            bump_cl_act<inprocess>(cl);
            #endif

            #ifdef STATS_NEEDED
            red_stats_extra[cl->stats.extra_pos].antec_data = antec_data;
            #endif

            break;
    }
}


#ifdef STATS_NEEDED
void Searcher::dump_sql_clause_data(
    const uint32_t glue
    , const uint32_t size
    , const uint32_t glue_before_minim
    , const uint32_t size_before_minim
    , const uint32_t old_decision_level
    , const uint64_t clid
    , const bool is_decision
) {
    solver->sql_stats->clause_stats(
        solver
        , clid
        , restartID
        , glue
        , glue_before_minim
        , size
        , size_before_minim
        , decision_level()
        , antec_data
        , old_decision_level
        , trail.size()
        , (int)rst.stable
        , hist
        , is_decision
    );
}
#endif

#ifdef FINAL_PREDICTOR
//As sqlitestats.cpp's clause_stats(), kept in memory for the predictor
void Searcher::set_clause_data(
    Clause* cl
    , const uint32_t orig_glue
    , const uint32_t glue_before_minim
    , const uint32_t size_before_minim
    , const uint32_t old_decision_level
) {
    assert(cl->red());
    auto& stats_extra = red_stats_extra[cl->stats.extra_pos];

    stats_extra.num_overlap_literals = antec_data.sum_size()-(antec_data.num()-1)-cl->size();
    stats_extra.size_before_minim = size_before_minim;
    stats_extra.decision_level = old_decision_level;
    stats_extra.learnt_rst_type = rst.stable;
    stats_extra.antecedents_longIrred = antec_data.longIrred;
    stats_extra.antecedents_longRed = antec_data.longRed;
    stats_extra.trailDepthHistLT_avg = hist.trailDepthHistLT.avg();
    stats_extra.conflSizeHistLT_avg = hist.conflSizeHistLT.avg();
    stats_extra.antec_data_sum_sizeHistLT_avg = hist.antec_data_sum_sizeHistLT.avg();
    stats_extra.branchDepthHistQueue_avg = hist.branchDepthHistQueue.avg_nocheck();
    stats_extra.trailDepthHist_avg = hist.trailDepthHist.avg_nocheck();


    stats_extra.glueHist_longterm_avg = hist.glueHist.getLongtTerm().avg();
    stats_extra.glueHist_avg = hist.glueHist.avg_nocheck();
    stats_extra.trail_depth_level = trail.size();
    stats_extra.glue_before_minim = glue_before_minim;
    stats_extra.overlapHistLT_avg = hist.overlapHistLT.avg();
    stats_extra.num_total_lits_antecedents = antec_data.sum_size();
    stats_extra.num_antecedents = antec_data.num();
    stats_extra.numResolutionsHistLT_avg =  hist.numResolutionsHistLT.avg();
    stats_extra.conflSizeHist_avg = hist.conflSizeHist.avg();
    stats_extra.glueHistLT_avg = hist.glueHistLT.avg();
    stats_extra.antecedents_binred = antec_data.binRed;
    stats_extra.antecedents_binIrred = antec_data.binIrred;

    stats_extra.orig_glue = orig_glue;
//     stats_extra.conflSizeHistLT_avg = hist.conflSizeHistLT.avg();
//     stats_extra.branchDepthHistQueue_avg =  hist.branchDepthHistQueue.avg_nocheck();

}
#endif


Clause* Searcher::handle_last_confl(
    const uint32_t glue,
    [[maybe_unused]] const uint32_t old_decision_level,
    [[maybe_unused]] const uint32_t glue_before_minim,
    [[maybe_unused]] const uint32_t size_before_minim,
    [[maybe_unused]] const bool is_decision,
    int32_t& ID
) {
    *frat << __PRETTY_FUNCTION__ << " begin\n";
    #ifdef STATS_NEEDED
    bool to_track = false;
    const double myrnd = ((double)rnd_uint(solver->mtrand,100000))/100000.0;
    //Unfortunately, we have to change the ratio data dumped as time goes on
    //or we run out of space on CNFs that take millions(!) of conflicts
    //to solve, such as e_rphp035_05.cnf
    double decaying_ratio = (8000.0*1000.0)/((double)sum_conflicts+1);
    if (decaying_ratio > 1.0) {
        decaying_ratio = 1.0;
    } else {
        //Make it more-than-linearly less
        decaying_ratio = ::pow(decaying_ratio, 1.1);
    }
    if (learnt_clause.size() > 2 && myrnd <= (conf.dump_individual_cldata_ratio*decaying_ratio)) {
        to_track = true;
    }
    #endif

    Clause* cl;
    ID = ++clause_id;
    if (frat->enabled()) {
        *frat << add << ID << learnt_clause;
        add_chain();
        *frat << fin;
    }

    if (learnt_clause.size() <= 2) {
        cl = nullptr;
    } else {
        cl = cl_alloc.Clause_new(learnt_clause
            , sum_conflicts
            , ID
        );
        cl->isRed = true;
        cl->stats.glue = glue;
        cl->stats.id = ID;
        #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
        red_stats_extra.push_back(ClauseStatsExtra());
        cl->stats.extra_pos = red_stats_extra.size()-1;
        auto& ext_stats = red_stats_extra[cl->stats.extra_pos];
        ext_stats.introduced_at_conflict = sum_conflicts;
        ext_stats.orig_glue = glue;
        ext_stats.orig_size = cl->size();
        #endif
        #ifdef STATS_NEEDED
        cl->stats.is_tracked = to_track;
        if (cl->stats.is_tracked) ext_stats.orig_ID = ID;
        if (to_track && sql_stats) sql_stats->update_id(ID, ID); // this is how fix_up_frat knows it's tracked
        #endif
        cl->stats.activity = 0.0f;
        ClOffset offset = cl_alloc.get_offset(cl);

        #ifdef STATS_NEEDED
        cl->stats.locked_for_data_gen = to_track &&
            (double)rnd_uint(solver->mtrand,100000)/100000.0 < conf.lock_for_data_gen_ratio;
        #endif

        //as kissat's learn: every tier starts with a full 'used' life
        cl->stats.keep = 0;
        cl->stats.used = CL_MAX_USED;

        cl->stats.which_red_array = 0;
        solver->long_red_cls[0].push_back(offset);
        if (conf.eager_subsume) {
            eager_subsume_last_learnt(*cl);
            last_learnt[last_learnt_at++ % 4] = LastLearnt{offset, cl->stats.id};
        }
    }

    #ifdef STATS_NEEDED
    if (solver->sql_stats
        && frat
        && conf.dump_individual_restarts_and_clauses
        && to_track
    ) {
        assert(cl); //we only dump non-binaries to SQL
        dump_this_many_cldata_in_stream--;
        dump_sql_clause_data(
            glue
            , learnt_clause.size()
            , glue_before_minim
            , size_before_minim
            , old_decision_level
            , ID
            , is_decision
        );
    }
    #endif

    if (cl) {
        #ifdef FINAL_PREDICTOR
        set_clause_data(cl, glue, glue_before_minim, size_before_minim, old_decision_level);
        #endif
        cl->stats.is_decision = is_decision;
    }

    *frat << __PRETTY_FUNCTION__ << " end\n";
    return cl;
}

//CaDiCaL's 'chronoreusetrail': backtrack only to the level of the best var above jump
uint32_t Searcher::chrono_reuse_trail_level(const uint32_t jump, const uint32_t max_level)
{
    if (jump >= max_level) return jump;
    const bool use_vsids = branch_strategy == branch::vsids;
    if (!use_vsids && branch_strategy != branch::vmtf) return jump;

    uint32_t best_var = var_Undef;
    uint32_t best_pos = 0;
    for (uint32_t i = trail_lim[jump]; i < trail.size(); i++) {
        const uint32_t v = trail[i].lit.var();
        if (best_var != var_Undef) {
            const bool better = use_vsids
                ? var_act_vsids[v] > var_act_vsids[best_var]
                : vmtf_btab[v] > vmtf_btab[best_var];
            if (!better) continue;
        }
        best_var = v;
        best_pos = i;
    }
    if (best_var == var_Undef) return jump;

    uint32_t res = jump;
    while (res < max_level && trail_lim[res] <= best_pos) res++;
    return res;
}

bool Searcher::handle_conflict(PropBy confl)
{
    stats.conflicts++;
    hist.num_conflicts_this_restart++;
    sum_conflicts++;
    confl_in_mode[rst.stable]++;
    for(uint32_t i = 0; i < long_red_cls.size(); i++)  longRedClsSizes[i] += long_red_cls[i].size();
    params.confl_this_rst++;

    ConflictData data = find_conflict_level(confl);
    if (data.nHighestLevel == 0) {
        verb_print(10, "find_conflict_level() gives 0, so UNSAT for whole formula. "
                "decLevel: " << decision_level());
        if (unsat_cl_ID == 0) {
            if (frat->enabled()) build_level0_confl_chain(confl);
            *frat << add << ++clause_id;
            add_chain();
            *frat << fin;
            set_unsat_cl_id(clause_id);
        }
        solver->ok = false;
        return false;
    }

    uint32_t backtrack_level;
    uint32_t glue;
    uint32_t glue_before_minim;
    uint32_t size_before_minim;
    analyze_conflict<false>(
        confl
        , backtrack_level  //return backtrack level here
        , glue             //return glue here
        , glue_before_minim         //return glue before minimization here
        , size_before_minim         //return glue before minimization here
    );
    if (otfs_driving) {
        //OTFS strengthened an existing clause into the asserting clause
        Clause* cl = cl_alloc.ptr(otfs_driving_cl);
        assert(var_data[(*cl)[0].var()].level > var_data[(*cl)[1].var()].level);
        const uint32_t new_btlevel = var_data[(*cl)[1].var()].level;
        cancel_until(new_btlevel);
        assert(value((*cl)[0]) == l_Undef);
        enqueue<false>((*cl)[0], new_btlevel, PropBy(otfs_driving_cl));
        if (branch_strategy == branch::vsids) vsids_decay_var_act();
        decayClauseAct<false>();
        frat_func_end();
        return true;
    }
    solver->datasync->signal_new_long_clause(learnt_clause);

    update_history_stats(backtrack_level, glue);
    uint32_t old_decision_level = decision_level();

    //Add decision-based clause in case it's short
    decision_clause.clear();
    if (conf.do_decision_based_cl
        && learnt_clause.size() > conf.decision_based_cl_min_learned_size
        && decision_level() <= conf.decision_based_cl_max_levels
        && decision_level() >= 2
    ) {
        chain.clear();
        for(int i = (int)trail_lim.size()-1; i >= 0; i--) {
            Lit l = ~trail[trail_lim[i]].lit;
            if (!seen[l.toInt()]) {
                decision_clause.push_back(l);
                seen[l.toInt()] = 1;
            }
        }
        for(Lit l: decision_clause) {
            seen[l.toInt()] = 0;
            assert(var_data[l.var()].reason == PropBy());
        }
    }

    if (frat->enabled()) lrat.to_hints(chain);

    // check chrono backtrack condition
    if (conf.diff_declev_for_chrono > -1
        && xorclauses.empty()
        && gmatrices.empty()
        && bnns.empty()
        && (((int)decision_level() - (int)backtrack_level) >= conf.diff_declev_for_chrono)
    ) {
        chrono_backtrack++;
        cancel_until(data.nHighestLevel -1);
    } else {
        non_chrono_backtrack++;
        uint32_t bt_level = backtrack_level;
        if (conf.do_chrono_reuse_trail
            && conf.diff_declev_for_chrono > -1
            && xorclauses.empty()
            && gmatrices.empty()
            && bnns.empty()
        ) {
            bt_level = chrono_reuse_trail_level(backtrack_level, data.nHighestLevel-1);
        }
        cancel_until(bt_level);
    }

    assert(value(learnt_clause[0]) == l_Undef);
    glue = std::min<uint32_t>(glue, CL_MAX_GLUE);
    int32_t ID;
    *frat << "normal learnt clause\n";
    Clause* cl = handle_last_confl(
        glue,
        old_decision_level,
        glue_before_minim,
        size_before_minim,
        false, // is decision?
        ID
    );
    attach_and_enqueue_learnt_clause<false>(cl, backtrack_level, true, ID);

    //Add decision-based clause
    // TODO FRAT -- this is broken because the reasons because of XOR for the propagations
    // that lead to UNSAT is not asked for. We could definitely do it by asking for
    // all XOR reasons for the propagations that lead to UNSAT
    if (!frat->enabled() && decision_clause.size() > 0) {
        *frat << "decision learnt clause!\n";
        chain.clear();
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
        cl = handle_last_confl(
            learnt_clause.size(), // glue is the number of decisions, i.e. the size of decision clause
            old_decision_level,
            learnt_clause.size(), // minimized glue is the same as glue before minim
            learnt_clause.size(),
            true, // is decision?
            ID
        );
        attach_and_enqueue_learnt_clause<false>(cl, backtrack_level, false, ID);
    }

    if (branch_strategy == branch::vsids) vsids_decay_var_act();
    decayClauseAct<false>();
    frat_func_end();
    return true;
}

void Searcher::resetStats()
{
    startTime = cpu_time();

    //Rest solving stats
    stats.clear();
    prop_stats.clear();
    #ifdef STATS_NEEDED
    lastSQLPropStats = prop_stats;
    lastSQLGlobalStats = stats;
    #endif

    lastCleanZeroDepthAssigns = trail.size();
}

#ifdef STATS_NEEDED
void Searcher::check_calc_satzilla_features(bool force)
{
    if (last_satzilla_feature_calc_confl == 0
        || (last_satzilla_feature_calc_confl + solver->conf.every_pred_reduce) < sum_conflicts
        || force
    ) {
        last_satzilla_feature_calc_confl = sum_conflicts+1;
        if (nVars() > 2
            && long_irred_cls.size() > 1
            && (bin_tri.irred_bins + bin_tri.red_bins) > 1
        ) {
            solver->last_solve_satzilla_feature = solver->calculate_satzilla_features();
        }
    }
}
#endif


void Searcher::print_restart_header()
{
    cout
    << "c"
    << " " << std::setw(4) << "res"
    << " " << std::setw(4) << "pol"
    << " " << std::setw(4) << "bran"
    << " " << std::setw(5) << "nres"
    << " " << std::setw(5) << "conf"
    << " " << std::setw(5) << "freevar"
    << " " << std::setw(5) << "IrrL"
    << " " << std::setw(5) << "IrrB"
    << " " << std::setw(7) << "l/longC"
    << " " << std::setw(7) << "l/allC";

    for(size_t i = 0; i < long_red_cls.size(); i++) {
        cout << " " << std::setw(4) << "RedL" << i;
    }

    cout
    << " " << std::setw(5) << "RedB"
    << " " << std::setw(7) << "l/longC"
    << " " << std::setw(7) << "l/allC"
    << endl;
}

void Searcher::print_restart_stat_line()
{
    if (restart_lines_since_header == 0) print_restart_header();
    restart_lines_since_header = (restart_lines_since_header+1) % 20;

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
    cout << solver->conf.prefix << "rst "
         << " " << std::setw(4) << (rst.stable ? "stb" : "foc")
         << " " << std::setw(4) << last_rephase
         << " " << std::setw(4) << branch_strategy_str_short
         << " " << std::setw(5) << sumRestarts();

    if (sum_conflicts >  20000) {
        cout << " " << std::setw(4) << sum_conflicts/1000 << "K";
    } else {
        cout << " " << std::setw(5) << sum_conflicts;
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
    PropStats thisPropStats = prop_stats - lastSQLPropStats;
    SearchStats thisStats = stats - lastSQLGlobalStats;
    solver->sql_stats->restart(
        restartID
        , rst.stable
        , thisPropStats
        , thisStats
        , solver
        , this
    );
    lastSQLPropStats = prop_stats;
    lastSQLGlobalStats = stats;
}
#endif

void Searcher::print_restart_stat()
{
    if (!conf.verbosity || conf.print_all_restarts) return;

    const bool regime_changed =
        rst.stable != last_print_stable
        || last_rephase != last_print_rephase
        || branch_strategy_str_short != last_print_branch;

    // Reports get sparser as the run goes on, otherwise an hours-long solve
    // emits one line per restart, i.e. hundreds of thousands of them.
    const uint64_t every = std::max<uint64_t>(
        conf.print_restart_line_every_n_confl, sum_conflicts/64);

    if (!regime_changed && sum_conflicts < lastRestartPrint + every) return;

    last_print_stable = rst.stable;
    last_print_rephase = last_rephase;
    last_print_branch = branch_strategy_str_short;
    lastRestartPrint = sum_conflicts;
    print_restart_stat_line();
}

void Searcher::reduce_db_if_needed()
{
    auto& rdb = *solver->reduceDB;
    if (rdb.lim_reduce == 0) rdb.lim_reduce = sum_conflicts + conf.reduceint;
    if (conf.reduce
        && !long_red_cls[0].empty()
        && sum_conflicts >= rdb.lim_reduce
    ) {
        compute_tier_limits();
        rdb.handle_reduce((uint32_t)rst.stable);
        cl_alloc.consolidate(solver);
    }
}

bool Searcher::clean_clauses_if_needed()
{
    #ifdef SLOW_DEBUG
    assert(decision_level() == 0);
    assert(qhead == trail.size());
    #endif

    const size_t newZeroDepthAss = trail.size() - lastCleanZeroDepthAssigns;
    if (newZeroDepthAss > 0
        && simpDB_props < 0
        && newZeroDepthAss > ((double)nVars()*0.05)
    ) {
        if (conf.verbosity >= 2) {
            cout << conf.prefix << "newZeroDepthAss : " << newZeroDepthAss
            << " -- "
            << (double)newZeroDepthAss/(double)nVars()*100.0
            << " % of active vars"
            << endl;
        }
        lastCleanZeroDepthAssigns = trail.size();
        if (!solver->clause_cleaner->remove_and_clean_all()) {
            return false;
        }

        cl_alloc.consolidate(solver);
        simpDB_props = (lit_stats.red_lits + lit_stats.irred_lits)<<5;
    }

    return okay();
}

void Searcher::rebuildOrderHeap() {
    verb_print(1, "[branch] rebuilding order heap for all branchings. Current branching: " <<
        branch_type_to_string(branch_strategy));

    vector<uint32_t> vs;
    vs.reserve(nVars());
    for (uint32_t v = 0; v < nVars(); v++) {
        if (var_data[v].removed != Removed::none
                || (value(v) != l_Undef && var_data[v].level == 0)) continue;
        else vs.push_back(v);
    }

    order_heap_vsids.build(vs);

    order_heap_rand.build(vs);

    rebuildOrderHeapVMTF(vs);
}

void Searcher::rebuildOrderHeapVMTF(vector<uint32_t>& vs)
{
    std::sort(vs.begin(), vs.end(),
              [&](const uint32_t& a, const uint32_t& b) -> bool
        {
            return vmtf_btab[a] < vmtf_btab[b]; //reverse order is needed for enqueue alter
        });

    vmtf_queue = Queue();
    vmtf_btab.clear();
    vmtf_links.clear();
    vmtf_btab.insert(vmtf_btab.end(), nVars(), 0);
    vmtf_links.insert(vmtf_links.end(), nVars(), Link());

    for(auto const& v: vs) vmtf_init_enqueue(v);
}

//CaDiCaL ties branching to the phase: VMTF while focused, VSIDS scores
//while stable. A setup string naming only one strategy fixes it.
CMSat::branch Searcher::pick_branch_strategy() const
{
    const bool has_vsids = conf.branch_strategy_setup.find("vsids") != string::npos;
    const bool has_vmtf = conf.branch_strategy_setup.find("vmtf") != string::npos;
    if (has_vsids && has_vmtf)
        return (rst.stable && conf.do_stabilize) ? branch::vsids : branch::vmtf;
    if (has_vsids) return branch::vsids;
    if (has_vmtf) return branch::vmtf;
    return branch::rand;
}

void Searcher::setup_branch_strategy()
{
    const auto want = pick_branch_strategy();
    if (want == branch_strategy) return;

    const auto old_branch_strategy = branch_strategy;
    branch_strategy = want;
    switch (want) {
        case branch::vsids: branch_strategy_str = "VSIDS"; branch_strategy_str_short = "vs"; break;
        case branch::vmtf: branch_strategy_str = "VMTF"; branch_strategy_str_short = "vmt"; break;
        case branch::rand: branch_strategy_str = "RAND"; branch_strategy_str_short = "rnd"; break;
        default: assert(false);
    }

    verb_print(1, "[branch]"
        <<  " adjusting to: " << branch_type_to_string(branch_strategy)
        <<  " (from: " << branch_type_to_string(old_branch_strategy) << ")"
        << " var_decay:" << var_decay);
}

inline void Searcher::dump_search_loop_stats(double my_time)
{
    #if defined(STATS_NEEDED)
    check_calc_satzilla_features();
    #endif

    dump_search_sql(my_time);
    if (conf.verbosity && conf.print_all_restarts) {
        print_restart_stat_line();
    }
    #ifdef STATS_NEEDED
    if (sql_stats
        && conf.dump_individual_restarts_and_clauses
    ) {
        dump_restart_sql();
    }
    #endif
    restartID++;
}

bool Searcher::must_abort(const lbool status) {
    if (status != l_Undef) {
        return true;
    }

    if (stats.conflicts >= max_confl_per_search_solve_call) return true;
    if (cpu_time() >= conf.maxTime) return true;
    if (solver->must_interrupt_asap()) return true;
    return false;
}

/*
CaDiCaL's rephasing: in an arithmetically increasing conflict interval the saved
phases are reset to the original phase, its inverse, the flipped current phase,
random phases, the best phase seen since the last reset, or the phase local
search came up with. Stable and focused phases follow different schedules.
*/
bool Searcher::rephasing() const
{
    if (!conf.do_rephase) return false;
    if (conf.polarity_mode != PolarityMode::polarmode_automatic) return false;
    return sum_conflicts > lim_rephase;
}

//'O'riginal phase, its 'I'nverse, 'F'lipping the current one, a random one ('#'),
//the 'B'est one seen since the last reset, or the one local search ('W') found
void Searcher::rephase_as(const char type)
{
    switch (type) {
        case 'O': for(auto& v: var_data) v.saved_polarity = conf.phase; break;
        case 'I': for(auto& v: var_data) v.saved_polarity = !conf.phase; break;
        case 'F': for(auto& v: var_data) v.saved_polarity = !v.saved_polarity; break;
        case '#': for(auto& v: var_data) v.saved_polarity = rnd_uint(mtrand, 1); break;
        case 'B': for(auto& v: var_data)
                      if (v.best_polarity_set) v.saved_polarity = v.best_polarity;
                  break;
        case 'W': { SLS sls(solver); sls.run_during_search(); break; }
        default: assert(false);
    }
}

void Searcher::rephase()
{
    assert(decision_level() == 0);
    num_rephased++;

    for(auto& v: var_data) v.target_polarity_set = false;
    target_assigned = 0;

    // The schedules of CaDiCaL's 'rephase': a one-off prefix, then a cycle.
    const bool single = !conf.do_stabilize;
    const bool walk = conf.doSLS && (single || rst.stable || conf.walknonstable);
    const char* prefix;
    const char* cycle;
    if (single) {
        prefix = "";
        cycle = walk ? "IBWFBW#BWOBW" : "IBFB#BOB";
    } else if (rst.stable) {
        prefix = "OI";
        cycle = walk ? "BWOBWI" : "BOBI";
    } else {
        prefix = "F";
        cycle = walk ? "#BWFBW" : "#BFB";
    }

    const size_t count = num_rephased_in[rst.stable]++;
    const size_t plen = strlen(prefix);
    const char type = count < plen ? prefix[count] : cycle[(count-plen) % strlen(cycle)];
    rephase_as(type);

    lim_rephase = sum_conflicts + conf.rephaseint * (num_rephased + 1);
    last_rephase_conflicts = sum_conflicts;
    rephased = type;
    last_rephase = type;

    verb_print(2, "[rephase] " << type << " num: " << num_rephased
        << " next at confl: " << lim_rephase);
}

void Searcher::rephase_if_needed()
{
    assert(okay());
    assert(decision_level() == 0);
    if (rephasing()) rephase();
}

bool Searcher::intree_if_needed()
{
    assert(okay());
    assert(decision_level() == 0);
    bool ret = okay();

    if (!bnns.empty()) conf.do_hyperbin_and_transred = false;
    //Without hyper-bin, intree falls back to propagate<true>(), which runs GJ
    //at every tree level: too expensive, and clashes with its reason re-parenting
    if (!conf.do_hyperbin_and_transred && !gmatrices.empty()) return ret;
    if (conf.doIntreeProbe && conf.doFindAndReplaceEqLits && !conf.never_stop_search &&
        sum_conflicts > next_intree
    ) {

        TimeScope ts(solver->time_tally, "intree-probe");
        auto repl = solver->var_replacer->get_num_replaced_vars();
        if (ret) ret &= solver->intree->intree_probe();
        if (ret) {
            auto repl2 = solver->var_replacer->get_num_replaced_vars();
            // Needed because replaced variables
            if (repl != repl2) rebuildOrderHeap();
        }
        next_intree = sum_conflicts + 65000.0*conf.global_next_multiplier;
    }

    return ret;
}

//Kissat's kissat_compute_and_set_tier_limits: tier1 is the glue by which
//50% of clause uses in this mode are covered, tier2 90%. The configured
//limits hold until enough uses were seen
void Searcher::compute_tier_limits()
{
    if (!conf.dynamic_tiers) return;
    const auto& hist = glue_used_hist[rst.stable];
    uint64_t total = 0;
    for(uint32_t g = 0; g < 65; g++) total += hist[g];
    if (total < 1000) {
        tier1_glue = conf.reducetier1glue;
        tier2_glue = conf.reducetier2glue;
        return;
    }
    uint64_t acc = 0;
    uint32_t t1 = 0, t2 = 0;
    bool t1_set = false;
    for(uint32_t g = 0; g < 65; g++) {
        acc += hist[g];
        if (!t1_set && acc*2 >= total) { t1 = g; t1_set = true; }
        if (acc*10 >= total*9) { t2 = g; break; }
    }
    //tier1 clauses are kept forever here (CaDiCaL's 'keep'), so never
    //raise it: on UTI-20-10p0 it drifted to 4 and the red DB ballooned
    t1 = std::min<uint32_t>(std::max<uint32_t>(t1, 1), conf.reducetier1glue);
    t2 = std::max<uint32_t>(t2, t1);
    if (t1 != tier1_glue || t2 != tier2_glue) {
        verb_print(2, "[tiers] " << (rst.stable ? "stable" : "focused")
            << " tier1: " << tier1_glue << "->" << t1 << " tier2: " << tier2_glue << "->" << t2);
    }
    tier1_glue = t1;
    tier2_glue = t2;
}

//Kissat's eagerly_subsume_last_learned: a subsumed recent learnt clause is
//demoted to a sure tier3 candidate, so the next reduce drops it first
void Searcher::eager_subsume_last_learnt(const Clause& newcl)
{
    for(const Lit l: newcl) seen2[l.toInt()] = 1;
    for(auto& ll: last_learnt) {
        if (ll.off == CL_OFFSET_MAX) continue;
        Clause* c = cl_alloc.ptr(ll.off);
        if (c->freed() || c->get_removed() || !c->red() || c->stats.id != ll.id
            || c->size() <= newcl.size() || c->stats.glue == CL_MAX_GLUE
        ) continue;
        uint32_t needed = newcl.size();
        uint32_t remain = c->size();
        for(const Lit l: *c) {
            if (seen2[l.toInt()] && !--needed) break;
            else if (--remain < needed) break;
        }
        if (needed) continue;
        c->stats.used = 0;
        c->stats.glue = CL_MAX_GLUE;
        ll = LastLearnt();
        eagerly_subsumed++;
    }
    for(const Lit l: newcl) seen2[l.toInt()] = 0;
}

//Kissat's focused/stable/switched/restarts statistics
void Searcher::print_mode_stats() const
{
    const uint64_t confls = confl_in_mode[0] + confl_in_mode[1];
    const uint64_t rsts = restarts_in_mode[0] + restarts_in_mode[1];
    for(uint32_t stable = 0; stable < 2; stable++) {
        cout << conf.prefix << (stable ? "stable  " : "focused ")
            << " conflicts " << std::setw(10) << confl_in_mode[stable]
            << " (" << std::setw(5) << std::fixed << std::setprecision(1)
            << stats_line_percent(confl_in_mode[stable], confls) << "%)"
            << " restarts " << std::setw(8) << restarts_in_mode[stable]
            << " (" << std::setw(5) << stats_line_percent(restarts_in_mode[stable], rsts) << "%)"
            << " confl/restart " << std::setw(8) << std::setprecision(1)
            << float_div(confl_in_mode[stable], restarts_in_mode[stable])
            << endl;
    }
    cout << conf.prefix << "mode switches " << mode_switches
        << " rephased " << num_rephased
        << " eagerly subsumed learnts " << eagerly_subsumed << endl;
}

//Kissat's '[ glue usage ]' table: how often clauses of each glue were
//reasons in conflict analysis, per search mode, with the tier marks
void Searcher::print_glue_usage() const
{
    for(uint32_t stable = 0; stable < 2; stable++) {
        uint64_t total = 0;
        for(uint32_t g = 0; g < 65; g++) total += glue_used_hist[stable][g];
        if (total == 0) continue;
        cout << conf.prefix << "glue usage in " << (stable ? "stable" : "focused")
            << " mode, " << total << " uses (final tier1 glue<=" << tier1_glue
            << " tier2 glue<=" << tier2_glue << ")" << endl;
        uint64_t acc = 0;
        uint32_t rows = 0;
        for(uint32_t g = 0; g < 65 && rows < 20; g++) {
            const uint64_t c = glue_used_hist[stable][g];
            if (c == 0) continue;
            acc += c;
            rows++;
            cout << conf.prefix << (stable ? "stable " : "focused") << " glue "
                << std::setw(2) << g << (g == 64 ? "+" : " ")
                << " used " << std::setw(10) << c
                << " " << std::setw(6) << std::fixed << std::setprecision(2)
                << stats_line_percent(c, total) << "%"
                << " accumulated " << std::setw(6) << stats_line_percent(acc, total) << "%"
                << (g == tier1_glue ? " tier1" : "")
                << (g == tier2_glue ? " tier2" : "")
                << endl;
            if (acc*100 >= total*95) break;
        }
    }
}

void Searcher::mark_hyper_bin_used(const int32_t id)
{
    for(auto& r: hyper_bin_ranges) {
        if (id >= r.start && id < r.end) { r.used[id-r.start] = 1; return; }
    }
}

void Searcher::add_hyper_bin_range(const int32_t start, const int32_t end)
{
    if (start >= end) return;
    hyper_bin_ranges.push_back({start, end, vector<uint8_t>(end-start, 0)});
    next_hyper_bin_clean = sum_conflicts + conf.hyperbin_keep_confl*conf.global_next_multiplier;
}

//Drop every hyper-bin of the recorded ranges that was never a reason in
//conflict analysis. Level 0 only: a deleted bin must not be a live reason
void Searcher::clean_unused_hyper_bins()
{
    if (hyper_bin_ranges.empty()) return;
    assert(decision_level() == 0);
    const double my_time = cpu_time();
    const auto unused = [&](const int32_t id) {
        for(const auto& r: hyper_bin_ranges)
            if (id >= r.start && id < r.end) return !r.used[id-r.start];
        return false;
    };

    uint64_t removed = 0;
    const size_t end = watches.size();
    for (size_t wsLit = 0; wsLit < end; wsLit++) {
        const Lit lit = Lit::toLit(wsLit);
        watch_subarray ws = watches[lit];
        if (ws.empty()) continue;
        Watched* j = ws.begin();
        for (Watched* w = ws.begin(); w != ws.end(); w++) {
            if (w->isBin() && w->red() && unused(w->get_id())) {
                if (lit.toInt() < w->lit2().toInt()) {
                    removed++;
                    *frat << del << w->get_id() << lit << w->lit2() << fin;
                }
                continue;
            }
            *j++ = *w;
        }
        ws.shrink_(ws.end()-j);
    }
    bin_tri.red_bins -= removed;
    uint64_t total = 0;
    for(const auto& r: hyper_bin_ranges) total += r.end - r.start;
    hyper_bins_cleaned += removed;
    hyper_bins_kept += total - removed;
    hyper_bin_ranges.clear();
    verb_print(1, "[hyper-bin-clean] removed: " << removed << " of: " << total
        << " red-bins now: " << bin_tri.red_bins
        << conf.print_times(cpu_time() - my_time));
}

bool Searcher::clean_hyper_bins_if_needed()
{
    assert(decision_level() == 0);
    if (!hyper_bin_ranges.empty() && sum_conflicts > next_hyper_bin_clean) {
        TimeScope ts(solver->time_tally, "hyper-bin-clean");
        clean_unused_hyper_bins();
    }
    return okay();
}

bool Searcher::str_impl_with_impl_if_needed()
{
    assert(okay());
    bool ret = okay();

    if (conf.doStrSubImplicit && sum_conflicts > next_str_impl_with_impl) {
        TimeScope ts(solver->time_tally, "str-impl");
        ret &= solver->dist_impl_with_impl->str_impl_w_impl();
        if (ret) solver->subsumeImplicit->subsume_implicit();
        next_str_impl_with_impl = sum_conflicts + 60000.0*conf.global_next_multiplier;
    }

    return ret;
}

bool Searcher::distill_bins_if_needed() {
    assert(okay());
    bool ret = okay();

    if (conf.do_distill_bin_clauses &&
        sum_conflicts > next_bins_distill)
    {
        TimeScope ts(solver->time_tally, "distill-bins");
        ret = solver->distill_bin_cls->distill();
        next_bins_distill = sum_conflicts + 20000.0*conf.global_next_multiplier
            *solver->distill_bin_cls->sched_backoff();
    }
    return ret;
}

bool Searcher::sub_str_with_bin_if_needed()
{
    assert(okay());
    bool ret = okay();

    //Subsumes and strengthens long clauses with binary clauses
    if (conf.do_distill_clauses && sum_conflicts > next_sub_str_with_bin) {
        TimeScope ts(solver->time_tally, "sub-str-cls-with-bin");
        ret = solver->dist_long_with_impl->distill_long_with_implicit(true);
        next_sub_str_with_bin = sum_conflicts + 25000.0*conf.global_next_multiplier;
    }

    return ret;
}

lbool Searcher::distill_clauses_if_needed()
{
    assert(decision_level() == 0);
    if (conf.do_distill_clauses && sum_conflicts > next_cls_distill) {
        TimeScope ts(solver->time_tally, "distill-cls");
        if (!solver->distill_long_cls->distill_red_and_irred()) return l_False;
        next_cls_distill = sum_conflicts + 15000.0*conf.global_next_multiplier;
    }

    return l_Undef;
}

bool Searcher::full_probe_if_needed()
{
    assert(decision_level() == 0);
    if (conf.do_full_probe && !conf.never_stop_search && sum_conflicts > next_full_probe) {
        full_probe_iter++;
        TimeScope ts(solver->time_tally, "full-probe");
        if (!solver->full_probe(full_probe_iter % 2)) return false;
        next_full_probe = sum_conflicts + 20000.0*conf.global_next_multiplier;
    }

    return okay();
}

lbool Searcher::solve(const uint64_t _max_confls) {
    assert(ok);
    assert(qhead == trail.size());
    max_confl_per_search_solve_call = _max_confls;
    TimeScope ts(solver->time_tally, "search");
    if (fast_backw.fast_backw_on && fast_backw.cur_max_confl == 0) {
        fast_backw.cur_max_confl = sum_conflicts + fast_backw.max_confl;
        fast_backw.start_sumConflicts = sum_conflicts;
    }
    num_search_called++;
    SLOW_DEBUG_DO(check_no_removed_or_freed_cl_in_watch());
    verb_print(6, __func__ << " called");

    gauss_disable_pending = false; //stale, matrices are new
    gauss_disabled_this_solve = 0;
    resetStats();
    lbool status = l_Undef;

    setup_branch_strategy();
    init_restart_sched();
    STATS_DO(check_calc_satzilla_features(true));

    SLOW_DEBUG_DO(assert(fast_backw.fast_backw_on || solver->check_order_heap_sanity()));
    while(stats.conflicts < max_confl_per_search_solve_call && status == l_Undef) {
        //trail may be non-empty due to restart trail reuse
        if (decision_level() == 0) {
            if (!conf.never_stop_search &&
                    (distill_clauses_if_needed() == l_False
                    || !full_probe_if_needed()
                    || !distill_bins_if_needed()
                    || !sub_str_with_bin_if_needed()
                    || !str_impl_with_impl_if_needed()
                    || !intree_if_needed()
                    || !clean_hyper_bins_if_needed())
            ) {
                assert(!frat->enabled() || unsat_cl_ID != 0);
                status = l_False;
                goto end;
            }
            SLOW_DEBUG_DO(assert(solver->check_order_heap_sanity()));
            rephase_if_needed();
        }

        assert(watches.get_smudged_list().empty());
        params.clear();
        params.max_confl_to_do = max_confl_per_search_solve_call-stats.conflicts;
        status = search();
        if (status == l_Undef) {
            setup_branch_strategy();
        }
        SLOW_DEBUG_DO(assert(fast_backw.fast_backw_on || solver->check_order_heap_sanity()));

        if (must_abort(status)) goto end;
    }

    end:
    finish_up_solve(status);
    return status;
}

void Searcher::init_restart_sched()
{
    if (rst.inited) return;
    rst.inited = true;

    rst.cur.fast = EMA(conf.emagluefast);
    rst.cur.slow = EMA(conf.emaglueslow);
    rst.lim_restart = sum_conflicts + conf.restartint;
    lim_rephase = sum_conflicts + conf.rephaseint;
    rst.inc_stabilize = conf.stabilizeint;
    rst.lim_stabilize = sum_conflicts + rst.inc_stabilize;
    if (conf.do_stabilize && conf.reluctantint) {
        rst.reluctant.enable(conf.reluctantint, conf.reluctantmax);
    } else {
        rst.reluctant.disable();
    }
}

//EMAs are per-phase: restore those of the previous same-kind phase
void Searcher::swap_restart_averages()
{
    std::swap(rst.cur, rst.saved);
    if (rst.swapped == 0) {
        rst.cur.fast = EMA(conf.emagluefast);
        rst.cur.slow = EMA(conf.emaglueslow);
    }
    rst.swapped++;
}

//Chanseok Oh's stable/focused phase alternation, as in CaDiCaL
bool Searcher::stabilizing()
{
    if (!conf.do_stabilize) return false;
    if (sum_conflicts >= rst.lim_stabilize) {
        rst.stable = !rst.stable;
        mode_switches++;
        rst.inc_stabilize =
            std::min<uint64_t>(rst.inc_stabilize * conf.stabilizefactor, conf.stabilizemaxint);
        rst.lim_stabilize = sum_conflicts + std::max<uint64_t>(rst.inc_stabilize, 1);
        swap_restart_averages();
        compute_tier_limits();
        verb_print(2, "[restart] "
            << (rst.stable ? "stable" : "focused") << " phase, until confl "
            << rst.lim_stabilize);
    }
    return rst.stable;
}

//Glucose-style: restart if fast glue EMA is a margin above the slow one
bool Searcher::restarting()
{
    if (!conf.do_restart) return false;
    if (decision_level() < assumptions.size() + 2) return false;
    if (stabilizing()) return rst.reluctant;
    if (sum_conflicts <= rst.lim_restart) return false;
    const double margin = (100.0 + conf.restartmargin) / 100.0;
    return rst.cur.fast >= margin * rst.cur.slow;
}

void Searcher::check_need_restart() {
    //It's expensive to check the time all the time
    if ((stats.conflicts & 0xff) == 0xff) {
        if (cpu_time() > conf.maxTime) params.must_stop = true;
        if (must_interrupt_asap())  {
            verb_print(3, "must_interrupt_asap() is set, restartig as soon as possible!");
            params.must_stop = true;
        }
    }

    //CaDiCaL ticks reluctant only during stable phases
    if (rst.stable) rst.reluctant.tick();
    if (restarting()) params.must_stop = true;

    //respect Searcher's limit
    if (params.confl_this_rst > params.max_confl_to_do) {
        verb_print(3, "Over limit of confl for this restart, restarting asap");
        params.must_stop = true;
    }

}

void Searcher::print_solution_varreplace_status() const
{
    for(size_t var = 0; var < nVarsOuter(); var++) {
        if (var_data[var].removed == Removed::replaced
            || var_data[var].removed == Removed::elimed
        ) {
            assert(value(var) == l_Undef || var_data[var].level == 0);
        }

    }
}

void Searcher::print_solution_type(const lbool status) const
{
    if (status == l_True) {
        verb_print(6, "Solution from Searcher is SAT");
    } else if (status == l_False) {
        verb_print(6, "Solution from Searcher is UNSAT");
        verb_print(6, "OK is: " << okay());
    } else {
        verb_print(6, "Solutions from Searcher is UNKNOWN");
    }
}

void Searcher::finish_up_solve(const lbool status) {
    print_solution_type(status);
    if (conf.verbosity >= 2 && status != l_Undef) print_matrix_stats();
    if (gauss_disabled_this_solve) {
        uint32_t active = 0;
        for(const auto& gqd: gqueuedata) active += !gqd.disabled;
        verb_print(1, "[gauss] disabled " << gauss_disabled_this_solve
            << " matrices in this search round, " << active << " of "
            << gqueuedata.size() << " still active");
    }

    if (status == l_True) {
        SLOW_DEBUG_DO(assert(fast_backw.fast_backw_on || solver->check_order_heap_sanity()));
        assert(solver->prop_at_head());
        model = assigns;
        cancel_until(0);
        assert(decision_level() == 0);

        //due to chrono BT we need to propagate once more
        PropBy confl = propagate<false>();
        assert(confl.isnullptr());
        assert(solver->prop_at_head());
        SLOW_DEBUG_DO(print_solution_varreplace_status());
    } else if (status == l_False) {
        if (conflict.size() == 0) {
            ok = false;
        }
        cancel_until(0);
        if (okay()) {
            //due to chrono BT we need to propagate once more
            PropBy confl = propagate<false>();
            assert(confl.isnullptr());
        }
    } else if (status == l_Undef) {
        //trail may be reused, undo that
        if (decision_level() != 0) {
            cancel_until(0);
            PropBy confl = propagate<false>();
            assert(confl.isnullptr());
        }
        assert(decision_level() == 0);
        assert(solver->prop_at_head());
    }

    stats.cpu_time = cpu_time() - startTime;
}

inline Lit Searcher::pickBranchLit() {

    uint32_t v = var_Undef;
    while(true) {
        switch (branch_strategy) {
            case branch::vsids:
                v = pick_var_vsids();
                break;
            case branch::vmtf:
                v = vmtf_pick_var();
                break;
            case branch::rand: {
                v = order_heap_rand.get_random_element(mtrand);
                while (v != var_Undef && value(v) != l_Undef) {
                    v = order_heap_rand.get_random_element(mtrand);
                }
                break;
            }
            default: {
                release_assert(false);
                break;
            }
        }
        if (v == var_Undef) break;
        if (var_data[v].removed == Removed::replaced) {
            vmtf_dequeue(v);
            continue;
        }
        assert(var_data[v].removed == Removed::none);
        break;
    }

    Lit next;
    if (v != var_Undef) {
        next = Lit(v, !pick_polarity(v));
    } else {
        next = lit_Undef;
    }

    SLOW_DEBUG_DO(assert(next == lit_Undef || solver->var_data[next.var()].removed == Removed::none));

    return next;
}

uint32_t Searcher::pick_var_vsids()
{
    uint32_t v = var_Undef;
    while (v == var_Undef || value(v) != l_Undef) {
        if (order_heap_vsids.empty()) return var_Undef; //Satisfying assignment found.
        v = order_heap_vsids.removeMin();
    }
    return v;
}

void Searcher::binary_based_morem_minim(vector<Lit>& cl)
{
    int64_t limit  = more_red_minim_limit_binary_actual;
    const size_t first_n_lits_of_cl =
        std::min<size_t>(conf.max_num_lits_more_more_red_min, cl.size());
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
                    if (frat->enabled()) lrat.binmin.push_back(i->get_id());
                }
                continue;
            }
            break;
        }
    }
}

void Searcher::minimise_redundant_more_more(vector<Lit>& cl)
{
    stats.furtherShrinkAttempt++;
    for (const Lit lit: cl) {
        seen[lit.toInt()] = 1;
    }

    binary_based_morem_minim(cl);

    //Finally, remove the literals that have seen[literal] = 0
    //Here, we can count do stats, etc.
    bool changedClause  = false;
    auto i = cl.begin();
    auto j = i;

    //never remove the 0th literal -- TODO this is a bad thing
    //we should be able to remove this, but I can't figure out how to
    //reorder the clause then
    seen[cl[0].toInt()] = 1;
    for (auto end = cl.end(); i != end; ++i) {
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

uint64_t Searcher::sumRestarts() const
{
    return stats.numRestarts + solver->get_stats().numRestarts;
}

size_t Searcher::hyper_bin_res_all(const bool check_for_set_values)
{
    size_t added = 0;

    for(auto const& b: solver->needToAddBinClause) {
        lbool val1 = value(b.getLit1());
        lbool val2 = value(b.getLit2());

        verb_print(6,
            "Attached hyper-bin: "
            << b.getLit1() << "(val: " << val1 << " )"
            << ", " << b.getLit2() << "(val: " << val2 << " )");

        //If binary is satisfied, skip
        if (check_for_set_values
            && (val1 == l_True || val2 == l_True)
        ) {
            //FRAT: emitted at creation, delete the never-attached bin
            *solver->frat << del << b.get_id() << b.getLit1() << b.getLit2() << fin;
            continue;
        }

        if (check_for_set_values) {
            assert(val1 == l_Undef && val2 == l_Undef);
        }

        int32_t ID;
        if (solver->frat->enabled()) {
            //the add was emitted with hints at creation, keep its ID
            ID = b.get_id();
        } else {
            ID = ++clause_id;
        }
        solver->attach_bin_clause(b.getLit1(), b.getLit2(), true, ID, false);
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
        for(auto const& b: uselessBin) {
            prop_stats.otf_hyper_time += 2;
            verb_print(10, "Removing binary clause: " << b
                << " except marked: " << except_marked);
            prop_stats.otf_hyper_time += solver->watches[b.getLit1()].size()/2;
            prop_stats.otf_hyper_time += solver->watches[b.getLit2()].size()/2;
            bool removed;
            if (except_marked) {
                bool rem1 = removeWBin_except_marked(
                    solver->watches, b.getLit1(), b.getLit2(), b.isRed(), b.get_id());
                bool rem2 = removeWBin_except_marked(
                    solver->watches, b.getLit2(), b.getLit1(), b.isRed(), b.get_id());
                assert(rem1 == rem2);
                removed = rem1;
            } else {
                removeWBin(solver->watches, b.getLit1(), b.getLit2(), b.isRed(), b.get_id());
                removeWBin(solver->watches, b.getLit2(), b.getLit1(), b.isRed(), b.get_id());
                removed = true;
            }
            if (!removed) continue;

            //Update stats
            if (b.isRed()) {
                solver->bin_tri.red_bins--;
                removedRed++;
            } else {
                solver->bin_tri.irred_bins--;
                removedIrred++;
                mark_elim_cand(b.getLit1());
                mark_elim_cand(b.getLit2());
            }
            *frat << del << b.get_id() << b.getLit1() << b.getLit2() << fin;

        }
    }
    uselessBin.clear();

    return std::make_pair(removedIrred, removedRed);
}

template<bool inprocess, bool red_also, bool distill_use>
PropBy Searcher::propagate() {
    uint32_t last_trail = trail.size();
    PropBy ret = propagate_core<inprocess, red_also, distill_use>();

    //Drat -- If declevel 0 propagation, we have to add the unitaries
    if (decision_level() == 0 && (frat->enabled())) {
        if (!ret.isnullptr()) {
            int32_t id;
            for(size_t i = last_trail; i < trail.size(); i++) {
                const auto propby = var_data[trail[i].lit.var()].reason;
                if (propby.getType() == PropByType::xor_t) get_xor_reason(propby, id);
            }
            // We need this check, because apparently GJ can set unsat during prop
            if (unsat_cl_ID == 0) {
                build_level0_confl_chain(ret);
                *frat << add << ++clause_id;
                add_chain();
                *frat << fin;
                set_unsat_cl_id(clause_id);
            }
        }
    }

    return ret;
}
template PropBy Searcher::propagate<true, false, true>();
template PropBy Searcher::propagate<true, true,  true>();
template PropBy Searcher::propagate<true>();
template PropBy Searcher::propagate<false>();

size_t Searcher::mem_used() const
{
    size_t mem = HyperEngine::mem_used();
    mem += var_act_vsids.capacity()*sizeof(double);
    mem += order_heap_vsids.mem_used();
    mem += order_heap_rand.mem_used();
    mem += vmtf_btab.capacity()*sizeof(uint64_t);
    mem += vmtf_links.capacity()*sizeof(Link);
    mem += learnt_clause.capacity()*sizeof(Lit);
    mem += hist.mem_used();
    mem += conflict.capacity()*sizeof(Lit);
    mem += model.capacity()*sizeof(lbool);
    mem += analyze_stack.mem_used();
    mem += assumptions.capacity()*sizeof(Lit);

    return mem;
}

void Searcher::fill_assumptions_set()
{
    SLOW_DEBUG_DO(for(auto x: var_data) assert(x.assumption == l_Undef));
    for(Lit p: assumptions) {
        p = solver->var_replacer->get_lit_replaced_with_outer(p);
        p = solver->map_outer_to_inter(p);
        // NOTE: this MAY set the same variable TWICE to different values!
        var_data[p.var()].assumption = p.sign() ? l_False : l_True;
    }
}

void Searcher::unfill_assumptions_set() {
    for(Lit p: assumptions) {
        p = solver->var_replacer->get_lit_replaced_with_outer(p);
        p = solver->map_outer_to_inter(p);
        var_data[p.var()].assumption = l_Undef;
    }
    SLOW_DEBUG_DO( for(auto x: var_data) assert(x.assumption == l_Undef));
}

void Searcher::vsids_decay_var_act()
{
    assert(branch_strategy == branch::vsids);
    var_inc_vsids *= (1.0 / var_decay);
}

void Searcher::consolidate_watches(const bool full)
{
    double t = cpu_time();
    if (full) {
        watches.full_consolidate();
    } else {
        watches.consolidate();
    }
    double time_used = cpu_time() - t;
    verb_print(1, "[mem] consolidate "
    << (full ? "full" : "mini")
    << conf.print_times(time_used));

    std::stringstream ss;
    ss << "consolidate " << (full ? "full" : "mini") << " watches";
    if (sql_stats) {
        sql_stats->time_passed_min(
            solver
            , ss.str()
            , time_used
        );
    }
}

//CaDiCaL's 'update_target_and_best'
inline void Searcher::update_target_and_best()
{
    const bool reset = rephased && sum_conflicts > last_rephase_conflicts;
    if (reset) {
        target_assigned = 0;
        if (rephased == 'B') best_assigned = 0;
        rephased = 0;
    }

    if (no_conflict_until > target_assigned) {
        for(auto& v: var_data) {
            v.target_polarity = v.saved_polarity;
            v.target_polarity_set = true;
        }
        target_assigned = no_conflict_until;
    }

    if (no_conflict_until > best_assigned) {
        for(auto& v: var_data) {
            v.best_polarity = v.saved_polarity;
            v.best_polarity_set = true;
        }
        best_assigned = no_conflict_until;
    }
}


//Normal running
template
void Searcher::cancel_until<true, false>(uint32_t level);

//During inprocessing, dont update anyting really (probing, distilling)
template
void Searcher::cancel_until<false, true>(uint32_t level);

//Lucky phases: assigns every variable, so the order heap must be filled back up
template
void Searcher::cancel_until<true, true>(uint32_t level);

template<bool do_insert_var_order, bool inprocess>
void Searcher::cancel_until(uint32_t blevel)
{

    if (decision_level() > blevel) {
        if (!inprocess) {
            update_target_and_best();
            no_conflict_until = std::min<uint32_t>(no_conflict_until, trail_lim[blevel]);
        }

        for (uint32_t i = 0; i < gmatrices.size(); i++)
            if (gmatrices[i] && !gqueuedata[i].disabled)
                gmatrices[i]->canceling();

        uint32_t i = trail_lim[blevel];
        uint32_t j = i;
        for (; i < trail.size() ; i++) {

            const uint32_t var = trail[i].lit.var();
            assert(value(var) != l_Undef);

            //Clear out BNN reason on backtrack
            if (var_data[var].reason.isBNN() &&
                var_data[var].reason.bnn_reason_set())
            {
                uint32_t reason_idx = var_data[var].reason.get_bnn_reason();
                bnn_reasons_empty_slots.push_back(reason_idx);
                var_data[var].reason = PropBy();
            }
            if (!bnns.empty()) reverse_prop(trail[i].lit);



            if (trail[i].lev <= blevel) {
                var_data[var].sublevel = j;
                trail[j++] = trail[i];
            } else {
                assigns[var] = l_Undef;
                if (do_insert_var_order) insert_var_order(var);
            }
        }
        trail.resize(j);
        qhead = trail_lim[blevel];
        trail_lim.resize(blevel);
    }

}

void Searcher::cancel_until_light()
{
    assert(decision_level() == 1);
    uint32_t i = trail_lim[0];
    for (; i < trail.size()
        ; i++
    ) {
        const uint32_t var = trail[i].lit.var();
        assert(value(var) != l_Undef);
        assigns[var] = l_Undef;
    }
    trail.resize(trail_lim[0]);
    qhead = trail_lim[0];
    trail_lim.resize(0);
}

void Searcher::check_var_in_branch_strategy(const uint32_t var, const branch str) const
{
    bool found = false;
    switch(str) {
        case branch::vsids:
            found = order_heap_vsids.inHeap(var);
            break;

        case branch::rand:
            found = order_heap_rand.inHeap(var);
            break;

        case branch::vmtf:
            uint32_t at = vmtf_queue.unassigned;
            while (at != numeric_limits<uint32_t>::max()) {
                if (at == var) {
                    found = true;
                    break;
                }
                at = vmtf_links[at].prev;
            }
            break;
    }

    if (!found) {
        cout << "ERROR: cannot find internal var " << var+1
        << " in branch strategy: " << branch_type_to_string(str) << endl;
    }
    release_assert(found);
}

void Searcher::check_all_in_vmtf_branch_strategy(const vector<uint32_t>& vars)
{
    for(auto const& v: vars) {
        assert(v < seen.size());
        seen[v] = 1;
    }

    uint32_t at = vmtf_queue.unassigned;
    while (at != numeric_limits<uint32_t>::max()) {
        seen[at] = 0;
        at = vmtf_links[at].prev;
    }

    for(auto const&v: vars) {
        if (seen[v] == 1) {
            cout << "ERROR: cannot find internal var " << v+1
            << " in VMTF" << endl;
            release_assert(false);
        }
    }
}

ConflictData Searcher::find_conflict_level(PropBy& pb) {
    ConflictData data;

    if (pb.getType() == PropByType::binary_t) {
        data.nHighestLevel = var_data[failBinLit.var()].level;

        if (data.nHighestLevel == decision_level()
            && var_data[pb.lit2().var()].level == decision_level()
        ) {
            return data;
        }

        uint32_t highestId = 0;
        // find the largest decision level in the clause
        uint32_t nLevel = var_data[pb.lit2().var()].level;
        if (nLevel > data.nHighestLevel) {
            highestId = 1;
            data.nHighestLevel = nLevel;
        }

        //TODO
        // we might want to swap here if highestID is not 0
        if (highestId != 0) {
            Lit back = pb.lit2();
            pb = PropBy(failBinLit, pb.isRedStep(), pb.get_id());
            failBinLit = back;
        }
    } else {
        Lit* lits = nullptr;
        uint32_t size = 0;
        int32_t ID;
        switch(pb.getType()) {
            case PropByType::clause_t: {
                Clause& conflCl = *cl_alloc.ptr(pb.get_offset());
                lits = conflCl.getData();
                size = conflCl.size();
                ID = conflCl.stats.id;
                break;
            }

            case PropByType::xor_t: {
                auto cl = get_xor_reason(pb, ID);
                lits = cl->data();
                size = cl->size();
                break;
            }

            case PropByType::bnn_t: {
                auto cl = get_bnn_reason(bnns[pb.getBNNidx()], lit_Undef);
                lits = cl->data();
                size = cl->size();
                break;
            }

            default:
                release_assert(false);
        }

        data.nHighestLevel = var_data[lits[0].var()].level;
        if (data.nHighestLevel == decision_level()
            && var_data[lits[1].var()].level == decision_level()
        ) {
            return data;
        }

        uint32_t highestId = 0;
        // find the largest decision level in the lits
        for (uint32_t nLitId = 1; nLitId < size; ++nLitId) {
            uint32_t nLevel = var_data[lits[nLitId].var()].level;
            if (nLevel > data.nHighestLevel) {
                highestId = nLitId;
                data.nHighestLevel = nLevel;
            }
        }

        if (highestId != 0) {
            std::swap(lits[0], lits[highestId]);
            if (highestId > 1 && pb.getType() == clause_t) {
                removeWCl(watches[lits[highestId]], pb.get_offset());
                watches[lits[0]].push(Watched(pb.get_offset(), lits[1]));
            }
        }
    }

    return data;
}

bool Searcher::check_order_heap_sanity() {
    if (conf.sampling_vars_set) {
        for(uint32_t outer_var: conf.sampling_vars) {
            outer_var = solver->var_replacer->get_var_replaced_with_outer(outer_var);
            uint32_t int_var = map_outer_to_inter(outer_var);

            assert(var_data[int_var].removed == Removed::none);

            if (int_var < nVars() &&
                var_data[int_var].removed == Removed::none &&
                value(int_var) == l_Undef
            ) {
                check_var_in_branch_strategy(int_var, branch::vsids);
                check_var_in_branch_strategy(int_var, branch::rand);
                check_var_in_branch_strategy(int_var, branch::vmtf);
            }
        }
    }

    vector<uint32_t> tmp;
    for(size_t i = 0; i < nVars(); i++) {
        if (var_data[i].removed == Removed::none && value(i) == l_Undef) {
            tmp.push_back(i);
            check_var_in_branch_strategy(i, branch::vsids);
            check_var_in_branch_strategy(i, branch::rand);
        }
    }
    check_all_in_vmtf_branch_strategy(tmp);
    order_heap_vsids.run_check([=] (uint32_t v) { assert(var_data[v].removed == Removed::none); });

    assert(order_heap_vsids.heap_property());
    assert(order_heap_rand.heap_property());

    return true;
}

bool Searcher::attach_xorclauses() {
    frat_func_start();
    SLOW_DEBUG_DO(for(const auto& gw: gwatches) assert(gw.empty()));

    if (!okay()) return okay();
    solver->remove_and_clean_detached_xors(xorclauses);
    if (!okay()) return okay();

    uint32_t j = 0;
    for(auto &x : xorclauses) {
        SLOW_DEBUG_DO(for(const auto& v: x) assert(var_data[v].removed == Removed::none));
        if (x.trivial()) {
            assert(x.reason_cl_ID == 0);
            continue;
        }
        if (x.size() == 2) {
            vector<Lit> lits = vars_to_lits(x.vars);
            lits[0] ^= !x.rhs;
            const auto id1 = ++clause_id;
            *frat << implyclfromx << id1 << lits << fratchain << x.xid << fin;
            solver->add_clause_int_frat(lits, id1);
            if (!okay()) return false;
            lits[0] ^= true; lits[1] ^= true;
            const auto id2 = ++clause_id;
            *frat << implyclfromx << id2 << lits << fratchain << x.xid << fin;
            solver->add_clause_int_frat(lits, id2);
            if (!okay()) return false;
            *frat << delx << x.xid << fin;
            continue;
        }
        assert(x.size() > 2);
        xorclauses[j++] = x;
        attach_xor_clause(j-1);
    }
    xorclauses.resize(j);
    frat_func_end();
    return okay();
}

// Moves XORs from matrixes back to xorclauses, and attaches them.
// Deletes all matrices.
// TODO: this does NOT add back clash variables
bool Searcher::clear_gauss_matrices(const bool destruct) {
    if (!destruct && frat->enabled()) for(auto& g: gmatrices) g->delete_reasons();

    xorclauses_updated = true;
    for(uint32_t i = 0; i < gqueuedata.size(); i++) {
        auto gqd = gqueuedata[i];
        verb_print(2, "[mat" << i << "] num_props       : " << print_value_kilo_mega(gqd.num_props));
        verb_print(2, "[mat" << i << "] num_conflicts   : " << print_value_kilo_mega(gqd.num_conflicts));
    }

    if (conf.verbosity) print_matrix_stats();
    for(uint32_t i = 0; i < gmatrices.size(); i++) {
        if (!gmatrices[i]) continue;
        gmatrices[i]->add_to(gauss_tot);
        gauss_tot.props += gqueuedata[i].num_props;
        gauss_tot.confls += gqueuedata[i].num_conflicts;
        gauss_tot.disabled += gqueuedata[i].disabled;
    }
    if (!destruct && okay()) for(EGaussian* g: gmatrices) g->move_back_xor_clauses();
    for(EGaussian* g: gmatrices) delete g;
    for(auto& w: gwatches) w.clear();
    gmatrices.clear();
    gqueuedata.clear();
    if (!destruct) {
        for(auto& gw: solver->gwatches) gw.clear();
        attach_xorclauses();
        if (okay()) solver->remove_and_clean_all();
    }
    return okay();
}

void Searcher::print_matrix_stats() {
    uint32_t tot = 0;
    for(const EGaussian* g: gmatrices) if (g) tot++;

    uint32_t printed = 0;
    for(EGaussian* g: gmatrices) {
        if (!g) continue;
        if (conf.verbosity == 1 && printed >= 10) {
            verb_print(1, "[g] ... and " << (tot-printed) << " more matrices not shown");
            break;
        }
        g->print_matrix_stats(conf.verbosity);
        printed++;
    }
}

void Searcher::check_assumptions_sanity() {
    set<uint32_t> ass_set;
    for(Lit p: assumptions) {
        p = solver->var_replacer->get_lit_replaced_with_outer(p);
        p = solver->map_outer_to_inter(p);
        ass_set.insert(p.var());
        assert(p.var() < var_data.size());
        assert(var_data[p.var()].removed == Removed::none);
        if (var_data[p.var()].assumption == l_Undef)
            cout << "ERROR: Assump " << p << " has .assumption : " << var_data[p.var()].assumption << endl;
        assert(var_data[p.var()].assumption != l_Undef);
    }
    // check that no other var has .assumption set
    for(uint32_t v = 0; v < nVars(); v++) {
        if (!ass_set.count(v)) assert(var_data[v].assumption == l_Undef);
        else assert(var_data[v].assumption != l_Undef);
    }
}

void Searcher::bump_var_importance_all(const uint32_t var)
{
    vsids_bump_var_act<false>(var);
    vmtf_bump_queue(var);
}

void Searcher::bump_var_importance(const uint32_t var)
{
    switch(branch_strategy) {
        case branch::vsids:
            vsids_bump_var_act<false>(var);
            break;

        case branch::rand:
            break;

        case branch::vmtf:
            // TODO this cannot be done, due to btab sorting requirements
            //vmtf_bump_queue(var);
            break;
    }
}

void Searcher::create_new_fast_backw_assumption()
{
    //Reset conflict limit
    fast_backw.cur_max_confl = sum_conflicts + fast_backw.max_confl;
    //max_confl_this_restart = params.conflictsDoneThisRestart + fast_backw.max_confl;

    //Remove indic
    const Lit indic = fast_backw._assumptions->at(fast_backw._assumptions->size()-1);
    assert(!indic.sign());
    fast_backw._assumptions->pop_back();

    //Backtrack
    if (decision_level() >= fast_backw._assumptions->size()) {
        cancel_until(fast_backw._assumptions->size());
    }

    //Add TRUE/FALSE duo
    uint32_t var = fast_backw.indic_to_var->at(indic.var());
    *fast_backw.test_indic = indic.var();
    *fast_backw.test_var = var;
    //cout << "Testing: " << *fast_backw.test_var << endl;
    Lit l = Lit(var, false);
    fast_backw._assumptions->push_back(l);

    Lit l2 = Lit(var+fast_backw.orig_num_vars, true);
    fast_backw._assumptions->push_back(l2);
}

lbool Searcher::new_decision_fast_backw()
{
    Lit next = lit_Undef;
    start:
    while (decision_level() < fast_backw._assumptions->size()) {
        // Perform user provided assumption:
        Lit p = fast_backw._assumptions->at(decision_level());
        p = solver->var_replacer->get_lit_replaced_with_outer(p);
        p = map_outer_to_inter(p);
        assert(var_data[p.var()].removed == Removed::none);

        if (value(p) == l_True) {
            // Dummy decision level:
            new_decision_level();
        } else if (value(p) == l_False) {
            //Deal with top 2 TRUE/FALSE
//             cout << "Testing ret: " << l_False << endl;
            fast_backw._assumptions->pop_back();
            fast_backw._assumptions->pop_back();
            fast_backw.non_indep_vars->push_back(*fast_backw.test_var);

            //We reached the bottom
            if (fast_backw._assumptions->size() == fast_backw.indep_vars->size()) {
                *fast_backw.test_indic = var_Undef;
                *fast_backw.test_var = var_Undef;
                return l_True;
            }

            create_new_fast_backw_assumption();

            continue;
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

        //1) No decision taken, because it's SAT
        //2) We are out of conflicts
        //Either way, it's basically independent
        if (next == lit_Undef|| sum_conflicts >  fast_backw.cur_max_confl) {
            if (sum_conflicts >  fast_backw.cur_max_confl) {
                fast_backw.indep_because_ran_out_of_confl++;
            }
            if (sum_conflicts-fast_backw.start_sumConflicts > 150ULL*1000ULL) {
                fast_backw.max_confl /= 2;
                fast_backw.start_sumConflicts = sum_conflicts;
                if (fast_backw.max_confl < 50) fast_backw.max_confl = 50;
//                 cout << "HALF" << endl;
            } else {
//                 cout << "DIFF: " << (sum_conflicts-fast_backw.start_sumConflicts)/1000 << " k" << endl;
            }

            //Let's fix this up.
            //backtrack until last.
            fast_backw._assumptions->pop_back();
            fast_backw._assumptions->pop_back();

            //Add this indic to the middle of assumptions.
            {
                vector<Lit> backup;
                backup.reserve(fast_backw._assumptions->size()+3);

                const uint32_t splice_into = fast_backw.indep_vars->size();
                for(uint32_t i = 0; i < splice_into; i++) {
                    backup.push_back(fast_backw._assumptions->at(i));
                }
                fast_backw.indep_vars->push_back(*fast_backw.test_var);
                backup.push_back(Lit(*fast_backw.test_indic, false));
                for(uint32_t i = splice_into; i < fast_backw._assumptions->size(); i++) {
                    const auto x = fast_backw._assumptions->at(i);
                    backup.push_back(x);
                }
                std::swap(*fast_backw._assumptions, backup);
                cancel_until(splice_into);
            }

            //We reached the bottom
            if (fast_backw._assumptions->size() == fast_backw.indep_vars->size()) {
                *fast_backw.test_var = var_Undef;
                *fast_backw.test_indic = var_Undef;
                return l_True;
            }

            create_new_fast_backw_assumption();
            goto start;
        }

        //Update stats
        stats.decisions++;
        sumDecisions++;
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    new_decision_level();
    enqueue<false>(next);

    return l_Undef;
}

void Searcher::find_largest_level(Lit* lits, uint32_t count, uint32_t start)
{
    for (uint32_t i = start; i < count; i++) {
        if (value(lits[i]) == l_Undef) {
            std::swap(lits[i], lits[start]);
            return;
        }
        if (level(lits[i]) > level(lits[start])) {
            std::swap(lits[i], lits[start]);
        }
    }
}

void Searcher::set_seed(const uint32_t seed) { mtrand.seed(seed); }

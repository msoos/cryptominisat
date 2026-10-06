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

#include "reducedb.h"
#include <sstream>
#include "constants.h"
#include "solver.h"
#include "solverconf.h"
#include "sqlstats.h"
#ifdef FINAL_PREDICTOR
#include "cl_predictors_xgb.h"
#include "predict_features_gen.h"
#endif

//a predictor-only build ranks the learnt DB at every reduce only by what
//its features use (see gen_pred_features.py); the STATS build dumps all
#if defined(FINAL_PREDICTOR) && !defined(STATS_NEEDED)
#define RANK_NEEDED(x) (predgen::x)
#else
#define RANK_NEEDED(x) true
#endif



#include <functional>
#include <cmath>
#include <limits>

using namespace CMSat;

namespace CMSat {

#if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
struct SortRedClsUIP1
{
    explicit SortRedClsUIP1(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    inline bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.uip1_used > y->stats.uip1_used;
    }
};

struct SortRedClsProps
{
    explicit SortRedClsProps(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    inline bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.props_made > y->stats.props_made;
    }
};

#endif

}

ReduceDB::ReduceDB(Solver* _solver) :
    solver(_solver)
{
}

ReduceDB::~ReduceDB()
{
    #ifdef FINAL_PREDICTOR
    delete predictors;
    #endif
}

//CaDiCaL-style reduce: mark unused, non-keep clauses, kill worst reducetarget%
void ReduceDB::mark_useless_redundant_clauses_as_garbage()
{
    vector<ClOffset> stack;
    stack.reserve(solver->long_red_cls[0].size());
    //predcands: the predictor also ranks what the rules below would keep
    #ifdef FINAL_PREDICTOR
    const int pred_cands = solver->conf.pred_cands;
    #else
    const int pred_cands = 0;
    #endif
    size_t normal_cands = 0;
    //data gen: a tracked clause is never removed. Whether the reduce would
    //remove it is decided as for any other clause, and kept in its 'gone' bit
    vector<ClOffset> tracked_cands;
    #ifdef FINAL_PREDICTOR
    vector<ClOffset> by_glue; //the candidates of the normal build
    #endif
    //predkeep: the score's rank replaces glue in the tier rules below
    bool pred_keep = false;
    #ifdef FINAL_PREDICTOR
    pred_keep = solver->conf.pred_keep != 0;
    if (pred_keep) {
        update_preds(solver->long_red_cls[0]);
        vector<double> scores;
        scores.reserve(solver->long_red_cls[0].size());
        for (const ClOffset offs: solver->long_red_cls[0]) {
            const Clause* cl = solver->cl_alloc.ptr(offs);
            scores.push_back(solver->red_stats_extra[cl->stats.extra_pos].pred_use);
        }
        if (!scores.empty()) {
            //the score above which a clause is in the best t1% / t2%
            const size_t n = scores.size();
            size_t at1 = (size_t)((double)n * (1.0 - solver->conf.pred_keep_t1/100.0));
            size_t at2 = (size_t)((double)n * (1.0 - solver->conf.pred_keep_t2/100.0));
            if (solver->conf.pred_keep == 2) {
                //as many tier1/tier2 clauses as glue would make: the score
                //decides which, the glue rule how many, per instance
                size_t n1 = 0, n2 = 0;
                for (const ClOffset offs: solver->long_red_cls[0]) {
                    const uint32_t g = solver->cl_alloc.ptr(offs)->stats.glue;
                    n1 += g <= solver->tier1_glue;
                    n2 += g <= solver->tier2_glue;
                }
                at1 = n - n1;
                at2 = n - n2;
                verb_print(2, "[pred] keep as glue would: tier1 " << n1 << " tier2 " << n2 << " of " << n);
            }
            //at == n: no clause is in the tier
            vector<double> tmp(scores);
            keep_t1 = keep_t2 = std::numeric_limits<double>::infinity();
            if (at1 < n) {
                std::nth_element(tmp.begin(), tmp.begin()+at1, tmp.end());
                keep_t1 = tmp[at1];
            }
            if (at2 < n) {
                std::nth_element(tmp.begin(), tmp.begin()+at2, tmp.end());
                keep_t2 = tmp[at2];
            }
        }
    }
    #endif
    for (const ClOffset offs: solver->long_red_cls[0]) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        SLOW_DEBUG_DO(assert(!cl->stats.marked_clause));
        {
            const uint32_t g = cl->stats.glue;
            rstats.live_tier[g <= solver->tier1_glue ? 0 : (g <= solver->tier2_glue ? 1 : 2)]++;
        }
        if (solver->clause_locked(*cl, offs)) { rstats.locked++; continue; } //reasons are kept, as in CaDiCaL
        const uint32_t used = cl->stats.used;
        rstats.used_hist[used == 0 ? 0 : (used <= 10 ? 1 : (used < CL_MAX_USED-1 ? 2 : (used == CL_MAX_USED-1 ? 3 : 4)))]++;
        //'used' is decremented below, after the predictor has seen it
        if (cl->stats.is_ternary_resolvent) {
            //like CaDiCaL's hyper resolvents: kept one round unless used
            if (used < CL_MAX_USED) cl->stats.marked_clause = true;
            continue;
        }
        const bool tracked = cl->stats.locked_for_data_gen;
        if (tracked && cl->stats.gone_for_data_gen) continue;
        vector<ClOffset>& cands = tracked ? tracked_cands : stack;
        if (cl->stats.keep) {
            rstats.kept_keep++;
            if (pred_cands >= 2) cands.push_back(offs);
            continue;
        }
        //Kissat's collect_reducibles: tier1 lives while 'used' lasts, tier2
        //only if used since the last reduce. Kissat always reduces tier3,
        //CaDiCaL keeps it if used: see reducekeepused
        const uint32_t glue = cl->stats.glue;
        bool tier1 = glue <= solver->tier1_glue;
        bool tier2 = glue <= solver->tier2_glue;
        #ifdef FINAL_PREDICTOR
        if (pred_keep) {
            const double sc = solver->red_stats_extra[cl->stats.extra_pos].pred_use;
            tier1 = sc >= keep_t1;
            tier2 = sc >= keep_t2;
        }
        #endif
        if ((tier1 && used)
            || ((tier2 || solver->conf.reduce_keep_used)
                && used >= CL_MAX_USED-1)
        ) {
            if (!tracked) rstats.kept_used++;
            if (pred_cands >= 1) cands.push_back(offs);
            continue;
        }
        cands.push_back(offs);
        if (tracked) continue;
        normal_cands++;
        #ifdef FINAL_PREDICTOR
        by_glue.push_back(offs);
        #endif
    }
    rstats.cands = normal_cands;
    //worst first: larger glue, then larger size, as in CaDiCaL
    auto glue_order = [this](const ClOffset a, const ClOffset b) {
        const Clause* c = solver->cl_alloc.ptr(a);
        const Clause* d = solver->cl_alloc.ptr(b);
        if (c->stats.glue != d->stats.glue) return c->stats.glue > d->stats.glue;
        return c->size() > d->size();
    };
    #ifdef FINAL_PREDICTOR
    //only what is ranked gets predicted: a third of the DB on a normal run.
    //The features must be what the STATS build dumped: before the decrement
    if (!solver->conf.dump_pred_distrib && !pred_keep) {
        const double my_time = cpu_time();
        update_preds(stack);
        update_preds(tracked_cands);
        verb_print(2, "[pred] predicted for " << stack.size() << " cands"
            << solver->conf.print_times(cpu_time()-my_time));
    }
    //what the normal build would keep, for the limits below
    std::stable_sort(by_glue.begin(), by_glue.end(), glue_order);
    //worst first: least predicted future use, then glue/size
    const auto& ext = solver->red_stats_extra;
    auto pred_of = [&](const Clause* c) { return ext[c->stats.extra_pos].pred_use; };
    auto order = [&](const ClOffset a, const ClOffset b) {
        const double pc = pred_of(solver->cl_alloc.ptr(a));
        const double pd = pred_of(solver->cl_alloc.ptr(b));
        if (pc != pd) return pc < pd;
        return glue_order(a, b);
    };
    #else
    const auto& order = glue_order;
    #endif
    std::stable_sort(stack.begin(), stack.end(), order);
    #ifndef FINAL_PREDICTOR
    const vector<ClOffset>& by_glue = stack;
    #endif

    for (const ClOffset offs: solver->long_red_cls[0]) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (solver->clause_locked(*cl, offs)) continue;
        if (cl->stats.used) cl->stats.used--;
    }

    //Kissat's mark_less_useful_clauses_as_garbage: the removed fraction
    //rises from reducelow towards reducehigh with log10 of the reductions
    double percent = solver->conf.reducetarget;
    if (solver->conf.reducelow < solver->conf.reducehigh) {
        const double high = solver->conf.reducehigh * 0.1;
        const double low = solver->conf.reducelow * 0.1;
        percent = high - (high - low) / std::log10((double)num_reductions + 9.0);
    }
    size_t target = 1e-2 * percent * (double)normal_cands;
    const size_t normal_target = target;
    #ifdef FINAL_PREDICTOR
    if (solver->conf.pred_thresh > 0) {
        size_t below = 0;
        while (below < stack.size()
            && pred_of(solver->cl_alloc.ptr(stack[below])) < solver->conf.pred_thresh) below++;
        rstats.pred_below_thresh += below;
        target = std::min(std::max(below, target/2), target*2);
    }
    #endif
    if (target > stack.size()) target = stack.size();
    cl_reduced = target;
    for (size_t i = 0; i < target; i++) {
        Clause* cl = solver->cl_alloc.ptr(stack[i]);
        cl->stats.marked_clause = true;
        rstats.removed++;
        const uint32_t g = cl->stats.glue;
        rstats.removed_tier[g <= solver->tier1_glue ? 0 : (g <= solver->tier2_glue ? 1 : 2)]++;
    }

    //a tracked clause no better than the best one removed is gone
    if (target > 0) {
        const ClOffset cut = stack[target-1];
        for (const ClOffset offs: tracked_cands) {
            if (!order(cut, offs)) solver->cl_alloc.ptr(offs)->stats.gone_for_data_gen = true;
        }
    }

    //CaDiCaL's lim.keptglue/keptsize, used to pick vivification candidates.
    //The normal build's also in the predictor build: what the model keeps
    //has no glue limit, and every learnt clause would count as likely kept
    lim_keptglue = lim_keptsize = 0;
    for (size_t i = normal_target; i < by_glue.size(); i++) {
        const Clause* cl = solver->cl_alloc.ptr(by_glue[i]);
        lim_keptglue = std::max(lim_keptglue, cl->stats.glue);
        lim_keptsize = std::max<uint32_t>(lim_keptsize, cl->size());
    }
}

//CaDiCaL-style flush: remove ALL redundant clauses not recently used
void ReduceDB::mark_clauses_to_be_flushed()
{
    for (const ClOffset offs: solver->long_red_cls[0]) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (solver->clause_locked(*cl, offs)) continue;
        const uint32_t used = cl->stats.used;
        if (used) cl->stats.used = used - 1;
        if (used >= CL_MAX_USED-1) continue;
        if (cl->stats.locked_for_data_gen) {
            cl->stats.gone_for_data_gen = true;
            continue;
        }
        cl->stats.marked_clause = true;
        cl_reduced++;
    }
}

void ReduceDB::remove_marked_clauses()
{
    auto& cls = solver->long_red_cls[0];
    size_t j = 0;
    for (const ClOffset offs: cls) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (!cl->stats.marked_clause) {
            cls[j++] = offs;
            continue;
        }
        cl->stats.marked_clause = false;
        solver->watches.smudge((*cl)[0]);
        solver->watches.smudge((*cl)[1]);
        solver->lit_stats.red_lits -= cl->size();
        *solver->frat << del << *cl << fin;
        cl->set_removed();
        delayed_clause_free.push_back(offs);
    }
    cls.resize(j);
}

void ReduceDB::handle_reduce([[maybe_unused]] const uint32_t cur_rst_type)
{
    solver->dump_memory_stats_to_sql();
    const double my_time = cpu_time();
    const size_t orig_size = solver->long_red_cls[0].size();
    assert(solver->watches.get_smudged_list().empty());
    assert(delayed_clause_free.empty());
    num_reductions++;

    if (inc_flush == 0) {
        inc_flush = solver->conf.flushint;
        lim_flush = solver->conf.flushint;
    }
    const bool flush = solver->conf.flush && solver->sum_conflicts >= lim_flush;
    if (flush) num_flushes++;

    cl_reduced = 0;
    rstats = ReduceStats();
    rstats.sum_red_before = orig_size;
    //Data is gathered and predictions are made exactly where the decision
    //is taken, so training and use see the same clause states
    #ifdef STATS_NEEDED
    if (solver->sql_stats) dump_sql_cl_data(cur_rst_type);
    #endif
    #ifdef FINAL_PREDICTOR
    if (!flush) predict_all_learnt(cur_rst_type);
    #endif
    if (flush) mark_clauses_to_be_flushed();
    else mark_useless_redundant_clauses_as_garbage();
    remove_marked_clauses();
    #if defined(FINAL_PREDICTOR) && !defined(STATS_NEEDED)
    //per-interval stats restart, as the STATS build does at its dump (a
    //build with both resets once, at the dump: twice would skip dump_no 1)
    for(const ClOffset offs: solver->long_red_cls[0]) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        solver->red_stats_extra[cl->stats.extra_pos].reset_rdb_stats(cl->stats);
    }
    #endif
    rstats_tot += rstats;

    solver->clean_occur_from_removed_clauses_only_smudged();
    for(ClOffset offset: delayed_clause_free) solver->free_cl(offset);
    delayed_clause_free.clear();
    SLOW_DEBUG_DO(solver->check_no_removed_or_freed_cl_in_watch());

    //Kissat's reduce interval: reduceint * sqrt(reductions). CaDiCaL's
    //linear reduceint*(n+1) gave 81 reductions on UTI-20-10p0 vs kissat's
    //231, so 'used' lives were never spent and the red DB kept growing
    int64_t delta = (int64_t)((double)solver->conf.reduceint * std::sqrt((double)num_reductions));
    if (delta < 1) delta = 1;
    lim_reduce = solver->sum_conflicts + delta;
    if (flush) {
        inc_flush *= solver->conf.flushfactor;
        lim_flush = solver->sum_conflicts + inc_flush;
    }

    verb_print(1, "[reduce] " << num_reductions
    << (flush ? " FLUSHED" : "")
    << " confl: " << solver->sum_conflicts
    << " red: " << orig_size << "->" << solver->long_red_cls[0].size()
    << " cands: " << rstats.cands
    << " rem: " << rstats.removed
    << " next: +" << delta
    << solver->conf.print_times(cpu_time()-my_time));
    verb_print(1, "[reduce]   rem t1/t2/t3: " << rstats.removed_tier[0] << "/" << rstats.removed_tier[1]
    << "/" << rstats.removed_tier[2]
    << " live t1/t2/t3: " << rstats.live_tier[0] << "/" << rstats.live_tier[1]
    << "/" << rstats.live_tier[2]
    << " tiers: " << solver->tier1_glue << "/" << solver->tier2_glue);
    verb_print(1, "[reduce]   kept-used: " << rstats.kept_used
    << " kept-keep: " << rstats.kept_keep
    << " locked: " << rstats.locked
    << " likely-kept lim glue/size: " << lim_keptglue << "/" << lim_keptsize);
    verb_print(2, "[reduce-used] life 0: " << rstats.used_hist[0]
    << " 1-10: " << rstats.used_hist[1] << " 11-29: " << rstats.used_hist[2]
    << " 30 (used before last reduce): " << rstats.used_hist[3]
    << " 31 (used since): " << rstats.used_hist[4]);

    if (solver->sql_stats) {
        solver->sql_stats->time_passed_min(
            solver
            , "dbclean"
            , cpu_time()-my_time
        );
    }
    total_time += cpu_time()-my_time;

    last_reducedb_num_conflicts = solver->sum_conflicts;
}

//CaDiCaL's likely_to_be_kept_clause under kissat's tier rules
bool ReduceDB::likely_to_be_kept(const Clause& cl) const
{
    if (cl.stats.keep) return true;
    //glue, also under --predkeep: the score steering this cost 10 points
    //(116% vs 106% of the normal build's conflicts on the held-out set)
    if (cl.stats.glue <= solver->tier1_glue && cl.stats.used) return true;
    if ((cl.stats.glue <= solver->tier2_glue || solver->conf.reduce_keep_used)
        && cl.stats.used >= CL_MAX_USED-1) return true;
    if (cl.stats.glue > lim_keptglue) return false;
    if (cl.size() > lim_keptsize) return false;
    return true;
}

void ReduceDB::print_reduce_stats() const
{
    const auto& r = rstats_tot;
    const string p = solver->conf.prefix;
    print_stats_line(p, "reductions", num_reductions,
        float_div(r.sum_red_before, num_reductions), "avg red cls at reduce");
    print_stats_line(p, "reduce interval avg",
        (uint64_t)(num_reductions ? solver->sum_conflicts / num_reductions : 0),
        float_div(solver->sum_conflicts, num_reductions) / (double)solver->conf.reduceint,
        "x reduceint");
    print_stats_line(p, "learnt cls uses", solver->sum_clause_uses(),
        stats_line_percent(solver->sum_clause_uses(), solver->sum_conflicts), "% of conflicts, as kissat's clauses_used");
    print_stats_line(p, "reduce candidates", r.cands,
        stats_line_percent(r.cands, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
    print_stats_line(p, "reduce removed", r.removed,
        stats_line_percent(r.removed, r.cands), "% of candidates");
    print_stats_line(p, "reduce removed tier1", r.removed_tier[0],
        stats_line_percent(r.removed_tier[0], r.removed), "% of removed");
    print_stats_line(p, "reduce removed tier2", r.removed_tier[1],
        stats_line_percent(r.removed_tier[1], r.removed), "% of removed");
    print_stats_line(p, "reduce removed tier3", r.removed_tier[2],
        stats_line_percent(r.removed_tier[2], r.removed), "% of removed");
    for(int t = 0; t < 3; t++) {
        print_stats_line(p, "avg red cls in tier" + std::to_string(t+1),
            float_div(r.live_tier[t], num_reductions),
            stats_line_percent(r.live_tier[t], r.live_tier[0] + r.live_tier[1] + r.live_tier[2]),
            "% of red cls seen");
    }
    {
        const uint64_t tot = r.used_hist[0]+r.used_hist[1]+r.used_hist[2]+r.used_hist[3]+r.used_hist[4];
        print_stats_line(p, "red cls used since last reduce", r.used_hist[4],
            stats_line_percent(r.used_hist[4], tot), "% of red cls seen");
        print_stats_line(p, "red cls with life 0", r.used_hist[0],
            stats_line_percent(r.used_hist[0], tot), "% of red cls seen");
    }
    print_stats_line(p, "reduce kept used", r.kept_used,
        stats_line_percent(r.kept_used, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
    print_stats_line(p, "reduce kept tier1-keep", r.kept_keep,
        stats_line_percent(r.kept_keep, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
    #ifdef FINAL_PREDICTOR
    print_stats_line(p, "reduce pred below thresh", r.pred_below_thresh,
        stats_line_percent(r.pred_below_thresh, r.cands), "% of candidates");
    print_out_of_range();
    #endif
    print_stats_line(p, "reduce kept locked", r.locked,
        stats_line_percent(r.locked, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
}

#if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
const CMSat::ClauseStats&
ReduceDB::get_median_stat(const vector<ClOffset>& all_learnt) const
{
    return solver->cl_alloc.ptr(all_learnt[all_learnt.size()/2])->stats;
}

const CMSat::ClauseStats&
ReduceDB::get_median_stat_dat(const vector<ClOffset>& all_learnt, const vector<val_and_pos>& dat) const
{
    return solver->cl_alloc.ptr(all_learnt[dat[dat.size()/2].pos])->stats;
}

void ReduceDB::prepare_features(vector<ClOffset>& all_learnt)
{
    //Prop and also save total_* data for stats
    if (RANK_NEEDED(NEED_PROP_RANKING)) {
        std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsProps(solver->cl_alloc));
    }
    //total_glue = 0;
    total_props = 0;
    total_uip1_used = 0;
    total_sum_uip1_used = 0;
    total_sum_props_used = 0;
    total_time_in_solver = 0;
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        stats_extra.prop_ranking = i+1;
        assert(cl->stats.glue > 0);
        stats_extra.update_rdb_stats(cl->stats);

        //total_glue += cl->stats.glue; CANNOT CALCULATE! ternaries have no glues
        total_props += cl->stats.props_made;
        total_uip1_used += cl->stats.uip1_used;
        total_sum_uip1_used += stats_extra.sum_uip1_used;
        total_sum_props_used += stats_extra.sum_props_made;
        assert(solver->sum_conflicts >= stats_extra.introduced_at_conflict);
        total_time_in_solver += solver->sum_conflicts - stats_extra.introduced_at_conflict;
    }
    if (all_learnt.empty()) {
        median_data.median_props = 0;
    } else {
        median_data.median_props = get_median_stat(all_learnt).props_made;
    }

    //UIP1
    if (RANK_NEEDED(NEED_UIP1_RANKING)) {
        std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsUIP1(solver->cl_alloc));
        for(size_t i = 0; i < all_learnt.size(); i++) {
            ClOffset offs = all_learnt[i];
            Clause* cl = solver->cl_alloc.ptr(offs);
            ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
            stats_extra.uip1_ranking = i+1;
        }
        if (all_learnt.empty()) {
            median_data.median_uip1_used = 0;
        } else {
            median_data.median_uip1_used = get_median_stat(all_learnt).uip1_used;
        }
    }

    // Sum UIP1 use/time
    vector<val_and_pos> dat(all_learnt.size());
    if (RANK_NEEDED(NEED_SUM_UIP1_PER_TIME_RANKING))
    for(uint32_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        dat[i].pos = i;
        dat[i].val = stats_extra.calc_sum_uip1_per_time(solver->sum_conflicts);
    }
    if (RANK_NEEDED(NEED_SUM_UIP1_PER_TIME_RANKING)) {
        std::sort(dat.begin(), dat.end(), [](const val_and_pos& a, const val_and_pos& b) {
            return a.val > b.val;
        });
        for(size_t i = 0; i < dat.size(); i++) {
            ClOffset offs = all_learnt[dat[i].pos];
            Clause* cl = solver->cl_alloc.ptr(offs);
            ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
            stats_extra.sum_uip1_per_time_ranking = i+1;
        }
        if (all_learnt.empty()) {
            median_data.median_sum_uip1_per_time = 0;
        } else {
            uint32_t extra_at = get_median_stat_dat(all_learnt, dat).extra_pos;
            const ClauseStatsExtra& stats_extra = solver->red_stats_extra[extra_at];
            median_data.median_sum_uip1_per_time = stats_extra.calc_sum_uip1_per_time(solver->sum_conflicts);
        }
    }

    // Sum props/time
    if (RANK_NEEDED(NEED_SUM_PROPS_PER_TIME_RANKING))
    for(uint32_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        dat[i].pos = i;
        dat[i].val = stats_extra.calc_sum_props_per_time(solver->sum_conflicts);
    }
    if (RANK_NEEDED(NEED_SUM_PROPS_PER_TIME_RANKING)) {
        std::sort(dat.begin(), dat.end(), [](const val_and_pos& a, const val_and_pos& b) {
            return a.val > b.val;
        });
        for(size_t i = 0; i < all_learnt.size(); i++) {
            ClOffset offs = all_learnt[dat[i].pos];
            Clause* cl = solver->cl_alloc.ptr(offs);
            ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
            stats_extra.sum_props_per_time_ranking = i+1;
        }
        if (all_learnt.empty()) {
            median_data.median_sum_props_per_time = 0;
        } else {
            uint32_t extra_at = get_median_stat_dat(all_learnt, dat).extra_pos;
            const ClauseStatsExtra& stats_extra = solver->red_stats_extra[extra_at];
            median_data.median_sum_props_per_time = stats_extra.calc_sum_props_per_time(solver->sum_conflicts);
        }
    }

    //We'll also compact solver->red_stats_extra
    uint32_t new_extra_pos = 0;
    vector<ClauseStatsExtra> new_red_stats_extra(all_learnt.size());
    for(uint32_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        dat[i].pos = i;
        dat[i].val = cl->stats.activity;
    }
    //the loop below also compacts red_stats_extra, so it runs regardless
    if (RANK_NEEDED(NEED_ACT_RANKING)) {
        std::sort(dat.begin(), dat.end(), [](const val_and_pos& a, const val_and_pos& b) {
            return a.val > b.val;
        });
    }
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[dat[i].pos];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        stats_extra.act_ranking = i+1;

        new_red_stats_extra[new_extra_pos] = stats_extra;
        cl->stats.extra_pos = new_extra_pos;
        new_extra_pos++;
    }
    if (all_learnt.empty()) {
        median_data.median_act = 0;
    } else {
        median_data.median_act = get_median_stat_dat(all_learnt, dat).activity;
    }
    std::swap(solver->red_stats_extra, new_red_stats_extra);
}
#endif

#ifdef STATS_NEEDED
void ReduceDB::dump_sql_cl_data(
    const uint32_t cur_rst_type
) {
    assert(solver->sql_stats);

    reduceDB_called++;
    double my_time = cpu_time();
    uint32_t non_locked_lev0 = 0;

    //Set up features
    vector<ClOffset> all_learnt;
    uint32_t num_locked_for_data_gen = 0;
    for(uint32_t lev = 0; lev < solver->long_red_cls.size(); lev++) {
        auto& cc = solver->long_red_cls[lev];
        for(const auto& offs: cc) {
            Clause* cl = solver->cl_alloc.ptr(offs);
            assert(!cl->get_removed());
            assert(cl->red());
            assert(!cl->freed());
            if (cl->stats.locked_for_data_gen) {
                assert(cl->stats.which_red_array == 0);
            } else if (cl->stats.which_red_array == 0) {
                non_locked_lev0++;
            }
            all_learnt.push_back(offs);
            num_locked_for_data_gen += cl->stats.locked_for_data_gen;
        }
    }
    if (all_learnt.empty()) {
        return;
    }
    prepare_features(all_learnt);

    AverageCommonDataRDB avgdata;
//     avgdata.avg_glue = (double)total_glue/(double)all_learnt.size();
    avgdata.avg_props = (double)total_props/(double)all_learnt.size();
    avgdata.avg_uip1_used = (double)total_uip1_used/(double)all_learnt.size();
    //uses per conflict of life, over all learnt clauses (it was also
    //divided by the clause count, which left values around 1e-8)
    if (total_time_in_solver > 0) {
        avgdata.avg_sum_uip1_per_time = (double)total_sum_uip1_used/(double)total_time_in_solver;
        avgdata.avg_sum_props_per_time = (double)total_sum_props_used/(double)total_time_in_solver;
    }


    //Dump common features
    solver->sql_stats->begin_transaction();
    solver->sql_stats->reduceDB_common(
        solver,
        reduceDB_called,
        all_learnt.size(),
        cur_rst_type,
        median_data,
        avgdata
    );

    //Dump clause features
    uint64_t added_to_db = 0;
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        if (cl->stats.is_tracked) {
            const bool locked = solver->clause_locked(*cl, offs);
            assert(stats_extra.orig_ID != 0);
            assert(stats_extra.orig_ID <= cl->stats.id);
            solver->sql_stats->reduceDB(
                solver
                , locked
                , cl
                , reduceDB_called
            );
            added_to_db++;
        }
        //ALL clauses' per-interval stats restart here, as in the predictor
        //build: otherwise untracked clauses accumulate props/uip1 forever
        //and the rankings/averages the tracked ones are dumped with are off
        stats_extra.reset_rdb_stats(cl->stats);
    }
    solver->sql_stats->end_transaction();

    verb_print(1, "[sql] added to DB " << added_to_db
        << " dump-ratio: " << solver->conf.dump_individual_cldata_ratio
        << " locked-perc: " << stats_line_percent(num_locked_for_data_gen, all_learnt.size())
        << " non-locked lev0: " << non_locked_lev0
        << solver->conf.print_times(cpu_time()-my_time));
    locked_for_data_gen_total += num_locked_for_data_gen;
    locked_for_data_gen_cls += all_learnt.size();
}
#endif


#ifdef FINAL_PREDICTOR
void ReduceDB::load_predictors()
{
    if (predictors != nullptr) return;
    predictors = new ClPredictorsXGB;

    if (solver->conf.pred_conf_location.empty()) {
        if (solver->conf.pred_anc) {
            cout << "ERROR: only the plain-label model is compiled in, --predanc 1 needs --predloc" << endl;
            exit(-1);
        }
        predictors->load_embedded_model();
        if (solver->conf.verbosity) {
            cout << solver->conf.prefix << "[pred] compiled-in model, sha1 " << predictor_disc_json_hash << endl;
        }
    } else {
        const string fname = solver->conf.pred_conf_location + "/predictor-"
            + (solver->conf.pred_anc ? "used_later_anc" : "used_later") + "-disc-xgb.json";
        predictors->load_model(fname);
        if (solver->conf.verbosity) {
            cout << solver->conf.prefix << "[pred] loaded the model " << fname << endl;
        }
    }
    if (!solver->conf.pred_dump_fname.empty()) predictors->open_dump(solver->conf.pred_dump_fname);
    if (solver->conf.verbosity && !predictors->provenance.empty()) {
        cout << solver->conf.prefix << "[pred] model trained on " << predictors->provenance << endl;
    }
}

//Fills the predicted use of every clause in offs
void ReduceDB::update_preds(const vector<ClOffset>& offs)
{
    if (offs.empty()) return;
    const int step_size = predictors->get_step_size();
    vector<float> data(step_size*offs.size(), 0);
    float* at = data.data();
    for(const ClOffset offset: offs) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        const auto& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        const double act_ranking_rel = safe_div(stats_extra.act_ranking, commdata.all_learnt_size);
        const double uip1_ranking_rel = safe_div(stats_extra.uip1_ranking, commdata.all_learnt_size);
        const double prop_ranking_rel = safe_div(stats_extra.prop_ranking, commdata.all_learnt_size);
        const double sum_uip1_per_time_ranking_rel = safe_div(stats_extra.sum_uip1_per_time_ranking, commdata.all_learnt_size);
        const double sum_props_per_time_ranking_rel = safe_div(stats_extra.sum_props_per_time_ranking, commdata.all_learnt_size);
        const int ret = predictors->set_up_input(
            cl,
            solver->sum_conflicts,
            act_ranking_rel,
            uip1_ranking_rel,
            prop_ranking_rel,
            stats_extra.sum_uip1_per_time_ranking,
            stats_extra.sum_props_per_time_ranking,
            sum_uip1_per_time_ranking_rel,
            sum_props_per_time_ranking_rel,
            commdata,
            solver,
            at
        );
        assert(ret == step_size);
        at += step_size;
    }
    count_out_of_range(data.data(), offs.size());
    predictors->predict_all(data.data(), offs.size());
    uint32_t i = 0;
    vector<double> dump_preds;
    for(const ClOffset offset: offs) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        auto& extra = solver->red_stats_extra[cl->stats.extra_pos];
        predictors->get_prediction_at(extra, i++);
        if (predictors->dumping()) dump_preds.push_back(extra.pred_use);
        if (solver->conf.pred_mimic) extra.pred_use = -((double)cl->stats.glue*1e7 + (double)cl->size());
        //eagerly subsumed: goes first, as in the normal build. The model
        //never saw such a clause
        if (cl->stats.glue == CL_MAX_GLUE) extra.pred_use = -std::numeric_limits<double>::infinity();
    }
    if (predictors->dumping()) predictors->write_dump(data.data(), dump_preds, offs.size());
    predictors->finish_all_predict();
}

void ReduceDB::predict_all_learnt(const uint32_t cur_rst_type)
{
    load_predictors();
    const double my_time = cpu_time();
    vector<ClOffset> all_learnt = solver->long_red_cls[0];
    prepare_features(all_learnt); //sorts all_learnt, compacts red_stats_extra
    commdata = ReduceCommonData(
        total_props,
        total_uip1_used,
        total_sum_uip1_used,
        total_sum_props_used,
        total_time_in_solver,
        all_learnt.size(),
        cur_rst_type,
        median_data);
    if (solver->conf.dump_pred_distrib) {
        update_preds(solver->long_red_cls[0]);
        dump_pred_distrib(solver->long_red_cls[0]);
    }
    verb_print(2, "[pred] features for " << all_learnt.size() << " cls"
        << solver->conf.print_times(cpu_time()-my_time));
}

//Every feature value outside the range the model was trained on (the
//model carries its 1st/99th percentiles). 1% of the training rows are
//outside by construction, so a few percent is normal; a run far longer
//or bigger than the training runs drives the age-like and size-like
//features out, and the model's verdict on it means little. Printed with
//the stats at the end
void ReduceDB::count_out_of_range(const float* data, const uint32_t rows)
{
    if (predictors->feature_lo.empty()) return; //the model carries no ranges
    if (feat_out_of_range.empty()) feat_out_of_range.resize(PRED_COLS, 0);
    for(uint32_t r = 0; r < rows; r++) {
        const float* at = data + (size_t)r * PRED_COLS;
        for(uint32_t c = 0; c < (uint32_t)PRED_COLS; c++) {
            const float v = at[c];
            if (std::isnan(v)) continue; //missing
            feat_values_seen++;
            const double lo = predictors->feature_lo[c];
            const double hi = predictors->feature_hi[c];
            if (std::isnan(lo)) continue; //no training range known
            if (v < lo || v > hi) feat_out_of_range[c]++;
        }
    }
}

void ReduceDB::print_out_of_range() const
{
    if (predictors == nullptr) return;
    const string p = solver->conf.prefix;
    if (predictors->feature_lo.empty()) {
        cout << p << "pred feats outside training range: unknown, the model carries no ranges" << endl;
        return;
    }
    if (feat_values_seen == 0 || feat_out_of_range.empty()) return;
    uint64_t tot = 0;
    for(const auto& n: feat_out_of_range) tot += n;
    print_stats_line(p, "pred feats outside training range", tot,
        stats_line_percent(tot, feat_values_seen), "% of values (1-2% is normal, see CLAUDE.md)");
    //the three worst
    vector<std::pair<uint64_t, uint32_t>> worst;
    for(uint32_t c = 0; c < feat_out_of_range.size(); c++) worst.push_back({feat_out_of_range[c], c});
    std::sort(worst.rbegin(), worst.rend());
    const uint64_t per_feat = feat_values_seen / PRED_COLS;
    for(uint32_t i = 0; i < 3 && i < worst.size() && worst[i].first > 0; i++) {
        cout << p << "   " << std::setw(5) << std::fixed << std::setprecision(1)
            << stats_line_percent(worst[i].first, per_feat) << "% outside: "
            << predgen::feature_names[worst[i].second] << endl;
    }
}

void ReduceDB::dump_pred_distrib(const vector<ClOffset>& offs)
{
    if (!solver->conf.dump_pred_distrib) return;
    const bool first = num_reductions == 1;
    std::ofstream distrib_file("pred_distrib.csv", first ? std::ios::out : std::ios::app);
    if (first) {
        distrib_file << "reduction,age,glue,used,pred_use" << endl;
    }
    for(const ClOffset off: offs) {
        const Clause* cl = solver->cl_alloc.ptr(off);
        const ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        distrib_file
        << num_reductions << ","
        << (solver->sum_conflicts - stats_extra.introduced_at_conflict) << ","
        << cl->stats.glue << "," << cl->stats.used
        << "," << stats_extra.pred_use
        << endl;
    }
}
#endif


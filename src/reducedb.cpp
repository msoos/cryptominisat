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
#include "constants.h"
#include "solver.h"
#include "solverconf.h"
#include "sqlstats.h"
#ifdef FINAL_PREDICTOR
#include "cl_predictors_xgb.h"
#include "cl_predictors_py.h"
#endif

// #define VERBOSE_DEBUG


#include <functional>
#include <cmath>

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
    stack.reserve(solver->longRedCls[0].size());
    for (const ClOffset offs: solver->longRedCls[0]) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        SLOW_DEBUG_DO(assert(!cl->stats.marked_clause));
        {
            const uint32_t g = cl->stats.glue;
            rstats.live_tier[g <= solver->tier1_glue ? 0 : (g <= solver->tier2_glue ? 1 : 2)]++;
        }
        if (solver->clause_locked(*cl, offs)) { rstats.locked++; continue; } //reasons are kept, as in CaDiCaL
        const uint32_t used = cl->stats.used;
        rstats.used_hist[used == 0 ? 0 : (used <= 10 ? 1 : (used < CL_MAX_USED-1 ? 2 : (used == CL_MAX_USED-1 ? 3 : 4)))]++;
        if (used) cl->stats.used = used - 1;
        if (cl->stats.is_ternary_resolvent) {
            //like CaDiCaL's hyper resolvents: kept one round unless used
            if (used < CL_MAX_USED) cl->stats.marked_clause = true;
            continue;
        }
        if (cl->stats.keep) { rstats.kept_keep++; continue; }
        if (cl->stats.locked_for_data_gen) continue;
        //Kissat's collect_reducibles: tier1 lives while 'used' lasts, tier2
        //only if used since the last reduce, tier3 is always a candidate
        const uint32_t glue = cl->stats.glue;
        if (glue <= solver->tier1_glue && used) { rstats.kept_used++; continue; }
        if (glue <= solver->tier2_glue && used >= CL_MAX_USED-1) { rstats.kept_used++; continue; }
        stack.push_back(offs);
    }
    rstats.cands = stack.size();
    #ifdef FINAL_PREDICTOR
    //worst first: least predicted future use, then glue/size as below
    const auto& ext = solver->red_stats_extra;
    auto pred_of = [&](const Clause* c) { return pred_score(ext[c->stats.extra_pos]); };
    std::stable_sort(stack.begin(), stack.end(),
        [&](const ClOffset a, const ClOffset b) {
            const Clause* c = solver->cl_alloc.ptr(a);
            const Clause* d = solver->cl_alloc.ptr(b);
            const double pc = pred_of(c);
            const double pd = pred_of(d);
            if (pc != pd) return pc < pd;
            if (c->stats.glue != d->stats.glue) return c->stats.glue > d->stats.glue;
            return c->size() > d->size();
        });
    #else
    //worst first: larger glue, then larger size, as in CaDiCaL
    std::stable_sort(stack.begin(), stack.end(),
        [this](const ClOffset a, const ClOffset b) {
            const Clause* c = solver->cl_alloc.ptr(a);
            const Clause* d = solver->cl_alloc.ptr(b);
            if (c->stats.glue != d->stats.glue) return c->stats.glue > d->stats.glue;
            return c->size() > d->size();
        });
    #endif

    //Kissat's mark_less_useful_clauses_as_garbage: the removed fraction
    //rises from reducelow towards reducehigh with log10 of the reductions
    double percent = solver->conf.reducetarget;
    if (solver->conf.reducelow < solver->conf.reducehigh) {
        const double high = solver->conf.reducehigh * 0.1;
        const double low = solver->conf.reducelow * 0.1;
        percent = high - (high - low) / std::log10((double)num_reductions + 9.0);
    }
    size_t target = 1e-2 * percent * (double)stack.size();
    if (target > stack.size()) target = stack.size();
    cl_reduced = target;
    for (size_t i = 0; i < target; i++) {
        Clause* cl = solver->cl_alloc.ptr(stack[i]);
        cl->stats.marked_clause = true;
        rstats.removed++;
        const uint32_t g = cl->stats.glue;
        rstats.removed_tier[g <= solver->tier1_glue ? 0 : (g <= solver->tier2_glue ? 1 : 2)]++;
    }

    //CaDiCaL's lim.keptglue/keptsize, used to pick vivification candidates
    lim_keptglue = lim_keptsize = 0;
    for (size_t i = target; i < stack.size(); i++) {
        const Clause* cl = solver->cl_alloc.ptr(stack[i]);
        lim_keptglue = std::max(lim_keptglue, cl->stats.glue);
        lim_keptsize = std::max<uint32_t>(lim_keptsize, cl->size());
    }
}

//CaDiCaL-style flush: remove ALL redundant clauses not recently used
void ReduceDB::mark_clauses_to_be_flushed()
{
    for (const ClOffset offs: solver->longRedCls[0]) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (solver->clause_locked(*cl, offs)) continue;
        const uint32_t used = cl->stats.used;
        if (used) cl->stats.used = used - 1;
        if (used >= CL_MAX_USED-1) continue;
        if (cl->stats.locked_for_data_gen) continue;
        cl->stats.marked_clause = true;
        cl_reduced++;
    }
}

void ReduceDB::remove_marked_clauses()
{
    auto& cls = solver->longRedCls[0];
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
        solver->litStats.redLits -= cl->size();
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
    const size_t orig_size = solver->longRedCls[0].size();
    assert(solver->watches.get_smudged_list().empty());
    assert(delayed_clause_free.empty());
    num_reductions++;

    if (inc_flush == 0) {
        inc_flush = solver->conf.flushint;
        lim_flush = solver->conf.flushint;
    }
    const bool flush = solver->conf.flush && solver->sumConflicts >= lim_flush;
    if (flush) num_flushes++;

    cl_reduced = 0;
    rstats = ReduceStats();
    rstats.sum_red_before = orig_size;
    //Data is gathered and predictions are made exactly where the decision
    //is taken, so training and use see the same clause states
    #ifdef STATS_NEEDED
    if (solver->sqlStats) dump_sql_cl_data(cur_rst_type);
    #endif
    #ifdef FINAL_PREDICTOR
    if (!flush) predict_all_learnt();
    #endif
    if (flush) mark_clauses_to_be_flushed();
    else mark_useless_redundant_clauses_as_garbage();
    remove_marked_clauses();
    #ifdef FINAL_PREDICTOR
    //per-interval stats restart, as the STATS build does at its dump
    for(const ClOffset offs: solver->longRedCls[0]) {
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
    lim_reduce = solver->sumConflicts + delta;
    if (flush) {
        inc_flush *= solver->conf.flushfactor;
        lim_flush = solver->sumConflicts + inc_flush;
    }

    verb_print(1, "[reduce] " << num_reductions
    << (flush ? " FLUSHED" : "")
    << " confl: " << solver->sumConflicts
    << " red: " << orig_size << "->" << solver->longRedCls[0].size()
    << " cands: " << rstats.cands
    << " rem: " << rstats.removed
    << " (t1/t2/t3: " << rstats.removed_tier[0] << "/" << rstats.removed_tier[1]
    << "/" << rstats.removed_tier[2] << ")"
    << " live t1/t2/t3: " << rstats.live_tier[0] << "/" << rstats.live_tier[1]
    << "/" << rstats.live_tier[2]
    << " kept-used: " << rstats.kept_used
    << " kept-keep: " << rstats.kept_keep
    << " locked: " << rstats.locked
    << " tiers: " << solver->tier1_glue << "/" << solver->tier2_glue
    << " next: +" << delta
    << solver->conf.print_times(cpu_time()-my_time));
    verb_print(2, "[reduce-used] life 0: " << rstats.used_hist[0]
    << " 1-10: " << rstats.used_hist[1] << " 11-29: " << rstats.used_hist[2]
    << " 30 (used before last reduce): " << rstats.used_hist[3]
    << " 31 (used since): " << rstats.used_hist[4]);

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "dbclean"
            , cpu_time()-my_time
        );
    }
    total_time += cpu_time()-my_time;

    last_reducedb_num_conflicts = solver->sumConflicts;
}

//CaDiCaL's likely_to_be_kept_clause under kissat's tier rules
bool ReduceDB::likely_to_be_kept(const Clause& cl) const
{
    if (cl.stats.keep) return true;
    if (cl.stats.glue <= solver->tier1_glue && cl.stats.used) return true;
    if (cl.stats.glue <= solver->tier2_glue && cl.stats.used >= CL_MAX_USED-1) return true;
    if (cl.stats.glue > lim_keptglue) return false;
    if (cl.size() > lim_keptsize) return false;
    return true;
}

void ReduceDB::print_reduce_stats() const
{
    const auto& r = rstats_tot;
    const string p = solver->conf.prefix;
    print_stats_line(p + "reductions", num_reductions,
        float_div(r.sum_red_before, num_reductions), "avg red cls at reduce");
    print_stats_line(p + "reduce interval avg",
        (uint64_t)(num_reductions ? solver->sumConflicts / num_reductions : 0),
        float_div(solver->sumConflicts, num_reductions) / (double)solver->conf.reduceint,
        "x reduceint");
    print_stats_line(p + "learnt cls uses", solver->sum_clause_uses(),
        stats_line_percent(solver->sum_clause_uses(), solver->sumConflicts), "% of conflicts, as kissat's clauses_used");
    print_stats_line(p + "reduce candidates", r.cands,
        stats_line_percent(r.cands, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
    print_stats_line(p + "reduce removed", r.removed,
        stats_line_percent(r.removed, r.cands), "% of candidates");
    print_stats_line(p + "reduce removed tier1", r.removed_tier[0],
        stats_line_percent(r.removed_tier[0], r.removed), "% of removed");
    print_stats_line(p + "reduce removed tier2", r.removed_tier[1],
        stats_line_percent(r.removed_tier[1], r.removed), "% of removed");
    print_stats_line(p + "reduce removed tier3", r.removed_tier[2],
        stats_line_percent(r.removed_tier[2], r.removed), "% of removed");
    for(int t = 0; t < 3; t++) {
        print_stats_line(p + "avg red cls in tier" + std::to_string(t+1),
            float_div(r.live_tier[t], num_reductions),
            stats_line_percent(r.live_tier[t], r.live_tier[0] + r.live_tier[1] + r.live_tier[2]),
            "% of red cls seen");
    }
    {
        const uint64_t tot = r.used_hist[0]+r.used_hist[1]+r.used_hist[2]+r.used_hist[3]+r.used_hist[4];
        print_stats_line(p + "red cls used since last reduce", r.used_hist[4],
            stats_line_percent(r.used_hist[4], tot), "% of red cls seen");
        print_stats_line(p + "red cls with life 0", r.used_hist[0],
            stats_line_percent(r.used_hist[0], tot), "% of red cls seen");
    }
    print_stats_line(p + "reduce kept used", r.kept_used,
        stats_line_percent(r.kept_used, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
    print_stats_line(p + "reduce kept tier1-keep", r.kept_keep,
        stats_line_percent(r.kept_keep, r.cands + r.kept_used + r.kept_keep + r.locked), "% of red cls seen");
    print_stats_line(p + "reduce kept locked", r.locked,
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
    std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsProps(solver->cl_alloc));
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
        assert(solver->sumConflicts >= stats_extra.introduced_at_conflict);
        total_time_in_solver += solver->sumConflicts - stats_extra.introduced_at_conflict;
    }
    if (all_learnt.empty()) {
        median_data.median_props = 0;
    } else {
        median_data.median_props = get_median_stat(all_learnt).props_made;
    }

    //UIP1
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

    // Sum UIP1 use/time
    vector<val_and_pos> dat(all_learnt.size());
    for(uint32_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        dat[i].pos = i;
        dat[i].val = stats_extra.calc_sum_uip1_per_time(solver->sumConflicts);
    }
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
        median_data.median_sum_uip1_per_time = stats_extra.calc_sum_uip1_per_time(solver->sumConflicts);
    }

    // Sum props/time
    for(uint32_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        dat[i].pos = i;
        dat[i].val = stats_extra.calc_sum_props_per_time(solver->sumConflicts);
    }
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
        median_data.median_sum_props_per_time = stats_extra.calc_sum_props_per_time(solver->sumConflicts);
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
    std::sort(dat.begin(), dat.end(), [](const val_and_pos& a, const val_and_pos& b) {
        return a.val > b.val;
    });
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
    assert(solver->sqlStats);

    reduceDB_called++;
    double my_time = cpu_time();
    uint32_t non_locked_lev0 = 0;

    //Set up features
    vector<ClOffset> all_learnt;
    uint32_t num_locked_for_data_gen = 0;
    for(uint32_t lev = 0; lev < solver->longRedCls.size(); lev++) {
        auto& cc = solver->longRedCls[lev];
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
    if (total_time_in_solver > 0) {
        avgdata.avg_sum_uip1_per_time = (double)total_sum_uip1_used/(double)(all_learnt.size()*total_time_in_solver);
        avgdata.avg_sum_props_per_time = (double)total_sum_props_used/(double)(all_learnt.size()*total_time_in_solver);
    }


    //Dump common features
    solver->sqlStats->begin_transaction();
    solver->sqlStats->reduceDB_common(
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
            solver->sqlStats->reduceDB(
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
    solver->sqlStats->end_transaction();

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
    if (solver->conf.predictor_type == "xgb") {
        predictors = new ClPredictorsXGB;
    } else if (solver->conf.predictor_type == "py") {
        predictors = new ClPredictorsPy;
    } else {
        cout << "ERROR: --predtype must be py or xgb" << endl;
        exit(-1);
    }

    if (solver->conf.pred_conf_location.empty()) {
        if (predictors->load_models_from_buffers() != 0) {
            cout << "ERROR: cannot load models from buffers" << endl;
            exit(-1);
        }
        if (solver->conf.verbosity) {
            cout << solver->conf.prefix << "[pred] predictor hashes: ";
            for(const auto& h: predictors->get_hashes()) cout << h << " ";
            cout << endl;
        }
    } else {
        vector<string> locations;
        const vector<string> tiers = {"short", "long", "forever"};
        for (uint32_t i = 0; i < 3; i ++) {
            locations.push_back(solver->conf.pred_conf_location + "/predictor-"
                + (solver->conf.pred_tables[i] == '0' ? "used_later" : "used_later_anc")
                + "-" + tiers[i] + "-" + solver->conf.predictor_type + ".json");
        }
        const int ret = predictors->load_models(
            locations[0], locations[1], locations[2],
            solver->conf.predict_best_feat_fname);
        if (ret == 0) {
            cout << "ERROR loading the predictors" << endl;
            exit(-1);
        }
        if (solver->conf.verbosity) {
            cout << solver->conf.prefix << "[pred] loaded predictors from: ";
            for(const auto& l: locations) cout << l << " ";
            cout << endl;
        }
    }
}

//Fills pred_short/long/forever_use of every clause in offs
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
            solver->sumConflicts,
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
    predictors->predict_all(data.data(), offs.size());
    uint32_t i = 0;
    for(const ClOffset offset: offs) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        predictors->get_prediction_at(solver->red_stats_extra[cl->stats.extra_pos], i++);
    }
    predictors->finish_all_predict();
}

void ReduceDB::predict_all_learnt()
{
    load_predictors();
    const double my_time = cpu_time();
    vector<ClOffset> all_learnt = solver->longRedCls[0];
    prepare_features(all_learnt); //sorts all_learnt, compacts red_stats_extra
    commdata = ReduceCommonData(
        total_props,
        total_uip1_used,
        total_sum_uip1_used,
        all_learnt.size(),
        median_data);
    update_preds(solver->longRedCls[0]);
    dump_pred_distrib(solver->longRedCls[0]);
    verb_print(2, "[pred] predicted for " << all_learnt.size() << " cls"
        << solver->conf.print_times(cpu_time()-my_time));
}

//The score a reduce ranks candidates by, see --predsortby
double ReduceDB::pred_score(const ClauseStatsExtra& e) const
{
    switch (solver->conf.pred_sort_by) {
        case 0: return e.pred_short_use;
        case 1: return e.pred_long_use;
        case 2: return e.pred_forever_use;
        default: return e.pred_short_use + e.pred_long_use + e.pred_forever_use;
    }
}

void ReduceDB::dump_pred_distrib(const vector<ClOffset>& offs)
{
    if (!solver->conf.dump_pred_distrib) return;
    const bool first = num_reductions == 1;
    std::ofstream distrib_file("pred_distrib.csv", first ? std::ios::out : std::ios::app);
    if (first) {
        distrib_file << "reduction,age,glue,used,pred_short_use,pred_long_use,pred_forever_use" << endl;
    }
    for(const ClOffset off: offs) {
        const Clause* cl = solver->cl_alloc.ptr(off);
        const ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        distrib_file
        << num_reductions << ","
        << (solver->sumConflicts - stats_extra.introduced_at_conflict) << ","
        << cl->stats.glue << "," << cl->stats.used
        << "," << stats_extra.pred_short_use
        << "," << stats_extra.pred_long_use
        << "," << stats_extra.pred_forever_use
        << endl;
    }
}
#endif


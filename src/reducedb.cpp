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
#include "solver.h"
#include "solverconf.h"
#include "sqlstats.h"

// #define VERBOSE_DEBUG

#include <functional>
#include <cmath>

using namespace CMSat;

double safe_div(double a, double b) {
    if (b == 0) {
        assert(a == 0);
        return 0;
    }
    return a/b;
}

struct SortRedClsGlue
{
    explicit SortRedClsGlue(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.glue < y->stats.glue;
    }
};

struct SortRedClsSize
{
    explicit SortRedClsSize(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->size() < y->size();
    }
};

struct SortRedClsAct
{
    explicit SortRedClsAct(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.activity > y->stats.activity;
    }
};

#if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
struct SortRedClsUIP1
{
    explicit SortRedClsUIP1(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
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

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.props_made > y->stats.props_made;
    }
};
#endif

#ifdef FINAL_PREDICTOR
struct SortRedClsPredShort
{
    explicit SortRedClsPredShort(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.pred_short_use > y->stats.pred_short_use;
    }
};

struct SortRedClsPredLong
{
    explicit SortRedClsPredLong(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.pred_long_use > y->stats.pred_long_use;
    }
};

struct SortRedClsPredForever
{
    explicit SortRedClsPredForever(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.pred_forever_use > y->stats.pred_forever_use;
    }
};
#endif


ReduceDB::ReduceDB(Solver* _solver) :
    solver(_solver)
{
    cl_stats.resize(3);
}

ReduceDB::~ReduceDB()
{
    #ifdef FINAL_PREDICTOR
    delete predictors;
    #endif
}

void ReduceDB::sort_red_cls(ClauseClean clean_type)
{
    #ifdef VERBOSE_DEBUG
    cout << "Before sort" << endl;
    uint64_t i = 0;
    for(const auto& x: solver->longRedCls[2]) {
        const ClOffset offset = x;
        Clause* cl = solver->cl_alloc.ptr(offset);
        cout << i << " offset: " << offset << " cl->stats.last_touched: " << cl->stats.last_touched
        << " act:" << std::setprecision(9) << cl->stats.activity
        << " which_red_array:" << cl->stats.which_red_array << endl
        << " -- cl:" << *cl << " tern:" << cl->stats.is_ternary_resolvent
        << endl;
    }
    #endif

    switch (clean_type) {
        case ClauseClean::glue : {
            std::sort(solver->longRedCls[2].begin(), solver->longRedCls[2].end(), SortRedClsGlue(solver->cl_alloc));
            break;
        }

        case ClauseClean::activity : {
            std::sort(solver->longRedCls[2].begin(), solver->longRedCls[2].end(), SortRedClsAct(solver->cl_alloc));
            break;
        }

        default: {
            assert(false && "Unknown cleaning type");
        }
    }
}

#if defined(NORMAL_CL_USE_STATS)
void ReduceDB::gather_normal_cl_use_stats()
{
    for(uint32_t i = 0; i < 3; i++) {
        ClauseStats cl_stat;
        for(const auto& offset: solver->longRedCls[i]) {
            Clause* cl = solver->cl_alloc.ptr(offset);

            cl->stats.dump_no++;
            cl_stat.add_in(*cl);
            cl->stats.reset_rdb_stats();
        }
        cl_stat.print(i);
        cl_stats[i] += cl_stat;
    }
}

#endif

//TODO maybe we chould count binary learnt clauses as well into the
//kept no. of clauses as other solvers do
void ReduceDB::handle_lev2()
{
    solver->dump_memory_stats_to_sql();
    size_t orig_size = solver->longRedCls[2].size();

    const double myTime = cpuTime();
    assert(solver->watches.get_smudged_list().empty());

    //lev2 -- clean
    int64_t num_to_reduce = solver->longRedCls[2].size();
    for(unsigned keep_type = 0
        ; keep_type < sizeof(solver->conf.ratio_keep_clauses)/sizeof(double)
        ; keep_type++
    ) {
        const uint64_t keep_num = (double)num_to_reduce*solver->conf.ratio_keep_clauses[keep_type];
        if (keep_num == 0) {
            continue;
        }
        sort_red_cls(static_cast<ClauseClean>(keep_type));
        mark_top_N_clauses_lev2(keep_num);
    }
    assert(delayed_clause_free.empty());
    cl_marked = 0;
    cl_ttl = 0;
    cl_locked_solver = 0;
    remove_cl_from_lev2();

    solver->clean_occur_from_removed_clauses_only_smudged();
    for(ClOffset offset: delayed_clause_free) {
        solver->free_cl(offset);
    }
    delayed_clause_free.clear();

    #ifdef SLOW_DEBUG
    solver->check_no_removed_or_freed_cl_in_watch();
    #endif

    if (solver->conf.verbosity >= 2) {
        cout << "c [DBclean lev2]"
        << " confl: " << solver->sumConflicts
        << " orig size: " << orig_size
        << " marked: " << cl_marked
        << " ttl:" << cl_ttl
        << " locked_solver:" << cl_locked_solver
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "dbclean-lev2"
            , cpuTime()-myTime
        );
    }
    total_time += cpuTime()-myTime;

    last_reducedb_num_conflicts = solver->sumConflicts;
}

#if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
void ReduceDB::set_prop_uip_act_ranks(vector<ClOffset>& all_learnt)
{
    std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsProps(solver->cl_alloc));
    total_glue = 0;
    total_props = 0;
    total_uip1_used = 0;
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        cl->stats.props_made_rank = i+1;
        assert(cl->stats.glue > 0);

        total_glue += cl->stats.glue;
        total_props += cl->stats.props_made;
        total_uip1_used += cl->stats.uip1_used;
    }
    if (all_learnt.empty()) {
        median_props = 0;
    } else {
        median_props = solver->cl_alloc.ptr(all_learnt[all_learnt.size()/2])->stats.props_made;
    }

    std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsUIP1(solver->cl_alloc));
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        cl->stats.uip1_used_rank = i+1;
    }
    if (all_learnt.empty()) {
        median_uip1_used = 0;
    } else {
        median_uip1_used = solver->cl_alloc.ptr(all_learnt[all_learnt.size()/2])->stats.uip1_used;
    }

    std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsAct(solver->cl_alloc));
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);
        cl->stats.act_rank = i+1;
    }
    if (all_learnt.empty()) {
        median_act = 0;
    } else {
        median_act = solver->cl_alloc.ptr(all_learnt[all_learnt.size()/2])->stats.activity;
    }

}
#endif

#ifdef STATS_NEEDED
void ReduceDB::dump_sql_cl_data(
    const uint32_t cur_rst_type
) {
    reduceDB_called++;
    double myTime = cpuTime();
    assert(solver->sqlStats);
    solver->sqlStats->begin_transaction();
    uint64_t added_to_db = 0;
    uint32_t non_locked_lev0 = 0;

    vector<ClOffset> all_learnt;
    uint32_t num_locked_for_data_gen = 0;
    for(uint32_t lev = 0; lev < solver->longRedCls.size(); lev++) {
        auto& cc = solver->longRedCls[lev];
        for(const auto& offs: cc) {
            Clause* cl = solver->cl_alloc.ptr(offs);
            assert(!cl->getRemoved());
            assert(cl->red());
            assert(!cl->freed());
            if (cl->stats.locked_for_data_gen) {
                assert(cl->stats.which_red_array == 0);
//                 cout << "glue:" << cl->stats.glue << endl;
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

    set_prop_uip_act_ranks(all_learnt);
    solver->sqlStats->reduceDB_common(
        solver,
        reduceDB_called,
        all_learnt.size(),
        cur_rst_type,
        median_act,
        median_uip1_used,
        median_props,
        (double)total_glue/(double)all_learnt.size(),
        (double)total_props/(double)all_learnt.size(),
        (double)total_uip1_used/(double)all_learnt.size()
    );

    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);

        //Only if selected to be dumped
        if (cl->stats.ID == 0) {
            continue;
        }

        const bool locked = solver->clause_locked(*cl, offs);
        solver->sqlStats->reduceDB(
            solver
            , locked
            , cl
            , reduceDB_called
        );
        added_to_db++;
        cl->stats.dump_no++;
        cl->stats.reset_rdb_stats();
    }
    solver->sqlStats->end_transaction();

    if (solver->conf.verbosity) {
        cout << "c [sql] added to DB " << added_to_db
        << " dump-ratio: " << solver->conf.dump_individual_cldata_ratio
        << " locked-perc: " << stats_line_percent(num_locked_for_data_gen, all_learnt.size())
        << " non-locked lev0: " << non_locked_lev0
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }
    locked_for_data_gen_total += num_locked_for_data_gen;
    locked_for_data_gen_cls += all_learnt.size();
}
#endif

void ReduceDB::handle_lev1()
{
    #ifdef VERBOSE_DEBUG
    cout << "c handle_lev1()" << endl;
    #endif

    uint32_t moved_w0 = 0;
    uint32_t used_recently = 0;
    uint32_t non_recent_use = 0;
    double myTime = cpuTime();
    size_t orig_size = solver->longRedCls[1].size();

    size_t j = 0;
    for(size_t i = 0
        ; i < solver->longRedCls[1].size()
        ; i++
    ) {
        const ClOffset offset = solver->longRedCls[1][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        #ifdef VERBOSE_DEBUG
        cout << "offset: " << offset << " cl->stats.last_touched: " << cl->stats.last_touched
        << " act:" << std::setprecision(9) << cl->stats.activity
        << " which_red_array:" << cl->stats.which_red_array << endl
        << " -- cl:" << *cl << " tern:" << cl->stats.is_ternary_resolvent
        << endl;
        #endif

        assert(!cl->stats.locked_for_data_gen);
        if (cl->stats.which_red_array == 0) {
            solver->longRedCls[0].push_back(offset);
            moved_w0++;
        } else if (cl->stats.which_red_array == 2) {
            assert(false && "we should never move up through any other means");
        } else {
            uint32_t must_touch = solver->conf.must_touch_lev1_within;
            if (cl->stats.is_ternary_resolvent) {
                must_touch *= solver->conf.ternary_keep_mult;
            }
            if (!solver->clause_locked(*cl, offset)
                && cl->stats.last_touched + must_touch < solver->sumConflicts
            ) {
                solver->longRedCls[2].push_back(offset);
                cl->stats.which_red_array = 2;

                //when stats are needed, activities are correctly updated
                //across all clauses
                //WARNING this changes the way things behave during STATS relative to non-STATS!
                #ifndef STATS_NEEDED
                cl->stats.activity = 0;
                solver->bump_cl_act<false>(cl);
                #endif
                non_recent_use++;
            } else {
                solver->longRedCls[1][j++] = offset;
                used_recently++;
            }
        }
    }
    solver->longRedCls[1].resize(j);

    if (solver->conf.verbosity >= 2) {
        cout << "c [DBclean lev1]"
        << " confl: " << solver->sumConflicts
        << " orig size: " << orig_size
        << " used recently: " << used_recently
        << " not used recently: " << non_recent_use
        << " moved w0: " << moved_w0
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "dbclean-lev1"
            , cpuTime()-myTime
        );
    }
    total_time += cpuTime()-myTime;
}

#ifdef FINAL_PREDICTOR
void ReduceDB::pred_move_to_lev1_and_lev0()
{
    //FOREVER
    uint32_t marked_forever = 0;
    uint32_t keep_forever = 300 * solver->conf.pred_forever_chunk_mult;
    std::sort(solver->longRedCls[2].begin(), solver->longRedCls[2].end(),
              SortRedClsPredForever(solver->cl_alloc));
    size_t j = 0;
    for(uint32_t i = 0; i < solver->longRedCls[2].size(); i ++) {
        const ClOffset offset = solver->longRedCls[2][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        if (i < keep_forever) {
            marked_forever++;
            cl->stats.which_red_array = 0;
            solver->longRedCls[0].push_back(offset);
        } else {
            solver->longRedCls[2][j++] =solver->longRedCls[2][i];
        }
    }
    solver->longRedCls[2].resize(j);


    // LONG
    uint32_t marked_long = 0;
    uint32_t keep_long = 2000 * solver->conf.pred_long_chunk_mult;
    std::sort(solver->longRedCls[2].begin(), solver->longRedCls[2].end(),
              SortRedClsPredLong(solver->cl_alloc));

    j = 0;
    for(uint32_t i = 0; i < solver->longRedCls[2].size(); i ++) {
        const ClOffset offset = solver->longRedCls[2][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        if (i < keep_long) {
            marked_long++;
            cl->stats.which_red_array = 1;
            solver->longRedCls[1].push_back(offset);
        } else {
            solver->longRedCls[2][j++] =solver->longRedCls[2][i];
        }
    }
    solver->longRedCls[2].resize(j);
}

void ReduceDB::update_preds(const vector<ClOffset>& offs)
{
    vector<float> data(PRED_COLS*offs.size());

    uint32_t data_at = 0;
    for(size_t i = 0
        ; i < offs.size()
        ; i++
    ) {
        const ClOffset offset = offs[i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        double act_ranking_rel = safe_div(cl->stats.act_rank, commdata.all_learnt_size);
        double uip1_ranking_rel = safe_div(cl->stats.uip1_used_rank, commdata.all_learnt_size);
        double prop_ranking_rel = safe_div(cl->stats.props_made_rank, commdata.all_learnt_size);

        cl->stats.pred_short_use = 0;
        cl->stats.pred_long_use = 0;
        cl->stats.pred_forever_use = 0;
        if (cl->stats.dump_no > 0) {
            predictors->set_up_input(
                cl,
                solver->sumConflicts,
                act_ranking_rel,
                uip1_ranking_rel,
                prop_ranking_rel,
                commdata,
                PRED_COLS,
                data.data()+data_at*PRED_COLS
            );
            data_at++;
        }
    }

    predictors->predict_all(data.data(), data_at);

    uint32_t retrieve_at = 0;
    for(const auto& offset: offs) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        if (cl->stats.dump_no > 0) {
            predictors->get_prediction_at(cl, retrieve_at);
            retrieve_at++;

            if (solver->conf.pred_adjust_for_cl_size != 0) {
                double divby = safe_div(solver->stats.litsRedFinal, solver->stats.conflStats.numConflicts) * solver->conf.pred_adjust_for_cl_size;
                if (!solver->conf.pred_adjust_for_cl_size_onlyforever) {
                    cl->stats.pred_short_use = 1.0/(divby + (double)cl->size())*(double)cl->stats.pred_short_use;
                    cl->stats.pred_long_use = 1.0/(divby + (double)cl->size())*(double)cl->stats.pred_long_use;
                }
                cl->stats.pred_forever_use = 1.0/(divby + (double)cl->size())*(double)cl->stats.pred_forever_use;
            }
        }
    }

    predictors->finish_all_predict();
}

void ReduceDB::update_preds_lev2()
{
    double myTime = cpuTime();
    update_preds(solver->longRedCls[2]);

    if (solver->conf.verbosity >= 2) {
        double predTime = cpuTime() - myTime;
        cout << "c [DBCL] main predtime: " << predTime << endl;
    }
}

void ReduceDB::clean_lev0_once_in_a_while()
{
    //Deal with FOREVER once in a while
    if (num_times_pred_called % solver->conf.pred_forever_check_every_n !=
        (solver->conf.pred_forever_check_every_n-1)
    ) {
        return;
    }
    update_preds(solver->longRedCls[0]);

    //Clean up FOREVER, move to LONG
    const uint32_t checked_every = solver->conf.pred_forever_check_every_n *
        solver->conf.every_pred_reduce;

    const uint32_t keep_forever = 2000.0 *
        pow((double)solver->sumConflicts/10000.0, solver->conf.pred_forever_size_pow) *
        (double)solver->conf.pred_forever_size_mult;

    std::sort(solver->longRedCls[0].begin(), solver->longRedCls[0].end(),
          SortRedClsPredForever(solver->cl_alloc));

    int j = 0;
    for(uint32_t i = 0; i < solver->longRedCls[0].size(); i ++) {
        const ClOffset offset = solver->longRedCls[0][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        assert(!cl->freed());

        uint32_t time_inside_solver = solver->sumConflicts - cl->stats.introduced_at_conflict;
        if (i < keep_forever ||
            (time_inside_solver < checked_every/2 &&
            solver->conf.pred_dontmove_until_timeinside == 1) ||
            (time_inside_solver < checked_every &&
            solver->conf.pred_dontmove_until_timeinside == 2)

        ) {
            assert(cl->stats.which_red_array == 0);
            solver->longRedCls[0][j++] = solver->longRedCls[0][i];
        } else {
            solver->longRedCls[1].push_back(offset);
            cl->stats.which_red_array = 1;
        }
    }
    solver->longRedCls[0].resize(j);
}

void ReduceDB::clean_lev1_once_in_a_while()
{
    //Deal with LONG once in a while
    if (num_times_pred_called % solver->conf.pred_long_check_every_n !=
        (solver->conf.pred_long_check_every_n-1)
    ) {
        return;
    }

    update_preds(solver->longRedCls[1]);

    const uint32_t checked_every = solver->conf.pred_long_check_every_n * solver->conf.every_pred_reduce;

    //Move to FOREVER
//     uint32_t j = 0;
//     for(uint32_t i = 0; i < solver->longRedCls[1].size(); i ++) {
//         const ClOffset offset = solver->longRedCls[1][i];
//         Clause* cl = solver->cl_alloc.ptr(offset);
//         assert(!cl->freed());
//
//         bool moved = false;
//         if (cl->stats.pred_forever_use < solver->conf.pred_forever_use) {
//             cl->stats.which_red_array = 0;
//             solver->longRedCls[0].push_back(offset);
//             long_upgraded++;
//             moved = true;
//         }
//
//         if (!moved) {
//             solver->longRedCls[1][j++] =solver->longRedCls[1][i];
//         }
//     }
//     solver->longRedCls[1].resize(j);


    //Clean up LONG
    std::sort(solver->longRedCls[1].begin(), solver->longRedCls[1].end(),
          SortRedClsPredLong(solver->cl_alloc));
    uint32_t keep_long = 15000 * solver->conf.pred_long_size_mult;
    int j = 0;
    for(uint32_t i = 0; i < solver->longRedCls[1].size(); i ++) {
        const ClOffset offset = solver->longRedCls[1][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        assert(!cl->freed());

        uint32_t time_inside_solver = solver->sumConflicts - cl->stats.introduced_at_conflict;
        if (i < keep_long ||
            (time_inside_solver < checked_every/2 &&
            solver->conf.pred_dontmove_until_timeinside == 1) ||
            (time_inside_solver < checked_every &&
            solver->conf.pred_dontmove_until_timeinside == 2)
        ) {
            assert(cl->stats.which_red_array == 1);
            solver->longRedCls[1][j++] =solver->longRedCls[1][i];
        } else {
            solver->longRedCls[2].push_back(offset);
            cl->stats.which_red_array = 2;
        }
    }
    solver->longRedCls[1].resize(j);
}

void ReduceDB::delete_from_lev2()
{
    // SHORT
    uint32_t keep_short = 15000 * solver->conf.pred_short_size_mult;
    std::sort(solver->longRedCls[2].begin(), solver->longRedCls[2].end(),
              SortRedClsPredShort(solver->cl_alloc));

    uint32_t j = 0;
    force_kept_short = 0;
    for(uint32_t i = 0; i < solver->longRedCls[2].size(); i ++) {
        const ClOffset offset = solver->longRedCls[2][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        assert(!cl->freed());

        if (i < keep_short
            || solver->clause_locked(*cl, offset)
            || cl->stats.dump_no == 0
        ) {
            if (solver->clause_locked(*cl, offset)
                || cl->stats.dump_no == 0)
            {
                keep_short++;
                force_kept_short++;
            }
            solver->longRedCls[2][j++] = solver->longRedCls[2][i];
        } else {
            short_deleted++;
            short_deleted_dump_no += cl->stats.dump_no;
            solver->watches.smudge((*cl)[0]);
            solver->watches.smudge((*cl)[1]);
            solver->litStats.redLits -= cl->size();

            *solver->drat << del << *cl << fin;
            cl->setRemoved();
            delayed_clause_free.push_back(offset);
        }
    }
    solver->longRedCls[2].resize(j);
    if (solver->conf.verbosity) {
        cout << "c SHORT force-kept: " << force_kept_short
        << " kept: " << solver->longRedCls[2].size() << endl;
    }
}

ReduceDB::ClauseStats ReduceDB::reset_clause_dats(const uint32_t lev)
{
    ClauseStats cl_stat;
    uint32_t tot_dumpno = 0;
    for(const auto& off: solver->longRedCls[lev]) {
        Clause* cl = solver->cl_alloc.ptr(off);
        assert(!cl->freed());

        tot_dumpno += cl->stats.dump_no;
        cl->stats.dump_no++;
        cl_stat.add_in(*cl);
        cl->stats.reset_rdb_stats();
    }

    if (solver->conf.verbosity) {
        cout << "c [DBCL pred]"
        << " lev: " << lev
        << " avg dumpno: " << std::fixed << std::setprecision(2) << std::setw(6)
        << ratio_for_stat(tot_dumpno, solver->longRedCls[lev].size())
        << endl;
    }

    return cl_stat;
}

void ReduceDB::reset_predict_stats()
{
    ClauseStats this_stats;
    this_stats = reset_clause_dats(0);
    if (solver->conf.verbosity) {
        this_stats.print(0);
    }
    cl_stats[0] += this_stats;

    this_stats = reset_clause_dats(1);
    if (solver->conf.verbosity) {
        this_stats.print(1);
    }
    cl_stats[1] += this_stats;

    this_stats = reset_clause_dats(2);
    if (solver->conf.verbosity) {
        this_stats.print(2);
    }
    cl_stats[2] += this_stats;
}

void ReduceDB::handle_lev2_predictor()
{
    num_times_pred_called++;
    if (predictors == NULL) {
        predictors = new ClPredictors;
        predictors->load_models(
            solver->conf.pred_conf_short,
            solver->conf.pred_conf_long,
            solver->conf.pred_conf_forever);
    }

    assert(delayed_clause_free.empty());
    double myTime = cpuTime();

    vector<ClOffset> all_learnt;
    for(const auto& cls: solver->longRedCls) {
        for(const auto& offs: cls) {
            all_learnt.push_back(offs);
        }
    }
    set_prop_uip_act_ranks(all_learnt);
    commdata = ReduceCommonData(
        total_props, total_glue, total_uip1_used,
        all_learnt.size(),
        median_act);

    long_upgraded = 0;
    short_deleted = 0;
    long_deleted = 0;
    forever_deleted = 0;

    short_deleted_dump_no = 0;
    long_deleted_dump_no = 0;
    forever_deleted_dump_no = 0;

    update_preds_lev2();
    pred_move_to_lev1_and_lev0();
    delete_from_lev2();
    clean_lev0_once_in_a_while();
    clean_lev1_once_in_a_while();
    reset_predict_stats();

    //Cleanup
    solver->clean_occur_from_removed_clauses_only_smudged();
    for(ClOffset offset: delayed_clause_free) {
        solver->free_cl(offset);
    }
    delayed_clause_free.clear();

    //Stats
    if (solver->conf.verbosity >= 1) {
        cout
        << "c [DBCL pred]"
        << " short: " << print_value_kilo_mega(solver->longRedCls[2].size())
        << " long: "  << print_value_kilo_mega(solver->longRedCls[1].size())
        << " forever: "  << print_value_kilo_mega(solver->longRedCls[0].size())
        << endl;

        if (solver->conf.verbosity >= 1) {
        cout
        << "c [DBCL pred] lev0: " << std::setw(10) << solver->longRedCls[0].size()
        << " del: " << std::setw(7) << forever_deleted
        << " del avg dumpno: " << std::setw(6) << safe_div(forever_deleted_dump_no, forever_deleted)
        << endl

        << "c [DBCL pred] lev1: " << std::setw(10) << solver->longRedCls[1].size()
        << " del: " << std::setw(7) << long_deleted
        << " del avg dumpno: " << std::setw(6) << safe_div(long_deleted_dump_no, long_deleted)
        << endl

        << "c [DBCL pred] lev2: " << std::setw(10) << solver->longRedCls[2].size()
        << " del: " << std::setw(7) << short_deleted
        << " del avg dumpno: " << std::setw(6) << safe_div(short_deleted_dump_no, short_deleted)
        << endl

        << "c [DBCL pred] long-upgrade:         "  << std::setw(7)  << long_upgraded
        << endl;
        }

        cout
        << "c [DBCL pred] "
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "dbclean-lev1"
            , cpuTime()-myTime
        );
    }
    total_time += cpuTime()-myTime;
}
#endif

void ReduceDB::mark_top_N_clauses_lev2(const uint64_t keep_num)
{
    #ifdef VERBOSE_DEBUG
    cout << "Marking top N clauses " << keep_num << endl;
    #endif

    size_t marked = 0;
    for(size_t i = 0
        ; i < solver->longRedCls[2].size() && marked < keep_num
        ; i++
    ) {
        const ClOffset offset = solver->longRedCls[2][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        #ifdef VERBOSE_DEBUG
        cout << "offset: " << offset << " cl->stats.last_touched: " << cl->stats.last_touched
        << " act:" << std::setprecision(9) << cl->stats.activity
        << " which_red_array:" << cl->stats.which_red_array << endl
        << " -- cl:" << *cl << " tern:" << cl->stats.is_ternary_resolvent
        << endl;
        #endif

        if (cl->used_in_xor()
            || cl->stats.ttl > 0
            || solver->clause_locked(*cl, offset)
            || cl->stats.which_red_array != 2
        ) {
            //no need to mark, skip
            #ifdef VERBOSE_DEBUG
            cout << "Not marking Skipping "<< endl;
            #endif
            continue;
        }

        if (!cl->stats.marked_clause) {
            marked++;
            cl->stats.marked_clause = true;
            #ifdef VERBOSE_DEBUG
            cout << "Not marking Skipping "<< endl;
            #endif
        }
    }
}

bool ReduceDB::cl_needs_removal(const Clause* cl, const ClOffset offset) const
{
    assert(cl->red());
    return !cl->used_in_xor()
         && !cl->stats.marked_clause
         && cl->stats.ttl == 0
         && !solver->clause_locked(*cl, offset);
}

void ReduceDB::remove_cl_from_lev2() {
    size_t i, j;
    for (i = j = 0
        ; i < solver->longRedCls[2].size()
        ; i++
    ) {
        ClOffset offset = solver->longRedCls[2][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        assert(cl->size() > 2);

        //move to another array
        if (cl->stats.which_red_array < 2) {
            cl->stats.marked_clause = 0;
            solver->longRedCls[cl->stats.which_red_array].push_back(offset);
            continue;
        }
        assert(cl->stats.which_red_array == 2);

        //Check if locked, or marked or ttl-ed
        if (cl->stats.marked_clause) {
            cl_marked++;
        } else if (cl->stats.ttl != 0) {
            cl_ttl++;
        } else if (solver->clause_locked(*cl, offset)) {
            cl_locked_solver++;
        }

        if (!cl_needs_removal(cl, offset)) {
            if (cl->stats.ttl == 1) {
                cl->stats.ttl = 0;
            }
            solver->longRedCls[2][j++] = offset;
            cl->stats.marked_clause = 0;
            continue;
        }

        //Stats Update
        solver->watches.smudge((*cl)[0]);
        solver->watches.smudge((*cl)[1]);
        solver->litStats.redLits -= cl->size();

        *solver->drat << del << *cl << fin;
        cl->setRemoved();
        #ifdef VERBOSE_DEBUG
        cout << "REMOVING offset: " << offset << " cl->stats.last_touched: " << cl->stats.last_touched
        << " act:" << std::setprecision(9) << cl->stats.activity
        << " which_red_array:" << cl->stats.which_red_array << endl
        << " -- cl:" << *cl << " tern:" << cl->stats.is_ternary_resolvent
        << endl;
        #endif
        delayed_clause_free.push_back(offset);
    }
    solver->longRedCls[2].resize(j);
}

ReduceDB::ClauseStats ReduceDB::ClauseStats::operator += (const ClauseStats& other)
{
    total_looked_at += other.total_looked_at;
    total_uip1_used += other.total_uip1_used;
    total_props += other.total_props;
    total_cls += other.total_cls;
    total_dump_nos += other.total_dump_nos;

    return *this;
}

#if defined(NORMAL_CL_USE_STATS) || defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
void ReduceDB::ClauseStats::add_in(const Clause& cl)
{
    total_cls++;
    total_props += cl.stats.props_made;
    total_uip1_used += cl.stats.uip1_used;
    total_looked_at += cl.stats.clause_looked_at;
    total_dump_nos += cl.stats.dump_no;
}
#endif

void ReduceDB::ClauseStats::print(uint32_t lev)
{
    if (total_looked_at == 0 || total_cls == 0) {
        return;
    }

    cout
    << "c [cl-stats " << lev << "]"
    << " (U+P)/lookedat: "
    << std::setw(7) << std::setprecision(4)
    << (double)(total_uip1_used+total_props)/(double)total_looked_at
    << " U/lookedat: "
    << std::setw(7) << std::setprecision(4)
    << (double)(total_uip1_used)/(double)total_looked_at
    << " (U+P)/cls: "
    << std::setw(7) << std::setprecision(4)
    << (double)(total_uip1_used)/(double)total_cls
    << " avg dumpno: "
    << std::setw(7) << std::setprecision(4)
    << (double)(total_dump_nos)/(double)total_cls
    << endl;
}

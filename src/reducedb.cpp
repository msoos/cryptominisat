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

#include "reducedb.h"
#include "solver.h"
#include "solverconf.h"
#include "sqlstats.h"
#ifdef FINAL_PREDICTOR
#include "cl_predictors.h"
#endif

// #define VERBOSE_DEBUG

#include <functional>
#include <cmath>

using namespace CMSat;

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
        << " -- cl:" << *cl << " tern:" << cl->is_ternary_resolvent
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
        mark_top_N_clauses(keep_num);
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

#ifdef STATS_NEEDED
void ReduceDB::dump_sql_cl_data(
    const string& cur_rst_type
) {
    double myTime = cpuTime();
    assert(solver->sqlStats);
    solver->sqlStats->begin_transaction();
    uint64_t added_to_db = 0;

    vector<ClOffset> all_learnt;
    for(uint32_t lev = 0; lev < solver->longRedCls.size(); lev++) {
        auto& cc = solver->longRedCls[lev];
        for(const auto& offs: cc) {
            Clause* cl = solver->cl_alloc.ptr(offs);
            assert(!cl->getRemoved());
            assert(!cl->freed());
            all_learnt.push_back(offs);
        }
    }

    std::sort(all_learnt.begin(), all_learnt.end(), SortRedClsAct(solver->cl_alloc));
    for(size_t i = 0; i < all_learnt.size(); i++) {
        ClOffset offs = all_learnt[i];
        Clause* cl = solver->cl_alloc.ptr(offs);

        //Only if selected to be dumped
        if (cl->stats.ID == 0) {
            continue;
        }

        const bool locked = solver->clause_locked(*cl, offs);
        const uint32_t act_ranking_top_10 = std::ceil((double)i/((double)all_learnt.size()/10.0))+1;
        //cout << "Ranking top 10: " << act_ranking_top_10 << " act: " << cl->stats.activity << endl;
        solver->sqlStats->reduceDB(
            solver
            , locked
            , cl
            , cur_rst_type
            , act_ranking_top_10
            , i+1
            , all_learnt.size()
        );
        added_to_db++;
        cl->stats.dump_number++;
        cl->stats.reset_rdb_stats();
    }
    solver->sqlStats->end_transaction();

    if (solver->conf.verbosity) {
        cout << "c [sql] added to DB " << added_to_db
        << " dump-ratio: " << solver->conf.dump_individual_cldata_ratio
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }
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
        << " -- cl:" << *cl << " tern:" << cl->is_ternary_resolvent
        << endl;
        #endif

        if (cl->stats.which_red_array == 0) {
            solver->longRedCls[0].push_back(offset);
            moved_w0++;
        } else if (cl->stats.which_red_array == 2) {
            assert(false && "we should never move up through any other means");
        } else {
            uint32_t must_touch = solver->conf.must_touch_lev1_within;
            if (cl->is_ternary_resolvent) {
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
void ReduceDB::handle_lev3_final_predictor()
{
    if (predictors == NULL) {
        predictors = new ClPredictors;
        predictors->load_models(solver->conf.pred_conf_short, solver->conf.pred_conf_long);
    }

    assert(delayed_clause_free.empty());
    uint32_t deleted = 0;
    uint32_t kept_short = 0;
    uint32_t kept_locked = 0;
    uint32_t kept_dump_no = 0;
    double myTime = cpuTime();
    uint32_t tot_dumpno = 0;
    uint32_t dumpno_zero = 0;
    uint32_t dumpno_nonz = 0;
    uint32_t moved_w0 = 0;
    uint32_t marked_long_keep = 0;
    uint32_t kept_long = 0;
    uint32_t tot_short_keep = 0;

    std::sort(solver->longRedCls[3].begin(), solver->longRedCls[3].end(),
              SortRedClsAct(solver->cl_alloc));

    size_t j = 0;
    for(size_t i = 0
        ; i < solver->longRedCls[3].size()
        ; i++
    ) {
        const ClOffset offset = solver->longRedCls[3][i];
        Clause* cl = solver->cl_alloc.ptr(offset);

        if (cl->stats.which_red_array == 0) {
            solver->longRedCls[0].push_back(offset);
            moved_w0++;
        } else {
            const uint32_t act_ranking_top_10 = \
                std::ceil((double)i/((double)solver->longRedCls[3].size()/10.0))+1;
            double act_ranking_rel = ((double)i+1)/(double)solver->longRedCls[3].size();
            assert(act_ranking_rel != 0);

            int64_t last_touched_diff = (int64_t)solver->sumConflicts-(int64_t)cl->stats.last_touched;

            //Check for long keep
            if (cl->stats.locked_long == 0
                && cl->stats.dump_number > 0
                && predictors->predict_long(
                cl
                , solver->sumConflicts
                , last_touched_diff
                , act_ranking_rel
                , act_ranking_top_10) > solver->conf.pred_keep_above

            ) {
                marked_long_keep++;
                cl->stats.locked_long = 5; //will be immediately decremented below
            }

            const bool locked = solver->clause_locked(*cl, offset);
            if (cl->stats.locked_long == 0
                && cl->stats.dump_number > 0 //don't delete 1st time around
                && !locked
                && predictors->predict_short(
                cl
                , solver->sumConflicts
                , last_touched_diff
                , act_ranking_rel
                , act_ranking_top_10+1) < (1.0f-solver->conf.pred_keep_above)
            ) {
                deleted++;
                solver->watches.smudge((*cl)[0]);
                solver->watches.smudge((*cl)[1]);
                solver->litStats.redLits -= cl->size();
                assert(cl->stats.dump_number > 0 && "or rdb1 data is wrong!");

                *solver->drat << del << *cl << fin;
                cl->setRemoved();
                delayed_clause_free.push_back(offset);
            } else {
                if (cl->stats.locked_long > 0) {
                    kept_long++;
                    cl->stats.locked_long--;
                } else if (locked) {
                    kept_locked++;
                } else if (cl->stats.dump_number == 0) {
                    kept_dump_no++;
                } else {
                    kept_short++;
                }
                solver->longRedCls[3][j++] = offset;
                tot_dumpno += cl->stats.dump_number;
                dumpno_zero += (cl->stats.dump_number == 0);
                dumpno_nonz += (cl->stats.dump_number != 0);
                cl->stats.dump_number++;
                cl->stats.rdb1_act_ranking_top_10 = act_ranking_top_10;
                cl->stats.rdb1_act_ranking_rel = act_ranking_rel;
                cl->stats.rdb1_last_touched_diff = last_touched_diff;
                cl->stats.rdb1_used_for_uip_creation = cl->stats.used_for_uip_creation;
                cl->stats.rdb1_propagations_made = cl->stats.propagations_made;
                cl->stats.reset_rdb_stats();
            }
        }
    }
    solver->longRedCls[3].resize(j);

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
        << " del: "    << print_value_kilo_mega(deleted)
        << " kshort: " << print_value_kilo_mega(kept_short)
        << " klong: "  << print_value_kilo_mega(kept_long)
        << " kdump0: " << print_value_kilo_mega(kept_dump_no)
        << " klock: "  << print_value_kilo_mega(kept_locked)
        << endl;

        cout << "c [DBCL pred]"
        << " marked-long: "<< print_value_kilo_mega(marked_long_keep)
//         << " conf-short: " << solver->conf.pred_conf_short
//         << " conf-long: " << solver->conf.pred_conf_long
        << endl;

        cout << "c [DBCL pred]"
        << " avg dumpno: " << std::fixed << std::setprecision(2)
        << ratio_for_stat(tot_dumpno, solver->longRedCls[3].size())
        << " dumpno_zero: "   << print_value_kilo_mega(dumpno_zero)
        << " dumpno_nonz: "   << print_value_kilo_mega(dumpno_nonz)
        << " tot_short_keep:" << print_value_kilo_mega(tot_short_keep)
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }
    assert(moved_w0 == 0);

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

void ReduceDB::mark_top_N_clauses(const uint64_t keep_num)
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
        << " -- cl:" << *cl << " tern:" << cl->is_ternary_resolvent
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
            if (cl->stats.ttl > 0) {
                cl->stats.ttl--;
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
        << " -- cl:" << *cl << " tern:" << cl->is_ternary_resolvent
        << endl;
        #endif
        delayed_clause_free.push_back(offset);
    }
    solver->longRedCls[2].resize(j);
}

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

#ifndef __REDUCEDB_H__
#define __REDUCEDB_H__

#include "clauseallocator.h"
#ifdef FINAL_PREDICTOR
#include "cl_predictors.h"
#endif

namespace CMSat {

class Solver;
class ClPredictors;
class ClusteringImp;

class ReduceDB
{
public:
    ReduceDB(Solver* solver);
    ~ReduceDB();
    double get_total_time() const {
        return total_time;
    }
    void handle_lev1();
    void handle_lev2();
    void gather_normal_cl_use_stats();
    #ifdef FINAL_PREDICTOR
    void handle_predictors();
    #endif
    void dump_sql_cl_data(const uint32_t cur_rst_type);

    #ifdef STATS_NEEDED
    uint32_t reduceDB_called = 0;
    uint64_t locked_for_data_gen_total = 0;
    uint64_t locked_for_data_gen_cls = 0;
    #endif

    struct ClauseStats
    {
        uint32_t total_uip1_used = 0;
        uint32_t total_props = 0;
        uint32_t total_cls = 0;
        uint64_t total_age = 0;

        void add_in(const Clause& cl, const uint64_t age);
        ClauseStats operator += (const ClauseStats& other);
        void print(uint32_t lev);
    };
    vector<ClauseStats> cl_stats;

private:

    Solver* solver;
    vector<ClOffset> delayed_clause_free;
    double total_time = 0.0;

    unsigned cl_marked;
    unsigned cl_ttl;
    unsigned cl_locked_solver;

    size_t last_reducedb_num_conflicts = 0;
    bool red_cl_too_young(const Clause* cl) const;
    void clear_clauses_stats(vector<ClOffset>& clauseset);
    ClauseStats reset_clause_dats(const uint32_t lev);

    bool cl_needs_removal(const Clause* cl, const ClOffset offset) const;
    void remove_cl_from_lev2();

    void sort_red_cls(ClauseClean clean_type);
    void mark_top_N_clauses_lev2(const uint64_t keep_num);

    #ifdef FINAL_PREDICTOR
    ClPredictors* predictors = NULL;
    uint32_t num_times_pred_called = 0;
    void update_preds_lev2();
    void pred_move_to_lev1_and_lev0();
    void delete_from_lev2();
    void clean_lev1_once_in_a_while();
    void clean_lev0_once_in_a_while();
    void reset_predict_stats();
    void update_preds(const vector<ClOffset>& offs);
    ReduceCommonData commdata;
    #endif

    void prepare_features(vector<ClOffset>& all_learnt);
    uint32_t total_glue = 0;
    uint32_t total_props = 0;
    uint32_t total_uip1_used = 0;
    uint32_t total_sum_uip1_used = 0;
    MedianCommonDataRDB median_data;
    uint32_t force_kept_short = 0;

    uint32_t short_deleted;
    uint32_t long_moved;
    uint32_t forever_moved;

    uint64_t short_deleted_age;

    uint32_t long_upgraded = 0;
};

}

#endif //__REDUCEDB_H__

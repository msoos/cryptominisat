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

#ifndef __REDUCEDB_H__
#define __REDUCEDB_H__

#include "clauseallocator.h"

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
    #ifdef FINAL_PREDICTOR
    void handle_lev2_predictor();
    #endif
    void dump_sql_cl_data(const string& cur_rst_type);

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

    bool cl_needs_removal(const Clause* cl, const ClOffset offset) const;
    void remove_cl_from_lev2();

    void sort_red_cls(ClauseClean clean_type);
    void mark_top_N_clauses(const uint64_t keep_num);

    #ifdef FINAL_PREDICTOR
    ClPredictors* predictors = NULL;
    uint32_t num_times_lev3_called = 0;
    #endif
};

}

#endif //__REDUCEDB_H__

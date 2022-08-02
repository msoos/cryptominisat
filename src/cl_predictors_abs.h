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

#ifndef _CLPREDICTORS_ABST_H__
#define _CLPREDICTORS_ABST_H__

#include <vector>
#include <cassert>
#include <string>
#include <cmath>
#include <xgboost/c_api.h>
#include "clause.h"

#define PRED_COLS 22

using std::vector;

namespace CMSat {

enum predict_type {short_pred=0, long_pred=1, forever_pred=2};

class Clause;
class Solver;

struct ReduceCommonData
{
    double safe_div(double a, double b) {
        if (b == 0) {
            assert(a == 0);
            return 0;
        }
        return a/b;
    }

    double   avg_props;
    //double   avg_glue; CANNOT COUNT, ternary has no glue!
    double   avg_uip;
    double   avg_sum_uip1_used;
    MedianCommonDataRDB  median_data;
    uint32_t all_learnt_size;

    ReduceCommonData() {}
    ReduceCommonData(
        uint32_t total_props,
//         uint32_t total_glue,
        uint32_t total_uip1_used,
        uint32_t total_sum_uip1_used,
        uint32_t size,
        const MedianCommonDataRDB& _median_data) :
            median_data(_median_data)
    {
        all_learnt_size = size;
        avg_props = safe_div(total_props, size);
        //avg_glue = safe_div(total_glue, size);
        avg_uip = safe_div(total_uip1_used, size);
        avg_sum_uip1_used = safe_div(total_sum_uip1_used, size);
    }
};

class ClPredictorsAbst
{
public:
    ClPredictorsAbst() {missing_val = nanf("");}
    virtual ~ClPredictorsAbst() {}
    virtual int load_models(const std::string& short_fname,
                     const std::string& long_fname,
                     const std::string& forever_fname,
                     const std::string& best_feats_fname) = 0;
    virtual int load_models_from_buffers() = 0;
    vector<std::string> get_hashes() const;

    virtual void predict_all(
        float* const data,
        const uint32_t num) = 0;

    virtual int get_step_size() {return PRED_COLS;}

    virtual int set_up_input(
        const CMSat::Clause* const cl,
        const uint64_t sumConflicts,
        const double   act_ranking_rel,
        const double   uip1_ranking_rel,
        const double   prop_ranking_rel,
        const double   sum_uip1_per_time_ranking,
        const double   sum_props_per_time_ranking,
        const double   sum_uip1_per_time_ranking_rel,
        const double   sum_props_per_time_ranking_rel,
        const ReduceCommonData& commdata,
        const Solver* solver,
        float* at);

    virtual void get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at) = 0;
    virtual void finish_all_predict() = 0;
    float missing_val;
};

}

#endif

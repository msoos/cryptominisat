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

#ifndef __CLPREDICTOR_H__
#define __CLPREDICTOR_H__

#define PRED_COLS 28


#include <vector>
#include <cassert>
#include <string>
#include <xgboost/c_api.h>
#include "clause.h"

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
    double   avg_glue;
    double   avg_uip;
    float    median_act;
    uint32_t all_learnt_size;

    ReduceCommonData() {}
    ReduceCommonData(
        uint32_t total_props,
        uint32_t total_glue,
        uint32_t total_uip1_used,
        uint32_t size,
        float _median_act)
    {
        all_learnt_size = size;
        median_act = _median_act;
        avg_props = safe_div(total_props, size);
        avg_glue = safe_div(total_glue, size);
        avg_uip = safe_div(total_uip1_used, size);
    }
};

class ClPredictors
{
public:
    ClPredictors();
    ~ClPredictors();
    void load_models(const std::string& short_fname,
                     const std::string& long_fname,
                     const std::string& forever_fname);

    float predict(
        predict_type pred_type,
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const double   act_ranking_rel,
        const double   uip1_ranking_rel,
        const double   prop_ranking_rel,
        const ReduceCommonData& commdata
    );

    void predict_all(
        float* data,
        uint32_t num);

    void set_up_input(
        const CMSat::Clause* const cl,
        const uint64_t sumConflicts,
        const double   act_ranking_rel,
        const double   uip1_ranking_rel,
        const double   prop_ranking_rel,
        const ReduceCommonData& commdata,
        const uint32_t cols,
        const Solver* solver,
        float* at);

    void get_prediction_at(RDBExtraData& extdata, const uint32_t at);
    void finish_all_predict();

private:
    float predict_one(int num);
    vector<BoosterHandle> handles;
    float train[PRED_COLS];
    DMatrixHandle dmat;

    const float *out_result_short;
    const float *out_result_long;
    const float *out_result_forever;
};

}

#endif

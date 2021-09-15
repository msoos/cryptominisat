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

#ifndef _CLPREDICTORS_H__
#define _CLPREDICTORS_H__

#include <vector>
#include <cassert>
#include <string>
#include <xgboost/c_api.h>
#include "clause.h"
#include "cl_predictors_abs.h"

using std::vector;

namespace CMSat {

class Clause;
class Solver;

class ClPredictors : public ClPredictorsAbst
{
public:
    ClPredictors();
    virtual ~ClPredictors();
    virtual void load_models(const std::string& short_fname,
                     const std::string& long_fname,
                     const std::string& forever_fname) override;
    virtual void load_models_from_buffers() override;

    float predict(
        predict_type pred_type,
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const double   act_ranking_rel,
        const double   uip1_ranking_rel,
        const double   prop_ranking_rel,
        const ReduceCommonData& commdata
    );

    virtual void predict_all(
        float* data,
        uint32_t num) override;

    virtual void get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at) override;
    virtual void finish_all_predict() override;

private:
    vector<BoosterHandle> handles;
    float train[PRED_COLS];
    DMatrixHandle dmat;

    const float *out_result_short;
    const float *out_result_long;
    const float *out_result_forever;
};

}

#endif

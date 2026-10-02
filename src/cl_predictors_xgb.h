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

#ifndef _CLPREDICTORS_XGB_H__
#define _CLPREDICTORS_XGB_H__

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

class ClPredictorsXGB : public ClPredictorsAbst
{
public:
    ClPredictorsXGB();
    virtual ~ClPredictorsXGB();
    virtual int load_models(const vector<std::string>& fnames,
                     const std::string& best_feats_fname) override;
    virtual int load_models_from_buffers(const vector<std::string>& tiers) override;

    virtual void predict_all(
        float* const data,
        const uint32_t num) override;

    virtual void get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at) override;
    virtual void finish_all_predict() override;

private:
    vector<BoosterHandle> handles;
    DMatrixHandle dmat;

    const float* out_result[PRED_MAX_MODELS];
    void new_handle();
    void check_num_features(const std::string& what);

    //debugging
    int num_dumps = 0;
};

}

#endif

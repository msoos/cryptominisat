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

#ifndef __CLPREDICTOR_H__
#define __CLPREDICTOR_H__

#define PRED_COLS 16

#include <vector>
#include <string>
#include <xgboost/c_api.h>

using std::vector;

namespace CMSat {

enum predict_type {short_pred=0, long_pred=1, forever_pred=2};

class Solver;
class Clause;

class ClPredictors
{
public:
    ClPredictors(Solver* solver);
    ~ClPredictors();
    void load_models(const std::string& short_fname,
                     const std::string& long_fname,
                     const std::string& forever_fname);

    float predict(
        predict_type pred_type,
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const int64_t  last_touched_diff,
        const double   act_ranking_rel,
        const uint32_t act_ranking_top_10);

    void predict(
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const int64_t last_touched_diff,
        const double   act_ranking_rel,
        const uint32_t act_ranking_top_10,
        float& p_short,
        float& p_long,
        float& p_forever);

    void start_adding_cls();
    const vector<vector<float>>& do_predict_many_alltypes();
    const vector<vector<float>>& do_predict_many_onetype(predict_type which);
    void add_single_cl(
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const int64_t  last_touched_diff,
        const double   act_ranking_rel,
        const uint32_t act_ranking_top_10);

private:
    float predict_one(int num);
    void set_up_input(
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const int64_t  last_touched_diff,
        const double   act_ranking_rel,
        const uint32_t act_ranking_top_10,
        const uint32_t cols,
        float* at);
    vector<BoosterHandle> handles;
    float train[PRED_COLS];
    vector<float> multi_data;
    DMatrixHandle dmat;
    Solver* solver;
    vector<vector<float>> multi_ret;
};

}

#endif

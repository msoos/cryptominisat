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

#define PRED_COLS 25

#include <vector>
#include <string>
#include <xgboost/c_api.h>

using std::vector;

namespace CMSat {

class Solver;
class Clause;

class ClPredictors
{
public:
    ClPredictors();
    ~ClPredictors();
    void load_models(std::string short_fname, std::string long_fname);
    float predict_short(const CMSat::Clause* cl
                , const uint64_t sumConflicts
                , const int64_t last_touched_diff
                , const double   act_ranking_rel
                , const uint32_t act_ranking_top_10);

    float predict_long(
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const int64_t  last_touched_diff,
        const double   act_ranking_rel,
        const uint32_t act_ranking_top_10);

private:
    float predict_one(int num, DMatrixHandle dmat);
    void set_up_input(
        const CMSat::Clause* cl,
        const uint64_t sumConflicts,
        const int64_t  last_touched_diff,
        const double   act_ranking_rel,
        const uint32_t act_ranking_top_10,
        float *train,
        const uint32_t cols);
    vector<BoosterHandle> handles;
    float train[PRED_COLS];
    DMatrixHandle dmat;
};

}

#endif

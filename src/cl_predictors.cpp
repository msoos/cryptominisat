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

#include "cl_predictors.h"
#include "clause.h"

using namespace CMSat;

ClPredictors::ClPredictors()
{
    BoosterHandle handle;
    int ret;

    handles.push_back(handle);
    ret = XGBoosterCreate(0, 0, &(handles[0]));
    assert(ret == 0);
    ret = XGBoosterSetParam(handles[0], "nthread", "1");
    assert(ret == 0);

    handles.push_back(handle);
    ret = XGBoosterCreate(0, 0, &(handles[1]));
    assert(ret == 0);
    ret = XGBoosterSetParam(handles[1], "nthread", "1");
    assert(ret == 0);
}

ClPredictors::~ClPredictors()
{
    for(auto& h: handles) {
        XGBoosterFree(h);
    }
}

void ClPredictors::load_models(std::string short_fname, std::string long_fname)
{
    int ret;
    ret = XGBoosterLoadModel(handles[0], short_fname.c_str());
    assert(ret == 0);
    ret =XGBoosterLoadModel(handles[1], long_fname.c_str());
    assert(ret == 0);
}

void ClPredictors::set_up_input(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t last_touched_diff,
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10,
    float *train,
    const uint32_t cols)
{
    float *at = train;
    int x = 0;
    at[x++] = cl->stats.glue_hist_long;                           //cl.glue_hist_long
    at[x++] = cl->stats.glue_hist_queue;                          //cl.glue_hist_queue
    at[x++] = cl->stats.glue_hist;                                //cl.glue_hist
    at[x++] = cl->stats.size_hist;                                //cl.size_hist
    at[x++] = cl->stats.glue_before_minim;                        //cl.glue_before_minim
    at[x++] = cl->stats.orig_glue;                                //cl.orig_glue
    at[x++] = cl->stats.glue;                                     //rdb0.glue
    at[x++] = cl->size();                                         //rdb0.size
    at[x++] = cl->stats.used_for_uip_creation;                    //rdb0.used_for_uip_creation
    at[x++] = cl->stats.rdb1_used_for_uip_creation;               //rdb1.used_for_uip_creation
    at[x++] = cl->stats.num_overlap_literals;                     //cl.num_overlap_literals
    at[x++] = cl->stats.antec_overlap_hist;                       //cl.antec_overlap_hist
    at[x++] = cl->stats.num_total_lits_antecedents;               //cl.num_total_lits_antecedents
    at[x++] = cl->stats.rdb1_last_touched_diff;                   //rdb1.last_touched_diff
    at[x++] = cl->stats.num_antecedents;                          //cl.num_antecedents
    at[x++] = cl->stats.branch_depth_hist_queue;                  //cl.branch_depth_hist_queue
    at[x++] = cl->stats.num_resolutions_hist_lt;                  //cl.num_resolutions_hist_lt
    at[x++] = cl->stats.trail_depth_hist_longer;                  //cl.trail_depth_hist_longer
    at[x++] = act_ranking_rel;                                    //rdb0_act_ranking_rel
    at[x++] = cl->stats.propagations_made;                        //rdb0.propagations_made
    at[x++] = cl->stats.rdb1_propagations_made;                   //rdb1.propagations_made
    at[x++] = act_ranking_top_10;                                 //rdb0.act_ranking_top_10
    at[x++] = cl->stats.rdb1_act_ranking_top_10;                  //rdb1.act_ranking_top_10
    at[x++] = cl->stats.is_decision;                              //cl.is_decision
    at[x++] = cl->is_ternary_resolvent;                           //rdb0.is_ternary_resolvent
    assert(x==cols);
}

float ClPredictors::predict_one(int num, DMatrixHandle dmat)
{
    bst_ulong out_len;
    const float *out_result;
    int ret = XGBoosterPredict(
        handles[num],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result
    );

    float retval = out_result[0];
    return retval;
}

float ClPredictors::predict_short(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t last_touched_diff,
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10)
{
    // convert to DMatrix
    int cols=25;
    float* train = new float[cols];
    set_up_input(cl, sumConflicts, last_touched_diff,
                 act_ranking_rel, act_ranking_top_10,
                 train, cols);
    int rows=1;
    DMatrixHandle dmat;
    int ret = XGDMatrixCreateFromMat((float *)train, rows, cols, -1, &dmat);
    assert(ret == 0);
    delete[] train;

    return predict_one(0, dmat);
}

float ClPredictors::predict_long(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t last_touched_diff,
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10)
{
    // convert to DMatrix
    int cols=25;
    float* train = new float[cols];
    set_up_input(cl, sumConflicts, last_touched_diff,
                 act_ranking_rel, act_ranking_top_10,
                 train, cols);
    int rows=1;
    DMatrixHandle dmat;
    int ret = XGDMatrixCreateFromMat((float *)train, rows, cols, -1, &dmat);
    assert(ret == 0);
    delete[] train;

    return predict_one(1, dmat);
}

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

#include "cl_predictors.h"
#include "clause.h"
#include <cmath>
#define MISSING_VAL -1.0f

#define safe_xgboost(call) {  \
  int err = (call); \
  if (err != 0) { \
    fprintf(stderr, "%s:%d: error in %s: %s\n", __FILE__, __LINE__, #call, XGBGetLastError());  \
    exit(1); \
  } \
}

using namespace CMSat;

ClPredictors::ClPredictors()
{
    handles.resize(3);
    safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::short_pred])))
    safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::long_pred])))
    safe_xgboost(XGBoosterCreate(0, 0, &(handles[predict_type::forever_pred])))

    for(int i = 0; i < 3; i++) {
        safe_xgboost(XGBoosterSetParam(handles[i], "nthread", "1"))
        //safe_xgboost(XGBoosterSetParam(handles[i], "verbosity", "3"))
    }
}

ClPredictors::~ClPredictors()
{
    for(auto& h: handles) {
        XGBoosterFree(h);
    }
}

void ClPredictors::load_models(const std::string& short_fname,
                               const std::string& long_fname,
                               const std::string& forever_fname)
{
    safe_xgboost(XGBoosterLoadModel(handles[predict_type::short_pred], short_fname.c_str()))
    safe_xgboost(XGBoosterLoadModel(handles[predict_type::long_pred], long_fname.c_str()))
    safe_xgboost(XGBoosterLoadModel(handles[predict_type::forever_pred], forever_fname.c_str()))

//     bst_ulong num_features = 0;
//     safe_xgboost(XGBoosterGetNumFeature(handles[predict_type::short_pred], &num_features));
//     cout << "num_features: " << num_features << endl;
//     assert(num_features == PRED_COLS);
//     safe_xgboost(XGBoosterGetNumFeature(handles[predict_type::long_pred], &num_features));
//     cout << "num_features: " << num_features << endl;
//     assert(num_features == PRED_COLS);
//     safe_xgboost(XGBoosterGetNumFeature(handles[predict_type::forever_pred], &num_features));
//     cout << "num_features: " << num_features << endl;
//     assert(num_features == PRED_COLS);
}

void ClPredictors::set_up_input(
    const CMSat::Clause* const cl,
    const uint64_t sumConflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const ReduceCommonData& commdata,
    const uint32_t cols,
    float* at)
{
    uint32_t x = 0;
    //glue 0 can happen in case it's a ternary resolvent clause
    //updated glue can actually be 1. Original glue cannot.
    assert(cl->stats.orig_glue != 1);

    assert(cl->stats.last_touched <= sumConflicts);
    assert(cl->stats.introduced_at_conflict <= sumConflicts);
    uint32_t last_touched_diff = sumConflicts - (uint64_t)cl->stats.last_touched;
    double time_inside_solver = sumConflicts - (uint64_t)cl->stats.introduced_at_conflict;


    at[x++] = uip1_ranking_rel;
   //rdb0.uip1_ranking_rel  -- 1


    if (last_touched_diff == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = act_ranking_rel/(double)last_touched_diff;
    }
    //(rdb0.act_ranking_rel/rdb0.last_touched_diff) -- 2


    at[x++] = prop_ranking_rel;
    //rdb0.prop_ranking_rel -- 3


    if (commdata.avg_props == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.props_made/(double)commdata.avg_props;
    }
    //(rdb0.props_made/rdb0_common.avg_props) -- 4


    at[x++] = (double)last_touched_diff;
    //rdb0.last_touched_diff -- 5


    if (cl->stats.is_ternary_resolvent ||
        commdata.median_act == 0
    ) { //glue_hist_avg not valid in this case
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue_hist_avg/(double)commdata.median_act;
    }
    //(cl.glue_hist_avg/rdb0_common.median_act) -- 6


    at[x++] = (double)cl->stats.glue;
    //rdb0.glue -- 7


    if (time_inside_solver  == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.sum_props_made/time_inside_solver;
    }
    //(rdb0.sum_props_made/cl.time_inside_solver) -- 8


    if (time_inside_solver == 0 ||
        commdata.avg_glue == 0 ||
        cl->stats.glue == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)cl->stats.sum_props_made/time_inside_solver)/
            ((double)cl->stats.glue/commdata.avg_glue);
    }
    //((rdb0.sum_props_made/cl.time_inside_solver)/(rdb0.glue/rdb0_common.avg_glue)) -- 9


    if (cl->stats.is_ternary_resolvent || //glue_before_minim does not exist for ternary
        time_inside_solver == 0 ||
        cl->stats.sum_uip1_used == 0 ||
        cl->stats.glue_before_minim == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2(cl->stats.glue_before_minim)/
            ((double)cl->stats.sum_uip1_used/time_inside_solver);
    }
    //(log2(cl.glue_before_minim)/(rdb0.sum_uip1_used/cl.time_inside_solver)) -- 10


    at[x++] = cl->stats.orig_glue;
    //cl.orig_glue -- 11


    if (cl->stats.is_ternary_resolvent ||
        cl->stats.num_antecedents == 0 ||
        cl->stats.num_total_lits_antecedents == 0) //num_antecedents does not exist for ternary
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2((double)cl->stats.num_antecedents)/(double)cl->stats.num_total_lits_antecedents;
    }
    //(log2(cl.num_antecedents)/cl.num_total_lits_antecedents) -- 12


    if (cl->stats.is_ternary_resolvent ||
        cl->stats.glue_before_minim == 0 //glue_hist_longterm_avg does not exist for ternary
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue_hist_longterm_avg/(double)cl->stats.glue_before_minim;
    }
    //(cl.glue_hist_longterm_avg/cl.glue_before_minim) -- 13


    at[x++] = cl->stats.is_ternary_resolvent;
    //rdb0.is_ternary_resolvent -- 14


    if (cl->stats.is_ternary_resolvent || //num_resolutions_hist_lt does not exist for ternary
        cl->stats.num_resolutions_hist_lt == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.discounted_props_made/(double)cl->stats.num_resolutions_hist_lt;
    }
    //(rdb0.discounted_props_made/cl.num_resolutions_hist_lt) -- 15


    if (cl->stats.discounted_props_made < 1e-20 || time_inside_solver == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)cl->stats.sum_uip1_used/time_inside_solver)/((double)cl->stats.discounted_props_made);
    }
    //((rdb0.sum_uip1_used/cl.time_inside_solver)/rdb0.discounted_props_made) -- 16


    if (commdata.avg_props == 0 || cl->stats.props_made == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)cl->stats.glue)/
            ((double)cl->stats.props_made/ (double)commdata.avg_props);
    }
    //(rdb0.glue/(rdb0.props_made/rdb0_common.avg_props)) -- 17


    if (cl->stats.is_ternary_resolvent || //num_total_lits_antecedents does not exist for ternary
        cl->stats.num_total_lits_antecedents == 0 ||
        time_inside_solver == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)cl->stats.sum_props_made/(double)time_inside_solver)/
            (double)cl->stats.num_total_lits_antecedents;
    }
    // ((rdb0.sum_props_made/cl.time_inside_solver)/cl.num_total_lits_antecedents) -- 18


    if (cl->stats.is_ternary_resolvent || //glue and glue_histlt_avg does not exist for ternary
        cl->stats.glue_histlt_avg > 1e-20 ||
        time_inside_solver == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue/(double)cl->stats.glue_histlt_avg;
    }
    // (rdb0.glue/cl.glue_histlt_avg) -- 19


    if (commdata.avg_uip == 0 ||
        cl->stats.uip1_used == 0 ||
        prop_ranking_rel == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)prop_ranking_rel/
            ((double)cl->stats.uip1_used/(double)commdata.avg_uip);
    }
    // (rdb0.prop_ranking_rel/(rdb0.uip1_used/rdb0_common.avg_uip1_used)) -- 20


    if (cl->stats.is_ternary_resolvent || // size_hist and overlap_hist do not exist for tri
        cl->stats.confl_size_hist == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.antec_overlap_hist_lt/
            ((double)cl->stats.confl_size_hist);
    }
    // (cl.antec_overlap_hist_lt/cl.confl_size_hist) -- 21


    if (cl->stats.is_ternary_resolvent || // glue_histlt_avg do not exist for tri
        cl->stats.uip1_used == 0

    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue_histlt_avg/
            ((double)cl->stats.uip1_used);
    }
    // (cl.glue_histlt_avg/rdb0.uip1_used) -- 22


    if (commdata.avg_glue == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->size()/
            ((double)commdata.avg_glue);
    }
    // (rdb0.size/rdb0_common.avg_glue) -- 23

//     cout << "c val: ";
//     for(uint32_t i = 0; i < cols; i++) {
//         cout << at[i] << " ";
//     }
//     cout << endl;

    assert(x==cols);
}

float ClPredictors::predict_one(int num)
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
    assert(ret == 0);
    assert(out_len == 1);

    float retval = out_result[0];
    return retval;
}

float ClPredictors::predict(
    predict_type pred_type,
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const ReduceCommonData& commdata)
{
    // convert to DMatrix
    set_up_input(
        cl,
        sumConflicts,
        act_ranking_rel,
        uip1_ranking_rel,
        prop_ranking_rel,
        commdata,
        PRED_COLS,
        train);

    safe_xgboost(XGDMatrixCreateFromMat((float *)train, 1, PRED_COLS, MISSING_VAL, &dmat))

    float val = predict_one(pred_type);
    safe_xgboost(XGDMatrixFree(dmat))

    return val;
}

void ClPredictors::predict(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const ReduceCommonData& commdata,
    float& p_short,
    float& p_long,
    float& p_forever)
{
    // convert to DMatrix
    set_up_input(
        cl,
        sumConflicts,
        act_ranking_rel,
        uip1_ranking_rel,
        prop_ranking_rel,
        commdata,
        PRED_COLS,
        train);

    safe_xgboost(XGDMatrixCreateFromMat((float *)train, 1, PRED_COLS, MISSING_VAL, &dmat))
    p_short = predict_one(short_pred);
    safe_xgboost(XGDMatrixFree(dmat))

    safe_xgboost(XGDMatrixCreateFromMat((float *)train, 1, PRED_COLS, MISSING_VAL, &dmat))
    p_long = predict_one(long_pred);
    safe_xgboost(XGDMatrixFree(dmat))

    safe_xgboost(XGDMatrixCreateFromMat((float *)train, 1, PRED_COLS, MISSING_VAL, &dmat))
    p_forever = predict_one(forever_pred);
    safe_xgboost(XGDMatrixFree(dmat))
}

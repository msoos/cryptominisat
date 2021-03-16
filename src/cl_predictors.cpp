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
#include "solver.h"
#include <cmath>
#define MISSING_VAL -1.0f
extern char predictor_short_json[];
extern unsigned int predictor_short_json_len;
extern const char* predictor_short_json_hash;

extern char predictor_long_json[];
extern unsigned int predictor_long_json_len;
extern const char* predictor_long_json_hash;

extern char predictor_forever_json[];
extern unsigned int predictor_forever_json_len;
extern const char* predictor_forever_json_hash;

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

void ClPredictors::load_models_from_buffers()
{
    safe_xgboost(XGBoosterLoadModelFromBuffer(
        handles[predict_type::short_pred], predictor_short_json, predictor_short_json_len));
    safe_xgboost(XGBoosterLoadModelFromBuffer(
        handles[predict_type::long_pred], predictor_long_json, predictor_long_json_len));
    safe_xgboost(XGBoosterLoadModelFromBuffer(
        handles[predict_type::forever_pred], predictor_forever_json, predictor_forever_json_len))
}

vector<std::string> ClPredictors::get_hashes() const
{
    vector<std::string> ret;
    ret.push_back(string(predictor_short_json_hash));
    ret.push_back(string(predictor_long_json_hash));
    ret.push_back(string(predictor_forever_json_hash));

    return ret;
}

void ClPredictors::set_up_input(
    const CMSat::Clause* const cl,
    const uint64_t sumConflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const double   sum_uip1_per_time_ranking_rel,
    const double   sum_props_per_time_ranking_rel,
    const ReduceCommonData& commdata,
    const uint32_t cols,
    const Solver* solver,
    float* at)
{
    uint32_t x = 0;
    //glue 0 can happen in case it's a ternary resolvent clause
    //updated glue can actually be 1. Original glue cannot.
    const ClauseStatsExtra& extra_stats = solver->red_stats_extra[cl->stats.extra_pos];
    assert(extra_stats.orig_glue != 1);

    assert(cl->stats.last_touched <= sumConflicts);
    assert(extra_stats.introduced_at_conflict <= sumConflicts);
    uint32_t last_touched_diff = sumConflicts - (uint64_t)cl->stats.last_touched;
    double time_inside_solver = sumConflicts - (uint64_t)extra_stats.introduced_at_conflict;


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


    if (cl->stats.is_ternary_resolvent || //glueHist_avg not valid for ternary
        extra_stats.glueHist_avg == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue/(double)extra_stats.glueHist_avg;
    }
    //(rdb0.glue/cl.glueHist_avg) -- 6


    at[x++] = (double)cl->stats.glue;
    //rdb0.glue -- 7


    if (time_inside_solver  == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.sum_props_made/time_inside_solver;
    }
    //(rdb0.sum_props_made/cl.time_inside_solver) -- 8


    if (time_inside_solver == 0 ||
        commdata.avg_glue == 0 ||
        cl->stats.glue == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (extra_stats.sum_props_made/time_inside_solver)/
            ((double)cl->stats.glue/commdata.avg_glue);
    }
    //((rdb0.sum_props_made/cl.time_inside_solver)/(rdb0.glue/rdb0_common.avg_glue)) -- 9


    if (cl->stats.is_ternary_resolvent || //glue_before_minim does not exist for ternary
        time_inside_solver == 0 ||
        extra_stats.sum_uip1_used == 0 ||
        extra_stats.glue_before_minim == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2(extra_stats.glue_before_minim)/
            ((double)extra_stats.sum_uip1_used/time_inside_solver);
    }
    //(log2(cl.glue_before_minim)/(rdb0.sum_uip1_used/cl.time_inside_solver)) -- 10


    at[x++] = extra_stats.orig_glue;
    //cl.orig_glue -- 11


    if (cl->stats.is_ternary_resolvent ||
        extra_stats.num_antecedents == 0 ||
        extra_stats.num_total_lits_antecedents == 0) //num_antecedents does not exist for ternary
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2((double)extra_stats.num_antecedents)/(double)extra_stats.num_total_lits_antecedents;
    }
    //(log2(cl.num_antecedents)/cl.num_total_lits_antecedents) -- 12


    //To protect against unset values being used
    assert(cl->stats.is_ternary_resolvent ||
        extra_stats.glueHist_longterm_avg > 0.9f);

    if (cl->stats.is_ternary_resolvent ||
        extra_stats.glue_before_minim == 0 //glueHist_longterm_avg does not exist for ternary
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.glueHist_longterm_avg/(double)extra_stats.glue_before_minim;
    }
    //(cl.glueHist_longterm_avg/cl.glue_before_minim) -- 13


    if (cl->stats.is_ternary_resolvent) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue/(double)solver->hist.conflSizeHistLT.avg();
    }
    //(rdb0.glue/rdb0_common.conflSizeHistLT_avg) -- 14


    //To protect against unset values being used
    assert(cl->stats.is_ternary_resolvent ||
        extra_stats.numResolutionsHistLT_avg > 0.9f);

    if (cl->stats.is_ternary_resolvent || //numResolutionsHistLT_avg does not exist for ternary
        extra_stats.numResolutionsHistLT_avg == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.discounted_props_made/(double)extra_stats.numResolutionsHistLT_avg;
    }
    //(rdb0.discounted_props_made/cl.numResolutionsHistLT_avg) -- 15


    if (extra_stats.discounted_props_made < 1e-20f || time_inside_solver == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)extra_stats.sum_uip1_used/time_inside_solver)/((double)extra_stats.discounted_props_made);
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
        extra_stats.num_total_lits_antecedents == 0 ||
        time_inside_solver == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)extra_stats.sum_props_made/(double)time_inside_solver)/
            (double)extra_stats.num_total_lits_antecedents;
    }
    // ((rdb0.sum_props_made/cl.time_inside_solver)/cl.num_total_lits_antecedents) -- 18


    //To protect against unset values being used
    assert(cl->stats.is_ternary_resolvent ||
        extra_stats.glueHistLT_avg > 0.9f);

    if (cl->stats.is_ternary_resolvent || //glue and glueHistLT_avg does not exist for ternary
        extra_stats.glueHistLT_avg != 0 ||
        time_inside_solver == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue/(double)extra_stats.glueHistLT_avg;
    }
    // (rdb0.glue/cl.glueHistLT_avg) -- 19


    if (commdata.avg_uip == 0 ||
        cl->stats.uip1_used == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)prop_ranking_rel/
            ((double)cl->stats.uip1_used/(double)commdata.avg_uip);
    }
    // (rdb0.prop_ranking_rel/(rdb0.uip1_used/rdb0_common.avg_uip1_used)) -- 20


    if (cl->stats.is_ternary_resolvent || // size_hist and overlap_hist do not exist for tri
        extra_stats.conflSizeHist_avg == 0
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.overlapHistLT_avg/
            ((double)extra_stats.conflSizeHist_avg);
    }
    // (cl.overlapHistLT_avg/cl.conflSizeHist_avg) -- 21


    if (cl->stats.is_ternary_resolvent || // glueHistLT_avg do not exist for tri
        cl->stats.uip1_used == 0

    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.glueHistLT_avg/
            ((double)cl->stats.uip1_used);
    }
    // (cl.glueHistLT_avg/rdb0.uip1_used) -- 22


    if (commdata.avg_glue == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->size()/
            ((double)commdata.avg_glue);
    }
    // (rdb0.size/rdb0_common.avg_glue) -- 23

    if (extra_stats.sum_uip1_used == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->size()/
            ((double)extra_stats.sum_uip1_used);
    }
    // (rdb0.size/rdb0.sum_uip1_used) -- 24


    at[x++] = extra_stats.discounted_uip1_used;
    // rdb0.discounted_uip1_used -- 25


    if (cl->stats.is_ternary_resolvent ||
        extra_stats.discounted_props_made < 1e-20f)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.glueHistLT_avg/
            ((double)extra_stats.discounted_props_made);
    }
    //(cl.glueHistLT_avg/rdb0.discounted_props_made) -- 26


    if (commdata.avg_uip == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = uip1_ranking_rel/(double)commdata.avg_uip;
    }
    // (rdb0.uip1_ranking_rel/rdb0_common.avg_uip1_used) -- 27


    if (cl->stats.is_ternary_resolvent ||
        commdata.avg_uip == 0 ||
        extra_stats.discounted_uip1_used < 1e-20f ||
        ((double)extra_stats.discounted_uip1_used/(double)commdata.avg_uip) < 1e-20
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)extra_stats.num_antecedents/((double)extra_stats.discounted_uip1_used/(double)commdata.avg_uip);
    }
    // (cl.num_antecedents/(rdb0.discounted_uip1_used/rdb0_common.avg_uip1_used)) -- 28


    at[x++] = sum_uip1_per_time_ranking_rel;
    // rdb0.sum_uip1_per_time_ranking_rel -- 29

    at[x++] = sum_props_per_time_ranking_rel;
    // rdb0.sum_props_per_time_ranking_rel -- 30

    if (extra_stats.antecedents_binred == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = extra_stats.glueHist_avg/(float)extra_stats.antecedents_binred;
    }
    // (cl.glueHist_avg/cl.atedecents_binRed) -- 31

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

void ClPredictors::predict_all(
    float* data,
    uint32_t num)
{
    safe_xgboost(XGDMatrixCreateFromMat(data, num, PRED_COLS, MISSING_VAL, &dmat))
    bst_ulong out_len;
    safe_xgboost(XGBoosterPredict(
        handles[short_pred],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result_short
    ))
    assert(out_len == num);

    safe_xgboost(XGBoosterPredict(
        handles[long_pred],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result_long
    ))
    assert(out_len == num);

    safe_xgboost(XGBoosterPredict(
        handles[forever_pred],
        dmat,
        0,  //0: normal prediction
        0,  //use all trees
        0,  //do not use for training
        &out_len,
        &out_result_forever
    ))
    assert(out_len == num);
}

void ClPredictors::get_prediction_at(ClauseStatsExtra& extdata, const uint32_t at)
{
    extdata.pred_short_use = out_result_short[at];
    extdata.pred_long_use = out_result_long[at];
    extdata.pred_forever_use = out_result_forever[at];
}

void CMSat::ClPredictors::finish_all_predict()
{
    safe_xgboost(XGDMatrixFree(dmat))
}

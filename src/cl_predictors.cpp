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
#include "solver.h"
#include <cmath>
//#define MISSING_VAL std::numeric_limits<float>::quiet_NaN()
#define MISSING_VAL  -1334556800.0f

using namespace CMSat;

ClPredictors::ClPredictors(Solver* _solver) :
    solver(_solver)
{
    BoosterHandle handle;
    int ret;

    handles.push_back(handle);
    ret = XGBoosterCreate(0, 0, &(handles[predict_type::short_pred]));
    assert(ret == 0);
    ret = XGBoosterSetParam(handles[predict_type::short_pred], "nthread", "1");
    assert(ret == 0);

    BoosterHandle handle2;
    handles.push_back(handle2);
    ret = XGBoosterCreate(0, 0, &(handles[predict_type::long_pred]));
    assert(ret == 0);
    ret = XGBoosterSetParam(handles[predict_type::long_pred], "nthread", "1");
    assert(ret == 0);

    BoosterHandle handle3;
    handles.push_back(handle3);
    ret = XGBoosterCreate(0, 0, &(handles[predict_type::forever_pred]));
    assert(ret == 0);
    ret = XGBoosterSetParam(handles[predict_type::forever_pred], "nthread", "1");
    assert(ret == 0);
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
    int ret;
    ret = XGBoosterLoadModel(handles[predict_type::short_pred], short_fname.c_str());
    assert(ret == 0);

    ret =XGBoosterLoadModel(handles[predict_type::long_pred], long_fname.c_str());
    assert(ret == 0);

    ret =XGBoosterLoadModel(handles[predict_type::forever_pred], forever_fname.c_str());
    assert(ret == 0);
}

void ClPredictors::set_up_input(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t  last_touched_diff,
    #ifdef EXTENDED_FEATURES
    const int64_t  rdb1_last_touched_diff,
    #endif
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10,
    const uint32_t cols,
    float* at)
{
    uint32_t x = 0;
    double orig_glue = cl->stats.orig_glue;
    assert(orig_glue != 1);
    //updated glue can actually be 1. Original glue cannot.

    double time_inside_solver = solver->sumConflicts - cl->stats.introduced_at_conflict;
    double tot_props_made = cl->stats.propagations_made+cl->stats.rdb1_propagations_made;

#ifdef EXTENDED_FEATURES
    double rdb1_act_ranking_rel = (double)cl->stats.rdb1_act_ranking_rel;
    double tot_last_touch_diffs = last_touched_diff + rdb1_last_touched_diff;

    at[x++] = (float)cl->stats.used_for_uip_creation;
    //rdb0.used_for_uip_creation

    at[x++] = (float)cl->stats.glue;
    //rdb0.glue

    at[x++] = (float)time_inside_solver;
    //cl.time_inside_solver

    at[x++] = (float)last_touched_diff;
    //rdb0.last_touched_diff

    at[x++] = (float)cl->stats.propagations_made;
    //rdb0.propagations_made

    at[x++] = (float)cl->stats.rdb1_propagations_made;
    //rdb1.propagations_made

    if (rdb1_act_ranking_rel == 1 ||
        rdb1_act_ranking_rel == 0 ||
        time_inside_solver == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)cl->stats.sum_uip1_used/time_inside_solver)/
            ::log2(rdb1_act_ranking_rel);
    }
    //((rdb0.sum_uip1_used/cl.time_inside_solver)/log2(rdb1_act_ranking_rel))

    if (rdb1_act_ranking_rel == 0 ||
        time_inside_solver == 0 ||
        cl->stats.sum_uip1_used == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2(rdb1_act_ranking_rel)/
            (cl->stats.sum_uip1_used/time_inside_solver);
    }
    // (log2(rdb1_act_ranking_rel)/(rdb0.sum_uip1_used/cl.time_inside_solver))

    if (cl->stats.glue_hist == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.propagations_made/(double)cl->stats.glue_hist;
    }
    // (rdb0.propagations_made/cl.glue_hist)

    if (cl->stats.glue == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = tot_props_made/cl->stats.glue;
    }
    // ((rdb0.propagations_made+rdb1.propagations_made)/rdb0.glue)

    if (act_ranking_rel == 0 || act_ranking_rel == 1) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)(cl->stats.glue)/::log2(act_ranking_rel);
    }
    // (rdb0.glue/log2(rdb0_act_ranking_rel))

    if (tot_last_touch_diffs == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = time_inside_solver/tot_last_touch_diffs;
    }
    // (cl.time_inside_solver/(rdb0.last_touched_diff+rdb1.last_touched_diff))

    if (act_ranking_rel == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.sum_uip1_used/act_ranking_rel;
    }
    // (rdb0.sum_uip1_used/rdb0_act_ranking_rel)

    at[x++] = tot_props_made/orig_glue;
    // ((rdb0.propagations_made+rdb1.propagations_made)/cl.orig_glue)

    if (time_inside_solver == 0 || cl->stats.sum_uip1_used == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)(cl->stats.glue)/
            (cl->stats.sum_uip1_used/time_inside_solver);
    }
    // (rdb0.glue/(rdb0.sum_uip1_used/cl.time_inside_solver))

    if (tot_props_made == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue_hist_long/tot_props_made;
    }
    // (cl.glue_hist_long/(rdb0.propagations_made+rdb1.propagations_made))

    if (time_inside_solver == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ((double)cl->stats.sum_uip1_used/time_inside_solver)/
            (double)orig_glue;
    }
    // ((rdb0.sum_uip1_used/cl.time_inside_solver)/cl.orig_glue)

    if (tot_props_made == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.glue_before_minim/tot_props_made;
    }
    // (cl.glue_before_minim/(rdb0.propagations_made+rdb1.propagations_made))

    if (cl->stats.propagations_made == 0 || cl->stats.antec_overlap_hist == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2((double)cl->stats.antec_overlap_hist)/(double)cl->stats.propagations_made;
    }
    // (log2(cl.antec_overlap_hist)/rdb0.propagations_made)

    if (act_ranking_rel == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.propagations_made/act_ranking_rel;
    }
    // (rdb0.propagations_made/rdb0_act_ranking_rel)

    if (cl->stats.glue == 0 || cl->stats.glue == 1) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = tot_props_made/::log2(cl->stats.glue);
    }
    // ((rdb0.propagations_made+rdb1.propagations_made)/log2(rdb0.glue))

    if (cl->stats.propagations_made == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = act_ranking_rel/(double)cl->stats.propagations_made;
    }
    // (rdb0_act_ranking_rel/rdb0.sum_propagations_made)
#endif

    if (cl->stats.num_resolutions_hist_lt == 0 ||
        cl->stats.num_resolutions_hist_lt == 1
    ) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = tot_props_made/::log2((double)cl->stats.num_resolutions_hist_lt);
    }
    //((rdb0.propagations_made+rdb1.propagations_made)/log2(cl.num_resolutions_hist_lt))

    at[x++] = tot_props_made/::log2(orig_glue);
    //((rdb0.propagations_made+rdb1.propagations_made)/log2(cl.orig_glue))

    if (time_inside_solver == 0 || cl->stats.sum_uip1_used == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2(cl->stats.glue_before_minim)/
            ((double)cl->stats.sum_uip1_used/time_inside_solver);
    }
    //(log2(cl.glue_before_minim)/(rdb0.sum_uip1_used/cl.time_inside_solver))

    if (cl->stats.glue == 0 || cl->stats.glue == 1) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.sum_uip1_used/::log2(cl->stats.glue);
    }
    //(rdb0.sum_uip1_used/log2(rdb0.glue))

    if (act_ranking_rel == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2(act_ranking_rel)/(double)cl->stats.orig_glue;
    }
    //(log2(rdb0_act_ranking_rel)/cl.orig_glue)

    if (time_inside_solver == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.propagations_made/(double)time_inside_solver;
    }
    //(rdb0.propagations_made/cl.time_inside_solver)

    if (cl->stats.num_antecedents == 0 ||
        cl->stats.num_total_lits_antecedents == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2((double)cl->stats.num_antecedents)/(double)cl->stats.num_total_lits_antecedents;
    }
    //(log2(cl.num_antecedents)/cl.num_total_lits_antecedents)

    if (cl->stats.glue_hist_long == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->size()/(double)cl->stats.glue_hist_long;
    }
    //(rdb0.size/cl.glue_hist_long)

    if (cl->stats.glue_hist_queue == 0 || cl->stats.glue_hist_queue == 1) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.propagations_made/
            ::log2((double)cl->stats.glue_hist_queue);
    }
    //(rdb0.propagations_made/log2(cl.glue_hist_queue)


    assert(cl->stats.orig_glue != 0);
    at[x++] = (double)cl->stats.propagations_made/(double)cl->stats.orig_glue;
    //(rdb0.propagations_made/cl.orig_glue)

    if (cl->stats.propagations_made == 0 ||
        cl->stats.num_resolutions_hist_lt == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2((double)cl->stats.num_resolutions_hist_lt)/
            (double)cl->stats.propagations_made;
    }
    //(log2(cl.num_resolutions_hist_lt)/rdb0.propagations_made)


    if (cl->stats.num_antecedents == 0 ||
        cl->stats.num_total_lits_antecedents == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.propagations_made/
            ((double)cl->stats.num_total_lits_antecedents/(double)cl->stats.num_antecedents);
    }
    //(rdb0.propagations_made/(cl.num_total_lits_antecedents/cl.num_antecedents))


    if (cl->stats.propagations_made == 0) {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = (double)cl->stats.confl_size_hist_lt/(double)cl->stats.propagations_made;
    }
    //(cl.size_hist/rdb0.propagations_made)

#ifndef EXTENDED_FEATURES
    at[x++] = (double)cl->stats.propagations_made/std::log2((double)cl->stats.antec_overlap_hist);
    //(rdb0.propagations_made/log2(cl.antec_overlap_hist))
#endif

    if (cl->stats.propagations_made == 0 ||
        cl->stats.branch_depth_hist_queue == 0)
    {
        at[x++] = MISSING_VAL;
    } else {
        at[x++] = ::log2((double)cl->stats.branch_depth_hist_queue)/
            (double)cl->stats.propagations_made;
    }
    //(log2(cl.branch_depth_hist_queue)/rdb0.propagations_made)


    at[x++] = (double)cl->stats.used_for_uip_creation/
        (double)cl->stats.glue_before_minim;;
    //(rdb0.used_for_uip_creation/cl.glue_before_minim)

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
    const int64_t  last_touched_diff,
    #ifdef EXTENDED_FEATURES
    const int64_t  rdb1_last_touched_diff,
    #endif
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10)
{
    // convert to DMatrix
    set_up_input(
        cl,
        sumConflicts,
        last_touched_diff,
        #ifdef EXTENDED_FEATURES
        rdb1_last_touched_diff,
        #endif
        act_ranking_rel,
        act_ranking_top_10,
        PRED_COLS,
        train);
    int rows=1;
    int ret = XGDMatrixCreateFromMat((float *)train, rows, PRED_COLS, MISSING_VAL, &dmat);
    assert(ret == 0);

    float val = predict_one(pred_type);
    XGDMatrixFree(dmat);

    return val;
}

void ClPredictors::predict(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t  last_touched_diff,
    #ifdef EXTENDED_FEATURES
    const int64_t  rdb1_last_touched_diff,
    #endif
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10,
    float& p_short,
    float& p_long,
    float& p_forever)
{
    // convert to DMatrix
    set_up_input(
        cl,
        sumConflicts,
        last_touched_diff,
        #ifdef EXTENDED_FEATURES
        rdb1_last_touched_diff,
        #endif
        act_ranking_rel,
        act_ranking_top_10,
        PRED_COLS,
        train);
    int rows=1;
    int ret;

    ret = XGDMatrixCreateFromMat((float *)train, rows, PRED_COLS, MISSING_VAL, &dmat);
    assert(ret == 0);
    p_short = predict_one(short_pred);
    XGDMatrixFree(dmat);

    ret = XGDMatrixCreateFromMat((float *)train, rows, PRED_COLS, MISSING_VAL, &dmat);
    assert(ret == 0);
    p_long = predict_one(long_pred);
    XGDMatrixFree(dmat);

    ret = XGDMatrixCreateFromMat((float *)train, rows, PRED_COLS, MISSING_VAL, &dmat);
    assert(ret == 0);
    p_forever = predict_one(forever_pred);
    XGDMatrixFree(dmat);
}

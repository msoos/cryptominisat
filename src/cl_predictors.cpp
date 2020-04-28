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
#define MISSING_VAL -1334556787

using namespace CMSat;

enum predict_type {short_pred=0, long_pred=1};

ClPredictors::ClPredictors(Solver* _solver) :
    solver(_solver)
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
    ret = XGBoosterLoadModel(handles[short_pred], short_fname.c_str());
    assert(ret == 0);

    ret =XGBoosterLoadModel(handles[long_pred], long_fname.c_str());
    assert(ret == 0);
}

void ClPredictors::set_up_input(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t last_touched_diff,
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10,
    const uint32_t cols)
{
    float *at = train;
    uint32_t x = 0;

    at[x++] =
        ((double)(cl->stats.propagations_made+cl->stats.rdb1_propagations_made))/
        ::log2((double)cl->stats.num_resolutions_hist_lt);
    //((rdb0.propagations_made+rdb1.propagations_made)/log2(cl.num_resolutions_hist_lt))

    //prevent divide by zero
    double orig_glue = cl->stats.orig_glue;
    assert(orig_glue != 1);
    at[x++] =
        ((double)(cl->stats.propagations_made+cl->stats.rdb1_propagations_made))/
        ::log2(orig_glue);
    //((rdb0.propagations_made+rdb1.propagations_made)/log2(cl.orig_glue))

    uint64_t time_inside_solver = solver->sumConflicts - cl->stats.introduced_at_conflict;
    if (time_inside_solver == 0) {
        //avoid divide by zero
        time_inside_solver = 1;
    }

    //prevent divide by zero
    double sum_uip1_used = cl->stats.sum_uip1_used;
    if (sum_uip1_used == 0) {
        sum_uip1_used = 1;
    }
    at[x++] =
        ::log2(cl->stats.glue_before_minim)/
        ((double)sum_uip1_used/(double)time_inside_solver);
    //(log2(cl.glue_before_minim)/(rdb0.sum_uip1_used/cl.time_inside_solver))


    //prevent divide by zero
    double glue = cl->stats.glue;
    if (glue == 1) {
        glue = 2;
    }
    at[x++] =
        (double)cl->stats.sum_uip1_used/
        ::log2(glue);
    //(rdb0.sum_uip1_used/log2(rdb0.glue))


    at[x++] = ::log2(act_ranking_rel)/(double)cl->stats.orig_glue;
    //(log2(rdb0_act_ranking_rel)/cl.orig_glue)

    at[x++] = (double)cl->stats.propagations_made/(double)time_inside_solver;
    //(rdb0.propagations_made/cl.time_inside_solver)

    at[x++] = ::log2((double)cl->stats.num_antecedents)/(double)cl->stats.num_total_lits_antecedents;
    //(log2(cl.num_antecedents)/cl.num_total_lits_antecedents)

//     at[x++] = cl->stats.glue_hist_long;                           //cl.glue_hist_long
//     at[x++] = cl->stats.glue_hist_queue;                          //cl.glue_hist_queue
//     at[x++] = cl->stats.glue_hist;                                //cl.glue_hist
//     at[x++] = cl->stats.size_hist;                                //cl.size_hist
//     at[x++] = cl->stats.glue_before_minim;                        //cl.glue_before_minim
//     at[x++] = cl->stats.orig_glue;                                //cl.orig_glue
//     at[x++] = cl->stats.glue;                                     //rdb0.glue
//     at[x++] = cl->size();                                         //rdb0.size
//     at[x++] = cl->stats.used_for_uip_creation;                    //rdb0.used_for_uip_creation
//     at[x++] = cl->stats.rdb1_used_for_uip_creation;               //rdb1.used_for_uip_creation
//     //at[x++] = cl->stats.num_overlap_literals;                     //cl.num_overlap_literals
//     at[x++] = MISSING_VAL;
//     at[x++] = cl->stats.antec_overlap_hist;                       //cl.antec_overlap_hist
//     at[x++] = cl->stats.num_total_lits_antecedents;               //cl.num_total_lits_antecedents
//     at[x++] = cl->stats.rdb1_last_touched_diff;                   //rdb1.last_touched_diff
//     at[x++] = cl->stats.num_antecedents;                          //cl.num_antecedents
//     at[x++] = cl->stats.branch_depth_hist_queue;                  //cl.branch_depth_hist_queue
//     at[x++] = cl->stats.num_resolutions_hist_lt;                  //cl.num_resolutions_hist_lt
//     at[x++] = cl->stats.trail_depth_hist_longer;                  //cl.trail_depth_hist_longer
// //     at[x++] = act_ranking_rel;                                    //rdb0_act_ranking_rel
//     at[x++] = MISSING_VAL;
//     at[x++] = cl->stats.propagations_made;                        //rdb0.propagations_made
//     at[x++] = cl->stats.rdb1_propagations_made;                   //rdb1.propagations_made
// //     at[x++] = act_ranking_top_10;                                 //rdb0.act_ranking_top_10
//     at[x++] = MISSING_VAL;
// //     at[x++] = cl->stats.rdb1_act_ranking_top_10;                  //rdb1.act_ranking_top_10
//     at[x++] = MISSING_VAL;
//     at[x++] = cl->stats.is_decision;                              //cl.is_decision
//     at[x++] = cl->is_ternary_resolvent;                           //rdb0.is_ternary_resolvent
// //     at[x++] = cl->stats.sum_uip1_used;                            //rdb0.sum_uip1_used
// //     at[x++] = cl->stats.dump_no;                                        //rdb0.dump_no

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
    assert(ret == 0);

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
    set_up_input(cl, sumConflicts, last_touched_diff,
                 act_ranking_rel, act_ranking_top_10,
                 PRED_COLS);
    int rows=1;
    int ret = XGDMatrixCreateFromMat((float *)train, rows, PRED_COLS, MISSING_VAL, &dmat);
    assert(ret == 0);

    float val = predict_one(short_pred, dmat);
    XGDMatrixFree(dmat);

    return val;
}

float ClPredictors::predict_long(
    const CMSat::Clause* cl,
    const uint64_t sumConflicts,
    const int64_t last_touched_diff,
    const double   act_ranking_rel,
    const uint32_t act_ranking_top_10)
{
    // convert to DMatrix
    set_up_input(cl, sumConflicts, last_touched_diff,
                 act_ranking_rel, act_ranking_top_10,
                 PRED_COLS);
    int rows=1;
    int ret = XGDMatrixCreateFromMat((float *)train, rows, PRED_COLS, MISSING_VAL, &dmat);
    assert(ret == 0);

    float val = predict_one(long_pred, dmat);
    XGDMatrixFree(dmat);

    return val;
}

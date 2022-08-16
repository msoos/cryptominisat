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

#include "cl_predictors_abs.h"
#include "solver.h"

using namespace CMSat;

extern const char* predictor_short_json_hash;
extern const char* predictor_long_json_hash;
extern const char* predictor_forever_json_hash;

vector<std::string> ClPredictorsAbst::get_hashes() const
{
    vector<std::string> ret;
    ret.push_back(string(predictor_short_json_hash));
    ret.push_back(string(predictor_long_json_hash));
    ret.push_back(string(predictor_forever_json_hash));

    return ret;
}

int ClPredictorsAbst::set_up_input(
    const CMSat::Clause* const cl,
    const uint64_t sumConflicts,
    const double   act_ranking_rel,
    const double   uip1_ranking_rel,
    const double   prop_ranking_rel,
    const double   sum_uip1_per_time_ranking,
    const double   sum_props_per_time_ranking,
    const double   sum_uip1_per_time_ranking_rel,
    const double   sum_props_per_time_ranking_rel,
    const ReduceCommonData& commdata,
    const Solver* solver,
    float* at)
{
    //glue 0 can happen in case it's a ternary resolvent clause
    //updated glue can actually be 1. Original glue cannot.
    const ClauseStatsExtra& extra_stats = solver->red_stats_extra[cl->stats.extra_pos];
    assert(extra_stats.orig_glue != 1);

    assert(cl->stats.last_touched_any <= sumConflicts);
    assert(extra_stats.introduced_at_conflict <= sumConflicts);
    uint32_t last_touched_any_diff = sumConflicts - (uint64_t)cl->stats.last_touched_any;
    double time_inside_solver = sumConflicts - (uint64_t)extra_stats.introduced_at_conflict;

    //To protect against unset values being used
    assert(cl->stats.is_ternary_resolvent ||
        extra_stats.glueHist_longterm_avg > 0.9f);

    uint32_t x = 0;
    ///////////////
    at[x++] = sum_uip1_per_time_ranking_rel;
//     rdb0.sum_uip1_per_time_ranking_rel
//     2
    at[x++] = sum_props_per_time_ranking_rel;
//     rdb0.sum_props_per_time_ranking_rel
//     3
    at[x++] = act_ranking_rel;
//     rdb0.act_ranking_rel
//     4
    at[x++] = uip1_ranking_rel;
//     rdb0.uip1_ranking_rel
//     5
    at[x++] = prop_ranking_rel;
//     rdb0.prop_ranking_rel
//     6

    ///////////////
    at[x++] = cl->stats.props_made;
//     rdb0.props_made
//     10
    at[x++] = cl->stats.uip1_used;
//     rdb0.uip1_used
//     11
    at[x++] = extra_stats.discounted_props_made;
//     rdb0.discounted_props_made
//     12
    at[x++] = extra_stats.discounted_props_made2;
//     rdb0.discounted_props_made2
//     13
    at[x++] = extra_stats.discounted_props_made3;
//     rdb0.discounted_props_made3
//     14
    at[x++] = extra_stats.discounted_uip1_used;
//     rdb0.discounted_uip1_used
//     15
    at[x++] = extra_stats.discounted_uip1_used2;
//     rdb0.discounted_uip1_used2
//     16
    at[x++] = extra_stats.discounted_uip1_used3;
//     rdb0.discounted_uip1_used3
//     17

    //////////////////
    if (cl->stats.is_ternary_resolvent ||
        extra_stats.glueHist_longterm_avg == 0 //glueHist_longterm_avg does not exist for ternary
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.glue_before_minim/(double)extra_stats.glueHist_longterm_avg;
    }
    //(cl.glue_before_minim/cl.glueHist_longterm_avg)
//     32


    if (commdata.avg_uip == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)cl->stats.uip1_used/(double)commdata.avg_uip;
    }
    //(rdb0.uip1_used/rdb0_common.avg_uip1_used)
//     33


    if (cl->stats.is_ternary_resolvent ||
        solver->hist.glueHistLT.avg() == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)cl->stats.glue/(double)solver->hist.glueHistLT.avg();
    }
    //(rdb0.glue/rdb0_common.glueHistLT_avg)
//     34


    if (commdata.avg_props == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)cl->stats.props_made/(double)commdata.avg_props;
    }
    //(rdb0.props_made/rdb0_common.avg_props)
//     35


    if (time_inside_solver == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.sum_props_made/(double)time_inside_solver;
    }
    //(rdb0.sum_props_made/cl.time_inside_solver)
//     36


    if (time_inside_solver == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.sum_uip1_used/(double)time_inside_solver;
    }
    //(rdb0.sum_uip1_used/cl.time_inside_solver)
//     37


    if (cl->stats.is_ternary_resolvent ||
        extra_stats.num_total_lits_antecedents == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.num_antecedents/(double)extra_stats.num_total_lits_antecedents;
    }
    //(cl.num_antecedents/cl.num_total_lits_antecedents)
//     38


    //////////////////
    if (cl->stats.is_ternary_resolvent ||
        extra_stats.trail_depth_level == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)cl->stats.glue/(double)extra_stats.trail_depth_level;
    }
    //(rdb0.glue/cl.trail_depth_level)
//     42


    if (cl->stats.is_ternary_resolvent
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.orig_glue;
    }
    //cl.orig_glue
//     43


    assert(x==PRED_COLS);
    return PRED_COLS;
}

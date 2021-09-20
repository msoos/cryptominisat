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
    const double   sum_uip1_per_time_ranking_rel,
    const double   sum_props_per_time_ranking_rel,
    const ReduceCommonData& commdata,
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
        at[x++] = missing_val;
    } else {
        at[x++] = act_ranking_rel/(double)last_touched_diff;
    }
    //(rdb0.act_ranking_rel/rdb0.last_touched_diff) -- 2


    if (commdata.avg_props == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)cl->stats.props_made/(double)commdata.avg_props;
    }
    //(rdb0.props_made/rdb0_common.avg_props) -- 3


    at[x++] = (double)last_touched_diff;
    //rdb0.last_touched_diff -- 4


    if (cl->stats.is_ternary_resolvent || //glueHist_avg not valid for ternary
        extra_stats.glueHist_avg == 0)
    {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)cl->stats.glue/(double)extra_stats.glueHist_avg;
    }
    //(rdb0.glue/cl.glueHist_avg) -- 5


    at[x++] = (double)cl->stats.glue;
    //rdb0.glue -- 6


    if (time_inside_solver  == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.sum_props_made/time_inside_solver;
    }
    //(rdb0.sum_props_made/cl.time_inside_solver) -- 7


    float myval_at_9;
    if (time_inside_solver == 0 ||
        commdata.avg_glue == 0 ||
        cl->stats.glue == 0) {
        myval_at_9 = missing_val;
    } else {
        myval_at_9 = (extra_stats.sum_props_made/time_inside_solver)/
            ((double)cl->stats.glue/commdata.avg_glue);
    }
    at[x++] = myval_at_9;
    //((rdb0.sum_props_made/cl.time_inside_solver)/(rdb0.glue/rdb0_common.avg_glue)) -- 8


    //To protect against unset values being used
    assert(cl->stats.is_ternary_resolvent ||
        extra_stats.glueHist_longterm_avg > 0.9f);

    if (cl->stats.is_ternary_resolvent ||
        extra_stats.glue_before_minim == 0 //glueHist_longterm_avg does not exist for ternary
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.glueHist_longterm_avg/(double)extra_stats.glue_before_minim;
    }
    //(cl.glueHist_longterm_avg/cl.glue_before_minim) -- 9


    if (extra_stats.discounted_props_made < 1e-20f || time_inside_solver == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = ((double)extra_stats.sum_uip1_used/time_inside_solver)/((double)extra_stats.discounted_props_made);
    }
    //((rdb0.sum_uip1_used/cl.time_inside_solver)/rdb0.discounted_props_made) -- 10


    if (commdata.avg_props == 0 || cl->stats.props_made == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = ((double)cl->stats.glue)/
            ((double)cl->stats.props_made/ (double)commdata.avg_props);
    }
    //(rdb0.glue/(rdb0.props_made/rdb0_common.avg_props)) -- 11


    if (commdata.avg_uip == 0 ||
        cl->stats.uip1_used == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)prop_ranking_rel/
            ((double)cl->stats.uip1_used/(double)commdata.avg_uip);
    }
    //(rdb0.prop_ranking_rel/(rdb0.uip1_used/rdb0_common.avg_uip1_used)) -- 12


    if (cl->stats.is_ternary_resolvent || // size_hist and overlap_hist do not exist for tri
        extra_stats.conflSizeHist_avg == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.overlapHistLT_avg/
            ((double)extra_stats.conflSizeHist_avg);
    }
    //(cl.overlapHistLT_avg/cl.conflSizeHist_avg) -- 13


    at[x++] = extra_stats.discounted_uip1_used;
    // rdb0.discounted_uip1_used -- 14


    if (cl->stats.is_ternary_resolvent ||
        extra_stats.discounted_props_made < 1e-20f)
    {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.glueHistLT_avg/
            ((double)extra_stats.discounted_props_made);
    }
    //(cl.glueHistLT_avg/rdb0.discounted_props_made) -- 15


    if (commdata.avg_uip == 0) {
        at[x++] = missing_val;
    } else {
        at[x++] = uip1_ranking_rel/(double)commdata.avg_uip;
    }
    //(rdb0.uip1_ranking_rel/rdb0_common.avg_uip1_used) -- 16


    if (cl->stats.is_ternary_resolvent ||
        commdata.avg_uip == 0 ||
        extra_stats.discounted_uip1_used < 1e-20f ||
        ((double)extra_stats.discounted_uip1_used/(double)commdata.avg_uip) < 1e-20
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (double)extra_stats.num_antecedents/((double)extra_stats.discounted_uip1_used/(double)commdata.avg_uip);
    }
    //(cl.num_antecedents/(rdb0.discounted_uip1_used/rdb0_common.avg_uip1_used)) -- 17

    if (extra_stats.antecedents_binred == 0 ||
        cl->stats.is_ternary_resolvent
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = extra_stats.glueHist_avg/(float)extra_stats.antecedents_binred;
    }
    //(cl.glueHist_avg/cl.atedecents_binRed) -- 18


    if (last_touched_diff == 0 ||
        commdata.avg_uip == 0
    ) {
        at[x++] = missing_val;
    } else {
        at[x++] = (act_ranking_rel/(double)last_touched_diff)*
            (uip1_ranking_rel/commdata.avg_uip);
    }
    //((rdb0.act_ranking_rel/rdb0.last_touched_diff)*(rdb0.uip1_ranking_rel/rdb0_common.avg_uip1_used)) -- 19


    if (cl->stats.is_ternary_resolvent) {
        at[x++] = missing_val;
    } else {
        at[x++] = extra_stats.antecedents_binIrred * commdata.avg_uip;
    }
    //(cl.atedecents_binIrred*rdb0_common.avg_uip1_used) -- 20

//     cout << "c val: ";
//     for(uint32_t i = 0; i < cols; i++) {
//         cout << at[i] << " ";
//     }
//     cout << endl;

    assert(x==PRED_COLS);
    return PRED_COLS;
}

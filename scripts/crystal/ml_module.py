#!/usr/bin/python3

import numpy as np
import pandas as pd
import time
import xgboost as xgb

columns = [
    "rdb0.uip1_ranking_rel",
    "(rdb0.act_ranking_rel/rdb0.last_touched_diff)",
    "(rdb0.props_made/rdb0_common.avg_props)",
    "rdb0.last_touched_diff",
    "(rdb0.glue/cl.glueHist_avg)",
    "rdb0.glue",
    "(rdb0.sum_props_made/cl.time_inside_solver)",
    "((rdb0.sum_props_made/cl.time_inside_solver)/(rdb0.glue/rdb0_common.avg_glue))",
    "(cl.glueHist_longterm_avg/cl.glue_before_minim)",
    "((rdb0.sum_uip1_used/cl.time_inside_solver)/rdb0.discounted_props_made)",
    "(rdb0.glue/(rdb0.props_made/rdb0_common.avg_props))",
    "(rdb0.prop_ranking_rel/(rdb0.uip1_used/rdb0_common.avg_uip1_used))",
    "(cl.overlapHistLT_avg/cl.conflSizeHist_avg)",
    "rdb0.discounted_uip1_used",
    "(cl.glueHistLT_avg/rdb0.discounted_props_made)",
    "(rdb0.uip1_ranking_rel/rdb0_common.avg_uip1_used)",
    "(cl.num_antecedents/(rdb0.discounted_uip1_used/rdb0_common.avg_uip1_used))",
    "(cl.glueHist_avg/cl.atedecents_binRed)",
    "((rdb0.act_ranking_rel/rdb0.last_touched_diff)*(rdb0.uip1_ranking_rel/rdb0_common.avg_uip1_used))",
    "(cl.atedecents_binIrred*rdb0_common.avg_uip1_used)"]

global models
models = []

def load_models(short_fname, long_fname, forever_fname):
    for fname in [short_fname, long_fname, forever_fname]:
        clf_xgboost = xgb.XGBRegressor(n_jobs=1)
        clf_xgboost.load_model(fname)
        models.append(clf_xgboost)

def predict(data):
    ret = []
    df = pd.DataFrame(data, columns=columns)
    for i in range(3):
        ret.append(models[i].predict(df))
    return ret

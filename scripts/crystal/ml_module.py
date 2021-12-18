#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.


import numpy as np
import pandas as pd
import xgboost as xgb
import os
from ccg import *

# to test memory safety
#import gc
# to test memory usage
#from memory_profiler import *

MISSING=np.NaN

raw_data = [
    "is_ternary_resolvent",
    "rdb0.which_red_array",
    "rdb0.last_touched",
    "rdb0.act_ranking_rel",
    "rdb0.uip1_ranking_rel",
    "rdb0.prop_ranking_rel",
    "rdb0.last_touched_diff",
    "cl.time_inside_solver",
    "rdb0.props_made",
    "rdb0_common.avg_props",
    #"rdb0_common.avg_glue", CANNOT DO, ternaries have no glue!
    "rdb0_common.avg_uip1_used",
    "rdb0_common.conflSizeHistLT_avg",
    "rdb0_common.glueHistLT_avg",
    "rdb0.sum_props_made",
    "rdb0.discounted_props_made",
    "rdb0.discounted_props_made2",
    "rdb0.discounted_props_made3",
    "rdb0.discounted_uip1_used",
    "rdb0.discounted_uip1_used2",
    "rdb0.discounted_uip1_used3",
    "rdb0.sum_uip1_used",
    "rdb0.uip1_used",
    "rdb0.size",
    "rdb0.sum_uip1_per_time_ranking",
    "rdb0.sum_props_per_time_ranking",
    "rdb0.sum_uip1_per_time_ranking_rel",
    "rdb0.sum_props_per_time_ranking_rel",
    "rdb0.is_distilled",
    "cl.glueHist_avg",
    "cl.atedecents_binIrred",
    "cl.glueHistLT_avg",
    "cl.glueHist_longterm_avg",
    "cl.num_antecedents",
    "cl.overlapHistLT_avg",
    "cl.conflSizeHist_avg",
    "cl.atedecents_binRed",
    "cl.num_total_lits_antecedents",
    "cl.numResolutionsHistLT_avg",
    "rdb0.glue",
    "cl.orig_glue",
    "cl.glue_before_minim",
    "cl.trail_depth_level",
    #"sum_uip1_per_time_ranking_rel",
    #"sum_props_per_time_ranking_rel",
]

# check reproducibility by dumping and checking against previous run's dump
def dump_or_check(fname, df):
    if check_file_exists(fname):
        df_saved = pd.read_pickle(fname)
        print("Checking equals...", fname)
        if not df.equals(df_saved):
            print("df.shape       :", df.shape)
            print("df_saved.shape :", df_saved.shape)
            assert df.shape == df_saved.shape
            for i in range(df):
                if df[i] != df_saved[i]:
                    print("Not equal:")
                    print(df[i])
                    print(df_saved[i])
                    exit(-1)

    else:
        df.to_pickle(fname)
        print("Not checking, writing: ", fname)


def check_file_exists(fname):
    return os.path.exists(fname)

def get_features(fname):
    best_features = []
    if not check_file_exists(fname):
        print("File '%s' not accessible" % fname)
        exit(-1)

    with open(fname, "r") as f:
        for l in f:
            l = l.strip()
            if len(l) == 0:
                continue

            if l[0] == "#":
                continue

            best_features.append(l)

    return best_features


def check_against_binary_dat(fname, df, df_raw):
    global best_features
    check_file_exists(fname)
    df2 = pd.read_csv(fname, sep=",", names=best_features)
    print("Checking binary vs python:", fname)

    for f in best_features:
        for i in range(df.shape[0]):
            s_pyt = float(df[f][i])
            s_bin = float(df2[f][i])
            if np.isnan(s_pyt) and np.isnan(s_bin):
                continue
            if np.isnan(s_pyt) and not np.isnan(s_bin):
                assert False
            if not np.isnan(s_pyt) and np.isnan(s_bin):
                assert False
            diff = abs(s_pyt - s_bin)

            if diff > 10e-5:
                for f_raw in list(df_raw):
                    print("pyt_raw:", df_raw[f_raw][i], " feat: ", f_raw)
                print("pyt:", s_pyt, " feat: ", f)
                print("bin:", s_bin, " feat: ", f)
                print("diff for feat ", f, ": ", diff)
            else:
                #print("OK for feat", f)
                pass
            assert diff < 10e-5

    #assert df.equals(df2)


models = []
best_features = []
feat_gen_exprs = []
feat_gen_funcs = []

def add_features(df, df2):
    for i, feat_gen_func in zip(range(len(best_features)), feat_gen_funcs):
        df2[:, i] = feat_gen_func(df)

def set_up_features(features_fname):
    global best_features
    global feat_gen_exprs
    global feat_gen_funcs
    best_features = get_features(features_fname)
    for i, feat in zip(range(len(best_features)), best_features):
        feat_gen_expr = ccg.to_source(ast.parse(feat))
        feat_gen_exprs.append(feat_gen_expr)
        create_function = "def a%d(df): return %s" % (i, feat_gen_expr)
        exec(create_function)
        exec("feat_gen_funcs.append(a%d)" % i)
    #print(feat_gen_funcs)


def load_models(short_fname, long_fname, forever_fname):
    global models
    for fname in [short_fname, long_fname, forever_fname]:
        clf_xgboost = xgb.XGBRegressor(n_jobs=1)
        new_fname = fname.replace("-py.", "-xgb.")
        if not os.path.exists(new_fname):
            new_fname = fname.replace("-py.", ".")
        clf_xgboost.load_model(new_fname)
        models.append(clf_xgboost)


num_called = 0

# to test memory usage
#@profile
def predict(data, check=False, dump=False):
    global num_called
    ret = []

    df = pd.DataFrame(data, columns=raw_data)
    transformed_data = np.empty((df.shape[0], len(best_features)), dtype=float)
    if check:
        dump_or_check('df_check'+str(num_called), df)

    add_features(df, transformed_data)
    df_final = pd.DataFrame(transformed_data, columns=best_features)
    df_final.replace([np.inf, np.NaN, np.inf, np.NINF, np.Infinity], MISSING, inplace=True)

    if check:
        check_against_binary_dat('bin_dump'+str(num_called)+".csv", df_final, df)

    for i in range(3):
        #x = models[i].predict(df_final)
        x = models[i].get_booster().inplace_predict(df_final)
        if check:
            dump_or_check('x-%d-%d' % (num_called, i), pd.DataFrame(x))
        ret.append(x)

    if dump:
        for i,name in zip(range(3), ["short", "long", "forever"]):
            df["x.used_later_%s" % name] = ret[i]

        df.to_pickle('df_dump_%s' % num_called)

    if check:
        dump_or_check('df_pred'+str(num_called), df_final)

    num_called += 1
    del df
    del df_final
    del transformed_data
    # gc.collect() # to test memory safety
    return ret

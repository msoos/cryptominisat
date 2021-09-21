#!/usr/bin/python3

import numpy as np
import pandas as pd
import time
import xgboost as xgb
import ast
import crystalcodegen as ccg
import pickle
import os

MISSING=np.NaN

# check reproducibility by dumping and checking against previous run's dump
def dump_or_check(fname, df):
    if check_file_exists(fname):
        picklefile = open(fname, 'rb')
        df_saved = pickle.load(picklefile)
        picklefile.close()
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
        picklefile = open(fname, 'wb')
        pickle.dump(df, picklefile)
        picklefile.close()
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

raw_data = [
    "cl.glue_before_minim",
    "rdb0.last_touched",
    "rdb0.act_ranking_rel",
    "rdb0.uip1_ranking_rel",
    "rdb0.prop_ranking_rel",
    "rdb0.last_touched_diff",
    "cl.time_inside_solver",
    "rdb0.props_made",
    "rdb0_common.avg_props",
    "rdb0_common.avg_glue",
    "rdb0_common.avg_uip1_used",
    "rdb0.sum_props_made",
    "rdb0.discounted_props_made",
    "rdb0.discounted_uip1_used",
    "rdb0.sum_uip1_used",
    "rdb0.uip1_used",
    "rdb0.glue",
    "cl.glueHist_avg",
    "cl.atedecents_binIrred",
    "cl.glueHistLT_avg",
    "cl.glueHist_longterm_avg",
    "cl.num_antecedents",
    "cl.overlapHistLT_avg",
    "cl.conflSizeHist_avg",
    "cl.atedecents_binRed"
    #"sum_uip1_per_time_ranking_rel",
    #"sum_props_per_time_ranking_rel",
]


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
    print(feat_gen_funcs)


def load_models(short_fname, long_fname, forever_fname):
    global models
    for fname in [short_fname, long_fname, forever_fname]:
        clf_xgboost = xgb.XGBRegressor(n_jobs=1)
        clf_xgboost.load_model(fname.replace("_py.", "_xgboost."))
        models.append(clf_xgboost)

num_called = 0
def predict(data, check=False):
    ret = []
    df = pd.DataFrame(data, columns=raw_data)
    outarray = np.empty((df.shape[0], len(best_features)), dtype=float)
    if check:
        global num_called
        dump_or_check('df_dat'+str(num_called), df)

    add_features(df, outarray)
    df_final = pd.DataFrame(outarray, columns=best_features)
    df_final.replace([np.inf, np.NaN, np.inf, np.NINF, np.Infinity], MISSING, inplace=True)

    for i in range(3):
        #x = models[i].predict(df_final)
        x = models[i].get_booster().inplace_predict(df_final)
        if check:
            dump_or_check('x-%d-%d' % (num_called, i), pd.DataFrame(x))
        ret.append(x)

    if check:
        dump_or_check('df_pred'+str(num_called), df_final)
        num_called += 1
    return ret

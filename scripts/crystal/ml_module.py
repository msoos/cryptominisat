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

MISSING=np.nan

# the raw columns the solver passes, set by set_up_features() from
# predict_features_gen.h so C++ and Python never disagree
raw_data = []

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


models = []
best_features = []
feat_gen_exprs = []
feat_gen_funcs = []

def add_features(df, df2):
    for i, feat_gen_func in zip(range(len(best_features)), feat_gen_funcs):
        df2[:, i] = feat_gen_func(df)

def set_up_features(features_fname, raw_names):
    global best_features
    global raw_data
    raw_data = list(raw_names)
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


# called from cl_predictors_py.cpp with one row of raw_data per clause,
# returns [short, long, forever] predictions
def predict(data):
    df = pd.DataFrame(data, columns=raw_data)
    transformed_data = np.empty((df.shape[0], len(best_features)), dtype=float)
    add_features(df, transformed_data)
    df_final = pd.DataFrame(transformed_data, columns=best_features)
    df_final.replace([np.inf, -np.inf], MISSING, inplace=True)

    ret = []
    for i in range(3):
        ret.append(models[i].get_booster().inplace_predict(df_final))
    return ret

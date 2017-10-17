#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pickle
import pandas as pd
import matplotlib.pyplot as plt
from pandas.tools.plotting import scatter_matrix

print(plt.get_backend())

df = None
with open("pandasdata.dat", "rb") as f:
    df = pickle.load(f)

# print(df.describe())
# plt.style.use = 'default'
pd.options.display.mpl_style = 'default'
features = ['glue', 'size', 'conflicts_this_restart', 'num_overlap_literals', 'num_antecedents', 'antecedents_avg_size', 'trail_depth_level', 'atedecents_binIrred', 'atedecents_binRed', 'atedecents_longIrred', 'atedecents_longRed', 'antecedents_glue_long_reds_avg', 'antecedents_glue_long_reds_var', 'antecedents_glue_long_reds_min', 'antecedents_glue_long_reds_max', 'antecedents_long_red_age_avg', 'antecedents_long_red_age_var', 'antecedents_long_red_age_min', 'antecedents_long_red_age_max', 'antecedents_antecedents_vsids_avg', 'cl.size_rel', 'cl.glue_rel', 'cl.num_antecedents_rel', 'cl.decision_level_rel', 'cl.backtrack_level_rel', 'cl.trail_depth_level_rel', 'cl.vsids_vars_rel']

# df = df.filter(items = features+["good"])
#print(df.describe())
#scatter_matrix(df, alpha=0.2, figsize=(6, 6), diagonal='kde')

for x in features[:1]:
    df2 = df.filter(items=["good", x])
    myplot = df2.groupby("good").hist()
    print("OK")

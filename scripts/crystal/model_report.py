#!/usr/bin/env python3
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

# One HTML page about a predictor model: what it was trained on (the
# attributes cldata_predict.py stores), what is in it (xgboost's own tree
# statistics and importances), and what it does (SHAP: direction and size
# of every feature's effect, dependence and interactions, exact for tree
# models). Nothing here is computed by hand: xgboost and shap do it all.
#
# usage: model_report.py model.json [frame.dat] -o report.html [--rows 3000]
#   model.json: a predictor-*.json in learn.sh's output dir, or
#               src/predict/predictor_*.json
#   frame.dat:  the comb-*.dat it was trained on, in learn.sh's output
#               dir; if left out, the model's train_frame attribute is
#               looked for next to the model
#
# needs: pip install --user shap   (and graphviz + the dot binary for the
# tree drawings; without dot the trees are given as text)

import argparse
import base64
import html
import io
import json
import os
import sys

import numpy as np
import pandas as pd
import xgboost as xgb
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import helper

parser = argparse.ArgumentParser()
parser.add_argument("model")
parser.add_argument("frame", nargs="?", default=None)
parser.add_argument("-o", "--out", required=True)
parser.add_argument("--rows", type=int, default=3000, help="rows for SHAP, default 3000")
parser.add_argument("--seed", type=int, default=0)
parser.add_argument("--trees", type=int, default=2, help="trees to draw/dump, default 2")
opts = parser.parse_args()


def fig_to_html(fig, width=900):
    buf = io.BytesIO()
    fig.savefig(buf, format="png", dpi=100, bbox_inches="tight")
    plt.close(fig)
    return '<img width="%d" src="data:image/png;base64,%s">' % (
        width, base64.b64encode(buf.getvalue()).decode())


def table(df, floatfmt="%.4g"):
    df = df.copy()
    for c in df.columns:  # percentages print as such
        if str(c).endswith("%"):
            df[c] = df[c].map(lambda x: "%.2f %%" % x)
    return df.to_html(float_format=lambda x: floatfmt % x, border=0, classes="t")


# ---- the model
booster = xgb.Booster()
booster.load_model(opts.model)
features = booster.feature_names
attrs = booster.attributes()
if opts.frame is None:
    if "train_frame" not in attrs:
        sys.exit("ERROR: the model does not say what frame it was trained on, give it")
    opts.frame = os.path.join(os.path.dirname(os.path.abspath(opts.model)), attrs["train_frame"])
    if not os.path.exists(opts.frame):
        sys.exit("ERROR: the model was trained on %s, not next to it; give the frame "
                 "(it is in learn.sh's output dir)" % attrs["train_frame"])
cfg = json.loads(booster.save_config())
learner = cfg["learner"]
n_trees = int(learner["gradient_booster"]["gbtree_model_param"]["num_trees"])
sections = []

# the file keeps the trees, not the training parameters (save_config()
# would give defaults), so the depth is measured
tdf = booster.trees_to_dataframe()
def tree_depth(t):
    nodes = t.set_index("ID")
    depth = {t.iloc[0]["ID"]: 0}
    for _, n in t.iterrows():
        for c in (n["Yes"], n["No"]):
            if isinstance(c, str) and c in nodes.index:
                depth[c] = depth[n["ID"]] + 1
    return max(depth.values())
depths = tdf.groupby("Tree").apply(tree_depth, include_groups=False)

sections.append(("Model", "<pre>%s</pre>" % html.escape("\n".join([
    "file:       %s" % opts.model,
    "objective:  %s" % learner["objective"]["name"],
    "trees:      %d, depth %d to %d (measured; the file keeps no training parameters)" % (
        n_trees, depths.min(), depths.max()),
    "features:   %d" % len(features),
    "base score: %s" % learner["learner_model_param"]["base_score"],
] + ["%-12s%s" % (k + ":", v) for k, v in sorted(attrs.items()) if k not in ("feature_lo", "feature_hi")]))))

# ---- the data, prepared as cldata_predict.py does
df = pd.read_pickle(opts.frame)
df = df.convert_dtypes(convert_integer=False, convert_string=False, convert_floating=False)
helper.make_missing_into_nan(df)
helper.add_features_from_list(df, features)
X_all = df[features].astype(np.float32).replace([np.inf, -np.inf], np.nan)
label = [c for c in df.columns if c.startswith("x.used_later") and c.endswith("_rel")]
label = label[0] if label else None
rng = np.random.RandomState(opts.seed)
idx = rng.choice(len(X_all), min(opts.rows, len(X_all)), replace=False)
X = X_all.iloc[idx].reset_index(drop=True)

# ---- the training ranges the model carries, against this frame
if "feature_lo" in attrs:
    lo = [float(v) for v in attrs["feature_lo"].split()]
    hi = [float(v) for v in attrs["feature_hi"].split()]
    rows = []
    for i, f in enumerate(features):
        v = X_all[f].dropna()
        rows.append({"feature": f, "p1 (model)": lo[i], "p99 (model)": hi[i],
                     "min (frame)": v.min(), "max (frame)": v.max(),
                     "outside %": 100.0 * ((v < lo[i]) | (v > hi[i])).mean(),
                     "missing %": 100.0 * X_all[f].isna().mean()})
    sections.append(("Training ranges", "<p>The 1st/99th percentile stored in the model, "
        "and the frame given here. On the training frame itself ~1% is above p99 and ~1% below "
        "p1 by construction, so ~2% outside is the floor, ~1% where p1 is also the minimum.</p>"
        + table(pd.DataFrame(rows).set_index("feature"))))

# ---- what is in the trees: xgboost's own statistics
splits = tdf[tdf["Feature"] != "Leaf"]
per_tree = tdf.groupby("Tree").agg(nodes=("ID", "size"), leaves=("Feature", lambda s: (s == "Leaf").sum()))
structure = "<pre>%s</pre>" % html.escape(
    "trees %d, nodes %d, leaves %d, splits %d; per tree: %.1f nodes, %.1f leaves\n" % (
        n_trees, len(tdf), (tdf["Feature"] == "Leaf").sum(), len(splits),
        per_tree["nodes"].mean(), per_tree["leaves"].mean())
    + "leaf values: min %.4g, max %.4g, mean %.4g" % (
        tdf.loc[tdf["Feature"] == "Leaf", "Gain"].min(),
        tdf.loc[tdf["Feature"] == "Leaf", "Gain"].max(),
        tdf.loc[tdf["Feature"] == "Leaf", "Gain"].mean()))
imp = pd.DataFrame({
    "gain": pd.Series(booster.get_score(importance_type="gain")),
    "total_gain": pd.Series(booster.get_score(importance_type="total_gain")),
    "splits": pd.Series(booster.get_score(importance_type="weight")),
    "cover": pd.Series(booster.get_score(importance_type="cover")),
}).reindex(features).fillna(0)
imp["root splits"] = splits[splits["Node"] == 0]["Feature"].value_counts().reindex(features).fillna(0).astype(int)
imp["total_gain %"] = 100.0 * imp["total_gain"] / imp["total_gain"].sum()
imp = imp.sort_values("total_gain", ascending=False)
unused = [f for f in features if imp.loc[f, "splits"] == 0]
fig, ax = plt.subplots(figsize=(9, 0.3 * len(features) + 1))
imp["total_gain %"].iloc[::-1].plot.barh(ax=ax)
ax.set_xlabel("share of total gain (%)")
sections.append(("What is in the trees",
    structure
    + "<p>xgboost importances: <b>gain</b> = average loss reduction per split on the feature, "
      "<b>total_gain</b> = summed over its splits (the usual importance), <b>splits</b> = how often "
      "it is split on, <b>cover</b> = average rows reaching those splits, <b>root splits</b> = trees "
      "whose first split is on it."
    + ("</p><p><b>Never split on:</b> %s" % ", ".join(unused) if unused else "") + "</p>"
    + table(imp[["total_gain %", "gain", "splits", "cover", "root splits"]])
    + fig_to_html(fig)))

# ---- what the model does: SHAP (exact for tree models)
import shap
explainer = shap.TreeExplainer(booster)
sv = explainer.shap_values(X)
mean_abs = pd.Series(np.abs(sv).mean(axis=0), index=features).sort_values(ascending=False)
pred = booster.predict(xgb.DMatrix(X, feature_names=features))
order = list(mean_abs.index)

fig = plt.figure(figsize=(10, 0.35 * len(features) + 1))
shap.summary_plot(sv, X, feature_names=features, show=False, max_display=len(features))
beeswarm = fig_to_html(plt.gcf(), width=500)

corr = pd.Series({f: np.corrcoef(X[f].fillna(X[f].median()), sv[:, i])[0, 1]
                  if X[f].nunique() > 1 else 0.0 for i, f in enumerate(features)})
shap_tab = pd.DataFrame({"mean |SHAP|": mean_abs,
                         "share %": 100.0 * mean_abs / mean_abs.sum(),
                         "direction (corr value vs SHAP)": corr.reindex(mean_abs.index)})
sections.append(("What the model does (SHAP)",
    "<p>On %d sampled training rows. Prediction = base value %.4g + the sum of the SHAP values; "
    "mean |SHAP| is how much a feature moves the prediction on average, direction is the "
    "correlation between a feature's value and its SHAP value (+: bigger value, bigger score).</p>" % (
        len(X), explainer.expected_value)
    + table(shap_tab)
    + "<p>Beeswarm: one dot per row; x = effect on the score, colour = the feature's value.</p>"
    + beeswarm))

# ---- dependence of the top features, coloured by the strongest interaction
deps = []
for f in order[:8]:
    fig = plt.figure(figsize=(7, 4))
    shap.dependence_plot(f, sv, X, feature_names=features, show=False, alpha=0.4)
    deps.append(fig_to_html(plt.gcf(), width=450))
sections.append(("Dependence of the top features",
    "<p>SHAP value against the feature's value; the colour is the feature SHAP picks as "
    "interacting most with it (vertical spread at one x = that interaction).</p>"
    + "\n".join(deps)))

# ---- exact interaction values on a smaller sample
n_int = min(600, len(X))
siv = explainer.shap_interaction_values(X.iloc[:n_int])
inter = np.abs(siv).mean(axis=0)
np.fill_diagonal(inter, 0)
pairs = []
for i in range(len(features)):
    for j in range(i + 1, len(features)):
        pairs.append((inter[i, j] * 2, features[i], features[j]))
pairs.sort(reverse=True)
ptab = pd.DataFrame(pairs[:15], columns=["mean |interaction|", "feature", "with"]).set_index("feature")
sections.append(("Strongest pairwise interactions",
    "<p>Exact SHAP interaction values on %d rows: how much of a feature's effect depends on "
    "another feature's value (the off-diagonal of the interaction matrix).</p>" % n_int + table(ptab)))

# ---- the first trees, a sanity check only: one tree is 1/40th of the
# model, the SHAP sections above say what the model does. Features are
# aliased f0..fN to keep the trees narrow; no per-node stats, the gains
# are in the importance table
legend = pd.DataFrame({"feature": features}, index=["f%d" % i for i in range(len(features))])
aliased = xgb.Booster()
aliased.load_model(opts.model)
aliased.feature_names = list(legend.index)
trees_html = ["<p><b>Sanity check only</b> (does the first split look sane?): the first %d of %d "
              "trees, each a small step of the ensemble. Feature aliases:</p>" % (
                  min(opts.trees, n_trees), n_trees) + table(legend)]
try:
    import graphviz  # noqa
    for t in range(min(opts.trees, n_trees)):
        g = xgb.to_graphviz(aliased, tree_idx=t, rankdir="LR")  # depth left to right, leaves stacked
        trees_html.append('<h3>tree %d</h3><div class="half">%s</div>' % (t, g.pipe(format="svg").decode()))
except Exception as e:  # no dot binary: the text dump
    dumps = aliased.get_dump(with_stats=False)
    for t in range(min(opts.trees, n_trees)):
        trees_html.append('<h3>tree %d</h3><pre class="half">%s</pre>' % (t, html.escape(dumps[t])))
    trees_html.append("<p>(drawn left to right with graphviz when the dot binary is installed; %s)</p>"
                      % html.escape(str(e)))
sections.append(("The first trees (sanity check)", "\n".join(trees_html)))

# ---- fit on the frame, if the label is there
if label is not None:
    y = df[label].iloc[idx].astype(float).values
    r = np.corrcoef(y, pred)[0, 1]
    from scipy.stats import spearmanr
    rho = spearmanr(y, pred).correlation
    fig, ax = plt.subplots(figsize=(6, 5))
    ax.scatter(y, pred, s=3, alpha=0.3)
    ax.set_xlabel(label)
    ax.set_ylabel("prediction")
    sections.append(("Fit on these rows",
        "<p>Pearson r = %.3f, Spearman rho = %.3f between %s and the prediction (train and test rows "
        "mixed: an upper bound, see learn.sh's 'ranking test' lines for the held-out numbers).</p>" % (
            r, rho, label) + fig_to_html(fig, width=500)))

# ---- write
body = "\n".join("<h2>%s</h2>\n%s" % (html.escape(t), c) for t, c in sections)
with open(opts.out, "w") as f:
    f.write("""<!doctype html><html><head><meta charset="utf-8"><title>%s</title>
<style>body{font-family:sans-serif;max-width:1000px;margin:auto;padding:1em}
.t{border-collapse:collapse;font-size:90%%}.t td,.t th{padding:2px 8px;text-align:right;border-bottom:1px solid #ddd}
.t th{text-align:left}pre{background:#f4f4f4;padding:.5em;overflow-x:auto}
.half{zoom:0.5}.half svg{max-width:100%%;height:auto}</style></head>
<body><h1>Predictor model report</h1><p>%s on %s</p>%s</body></html>""" % (
        html.escape(os.path.basename(opts.model)), html.escape(opts.model), html.escape(opts.frame), body))
print("wrote", opts.out)

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

# Sorts the features of a best_features file into groups, for the group
# ablation (ablate_groups.sh): a feature is in a group if any raw column
# it uses matches the group, and a feature can be in several.
#
# usage: feature_groups.py best_features.txt            # list the groups
#        feature_groups.py best_features.txt --without recency -o out.txt

import argparse
import ast
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_pred_features

# group -> what a raw column must contain to belong
GROUPS = {
    "recency":  ["discounted_", "props_made", "uip1_used", "rdb0.used", "last_touched", "sum_props", "sum_uip1"],
    "snapshot": ["cl.glueHist", "cl.trailDepth", "cl.conflSize", "cl.numResolutions", "cl.antec_data",
                 "cl.overlap", "cl.branchDepth", "cl.decision_level", "cl.trail_depth", "cl.antecedents_",
                 "cl.num_antecedents", "cl.num_total_lits", "cl.num_overlap", "cl.size_before_minim",
                 "cl.glue_before_minim", "cl.cur_restart_type"],
    "sizeglue": ["rdb0.size", "rdb0.glue", "cl.orig_size", "cl.orig_glue"],
    "rankings": ["_ranking"],
    "context":  ["rdb0_common."],
    "cost":     ["visited", "lit_act_rel", "lit_vmtf_rel", "num_assigned", "num_false_lev0"],
    "age":      ["time_inside_solver", "introduced_at"],
}


def groups_of(feat):
    cols = set()
    gen_pred_features.to_cpp(ast.parse(feat, mode="eval"), cols)
    ret = set()
    for g, pats in GROUPS.items():
        if any(p in c for c in cols for p in pats):
            ret.add(g)
    return ret


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("features")
    parser.add_argument("--without", default=None, help="write the list without this group")
    parser.add_argument("-o", "--out", default=None)
    opts = parser.parse_args()
    feats = gen_pred_features.read_features(opts.features)
    if opts.without:
        kept = [f for f in feats if opts.without not in groups_of(f)]
        out = open(opts.out, "w") if opts.out else sys.stdout
        out.write("# %s without the group %s (%d of %d features)\n" % (
            os.path.basename(opts.features), opts.without, len(kept), len(feats)))
        for f in kept:
            out.write(f + "\n")
        if opts.out:
            print("%s: %d features without %s" % (opts.out, len(kept), opts.without))
    else:
        for g in GROUPS:
            members = [f for f in feats if g in groups_of(f)]
            print("%-9s %2d: %s" % (g, len(members), ", ".join(members)[:150]))
        none = [f for f in feats if not groups_of(f)]
        if none:
            print("no group  %2d: %s" % (len(none), ", ".join(none)))

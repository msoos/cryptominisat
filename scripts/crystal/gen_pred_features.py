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

# Generates the C++ that computes the predictor's input features at run
# time from a best_features file, so the feature list is the single source
# of truth for training (pandas eval of the same expressions) and solving.
#
# usage: gen_pred_features.py best_features.txt -o predict_features_gen.h
#        gen_pred_features.py --list-raw      # raw columns the solver can compute
#
# A feature is an expression over raw columns with + - * / and numbers.
# Any raw column that is not available (ternary resolvents have no
# clause_stats, no glue...) or a division by zero makes the feature missing.

import argparse
import ast
import sys

# raw column -> (C++ expression over `in`, condition under which it is missing)
TERN = "in.cl->stats.is_ternary_resolvent"
NOGLUE = "(in.cl->stats.is_ternary_resolvent || in.cl->stats.glue == CL_MAX_GLUE)"
RAW = {
    # reduceDB: the clause's state at this reduce
    "rdb0.props_made": ("in.cl->stats.props_made", None),
    "rdb0.uip1_used": ("in.cl->stats.uip1_used", None),
    "rdb0.sum_props_made": ("in.e.sum_props_made", None),
    "rdb0.sum_uip1_used": ("in.e.sum_uip1_used", None),
    "rdb0.discounted_props_made": ("in.e.discounted_props_made", None),
    "rdb0.discounted_props_made2": ("in.e.discounted_props_made2", None),
    "rdb0.discounted_props_made3": ("in.e.discounted_props_made3", None),
    "rdb0.discounted_uip1_used": ("in.e.discounted_uip1_used", None),
    "rdb0.discounted_uip1_used2": ("in.e.discounted_uip1_used2", None),
    "rdb0.discounted_uip1_used3": ("in.e.discounted_uip1_used3", None),
    "rdb0.glue": ("in.cl->stats.glue", NOGLUE),
    "rdb0.size": ("in.cl->size()", None),
    "rdb0.used": ("in.cl->stats.used", None),
    "rdb0.is_ternary_resolvent": ("in.cl->stats.is_ternary_resolvent", None),
    "rdb0.is_decision": ("in.cl->stats.is_decision", None),
    "rdb0.is_distilled": ("in.cl->distilled", None),
    "rdb0.last_touched_any_diff": ("(in.sumConflicts - (uint64_t)in.cl->stats.last_touched_any)", None),
    "rdb0.activity_rel": ("((double)in.cl->stats.activity/in.s->get_cla_inc())", None),
    "rdb0.introduced_at_conflict": ("in.e.introduced_at_conflict", None),
    "rdb0.act_ranking": ("in.e.act_ranking", None),
    "rdb0.prop_ranking": ("in.e.prop_ranking", None),
    "rdb0.uip1_ranking": ("in.e.uip1_ranking", None),
    "rdb0.sum_uip1_per_time_ranking": ("in.e.sum_uip1_per_time_ranking", None),
    "rdb0.sum_props_per_time_ranking": ("in.e.sum_props_per_time_ranking", None),
    "rdb0.act_ranking_rel": ("in.act_ranking_rel", None),
    "rdb0.prop_ranking_rel": ("in.prop_ranking_rel", None),
    "rdb0.uip1_ranking_rel": ("in.uip1_ranking_rel", None),
    "rdb0.sum_uip1_per_time_ranking_rel": ("in.sum_uip1_per_time_ranking_rel", None),
    "rdb0.sum_props_per_time_ranking_rel": ("in.sum_props_per_time_ranking_rel", None),

    # reduceDB_common: the whole learnt DB at this reduce
    "rdb0_common.tot_cls_in_db": ("in.c.all_learnt_size", None),
    "rdb0_common.cur_restart_type": ("in.c.cur_rst_type", None),
    "rdb0_common.avg_props": ("in.c.avg_props", None),
    "rdb0_common.avg_uip1_used": ("in.c.avg_uip", None),
    "rdb0_common.avg_sum_uip1_per_time": ("in.c.avg_sum_uip1_per_time", None),
    "rdb0_common.avg_sum_props_per_time": ("in.c.avg_sum_props_per_time", None),
    "rdb0_common.median_act": ("in.c.median_data.median_act", None),
    "rdb0_common.median_uip1_used": ("in.c.median_data.median_uip1_used", None),
    "rdb0_common.median_props": ("in.c.median_data.median_props", None),
    "rdb0_common.median_sum_uip1_per_time": ("in.c.median_data.median_sum_uip1_per_time", None),
    "rdb0_common.median_sum_props_per_time": ("in.c.median_data.median_sum_props_per_time", None),
    "rdb0_common.num_vars": ("in.s->nVars()", None),
    "rdb0_common.num_long_irred_cls": ("in.s->longIrredCls.size()", None),
    "rdb0_common.num_long_irred_cls_lits": ("in.s->litStats.irredLits", None),
    "rdb0_common.num_long_red_cls": ("in.s->longRedCls[0].size()", None),
    "rdb0_common.num_long_red_cls_lits": ("in.s->litStats.redLits", None),
    "rdb0_common.num_bin_irred_cls": ("in.s->binTri.irredBins", None),
    "rdb0_common.num_bin_red_cls": ("in.s->binTri.redBins", None),
    "rdb0_common.trailDepthHistLT_avg": ("in.s->hist.trailDepthHistLT.avg()", None),
    "rdb0_common.backtrackLevelHistLT_avg": ("in.s->hist.backtrackLevelHistLT.avg()", None),
    "rdb0_common.conflSizeHistLT_avg": ("in.s->hist.conflSizeHistLT.avg()", None),
    "rdb0_common.numResolutionsHistLT_avg": ("in.s->hist.numResolutionsHistLT.avg()", None),
    "rdb0_common.glueHistLT_avg": ("in.s->hist.glueHistLT.avg()", None),
    "rdb0_common.antec_data_sum_sizeHistLT_avg": ("in.s->hist.antec_data_sum_sizeHistLT.avg()", None),
    "rdb0_common.overlapHistLT_avg": ("in.s->hist.overlapHistLT.avg()", None),

    # clause_stats: what was known when the clause was learnt. Ternary
    # resolvents were never learnt, all of these are missing for them
    "cl.orig_glue": ("in.e.orig_glue", TERN),
    "cl.glue_before_minim": ("in.e.glue_before_minim", TERN),
    "cl.orig_size": ("in.e.orig_size", TERN),
    "cl.size_before_minim": ("in.e.size_before_minim", TERN),
    "cl.num_overlap_literals": ("in.e.num_overlap_literals", TERN),
    "cl.num_antecedents": ("in.e.num_antecedents", TERN),
    "cl.num_total_lits_antecedents": ("in.e.num_total_lits_antecedents", TERN),
    "cl.is_decision": ("in.cl->stats.is_decision", TERN),
    "cl.decision_level": ("in.e.decision_level", TERN),
    "cl.trail_depth_level": ("in.e.trail_depth_level", TERN),
    "cl.cur_restart_type": ("in.e.learnt_rst_type", TERN),
    "cl.antecedents_binIrred": ("in.e.antecedents_binIrred", TERN),
    "cl.antecedents_binRed": ("in.e.antecedents_binred", TERN),
    "cl.antecedents_longIrred": ("in.e.antecedents_longIrred", TERN),
    "cl.antecedents_longRed": ("in.e.antecedents_longRed", TERN),
    "cl.trailDepthHistLT_avg": ("in.e.trailDepthHistLT_avg", TERN),
    "cl.conflSizeHistLT_avg": ("in.e.conflSizeHistLT_avg", TERN),
    "cl.glueHistLT_avg": ("in.e.glueHistLT_avg", TERN),
    "cl.numResolutionsHistLT_avg": ("in.e.numResolutionsHistLT_avg", TERN),
    "cl.antec_data_sum_sizeHistLT_avg": ("in.e.antec_data_sum_sizeHistLT_avg", TERN),
    "cl.overlapHistLT_avg": ("in.e.overlapHistLT_avg", TERN),
    "cl.branchDepthHistQueue_avg": ("in.e.branchDepthHistQueue_avg", TERN),
    "cl.trailDepthHist_avg": ("in.e.trailDepthHist_avg", TERN),
    "cl.conflSizeHist_avg": ("in.e.conflSizeHist_avg", TERN),
    "cl.glueHist_avg": ("in.e.glueHist_avg", TERN),
    "cl.glueHist_longterm_avg": ("in.e.glueHist_longterm_avg", TERN),
    "cl.time_inside_solver": ("(in.sumConflicts - (uint64_t)in.e.introduced_at_conflict)", None),
}


def cname(raw):
    return "raw_" + raw.replace(".", "_")


def flatten_name(node):
    """rdb0.glue parses as Attribute(Name('rdb0'), 'glue')"""
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return flatten_name(node.value) + "." + node.attr
    raise ValueError("not a column: %s" % ast.dump(node))


def to_cpp(node, used):
    if isinstance(node, ast.Expression):
        return to_cpp(node.body, used)
    if isinstance(node, (ast.Name, ast.Attribute)):
        name = flatten_name(node)
        if name not in RAW:
            raise ValueError("raw column '%s' cannot be computed by the solver, see --list-raw" % name)
        used.add(name)
        return "%s(in, miss)" % cname(name)
    if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
        return "(double)%r" % node.value
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        return "(-%s)" % to_cpp(node.operand, used)
    if isinstance(node, ast.BinOp):
        l = to_cpp(node.left, used)
        r = to_cpp(node.right, used)
        if isinstance(node.op, ast.Div):
            return "fdiv(%s, %s, miss)" % (l, r)
        ops = {ast.Add: "+", ast.Sub: "-", ast.Mult: "*"}
        if type(node.op) not in ops:
            raise ValueError("unsupported operator in %s" % ast.dump(node))
        return "(%s %s %s)" % (l, ops[type(node.op)], r)
    raise ValueError("unsupported expression: %s" % ast.dump(node))


def read_features(fname):
    feats = []
    with open(fname) as f:
        for l in f:
            l = l.strip()
            if l and l[0] != "#":
                feats.append(l)
    return feats


def generate(fname, out):
    feats = read_features(fname)
    used = set()
    body = []
    for i, feat in enumerate(feats):
        expr = to_cpp(ast.parse(feat, mode="eval"), used)
        body.append("    //%d: %s" % (i, feat))
        body.append("    { bool miss = false; const double v = %s;" % expr)
        body.append("      at[%d] = miss ? missing_val : (float)v; }" % i)

    raw_funcs = []
    for name, (expr, misscond) in RAW.items():
        if misscond is None:
            raw_funcs.append("static inline double %s(const In& in, bool&) { return (double)(%s); }"
                             % (cname(name), expr))
        else:
            raw_funcs.append("static inline double %s(const In& in, bool& miss) { if (%s) { miss = true; return 0; } return (double)(%s); }"
                             % (cname(name), misscond, expr))
    raw_fill = []
    for i, name in enumerate(RAW):
        raw_fill.append("    { bool miss = false; const double v = %s(in, miss); at[%d] = miss ? missing_val : (float)v; }"
                        % (cname(name), i))

    out.write("""// GENERATED by scripts/crystal/gen_pred_features.py from %s -- do not edit
#pragma once
#include <cstdint>
#include "clause.h"
#include "solver.h"
#include "cl_predictors_abs.h"

#define PRED_COLS %d

namespace CMSat { namespace predgen {

struct In {
    const Clause* cl;
    const ClauseStatsExtra& e;
    const Solver* s;
    const ReduceCommonData& c;
    uint64_t sumConflicts;
    double act_ranking_rel;
    double uip1_ranking_rel;
    double prop_ranking_rel;
    double sum_uip1_per_time_ranking_rel;
    double sum_props_per_time_ranking_rel;
};

static inline double fdiv(double a, double b, bool& miss) { if (b == 0) { miss = true; return 0; } return a/b; }

%s

//every raw column, in this order, for the Python predictor (ml_module.py)
static const int NUM_RAW = %d;
static const char* const raw_names[] = {
%s
};

static inline void fill_raw(const In& in, const float missing_val, float* at)
{
%s
}

//the features of %s, in file order
static inline void fill_features(const In& in, const float missing_val, float* at)
{
%s
}

}} //namespace
""" % (fname, len(feats), "\n".join(raw_funcs), len(RAW),
       "\n".join('    "%s",' % n for n in RAW), "\n".join(raw_fill),
       fname, "\n".join(body)))
    return feats, used


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("features", nargs="?", help="best_features file")
    parser.add_argument("-o", "--out", default=None, help="output header, default stdout")
    parser.add_argument("--list-raw", action="store_true", help="print the raw columns the solver can compute")
    opts = parser.parse_args()

    if opts.list_raw:
        for n in RAW:
            print(n)
        sys.exit(0)
    if opts.features is None:
        parser.error("need a best_features file")
    if opts.out:
        with open(opts.out, "w") as f:
            feats, used = generate(opts.features, f)
    else:
        feats, used = generate(opts.features, sys.stdout)
    print("%d features over %d raw columns -> %s" % (len(feats), len(used), opts.out or "stdout"),
          file=sys.stderr)

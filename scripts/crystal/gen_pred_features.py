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
import os
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
    "rdb0.last_touched_any_diff": ("(in.sum_conflicts - (uint64_t)in.cl->stats.last_touched_any)", None),
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
    # the cost of keeping the clause, and whether its variables are where
    # the search is (see Searcher::cl_lit_act_rel & co)
    "rdb0.visited": ("in.cl->stats.visited", None),
    "rdb0.sum_visited": ("in.e.sum_visited", None),
    "rdb0.discounted_visited": ("in.e.discounted_visited", None),
    "rdb0.lit_act_rel": ("in.s->cl_lit_act_rel(*in.cl)", None),
    "rdb0.lit_vmtf_rel": ("in.s->cl_lit_vmtf_rel(*in.cl)", None),
    "rdb0.num_assigned": ("in.s->cl_num_assigned(*in.cl)", None),
    "rdb0.num_false_lev0": ("in.s->cl_num_false_lev0(*in.cl)", None),

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
    "rdb0_common.num_long_irred_cls": ("in.s->long_irred_cls.size()", None),
    "rdb0_common.num_long_irred_cls_lits": ("in.s->lit_stats.irred_lits", None),
    "rdb0_common.num_long_red_cls": ("in.s->long_red_cls[0].size()", None),
    "rdb0_common.num_long_red_cls_lits": ("in.s->lit_stats.red_lits", None),
    "rdb0_common.num_bin_irred_cls": ("in.s->bin_tri.irred_bins", None),
    "rdb0_common.num_bin_red_cls": ("in.s->bin_tri.red_bins", None),
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
    "cl.time_inside_solver": ("(in.sum_conflicts - (uint64_t)in.e.introduced_at_conflict)", None),
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


# Raw columns whose scale is the run's length or the instance's size. A
# tree model clamps every value beyond its training range, and the solver
# will run a hundred times longer than any training run and on bigger
# instances, so such a column may only appear divided by another of the
# same scale (a share of the clause's life, a rate per conflict, ...)
SCALE = {
    "run": ["cl.time_inside_solver", "rdb0.last_touched_any_diff", "rdb0.introduced_at_conflict",
            "rdb0.sum_props_made", "rdb0.sum_uip1_used", "rdb0.sum_visited", "rdb0.dump_no",
            "rdb0.act_ranking", "rdb0.prop_ranking", "rdb0.uip1_ranking",
            "rdb0.sum_uip1_per_time_ranking", "rdb0.sum_props_per_time_ranking"],
    "size": ["rdb0_common.num_vars", "rdb0_common.num_long_irred_cls", "rdb0_common.num_long_irred_cls_lits",
             "rdb0_common.num_long_red_cls", "rdb0_common.num_long_red_cls_lits",
             "rdb0_common.num_bin_irred_cls", "rdb0_common.num_bin_red_cls", "rdb0_common.tot_cls_in_db"],
}


def scale_degree(node):
    """{scale class: exponent} of an expression: a column of class c is
    c^1, a/b subtracts, a*b adds, a+b and a-b must agree"""
    if isinstance(node, ast.Expression):
        return scale_degree(node.body)
    if isinstance(node, (ast.Name, ast.Attribute)):
        name = flatten_name(node)
        for cls, cols in SCALE.items():
            if name in cols:
                return {cls: 1}
        return {}
    if isinstance(node, ast.Constant):
        return {}
    if isinstance(node, ast.UnaryOp):
        return scale_degree(node.operand)
    if isinstance(node, ast.BinOp):
        l, r = scale_degree(node.left), scale_degree(node.right)
        if isinstance(node.op, ast.Div):
            return {c: l.get(c, 0) - r.get(c, 0) for c in set(l) | set(r)}
        if isinstance(node.op, ast.Mult):
            return {c: l.get(c, 0) + r.get(c, 0) for c in set(l) | set(r)}
        if l != r:
            raise ValueError("adding things of different scale: %s" % ast.dump(node))
        return l
    raise ValueError("unsupported expression: %s" % ast.dump(node))


def check_scale_free(feat):
    deg = {c: d for c, d in scale_degree(ast.parse(feat, mode="eval")).items() if d != 0}
    if deg:
        raise ValueError("'%s' scales with the %s (%s): a tree model clamps it beyond the training "
                         "runs. Divide it by another column of that scale, or --allow-absolute"
                         % (feat, " and ".join("%s %s" % (("run length" if c == "run" else "instance size"), "^%d" % d)
                                               for c, d in deg.items()), ", ".join(deg)))


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


def read_ranges(fname, feats):
    """<list>.ranges, from feature_ranges.py: the training range of each
    feature; nan where unknown"""
    lo = {f: float("nan") for f in feats}
    hi = dict(lo)
    if fname and os.path.exists(fname):
        with open(fname) as f:
            for l in f:
                if l.startswith("#") or not l.strip():
                    continue
                name, a, b = l.rstrip("\n").split("\t")
                if name in lo:
                    lo[name], hi[name] = float(a), float(b)
    return [lo[f] for f in feats], [hi[f] for f in feats]


def generate(fname, out, allow_absolute=False, ranges=None):
    feats = read_features(fname)
    feat_lo, feat_hi = read_ranges(ranges if ranges else fname + ".ranges", feats)
    used = set()
    body = []
    for i, feat in enumerate(feats):
        if not allow_absolute:
            check_scale_free(feat)
        expr = to_cpp(ast.parse(feat, mode="eval"), used)
        body.append("    //%d: %s" % (i, feat))
        body.append("    { bool miss = false; const double v = %s;" % expr)
        body.append("      at[%d] = to_float(v, miss, missing_val); }" % i)

    raw_funcs = []
    for name, (expr, misscond) in RAW.items():
        if misscond is None:
            raw_funcs.append("static inline double %s(const In& in, bool&) { return (double)(%s); }"
                             % (cname(name), expr))
        else:
            raw_funcs.append("static inline double %s(const In& in, bool& miss) { if (%s) { miss = true; return 0; } return (double)(%s); }"
                             % (cname(name), misscond, expr))
    # what the per-reduce bookkeeping must compute for these features: each
    # ranking is a sort of the whole learnt DB at every reduce
    needs = {
        "NEED_ACT_RANKING": ["rdb0.act_ranking", "rdb0.act_ranking_rel", "rdb0_common.median_act"],
        "NEED_UIP1_RANKING": ["rdb0.uip1_ranking", "rdb0.uip1_ranking_rel", "rdb0_common.median_uip1_used"],
        "NEED_PROP_RANKING": ["rdb0.prop_ranking", "rdb0.prop_ranking_rel", "rdb0_common.median_props"],
        "NEED_SUM_UIP1_PER_TIME_RANKING": ["rdb0.sum_uip1_per_time_ranking", "rdb0.sum_uip1_per_time_ranking_rel",
                                          "rdb0_common.median_sum_uip1_per_time"],
        "NEED_SUM_PROPS_PER_TIME_RANKING": ["rdb0.sum_props_per_time_ranking", "rdb0.sum_props_per_time_ranking_rel",
                                           "rdb0_common.median_sum_props_per_time"],
    }
    need_lines = ["static constexpr bool %s = %s;" % (k, "true" if any(c in used for c in cols) else "false")
                  for k, cols in needs.items()]

    raw_fill = []
    for i, name in enumerate(RAW):
        raw_fill.append("    { bool miss = false; const double v = %s(in, miss); at[%d] = to_float(v, miss, missing_val); }"
                        % (cname(name), i))

    out.write("""// GENERATED by scripts/crystal/gen_pred_features.py from %s -- do not edit
#pragma once
#include <cstdint>
#include <cmath>
#include <cfloat>
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
    uint64_t sum_conflicts;
    double act_ranking_rel;
    double uip1_ranking_rel;
    double prop_ranking_rel;
    double sum_uip1_per_time_ranking_rel;
    double sum_props_per_time_ranking_rel;
};

static inline double fdiv(double a, double b, bool& miss) { if (b == 0) { miss = true; return 0; } return a/b; }

//features are float32 (as when training); a value beyond float32 is
//missing, and must not reach the cast: main_exe.cpp traps FE_OVERFLOW
static inline float to_float(double v, bool miss, float missing_val)
{
    if (miss || !std::isfinite(v) || std::fabs(v) > (double)FLT_MAX) return missing_val;
    return (float)v;
}

%s

//the per-reduce rankings these features need (the rest are skipped)
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

//the features of %s, in file order: the names a model must have been
//trained on (checked at load), their 1st/99th percentile in the training
//data (nan = unknown; values outside are counted and reported), and the
//code that computes them
static const char* const feature_names[] = {
%s
};
static const double feature_lo[] = { %s };
static const double feature_hi[] = { %s };
static inline void fill_features(const In& in, const float missing_val, float* at)
{
%s
}

}} //namespace
""" % (fname, len(feats), "\n".join(raw_funcs), "\n".join(need_lines), len(RAW),
       "\n".join('    "%s",' % n for n in RAW), "\n".join(raw_fill),
       fname, "\n".join('    "%s",' % f.replace('"', '\\"') for f in feats),
       ", ".join("NAN" if v != v else repr(v) for v in feat_lo),
       ", ".join("NAN" if v != v else repr(v) for v in feat_hi), "\n".join(body)))
    return feats, used


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("features", nargs="?", help="best_features file")
    parser.add_argument("-o", "--out", default=None, help="output header, default stdout")
    parser.add_argument("--list-raw", action="store_true", help="print the raw columns the solver can compute")
    parser.add_argument("--ranges", default=None, help="training ranges file, default <features>.ranges if it exists")
    parser.add_argument("--allow-absolute", action="store_true",
                        help="accept features that scale with the run length or the instance size (see SCALE)")
    opts = parser.parse_args()

    if opts.list_raw:
        for n in RAW:
            print(n)
        sys.exit(0)
    if opts.features is None:
        parser.error("need a best_features file")
    if opts.out:
        with open(opts.out, "w") as f:
            feats, used = generate(opts.features, f, opts.allow_absolute, opts.ranges)
    else:
        feats, used = generate(opts.features, sys.stdout, opts.allow_absolute, opts.ranges)
    print("%d features over %d raw columns -> %s" % (len(feats), len(used), opts.out or "stdout"),
          file=sys.stderr)

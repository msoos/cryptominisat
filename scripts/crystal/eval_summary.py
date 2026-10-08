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

# The A/B table from the solver outputs eval_corpus.sh wrote
# (<cnf>.<config>.s<seed>): per instance and config the conflicts and
# time (geometric mean over the seeds), then per config against the
# base: the geometric mean of the per-instance ratios with a bootstrap
# interval over the instances (conflicts, time, and bogoprops: the cost
# that does not move with the load of the box), solved runs, PAR2, and the noise floor
# (every seed of a config against its first seed: what a difference
# between two single runs is worth).
#
# usage: eval_summary.py <evaldir> [--base normal] [--timeout T]

import argparse
import glob
import os
import re

import numpy as np


def parse(fname):
    res = {"solved": False, "confl": None, "time": None, "bogo": None}
    with open(fname, errors="replace") as f:
        for l in f:
            if l.startswith("s SATISFIABLE") or l.startswith("s UNSATISFIABLE"):
                res["solved"] = True
            elif l.startswith("c conflicts") and res["confl"] is None:
                res["confl"] = float(l.split()[3])
            elif "Total time (this thread)" in l and res["time"] is None:
                res["time"] = float(l.split()[6])
            elif l.startswith("c Mbogo-props") and res["bogo"] is None:
                res["bogo"] = float(l.split()[3])
    if res["confl"] is None or res["time"] is None:
        res["solved"] = False
    return res


def gmean(x):
    return float(np.exp(np.mean(np.log(np.maximum(np.asarray(x, dtype=float), 1e-3)))))


def ratio_ci(ratios, rng, n=2000):
    """geometric mean of the per-instance ratios, 95% bootstrap interval"""
    logs = np.log(np.asarray(ratios, dtype=float))
    if len(logs) == 0:
        return None
    boots = [logs[rng.integers(0, len(logs), len(logs))].mean() for _ in range(n)]
    return (100*np.exp(logs.mean()), 100*np.exp(np.percentile(boots, 2.5)), 100*np.exp(np.percentile(boots, 97.5)))


def fmt_ci(ci):
    return "n/a" if ci is None else "%.0f%% [%.0f, %.0f]" % ci


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("evaldir")
    parser.add_argument("--base", default="normal")
    parser.add_argument("--timeout", type=float, default=None, help="for PAR2: an unsolved run counts 2x this")
    opts = parser.parse_args()

    # runs[instance][config][seed]
    runs = {}
    configs = []
    for fname in sorted(glob.glob(os.path.join(opts.evaldir, "*.s[0-9]*"))):
        m = re.match(r"(.*)\.([^.]+)\.s(\d+)$", os.path.basename(fname))
        if not m:
            continue
        inst, config, seed = m.group(1), m.group(2), int(m.group(3))
        runs.setdefault(inst, {}).setdefault(config, {})[seed] = parse(fname)
        if config not in configs:
            configs.append(config)
    if opts.base not in configs:
        print("ERROR: no runs of the base config '%s' in %s" % (opts.base, opts.evaldir))
        exit(1)
    configs = [opts.base] + [c for c in configs if c != opts.base]
    rng = np.random.default_rng(0)

    print("%-34s %s" % ("instance", "  ".join("%-30s" % ("%s confl / s / solved" % c) for c in configs)))
    for inst in sorted(runs):
        out = "%-34s " % inst[:34]
        for c in configs:
            rs = [r for r in runs[inst].get(c, {}).values()]
            ok = [r for r in rs if r["solved"]]
            if ok:
                out += "%-32s" % ("%10.0f %8.1f  %d/%d" % (
                    gmean([r["confl"] for r in ok]), gmean([r["time"] for r in ok]), len(ok), len(rs)))
            else:
                out += "%-32s" % ("%10s %8s  0/%d" % ("-", "-", len(rs)))
        print(out)

    def paired(config, base, what, seeds_c=None, seeds_b=None):
        """per-instance ratio config/base over the seeds both solved"""
        ratios = []
        for inst in sorted(runs):
            rc, rb = runs[inst].get(config, {}), runs[inst].get(base, {})
            if seeds_c is None:
                pairs = [(rc[s], rb[s]) for s in rc if s in rb]
            else:
                pairs = [(rc[a], rb[b]) for a, b in zip(seeds_c, seeds_b) if a in rc and b in rb]
            pairs = [(a, b) for a, b in pairs if a["solved"] and b["solved"] and a[what] and b[what]]
            if pairs:
                ratios.append(gmean([a[what] for a, b in pairs]) / gmean([b[what] for a, b in pairs]))
        return ratios

    def par2(config):
        tot, n = 0.0, 0
        for inst in runs:
            for r in runs[inst].get(config, {}).values():
                tot += r["time"] if r["solved"] else 2*opts.timeout
                n += 1
        return tot / max(n, 1)

    print()
    for c in configs:
        rs = [r for inst in runs for r in runs[inst].get(c, {}).values()]
        out = "solved: %-10s %d of %d runs" % (c, sum(r["solved"] for r in rs), len(rs))
        if opts.timeout:
            out += "  PAR2 %.1f s" % par2(c)
        print(out)
    for c in configs[1:]:
        rc = paired(c, opts.base, "confl")
        print("total: %s vs %s, %d instances: conflicts %s  time %s  bogoprops %s" % (
            c, opts.base, len(rc), fmt_ci(ratio_ci(rc, rng)), fmt_ci(ratio_ci(paired(c, opts.base, "time"), rng)),
            fmt_ci(ratio_ci(paired(c, opts.base, "bogo"), rng))))

    # the noise floor
    for c in configs:
        seeds = sorted(set(s for inst in runs for s in runs[inst].get(c, {})))
        if len(seeds) < 2:
            continue
        out = []
        for s in seeds[1:]:
            ci = ratio_ci(paired(c, c, "confl", [s], [seeds[0]]), rng)
            out.append("s%d %s" % (s, "n/a" if ci is None else "%.0f%%" % ci[0]))
        print("noise: %s, conflicts of each seed vs seed %d: %s" % (c, seeds[0], "  ".join(out)))

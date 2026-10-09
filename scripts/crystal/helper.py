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

# pylint: disable=invalid-name,line-too-long,too-many-locals,consider-using-sys-exit

import numpy as np
import pandas as pd
import sklearn
import sklearn.metrics
import re
import ast
import math
import time
import os.path
import sqlite3
import bisect
import functools
from ccg import *


def rowhash(a, b):
    """order by this = a fixed pseudo-random order of the (clause, reduce) rows"""
    x = (a * 0x9E3779B97F4A7C15 + b + 0x632BE59BD9B4E019) & 0xFFFFFFFFFFFFFFFF
    x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    return (x ^ (x >> 31)) >> 1


class QueryHelper:
    def __init__(self, dbfname):
        if not os.path.isfile(dbfname):
            print("ERROR: Database file '%s' does not exist" % dbfname)
            exit(-1)

        self.conn = sqlite3.connect(dbfname)
        self.conn.create_function("rowhash", 2, rowhash, deterministic=True)
        self.c = self.conn.cursor()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()


class QueryFill (QueryHelper):
    def create_indexes(self, verbose=False, used_clauses_suffix=""):
        t = time.time()
        print("Recreating indexes...")
        queries = """
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        create index `idxclidUCLS-2` on `used_clauses{suffix}` ( `clauseID`, `used_at`);
        create index `idxclidUCLS-3` on `used_clauses_anc{suffix}` ( `clauseID`, `used_at`);
        create index `idxcl_last_in_solver-1` on `cl_last_in_solver` ( `clauseID`, `conflicts`);
        """.format(suffix=used_clauses_suffix)
        for l in queries.split('\n'):
            t2 = time.time()

            if verbose:
                print("Creating index: ", l)
            self.c.execute(l)
            if verbose:
                print("Index creation T: %-3.2f s" % (time.time() - t2))

        print("indexes created T: %-3.2f s" % (time.time() - t))

    def delete_and_create_used_laters(self):
        tables = ["used_later", "used_later_anc"]
        for table in tables:
            q = """
                DROP TABLE IF EXISTS `{table}_disc`;
            """
            self.c.execute(q.format(table=table))

        # Create and fill used_later_X tables
        q_create = """
        create table `{table}_disc` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later` float,
            `percentile_fit` float DEFAULT NULL,
            `rel` float DEFAULT NULL
        );"""
        # NOTE: "percentile_fit" is the top percentile this use belongs to. Filled in later.
        # "rel": the share of the clauses at the same reduce that are used
        # less, 0..1. What reduce ranks by, and the same scale on every instance

        for table in tables:
            self.c.execute(q_create.format(table=table))

        idxs = """
        create index `{table}_disc_idx3` on `{table}_disc` (`used_later`);
        create index `{table}_disc_idx1` on `{table}_disc` (`clauseID`, `rdb0conflicts`);
        create index `{table}_disc_idx2` on `{table}_disc` (`clauseID`, `rdb0conflicts`, `used_later`);"""

        t = time.time()
        for table in tables:
            for l in idxs.format(table=table).split('\n'):
                self.c.execute(l)

        print("used_later* dropped and recreated T: %-3.2f s" % (time.time() - t))

    # Reduce time: the k-th reduce is at k, linear in the conflicts in
    # between. The reduce interval grows with the run, so a horizon in
    # conflicts is many reduces early and less than one late
    def add_reduce_time(self, horizon):
        confls = [r[0] for r in self.c.execute(
            "select distinct conflicts from reduceDB_common order by conflicts")]
        assert len(confls) >= 2, "fewer than 2 reduces"
        self.c.execute("drop table if exists reduce_time")
        self.c.execute("create table reduce_time (conflicts bigint, idx int, horizon_confl bigint)")
        self.c.execute("create index `idxrtime` on reduce_time (conflicts)")
        for i, confl in enumerate(confls):
            hor = confls[i+horizon] if i+horizon < len(confls) else None
            self.c.execute("insert into reduce_time values (?, ?, ?)", (confl, i, hor))

        def rt(x):
            i = bisect.bisect_right(confls, x) - 1
            i = min(max(i, 0), len(confls) - 2)
            return i + (x - confls[i]) / float(confls[i+1] - confls[i])
        self.conn.create_function("rt", 1, rt, deterministic=True)
        for t in ["used_clauses", "used_clauses_anc"]:
            if "used_at_rt" not in [r[1] for r in self.c.execute("pragma table_info(%s)" % t)]:
                self.c.execute("alter table %s add column used_at_rt float" % t)
                self.c.execute("update %s set used_at_rt = rt(used_at)" % t)

    # used_later_X is summed from used_clauses, used_later_anc_X from
    # used_clauses_anc (uses of the clause AND of its descendants).
    # A row counts only if the clause stayed in the solver for the horizon
    def fill_used_later(self, halflife, table="used_later"):
        used_clauses = "used_clauses" if table == "used_later" else "used_clauses_anc"

        # the use is discounted by how far away it is, halving every
        # HALFLIFE reduces, counted for two half-lives
        self.add_reduce_time(2*halflife)
        use = "sum(ucl.weight * pow(0.5, (ucl.used_at_rt - rr.idx)/%d.0))" % halflife
        horizon = "rr.horizon_confl"

        # Note: "weight" below is because a child has less weight than a parent
        #       discount is 0.5 (as per fix_up_xlrup), so a child is 0.5, a grand-child
        #       is 0.25 etc. Below 0.05 we don't care and it's a 0.
        q_fill = """
        insert into {table}_disc
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , {use} as `used_later`

        FROM
        reduceDB as rdb0
        left join reduce_time as rr on rr.conflicts = rdb0.conflicts
        join cl_last_in_solver
        on cl_last_in_solver.clauseID = rdb0.clauseID

        -- reduceDB is always present, {used_clauses} may not be, hence left join
        left join {used_clauses} as ucl
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > (rdb0.conflicts)
            and ucl.used_at <= {horizon})

        WHERE
        rdb0.clauseID != 0
        and cl_last_in_solver.conflicts >= {horizon}
        -- an eagerly subsumed clause (no glue) is deleted at the next
        -- reduce: only a locked one lives on, and those rows are not real
        and not (rdb0.glue is null and rdb0.is_ternary_resolvent = 0 and exists (
            select 1 from reduceDB as p where p.clauseID = rdb0.clauseID
            and p.conflicts < rdb0.conflicts and p.glue is null))

        group by rdb0.clauseID, rdb0.conflicts;"""

        t = time.time()
        q = q_fill.format(
            used_clauses=used_clauses, use=use,
            horizon=horizon, table=table)
        self.c.execute(q)

        q_fix_null = "update {table}_disc set used_later = 0 where used_later is NULL".format(
            table=table)
        self.c.execute(q_fix_null)

        # percent_rank: ties all get the rank of their first row, so the
        # never-used clauses of a reduce are all 0
        self.c.execute("drop table if exists ranked")
        self.c.execute("""
        create temp table ranked as
            select rowid as rid,
            percent_rank() over (partition by rdb0conflicts order by used_later) as r
            from {table}_disc""".format(table=table))
        self.c.execute("create index ranked_idx on ranked (rid)")
        self.c.execute("""
        update {table}_disc set rel = (select r from ranked where ranked.rid = {table}_disc.rowid)
        """.format(table=table))
        self.c.execute("drop table ranked")


        q_num = "select count(*) from {table}_disc".format(table=table)
        self.c.execute(q_num)
        rows = self.c.fetchall()
        for row in rows:
            num = row[0]

        if table == "used_later" and num == 0:
            print("WARNING: number of rows in {table}_disc is 0: the run is shorter than "
                  "two half-lives, there will be no frame".format(table=table))


        print("%s_disc filled T: %-3.2f s -- num rows: %d" %
              (table, time.time() - t, num))

    def fill_used_later_perc_fit(self, table):
        print("Filling percentile_fit for {table}_disc".format(table=table))

        q = """
        update {table}_disc
        set percentile_fit = (
            select max({table}_percentiles.percentile)
            from {table}_percentiles
            where
            {table}_percentiles.type_of_dat="disc"
            and {table}_percentiles.percentile_descr="top_non_zero"
            and {table}_percentiles.val >= {table}_disc.used_later);
        """
        t = time.time()
        self.c.execute(q.format(table=table))
        print("%s_disc percentile filled T: %-3.2f s" % (table, time.time() - t))



def get_features(fname):
    best_features = []
    check_file_exists(fname)
    with open(fname, "r") as f:
        for l in f:
            l = l.strip()
            if len(l) == 0:
                continue

            if l[0] == "#":
                continue

            best_features.append(l)

    return best_features


# Thousands of computed columns inserted one by one fragment the frame
# and make every access crawl: while _pending is a dict, new columns are
# collected there and concatenated to the frame in one go at the end
_pending = None

def _col(df, name):
    if _pending is not None and name in _pending:
        return _pending[name]
    return df[name]

def _set(df, name, val):
    if _pending is not None:
        # float32 as in the solver, and half the memory
        with np.errstate(over="ignore"):
            _pending[name] = val.astype(np.float32)
    else:
        df[name] = val

def _flush_pending(df):
    global _pending
    if not _pending:
        _pending = None
        return df
    df = pd.concat([df, pd.DataFrame(_pending, index=df.index)], axis=1)
    _pending = None
    return df

def add_label_options(parser):
    parser.add_option("--halflife", default=4, type=int, dest="halflife",
                      help="a use this many reduces away counts half. Default: %default")


def helper_divide(dividend, divisor, df, features, verb, name=None):
    """
    to be used like:
    import functools
    divide = functools.partial(helper.divide, df=df, features=features, verb=options.verbose)
    """
    if verb:
        print("Dividing. dividend: '%s' divisor: '%s' " % (dividend, divisor))

    if name is None:
        name = "(%s/%s)" % (dividend, divisor)

    _set(df, name, _col(df, dividend).div(_col(df, divisor)))
    return name

def helper_larger_than(lhs, rhs, df, features, verb):
    """
    to be used like:
    import functools
    larger_than = functools.partial(helper.larger_than, df=df, features=features, verb=options.verbose)
    """

    # divide
    if verb:
        print("Calulating '%s' >: '%s' " % (lhs, rhs))

    name = "(" + lhs + ">" + rhs + ")"
    _set(df, name, (_col(df, lhs) > _col(df, rhs)).astype(int))
    return name

def helper_add(toadd, df, features, verb):
    """
    to be used like:
    import functools
    larger_than = functools.partial(helper.larger_than, df=df, features=features, verb=options.verbose)
    """

    # add
    if verb:
        print("Calulating: the feature addition of: %s", toadd)

    name = "("
    for i in range(1, len(toadd)):
        name = toadd[i]
        if i < len(toadd)-1:
            name+="+"
    name += ")"

    df[name] = df[toadd[0]]
    for i in range(1, len(toadd)):
        df[name] += df[toadd[i]]
    return name


def dangerous(conn):
    conn.execute("PRAGMA journal_mode = MEMORY")
    conn.execute("PRAGMA synchronous = OFF")


def drop_idxs(conn):
    q = """
    SELECT name FROM sqlite_master WHERE type == 'index'
    """
    conn.execute(q)
    rows = conn.fetchall()
    queries = ""
    for row in rows:
        #print("Will delete index:", row[0])
        queries += "drop index if exists `%s`;\n" % row[0]

    t = time.time()
    for q in queries.split("\n"):
        conn.execute(q)

    print("Removed indexes: T: %-3.2f s"% (time.time() - t))


def get_columns(tablename, verbose, conn):
    q = "pragma table_info(%s);" % tablename
    conn.execute(q)
    rows = conn.fetchall()
    columns = []
    for row in rows:
        if verbose:
            print("Using column in table {tablename}: {col}".format(
                tablename=tablename, col=row[1]))
        columns.append(row[1])

    return columns

def query_fragment(tablename, not_cols, short_name, verbose, conn):
    cols = get_columns(tablename, verbose, conn)
    filtered_cols = list(set(cols).difference(not_cols))
    ret = ""
    for col in filtered_cols:
        ret += ", {short_name}.`{col}` as `{short_name}.{col}`\n".format(
            col=col, short_name=short_name)

    if verbose:
        print("query for short name {short_name}: {ret}".format(
            short_name=short_name, ret=ret))

    return ret


# to check for too large or NaN values:
def check_too_large_or_nan_values(df, features=None):
    print("Checking for too large or NaN values...")
    if features is None:
        features = df.columns.values.flatten().tolist()

    index = 0
    for index, row in df[features].iterrows():
        print("-------------")
        print("At row index: ", index)
        for val, name in zip(row, features):
            print("Name: '%s', val: %s" % (name, val))
            if type(val) == str:
                continue

            if math.isnan(val) or not np.isfinite(val) or val > np.finfo(np.float32).max:
                print("issue with feature '%s' Value: '%s'  Type: '%s" % (
                    name, val, type(val)))
        index += 1

    print("Checking finished.")


def calc_min_split_point(df, min_samples_split):
    split_point = int(float(df.shape[0])*min_samples_split)
    if split_point < 10:
        split_point = 10
    print("Minimum split point: ", split_point)
    return split_point


def error_format(error):
    if error is None:
        return "XXX"
    else:
        return "{0:<2.2E}".format(error)


def calc_regression_error(data, features, to_predict, clf, toprint,
                  average="binary", highlight=False):
    X_data = data[features]
    y_data = data[to_predict]
    print("Number of elements:", X_data.shape)
    if data.shape[0] <= 1:
        print("Cannot calculate regression error, too few elements")
        return None
    y_pred = clf.predict(X_data)
    main_error = sklearn.metrics.mean_squared_error(y_data, y_pred)
    print("Mean squared error is: %9s" % error_format(main_error))
    median_absolute_error = sklearn.metrics.median_absolute_error(y_data, y_pred)
    print("Median abs error is  : %9s" % error_format(median_absolute_error))

    # use distrib
    for start,end in [(0,10), (1,10), (10, 100), (100, 1000), (1000,10000), (10000, 1000000)]:
        x = "--> Strata  %6d <= %21s < %8d " % (start, to_predict, end)
        myfilt = data[(data[to_predict] >= start) & (data[to_predict] < end)]
        X_data = myfilt[features]
        y_data = myfilt[to_predict]
        y = " -- elems: {:12}".format(str(X_data.shape))
        if myfilt.shape[0] <= 1:
            msqe = None
            med_abs_err = None
            mean_err = None
        else:
            y_pred = clf.predict(X_data)
            msqe = sklearn.metrics.mean_squared_error(y_data, y_pred)
            med_abs_err = sklearn.metrics.median_absolute_error(y_data, y_pred)
            mean_err = (y_data - y_pred).sum()/len(y_data)
        print("{} {}   msqe: {:9s}   mabse: {:9s} abs: {:9s}".format(
            x, y,  error_format(msqe), error_format(med_abs_err), error_format(mean_err)))

    # glue distrib
    for start,end in [(0,3), (3,8), (8, 15), (15, 25), (25,50), (50, 100), (100, 1000000)]:
        x = "--> Strata  %6d <= %21s < %8d " % (start, "rdb0.glue", end)
        myfilt = data[(data["rdb0.glue"] >= start) & (data["rdb0.glue"] < end)]
        X_data = myfilt[features]
        y_data = myfilt[to_predict]
        y = " -- elems: {:12}".format(str(X_data.shape))
        if myfilt.shape[0] <= 1:
            msqe = None
            med_abs_err = None
            mean_err = None
        else:
            y_pred = clf.predict(X_data)
            msqe = sklearn.metrics.mean_squared_error(y_data, y_pred)
            med_abs_err = sklearn.metrics.median_absolute_error(y_data, y_pred)
            mean_err = (y_data - y_pred).sum()/len(y_data)
        print("{} {}   msqe: {:9s}   mabse: {:9s} abs: {:9s}".format(
            x, y,  error_format(msqe), error_format(med_abs_err), error_format(mean_err)))

    return main_error


def family(fname, families=None):
    """the family of a CNF: from the 'family filename' lines of the file
    in $FAMILIES if given, else the leading letters of the name"""
    base = os.path.basename(str(fname))
    if families is None:
        families = read_families()
    if base in families:
        return families[base]
    m = re.match(r"([A-Za-z]+)", base)
    if m:
        return m.group(1).lower()
    return "num%d" % len(re.match(r"(\d*)", base).group(1))


@functools.lru_cache(maxsize=None)
def read_families():
    ret = {}
    if os.environ.get("FAMILIES"):
        with open(os.environ["FAMILIES"]) as f:
            for l in f:
                l = l.split()
                if len(l) >= 2:
                    ret[os.path.basename(l[1])] = l[0]
    return ret


def sample_weights(df, how):
    """none: 1; strata: undo the sampling (x.weight = rows the row stands
    for); instance: also every instance counts the same; family: also
    every family counts the same. Mean 1."""
    w = pd.Series(1.0, index=df.index)
    if how == "none":
        return w
    w = df["x.weight"].astype(float)
    if how in ["instance", "family"]:
        fname = df["fname"].astype(str)
        w = w / w.groupby(fname).transform("sum")
        if how == "family":
            fam = fname.map(family)
            w = w / fname.groupby(fam).transform("nunique")
    return w / w.mean()


# the solver's reduce: ReduceDB::mark_useless_redundant_clauses_as_garbage
TIER1_GLUE = 2    # conf.reducetier1glue
MAX_USED = 31     # CL_MAX_USED: 'used' of a clause learnt or used since the last reduce
ROUNDS = 1        # conf.reduce_rounds of the predictor builds: reduces it is kept for after that
REMOVE = 0.75     # conf.reducetarget, the share of the candidates removed


def best_rows(score, n, glue, size):
    """the n best rows: highest score, then the glue order"""
    keep = np.zeros(len(score), dtype=bool)
    keep[np.lexsort((size, glue, -score))[:n]] = True
    return keep


def rule_then_best(rule, score, glue, size, remove=REMOVE):
    """what a reduce keeps: the rows of the rule, the best of the rest"""
    cands = np.flatnonzero(~rule)
    n_keep = len(cands) - int(remove * len(cands))
    keep = rule.copy()
    keep[cands[best_rows(score[cands], n_keep, glue[cands], size[cands])]] = True
    return keep


def share_needed(score, truth, glue, size, use):
    """the share of the rows that has to be kept, best score first, to
    hold 'use' of the truth"""
    if use <= 0:
        return 0.0
    got = np.cumsum(truth[np.lexsort((size, glue, -score))])
    return min(1.0, (np.searchsorted(got, use - 1e-9) + 1) / len(score))


def kept_by_rule(glue, used, rounds=ROUNDS, tier1=TIER1_GLUE):
    """the rows a reduce does not look at: tier1 with 'used' life left,
    and learnt or used in the last 'rounds' reduce intervals"""
    return ((glue <= tier1) & (used > 0)) | (used > MAX_USED - rounds)


def policies_of_reduce(g, pred, truth, rounds=ROUNDS, remove=REMOVE, tier1=TIER1_GLUE):
    """policy -> the rows it keeps at one reduce, and the rows kept by rule"""
    glue = g["rdb0.glue"].to_numpy(dtype=float)
    size = g["rdb0.size"].to_numpy(dtype=float)
    rule = kept_by_rule(glue, g["rdb0.used"].to_numpy(dtype=float), rounds, tier1)
    ret = {"normal": rule_then_best(rule, -glue, glue, size, remove)}
    ret["order"] = rule_then_best(rule, pred, glue, size, remove)
    ret["oracle"] = rule_then_best(rule, truth, glue, size, remove)
    return ret, rule


def needed_of_reduce(g, pred, truth, rule, use):
    """order -> the share of all the rows it has to keep, on top of the
    rule's, to hold 'use' of the truth"""
    glue = g["rdb0.glue"].to_numpy(dtype=float)
    size = g["rdb0.size"].to_numpy(dtype=float)
    c = ~rule
    if not c.any():
        return {k: 100.0 for k in ("glue", "order", "oracle")}
    need = use - truth[rule].sum()
    return {k: 100.0 * (rule.sum() + share_needed(sc[c], truth[c], glue[c], size[c], need) * c.sum()) / len(truth)
            for k, sc in (("glue", -glue), ("order", pred), ("oracle", truth))}


def best_split(df, pred, label, min_rows=50, rounds=ROUNDS, remove=REMOVE, tier1=TIER1_GLUE, steps=20):
    """What a per-reduce count could win: the candidates in the model's
    order, and the budget of a run (the share 1-'remove' of the candidates
    of every sampled reduce) split over its reduces with hindsight, greedy
    over blocks of 1/steps of a reduce's candidates. Returns the use kept
    (as policy_per_reduce) by the same share everywhere and by that
    split, mean over the instances."""
    df = df.reset_index(drop=True)
    pred = np.asarray(pred, dtype=float)
    res = []
    for _, gi in df.groupby(df["fname"].astype(str)):
        curves = []
        ruleuse = 0.0
        for _, g in gi.groupby("rdb0_common.conflicts"):
            g = g[(g["rdb0.is_ternary_resolvent"] == 0) & g["rdb0.glue"].notna()]
            truth = g[label].to_numpy(dtype=float)
            if len(g) < min_rows or truth.sum() <= 0:
                continue
            glue = g["rdb0.glue"].to_numpy(dtype=float)
            size = g["rdb0.size"].to_numpy(dtype=float)
            cand = ~kept_by_rule(glue, g["rdb0.used"].to_numpy(dtype=float), rounds, tier1)
            w = truth / truth.sum()
            order = np.lexsort((size[cand], glue[cand], -pred[g.index.to_numpy()][cand]))
            cum = np.concatenate([[0.0], np.cumsum(w[cand][order])])
            n = int(cand.sum())
            curves.append((n, [cum[int(round(n * k / float(steps)))] for k in range(steps + 1)],
                           cum[n - int(remove * n)]))
            ruleuse += w[~cand].sum()
        if len(curves) < 3:
            continue
        budget = sum(n - int(remove * n) for n, _, _ in curves)
        at = [0] * len(curves)
        spent = 0.0
        got = 0.0
        while True:
            best = None
            for i, (n, cv, _) in enumerate(curves):
                for j in range(at[i] + 1, steps + 1):
                    cost = n * (j - at[i]) / float(steps)
                    if cost <= 0 or spent + cost > budget + 1e-9:
                        continue
                    gain = (cv[j] - cv[at[i]]) / cost
                    if best is None or gain > best[0]:
                        best = (gain, i, j, cost)
            if best is None:
                break
            _, i, j, cost = best
            got += curves[i][1][j] - curves[i][1][at[i]]
            spent += cost
            at[i] = j
        res.append((100.0 * (ruleuse + sum(f for _, _, f in curves)) / len(curves),
                    100.0 * (ruleuse + got) / len(curves)))
    if not res:
        return None
    return tuple(np.mean(np.array(res), axis=0))


def candidate_quarters(df, pred, label, min_cands=40, rounds=ROUNDS, tier1=TIER1_GLUE):
    """Does the score mean what it seems to? The candidates of a reduce in
    quarters, best first, by the model and by glue, size: the share of the
    quarter's clauses that are used later and the share of the candidates'
    future use it holds. Mean over the reduces of an instance, then over
    the instances. The reduce keeps the first quarter."""
    df = df.reset_index(drop=True)
    pred = np.asarray(pred, dtype=float)
    per_inst = []
    for _, gi in df.groupby(df["fname"].astype(str)):
        acc = []
        for _, g in gi.groupby("rdb0_common.conflicts"):
            g = g[(g["rdb0.is_ternary_resolvent"] == 0) & g["rdb0.glue"].notna()]
            glue = g["rdb0.glue"].to_numpy(dtype=float)
            size = g["rdb0.size"].to_numpy(dtype=float)
            cand = ~kept_by_rule(glue, g["rdb0.used"].to_numpy(dtype=float), rounds, tier1)
            truth = g[label].to_numpy(dtype=float)[cand]
            if cand.sum() < min_cands or truth.sum() <= 0:
                continue
            p = pred[g.index.to_numpy()][cand]
            r = {}
            for name, order in (("model", np.lexsort((size[cand], glue[cand], -p))),
                                ("glue", np.lexsort((size[cand], glue[cand])))):
                for q, part in enumerate(np.array_split(truth[order], 4)):
                    r["%s used %d" % (name, q + 1)] = 100.0 * (part > 0).mean()
                    r["%s use %d" % (name, q + 1)] = 100.0 * part.sum() / truth.sum()
            acc.append(r)
        if acc:
            per_inst.append(pd.DataFrame(acc).mean())
    if not per_inst:
        return None
    m = pd.DataFrame(per_inst).mean()
    return pd.DataFrame({"%s %s" % (name, what): [m["%s %s %d" % (name, what, q)] for q in range(1, 5)]
                         for name in ("model", "glue") for what in ("used", "use")},
                        index=["quarter %d" % q for q in range(1, 5)])


def policy_per_reduce(df, pred, label, min_rows=50, rounds=ROUNDS, remove=REMOVE, tier1=TIER1_GLUE):
    """The solver's reduce replayed on a fair sample of the clauses at
    some reduces. By rule it keeps the tier1 clauses with 'used' life
    left and the ones learnt or used in the last 'rounds' reduce
    intervals; of the rest (the candidates) the share 'remove' goes.
    The share of the future use (label) that is kept by
      normal  the normal build: the best candidates by glue, size
      order   the predictor build: the candidates by the model
      oracle  the candidates by their future use: the most an order of
              the candidates can keep
    and for the use that the normal order holds at the default knobs,
    the share of the clauses that has to be kept when the candidates go by
      glue needs, order needs, oracle needs
    Mean over the reduces of an instance. 'rule cls' / 'rule use': the
    share of the clauses the rule keeps, and of the use they hold; 'cls':
    the share of the clauses kept."""
    df = df.reset_index(drop=True)
    pred = np.asarray(pred, dtype=float)
    rows = []
    for fname, gi in df.groupby(df["fname"].astype(str)):
        acc = []
        for _, g in gi.groupby("rdb0_common.conflicts"):
            g = g[(g["rdb0.is_ternary_resolvent"] == 0) & g["rdb0.glue"].notna()]
            truth = g[label].to_numpy(dtype=float)
            if len(g) < min_rows or truth.sum() <= 0:
                continue
            p = pred[g.index.to_numpy()]
            kept, rule = policies_of_reduce(g, p, truth, rounds, remove, tier1)
            r = {"rule cls": 100.0 * rule.mean(), "rule use": 100.0 * truth[rule].sum() / truth.sum(),
                 "cls": 100.0 * kept["normal"].mean()}
            for k, v in kept.items():
                r[k] = 100.0 * truth[v].sum() / truth.sum()
            solver = policies_of_reduce(g, p, truth)[0]["normal"]
            for k, v in needed_of_reduce(g, p, truth, rule, truth[solver].sum()).items():
                r[k + " needs"] = v
            acc.append(r)
        if acc:
            r = pd.DataFrame(acc).mean().to_dict()
            r["instance"] = os.path.basename(fname)[:34]
            r["reduces"] = len(acc)
            rows.append(r)
    if not rows:
        return None
    res = pd.DataFrame(rows)
    return res[["instance", "reduces"] + [c for c in res.columns if c not in ("instance", "reduces")]]


def by_reduce(df):
    """the row order that puts the rows of a reduce together, and the
    reduce of every row in that order: what a ranking objective needs"""
    key = (df["fname"].astype(str) + "/" + df["rdb0_common.conflicts"].astype(str)).to_numpy()
    order = np.argsort(key, kind="stable")
    return order, pd.factorize(key[order])[0]


def fit(clf, df, X, y, weights=None):
    """fits clf on the rows of df. A ranker (objective rank) learns the
    order within each reduce: rows grouped by reduce, y = used at all, no
    row weights (xgboost takes one per group only)"""
    import xgboost
    if not isinstance(clf, xgboost.XGBRanker):
        clf.fit(X, y, sample_weight=weights)
        return clf
    order, qid = by_reduce(df)
    clf.fit(X.iloc[order], (np.asarray(y, dtype=float)[order] > 0).astype(int), qid=qid)
    return clf


def check_file_exists(fname):
    try:
        f = open(fname)
    except IOError:
        print("File '%s' not accessible" % fname)
        exit(-1)
    finally:
        f.close()


def output_to_classical_dot(clf, features, fname):

    feat_tmp = []
    for f in features:
        x = str(f)
        x = x.replace("rdb0.", "")
        x = x.replace("cl.", "")
        x = x.replace("HistLT.", "History_Long_Term")
        x = x.replace("rdb0_common.", "all_learnts")
        feat_tmp.append(x)

    sklearn.tree.export_graphviz(clf, out_file=fname,
                                 feature_names=feat_tmp,
                                 #class_names=clf.classes_,
                                 filled=True, rounded=True,
                                 special_characters=True,
                                 proportion=True)
    print("Run dot:")
    print("dot -Tpng {fname} -o {fname}.png".format(fname=fname))
    print("gwenview {fname}.png".format(fname=fname))


def add_features_from_fname(df, features_fname, verbose=False):
    print("Adding features...")
    if not os.path.exists(features_fname):
        print("ERROR: Feature file '%s' does not exist" % features_fname)
        exit(-1)

    cldata_add_minimum_computed_features(df, verbose)
    best_features = get_features(features_fname)
    for feat in best_features:
        toeval = ccg.to_source(ast.parse(feat))
        print("Adding feature %s as eval %s" % (feat, toeval))
        df[feat] = eval(toeval)


def add_features_from_list(df, best_features, verbose=False):
    print("Adding features...")
    cldata_add_minimum_computed_features(df, verbose)
    for feat in best_features:
        toeval = ccg.to_source(ast.parse(feat))
        print("Adding feature %s as eval %s" % (feat, toeval))
        df[feat] = eval(toeval)


def make_missing_into_nan(df):
    print("Making None into NaN...")
    def make_none_into_nan(x):
        if x is None:
            return np.nan
        else:
            return x

    for col in list(df):
        if type(None) in df[col].apply(type).unique():
            df[col] = df[col].apply(make_none_into_nan)
    print("Done.")

def cldata_add_minimum_computed_features(df, verbose):
    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    divide("rdb0.act_ranking", "rdb0_common.tot_cls_in_db", name="rdb0.act_ranking_rel")
    divide("rdb0.prop_ranking", "rdb0_common.tot_cls_in_db", name="rdb0.prop_ranking_rel")
    divide("rdb0.uip1_ranking", "rdb0_common.tot_cls_in_db", name="rdb0.uip1_ranking_rel")
    divide("rdb0.sum_uip1_per_time_ranking", "rdb0_common.tot_cls_in_db",
           name="rdb0.sum_uip1_per_time_ranking_rel")
    divide("rdb0.sum_props_per_time_ranking", "rdb0_common.tot_cls_in_db",
           name="rdb0.sum_props_per_time_ranking_rel")

    df["rdb0_common.tot_irred_cls"] = df["rdb0_common.num_bin_irred_cls"] + df["rdb0_common.num_long_irred_cls"]
    divide("rdb0_common.tot_irred_cls", "rdb0_common.num_vars")
    divide("rdb0_common.num_long_irred_cls", "rdb0_common.num_long_irred_cls_lits")
    divide("rdb0_common.num_long_irred_cls_lits", "rdb0_common.num_vars")
    divide("rdb0_common.num_long_irred_cls", "rdb0_common.num_vars")


# the per-reduce aggregates a clause's own numbers are compared against
RATIO_COMMON = [
    "rdb0_common.avg_props", "rdb0_common.avg_uip1_used",
    "rdb0_common.avg_sum_uip1_per_time", "rdb0_common.avg_sum_props_per_time",
    "rdb0_common.median_act", "rdb0_common.median_uip1_used", "rdb0_common.median_props",
    "rdb0_common.median_sum_uip1_per_time", "rdb0_common.median_sum_props_per_time",
    "rdb0_common.num_vars", "rdb0_common.num_long_irred_cls", "rdb0_common.num_long_irred_cls_lits",
    "rdb0_common.num_long_red_cls", "rdb0_common.num_long_red_cls_lits",
    "rdb0_common.trailDepthHistLT_avg", "rdb0_common.backtrackLevelHistLT_avg",
    "rdb0_common.conflSizeHistLT_avg", "rdb0_common.numResolutionsHistLT_avg",
    "rdb0_common.glueHistLT_avg", "rdb0_common.antec_data_sum_sizeHistLT_avg",
    "rdb0_common.overlapHistLT_avg",
]
# clause columns that are IDs, flags or ranks: no ratios of those
RATIO_SKIP = ("clauseID", "dump_no", "is_", "_ranking", "introduced_at", "restartID",
              "conflicts", "cur_restart_type", "is_decision")


def cldata_add_ratio_features(df, verbose):
    """The clause against its reduce: every clause-level column over
    every DB-wide aggregate, the learning-time history over the same
    history now, and benefit over cost. ~800 columns instead of
    all_computed's 4700 pairs, and all of them mean the same thing on
    every instance. Returns the new frame"""
    global _pending
    print("Adding ratio features...")
    cldata_add_minimum_computed_features(df, verbose)
    _pending = {}
    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    cols = list(df)
    clause_cols = [c for c in cols if (c.startswith("rdb0.") or c.startswith("cl."))
                   and not any(s in c for s in RATIO_SKIP)
                   and pd.api.types.is_numeric_dtype(df[c])]
    common = [c for c in RATIO_COMMON if c in cols]
    for a in clause_cols:
        for b in common:
            divide(a, b)
    # the history when the clause was learnt against the history now
    for c in cols:
        if c.startswith("cl.") and c.endswith("HistLT_avg") and ("rdb0_common." + c[3:]) in cols:
            divide(c, "rdb0_common." + c[3:])
    # benefit over cost, and cost per literal
    if "rdb0.visited" in cols:
        for a in ["rdb0.props_made", "rdb0.uip1_used"]:
            divide(a, "rdb0.visited")
        for a in ["rdb0.sum_props_made", "rdb0.sum_uip1_used"]:
            divide(a, "rdb0.sum_visited")
        for a in ["rdb0.discounted_props_made", "rdb0.discounted_uip1_used"]:
            divide(a, "rdb0.discounted_visited")
        divide("rdb0.visited", "rdb0.size")
        divide("rdb0.visited", "cl.time_inside_solver")
    # trend: the fast discount over the slow one
    for base in ["discounted_uip1_used", "discounted_props_made"]:
        divide("rdb0.%s2" % base, "rdb0.%s" % base)
        divide("rdb0.%s" % base, "rdb0.%s3" % base)
    df = _flush_pending(df)
    print("Ratio features added, now %d columns" % df.shape[1])
    return df


def cldata_add_computed_features(df, verbose):
    """returns the new frame"""
    global _pending
    print("Adding computed features...")
    cldata_add_minimum_computed_features(df, verbose)
    _pending = {}

    del df["cl.conflicts"]
    del df["cl.restartID"]
    del df["rdb0.introduced_at_conflict"]

    divide = functools.partial(helper_divide, df=df, features=list(df), verb=verbose)
    larger_than = functools.partial(helper_larger_than, df=df, features=list(df), verb=verbose)
    add = functools.partial(helper_add, df=df, features=list(df), verb=verbose)

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    print("size/glue/trail rel...")
    divide("cl.trail_depth_level", "cl.trailDepthHistLT_avg")
    divide("cl.trail_depth_level", "cl.trailDepthHist_avg")

    divide("cl.num_total_lits_antecedents", "cl.num_antecedents")

    del df["rdb0.uip1_ranking"]
    del df["rdb0.prop_ranking"]
    del df["rdb0.act_ranking"]
    del df["rdb0.sum_uip1_per_time_ranking"]
    del df["rdb0.sum_props_per_time_ranking"]
    del df["rdb0_common.tot_cls_in_db"]

    # divide by avg and median
    divide("rdb0.uip1_used", "rdb0_common.avg_uip1_used")
    divide("rdb0.props_made", "rdb0_common.avg_props")

    divide("rdb0.uip1_used", "rdb0_common.median_uip1_used")
    divide("rdb0.props_made", "rdb0_common.median_props")

    time_in_solver = "cl.time_inside_solver"
    sum_props_per_time = divide("rdb0.sum_props_made", time_in_solver)
    sum_uip1_per_time = divide("rdb0.sum_uip1_used", time_in_solver)
    divide(sum_props_per_time, "rdb0_common.median_sum_uip1_per_time")
    divide(sum_uip1_per_time, "rdb0_common.median_sum_props_per_time")
    divide(sum_props_per_time, "rdb0_common.avg_sum_uip1_per_time")
    divide(sum_uip1_per_time, "rdb0_common.avg_sum_props_per_time")
    #del df[time_in_solver]

    divisors = [
        "cl.conflSizeHistLT_avg"
        , "cl.glueHistLT_avg"
        , "rdb0.glue"
        , "rdb0.size"
        # , "cl.orig_connects_num_communities"
        # , "rdb0.connects_num_communities"
        , "cl.orig_glue"
        , "cl.glue_before_minim"
        , "cl.glueHist_avg"
        , "cl.glueHist_longterm_avg"
        # , "cl.decision_level_hist"
        , "cl.numResolutionsHistLT_avg"
        , "cl.trailDepthHistLT_avg"
        , "cl.trailDepthHist_avg"
        , "cl.branchDepthHistQueue_avg"
        , "cl.overlapHistLT_avg"
        , "(cl.num_total_lits_antecedents/cl.num_antecedents)"
        , "cl.num_antecedents"
        , "rdb0.act_ranking_rel"
        , "rdb0.prop_ranking_rel"
        , "rdb0.uip1_ranking_rel"
        , "rdb0.sum_uip1_per_time_ranking_rel"
        , "rdb0.sum_props_per_time_ranking_rel"
        , "cl.time_inside_solver"
        # , "cl.num_overlap_literals"
        ]

    # discounted stuff
    divide("rdb0.discounted_uip1_used", "rdb0_common.avg_uip1_used")
    divide("rdb0.discounted_props_made", "rdb0_common.avg_props")
    divide("rdb0.discounted_uip1_used", "rdb0_common.median_uip1_used")
    divide("rdb0.discounted_props_made", "rdb0_common.median_props")
    #==
    divide("rdb0.discounted_uip1_used2", "rdb0_common.avg_uip1_used")
    divide("rdb0.discounted_props_made2", "rdb0_common.avg_props")
    divide("rdb0.discounted_uip1_used2", "rdb0_common.median_uip1_used")
    divide("rdb0.discounted_props_made2", "rdb0_common.median_props")

    sum_uip1_per_time = divide("rdb0.sum_uip1_used", "cl.time_inside_solver")
    sum_props_per_time = divide("rdb0.sum_props_made", "cl.time_inside_solver")
    antec_rel = divide("cl.num_total_lits_antecedents", "cl.antec_data_sum_sizeHistLT_avg")
    divisors.append(sum_uip1_per_time)
    divisors.append(sum_props_per_time)
    divisors.append("rdb0.discounted_uip1_used")
    divisors.append("rdb0.discounted_props_made")
    divisors.append(antec_rel)

    orig_cols = list(df)

    # Thanks to Chai Kian Ming Adam for the idea of using LOG instead of SQRT
    # add LOG
    if False:
        toadd = []
        for divisor in divisors:
            x = "log2("+divisor+")"
            df[x] = df[divisor].apply(np.log2)
            toadd.append(x)
        divisors.extend(toadd)

    # relative data
    cols = list(df) + list(_pending.keys())
    for col in cols:
        if ("rdb" in col or "cl." in col) and "restart_type" not in col and "tot_cls_in" not in col:
            for divisor in divisors:
                divide(divisor, col)
                divide(col, divisor)

    # smaller/larger than
    print("smaller-or-greater comparisons...")
    if False:
        for col in cols:
            if "avg" in col or "median" in col:
                for divisor in divisors:
                    larger_than(col, divisor)

    # smaller-or-greater comparisons
    #if not short:
        #larger_than("cl.antec_data_sum_sizeHistLT_avg", "cl.num_total_lits_antecedents")
        #larger_than("cl.overlapHistLT_avg", "cl.num_overlap_literals")

    # print("flatten/list...")
    #old = set(df.columns.values.flatten().tolist())
    #df = df.dropna(how="all")
    #new = set(df.columns.values.flatten().tolist())
    #if len(old - new) > 0:
        #print("ERROR: a NaN number turned up")
        #print("columns: ", (old - new))
        #assert(False)
        #exit(-1)

    df = _flush_pending(df)
    print("Computed features added, now %d columns" % df.shape[1])
    return df

def print_datatypes(df):
    pd.set_option('display.max_rows', len(df.dtypes))
    print(df.dtypes)
    pd.reset_option('display.max_rows')

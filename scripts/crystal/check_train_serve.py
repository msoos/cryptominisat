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

# Does the solver predict from what the model was trained on? One run of
# the stats+predictor build writes both the SQLite data (which becomes
# the frames) and, with --preddump, what its predictor was fed. For every
# frame row this finds the same clause at the same reduce in the dump and
# compares the model's features, computed from the frame the way training
# does, with the ones the solver computed.
#
# usage: check_train_serve.py dump frame.dat [frame.dat ...] [-f best_features.txt]

import argparse
import os
import pickle
import sys

import numpy as np

import gen_pred_features
import holdout_eval


def read_dump(fname):
    """(conflicts, tracked ID) -> row of features"""
    ret = {}
    with open(fname, "rb") as f:
        num_raw, num_feat = np.fromfile(f, dtype=np.uint32, count=2)
        while True:
            head = np.fromfile(f, dtype=np.uint32, count=1)
            if len(head) == 0:
                break
            num = int(head[0])
            confl = int(np.fromfile(f, dtype=np.uint64, count=1)[0])
            ids = np.fromfile(f, dtype=np.int64, count=num)
            f.seek(num * int(num_raw) * 8, os.SEEK_CUR)
            feats = np.fromfile(f, dtype=np.float32, count=num * int(num_feat)).reshape(num, int(num_feat))
            f.seek(num * 8, os.SEEK_CUR)
            for i in np.flatnonzero(ids):
                ret[(confl, int(ids[i]))] = feats[i]
    return ret


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("dump")
    parser.add_argument("frames", nargs="+")
    parser.add_argument("-f", "--features", default=os.path.join(here, "best_features.txt"))
    parser.add_argument("--rtol", type=float, default=1e-4)
    opts = parser.parse_args()

    feats = gen_pred_features.read_features(opts.features)
    dump = read_dump(opts.dump)
    if not dump:
        sys.exit("ERROR: no tracked clause in the dump: not a stats+predictor build's?")
    failed = 0
    for fname in opts.frames:
        with open(fname, "rb") as f:
            df = pickle.load(f)
        df, X = holdout_eval.prepare(df, feats)
        want = X.to_numpy(dtype=np.float32)
        keys = list(zip(df["rdb0_common.conflicts"].astype(np.int64), df["sum_cl_use.clauseID"].astype(np.int64)))
        found = 0
        bad = np.zeros(len(feats), dtype=np.int64)
        example = {}
        for r, key in enumerate(keys):
            got = dump.get(key)
            if got is None:
                continue
            found += 1
            gm, wm = np.isnan(got), np.isnan(want[r])
            with np.errstate(invalid="ignore"):
                far = np.abs(got - want[r]) > opts.rtol * np.maximum(np.abs(want[r]), 1e-6)
            diff = (gm != wm) | (~gm & ~wm & far)
            bad += diff
            for c in np.flatnonzero(diff):
                example.setdefault(c, (key, float(got[c]), float(want[r][c])))
        base = os.path.basename(fname)
        # the solver predicts for the candidates only
        if found < 100:
            print("FAIL: %s: only %d of %d rows are in the dump" % (base, found, len(keys)))
            failed += 1
        for c in np.flatnonzero(bad):
            key, g, w = example[c]
            print("FAIL: %s: '%s' differs in %d of %d rows, e.g. clause %d at %d: solver %r, frame %r" % (
                base, feats[c], bad[c], found, key[1], key[0], g, w))
            failed += 1
        print("%s: %d rows found in the dump, %d of %d features agree" % (base, found, len(feats) - int((bad > 0).sum()), len(feats)))
    print("%d failed" % failed)
    sys.exit(1 if failed else 0)

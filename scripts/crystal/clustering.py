#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2018  Mate Soos
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

import operator
import re
import pandas as pd
import pickle
import sklearn
import sklearn.svm
import sklearn.tree
import sklearn.cluster
from sklearn.preprocessing import StandardScaler
import argparse
import sys
import numpy as np
import sklearn.metrics
import time
import itertools
import math
import matplotlib.pyplot as plt
import sklearn.ensemble
import os
import helper
import decimal
try:
    from termcolor import cprint
except ImportError:
    termcolor_avail = False
else:
    termcolor_avail = True

ver = sklearn.__version__.split(".")
if int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split


CLUST_TYPE_FILES = 0
CLUST_TYPE_USEFULNESS = 1

def get_cluster_name(clust_type):
    name = None
    if clust_type == CLUST_TYPE_FILES:
        name = "clust_f"
    elif clust_type == CLUST_TYPE_USEFULNESS:
        name = "clust_u"
    else:
        assert False

    return name

def get_clustering_label(df, feats_used: list, scaler, clustering):
    this_feats = list(df)
    for f in feats_used:
        if f not in this_feats:
            print("ERROR: Feature '%s' is in the training file but not in the current one!" % f)
            exit(-1)

    df_clust = df[feats_used].astype(float).copy()
    df_clust = scaler.transform(df_clust)
    return clustering.predict(df_clust)


def dump_dataframe(df, fname: str):
    with open(fname, "wb") as f:
        pickle.dump(df, f)
    print("Dumped to file %s" % fname)


class Clustering:
    def __init__(self, df, clust_type):
        self.df = df
        self.clust_type = clust_type

    def reformat_feats(self, feats):
        feats_clean = []
        for feat in feats:
            assert "szfeat_cur.conflicts" not in feat
            c = str(feat)
            c = c.replace(".irred_", ".irred_cl_distrib.")
            c = c.replace(".red_", ".red_cl_distrib.")
            c = c.replace("szfeat_cur.", "{val}.")

            feats_clean.append(c)
        assert len(feats_clean) == len(feats)

        return feats_clean

    def create_code_for_cluster_centers(self, clust, scaler, sz_feats):
        sz_feats_clean = self.reformat_feats(sz_feats)
        f = open("{basedir}/clustering_imp.cpp".format(basedir=options.basedir), 'w')

        helper.write_mit_header(f)
        f.write("""
#include "satzilla_features.h"
#include "clustering.h"
#include <cmath>

using namespace CMSat;

ClusteringImp::ClusteringImp() {{
    set_up_centers();
}}

ClusteringImp::~ClusteringImp() {{
}}

""".format(clusters=options.clusters))

        f.write("void ClusteringImp::set_up_centers() {\n")
        f.write("\n        centers.resize(%d);\n" % options.clusters)
        for i in range(options.clusters):
            f.write("\n        // Doing cluster center %d\n" % i)
            f.write("\n        centers[%d].resize(%d);\n" % (i, len(sz_feats_clean)) )
            f.write("\n        used_clusters.push_back(%d);\n" % i)
            for i2 in range(len(sz_feats_clean)):
                feat = sz_feats_clean[i2]
                center = clust.cluster_centers_[i][i2]
                f.write("        centers[{num}][{feat}] = {center}L;\n".format(
                    num=i, feat=i2, center=center))

        f.write("    }\n")

        f.write("""
double sq(double x) {
    return x*x;
}

double ClusteringImp::norm_dist(const SatZillaFeatures& a, const std::vector<double>& center) const {
    double dist = 0;
    double tmp;
""")
        for feat, i in zip(sz_feats_clean, range(10000)):
            printed_scale = "{scale:3.10f}".format(scale=scaler.scale_[i])
            if decimal.Decimal(printed_scale) == decimal.Decimal("0.0") :
                    f.write("         // feature {feat} would have caused a division by zero, avoiding\n".\
                        format(feat=feat.format(val="a")))
                    continue

            f.write("        tmp = ((double){feat}-{mean:3.9f})/{printed_scale};\n".format(
                feat=feat.format(val="a"),
                 mean=scaler.mean_[i],
                 printed_scale=printed_scale
                 ))

            f.write("    dist+=sq(tmp-center[{feat}]);\n\n".format(
                feat=i
                ))

        f.write("""
    return dist;
}\n""")

        f.write("""
int ClusteringImp::which_is_closest(const SatZillaFeatures& p) const {
    double closest_dist = std::numeric_limits<double>::max();
    int closest = -1;
    for (int i: used_clusters) {
        double dist = norm_dist(p, centers[i]);
        if (dist < closest_dist) {
            closest_dist = dist;
            closest = i;
        }
    }
    return closest;
    }\n""")

    def select_features_files(self):
        features = list(self.df)
        for f in list(features):
            if any(ext in f for ext in ["var", "vcg", "pnr", "min", "max", "std"]):
                features.remove(f)

        # features from dataframe
        feats_used = []
        for feat in features:
            if "szfeat_cur" in feat and "szfeat_cur.conflicts" not in feat:
                feats_used.append(feat)

        return feats_used

    def select_features_usefulness(self):
        features = list(self.df)
        feats_used = helper.get_features(options.best_features_fname)

        return feats_used

    def cluster(self):
        if self.clust_type == CLUST_TYPE_FILES:
            self.feats_used = self.select_features_files()
        elif self.clust_type == CLUST_TYPE_USEFULNESS:
            self.feats_used = self.select_features_usefulness()
        else:
            assert False

        print("Features used: ", self.feats_used)
        print("Number of features used: ", len(self.feats_used))

        if options.check_row_data:
            helper.check_too_large_or_nan_values(self.df, self.feats_used)
        else:
            helper.check_too_large_or_nan_values(self.df.sample(100), self.feats_used)

        # fit to slice that only includes CNF features
        df_clust = self.df[self.feats_used].astype(float).copy()
        if options.scale:
            self.scaler = StandardScaler()
            self.scaler.fit(df_clust)
            if options.verbose:
                print("Scaler:")
                print(" -- ", self.scaler.mean_)
                print(" -- ", self.scaler.scale_)

            if options.verbose:
                df_clust_back = df_clust.copy()
            df_clust = self.scaler.transform(df_clust)
        else:
            class ScalerNone:
                def __init__(self):
                    self.mean_ = [0.0 for n in range(df_clust.shape[1])]
                    self.scale_ = [1.0 for n in range(df_clust.shape[1])]
            self.scaler = ScalerNone()

        # test scaler's code generation
        if options.scale and options.verbose:
            # we rely on this later in code generation
            # for scaler.mean_
            # for scaler.scale_
            # for cluster.cluster_centers_
            for i in range(df_clust_back.shape[1]):
                assert df_clust_back.columns[i] == self.feats_used[i]

            # checking that scaler works as expected
            for feat in range(df_clust_back.shape[1]):
                df_clust_back[df_clust_back.columns[feat]
                              ] -= self.scaler.mean_[feat]
                df_clust_back[df_clust_back.columns[feat]
                              ] /= self.scaler.scale_[feat]

            print(df_clust_back.head()-df_clust.head())
            print(df_clust_back.head())
            print(df_clust.head())

        self.clust = sklearn.cluster.KMeans(n_clusters=options.clusters, random_state=prng)
        self.clust.fit(df_clust)
        self.df["clust"] = self.clust.labels_

        # print information about the clusters
        if options.verbose:
            print(self.feats_used)
            print(self.clust.labels_)
            print(self.clust.cluster_centers_)
            print(self.clust.get_params())

        # code for cluster centers
        self.create_code_for_cluster_centers(
            self.clust, self.scaler, self.feats_used)

        return self.feats_used, self.scaler, self.clust


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.pandas"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("--seed", default=None, type=int,
                        dest="seed", help="Seed of PRNG")
    parser.add_argument("fnames", nargs='+', metavar='FILES',
                        help="Files to parse, first file is the one to base data off of")
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        dest="verbose", help="Print more output")
    parser.add_argument("--printfeat", action="store_true", default=False,
                        dest="print_features", help="Print features")
    parser.add_argument("--check", action="store_true", default=False,
                        dest="check_row_data", help="Check row data for NaN or float overflow")
    parser.add_argument("--bestfeatfile",
                        default="../../scripts/crystal/best_features_usefulness_clust.txt",
                        type=str,
                        dest="best_features_fname",
                        help="Name and position of best features file that lists the best features in order")

    # clustering
    parser.add_argument("--clusters", default=4, type=int,
                        dest="clusters", help="How many clusters to use")
    parser.add_argument("--scale", default=False, action="store_true",
                        dest="scale", help="Scale clustering")
    parser.add_argument("--nocomputed", default=True, action="store_false",
                        dest="computed", help="Don't add computed features")
    parser.add_argument("--samples", default=1000,
                        dest="samples_per_file", help="Samples per file")

    # number of configs to generate
    parser.add_argument("--confs", default="2-2", type=str,
                        dest="confs", help="Configs to generate")

    # code
    parser.add_argument("--basedir", type=str,
                        dest="basedir", help="The base directory of where the CryptoMiniSat source code is")

    options = parser.parse_args()
    prng = np.random.RandomState(options.seed)

    if options.basedir is None:
        print("ERROR: must set basedir")
        exit(-1)

    if options.fnames is None or len(options.fnames) == 0:
        print("ERROR: You must give the pandas file!")
        exit(-1)

    fnames = [f for f in options.fnames if "clust" not in f]
    print("Will add clustering to files: ")
    for f in fnames:
        print("->", f)

    if options.clusters <= 0:
        print("ERROR: You must give a '--clusters' option that is greater than 0")
        exit(-1)

    samples = None
    for f in fnames:
        print("===-- Sampling file %s --" % f)
        df = pd.read_pickle(f)
        print("options.samples_per_file:", options.samples_per_file)
        print("df.shape:", df.shape)
        new_samples = df.sample(options.samples_per_file, replace=True,
                                random_state=prng)
        samples = new_samples.append(samples)
        print("samples.shape:", samples.shape)
        del df

    if options.computed:
        helper.cldata_add_computed_features(samples, options.verbose)

    # clustering_setup is:
    #   feats_used, scaler, clust
    clustering_setup = [None, None]
    todo = [CLUST_TYPE_FILES, CLUST_TYPE_USEFULNESS]
    todo = [CLUST_TYPE_FILES]
    for clust_type in todo:
        c = Clustering(samples, clust_type)
        clustering_setup[clust_type] = c.cluster()
        name = get_cluster_name(clust_type)
        if termcolor_avail:
            cprint("===-- K-Means clustering **created ** type: %s --" % name,
               "green", "on_grey")
        else:
            cprint("===-- K-Means clustering **created ** type: %s --" % name)

    del samples

    for f in fnames:
        print("===-- Clustering file %s" % (f))
        df_orig = pd.read_pickle(f)
        df = df_orig.copy()
        if options.computed:
            helper.cldata_add_computed_features(df, options.verbose)

        for clust_type in todo:
            name = get_cluster_name(clust_type)
            print("===-- type: %s --" % name)
            df_orig[name] = get_clustering_label(df, *clustering_setup[clust_type])
            print("Distribution: \n%s" % df_orig[name].value_counts())

        cleanname = re.sub(r'\.dat$', '', f)
        dump_dataframe(df_orig, cleanname+"-clustered.dat")
        del df
        del df_orig

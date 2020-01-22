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
ver = sklearn.__version__.split(".")
if int(ver[1]) < 20:
    from sklearn.cross_validation import train_test_split
else:
    from sklearn.model_selection import train_test_split


def add_clustering_label(df, feats_used: list, scaler, clust):
    this_feats = list(df)
    for f in feats_used:
        if f not in this_feats:
            print("ERROR: Features %s is in the other file but not in this one!" % f)
            exit(-1)

    df_clust = df[feats_used].astype(float).copy()
    df_clust[feats_used] = scaler.transform(df_clust)
    df["clust"] = clust.predict(df_clust)


def dump_clustered(df, used_clusters: list, fname: str):
    df = df[df["clust"].isin(used_clusters)]
    with open(fname, "wb") as f:
        pickle.dump(df, f)

    print("Dumped to file %s" % fname)


class Clustering:
    def __init__(self, df):
        self.df = df

    def clean_sz_feats(self, sz_feats):
        sz_feats_clean = []
        for feat in sz_feats:
            assert "szfeat_cur.conflicts" not in feat
            c = str(feat)
            c = c.replace(".irred_", ".irred_cl_distrib.")
            c = c.replace(".red_", ".red_cl_distrib.")
            c = c.replace("szfeat_cur.", "{val}.")

            sz_feats_clean.append(c)
        assert len(sz_feats_clean) == len(sz_feats)

        return sz_feats_clean

    def create_code_for_cluster_centers(self, clust, scaler, sz_feats):
        sz_feats_clean = self.clean_sz_feats(sz_feats)
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
        f.write("\n        centers.resize(%d);\n" % len(self.used_clusters))
        for i in self.used_clusters:

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
            f.write("        tmp = ((double){feat}-{mean:3.9f})/{scale:3.9f};\n".format(
                feat=feat.format(val="a"),
                 mean=scaler.mean_[i],
                 scale=scaler.scale_[i]
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

    def write_all_predictors_file(self, fnames, functs, conf_num, longsh):
        f = open("{basedir}/all_predictors_{name}_conf{conf_num}.h".format(
            basedir=options.basedir, name=longsh,
            conf_num=conf_num), "w")

        helper.write_mit_header(f)
        f.write("""///auto-generated code. Under MIT license.
#ifndef ALL_PREDICTORS_{name}_conf{conf_num}_H
#define ALL_PREDICTORS_{name}_conf{conf_num}_H\n\n""".format(name=longsh, conf_num=conf_num))
        f.write('#include "clause.h"\n')
        f.write('#include "predict_func_type.h"\n\n')
        for _, fname in fnames.items():
            f.write('#include "predict/%s"\n' % fname)

        f.write('#include <vector>\n')
        f.write('using std::vector;\n\n')

        f.write("namespace CMSat {\n")

        f.write("\nvector<keep_func_type> should_keep_{name}_conf{conf_num}_funcs = {{\n".format(
            conf_num=conf_num, name=longsh, clusters=options.clusters))

        for i in range(options.clusters):
            dummy = ""
            if i not in self.used_clusters:
                # just use a dummy one. will never be called
                func = next(iter(functs.values()))
                dummy = " /*dummy function, cluster too small*/"
            else:
                # use the correct one
                func = functs[i]

            f.write("    CMSat::{func}{dummy}".format(func=func, dummy=dummy))
            if i < options.clusters-1:
                f.write(",\n")
            else:
                f.write("\n")
        f.write("};\n\n")

        f.write("} //end namespace\n\n")
        f.write("#endif //ALL_PREDICTORS\n")

    def check_clust_distr(self):
        print("Checking cluster distribution....")

        # print distribution
        dist = {}
        for x in self.clust.labels_:
            if x not in dist:
                dist[x] = 1
            else:
                dist[x] += 1
        print(dist)

        self.used_clusters = []
        minimum = int(self.df.shape[0]*options.minimum_cluster_rel)
        for clust_num, clauses in dist.items():
            if clauses < minimum:
                print("== !! Will skip cluster %d !! ==" % clust_num)
            else:
                self.used_clusters.append(clust_num)

        if options.show_class_dist:
            print("Exact class contents follow")
            for clno in range(options.clusters):
                x = self.df[(self.df.clust == clno)]
                fname_dist = {}
                for _, d in x.iterrows():
                    fname = d['fname']
                    if fname not in fname_dist:
                        fname_dist[fname] = 1
                    else:
                        fname_dist[fname] += 1

                skipped = "SKIPPED"
                if clno in self.used_clusters:
                    skipped = ""
                print("\n\nFile name distribution in {skipped} cluster {clno} **".format(
                    clno=clno, skipped=skipped))

                sorted_x = sorted(fname_dist.items(),
                                  key=operator.itemgetter(0))
                for a, b in sorted_x:
                    print("--> %-10s : %s" % (b, a))
            print("\n\nClass contents finished.\n")

        self.used_clusters = sorted(self.used_clusters)

    def cluster(self):
        features = list(self.df)

        # features from dataframe
        self.feats_used = []
        for feat in features:
            if "szfeat_cur" in feat and "szfeat_cur.conflicts" not in feat:
                self.feats_used.append(feat)

        # print("Features used: ", self.feats_used)
        print("Number of features used: ", len(self.feats_used))

        if options.check_row_data:
            helper.check_too_large_or_nan_values(df, self.feats_used)

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
            df_clust[self.feats_used] = self.scaler.transform(df_clust)
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

        self.clust = sklearn.cluster.KMeans(n_clusters=options.clusters)
        self.clust.fit(df_clust)
        self.df["clust"] = self.clust.labels_

        # print information about the clusters
        if options.verbose:
            print(self.feats_used)
            print(self.clust.labels_)
            print(self.clust.cluster_centers_)
            print(self.clust.get_params())
        self.check_clust_distr()

        # code for clusters
        if options.basedir is None:
            return

        # code for cluster centers
        self.create_code_for_cluster_centers(
            self.clust, self.scaler, self.feats_used)

        # code for predictors
        fnames = {}
        functs = {}

        conf_from, conf_to = helper.parse_configs(options.confs)
        for conf in range(conf_from, conf_to):
            for name in ["long", "short"]:
                for clno in self.used_clusters:
                    funcname = "should_keep_{name}_conf{conf_num}_cluster{clno}".format(
                        clno=clno, name=name, conf_num=conf)
                    functs[clno] = funcname

                    fname = "final_predictor_{name}_conf{conf_num}_cluster{clno}.h".format(
                        clno=clno, name=name, conf_num=conf)
                    fnames[clno] = fname

                self.write_all_predictors_file(
                    fnames, functs, conf_num=conf, longsh=name)

        print("Used clusters: ", self.used_clusters)
        return self.feats_used, self.scaler, self.clust, self.used_clusters


if __name__ == "__main__":
    usage = "usage: %(prog)s [options] file.pandas"
    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument("fnames", nargs='+', metavar='FILES',
                        help="Files to parse, first file is the one to base data off of")
    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        dest="verbose", help="Print more output")
    parser.add_argument("--printfeat", action="store_true", default=False,
                        dest="print_features", help="Print features")
    parser.add_argument("--check", action="store_true", default=False,
                        dest="check_row_data", help="Check row data for NaN or float overflow")

    # clustering
    parser.add_argument("--clusters", default=1, type=int,
                        dest="clusters", help="How many clusters to use")
    parser.add_argument("--clustmin", default=0.05, type=float, metavar="RATIO",
                        dest="minimum_cluster_rel", help="What's the minimum size of the cluster relative to the original set of data.")
    parser.add_argument("--scale", default=False, action="store_true",
                        dest="scale", help="Scale clustering")
    parser.add_argument("--distr", default=False, action="store_true",
                        dest="show_class_dist", help="Show class distribution")
    parser.add_argument("--nocomputed", default=False, action="store_true",
                        dest="no_computed", help="Don't add computed features")

    # number of configs to generate
    parser.add_argument("--confs", default="2-2", type=str,
                        dest="confs", help="Configs to generate")

    # code
    parser.add_argument("--basedir", type=str,
                        dest="basedir", help="The base directory of where the CryptoMiniSat source code is")

    options = parser.parse_args()

    if options.fnames is None or len(options.fnames) == 0:
        print("ERROR: You must give the pandas file!")
        exit(-1)
    else:
        print("Will base clustering on file:", options.fnames[0])
        print("Will add clustering to files: ")
        for f in options.fnames:
            print("->", f)

    if options.clusters <= 0:
        print("ERROR: You must give a '--clusters' option that is greater than 0")
        exit(-1)

    feats_used, scaler, clust, used_clusters = [None, None, None, None]
    for f in options.fnames:
        if "-clustered.dat" in f:
            print("Skipping file, seems clustered: %s" % f)
            continue

        print("=====================")
        print("-- Reading file %s --" % f)
        print("=====================")
        df = pd.read_pickle(f)
        if not options.no_computed:
            helper.add_computed_szfeat_for_clustering(df)

        if clust is None:
            print("!! Using this file for generating clustering !!")
            c = Clustering(df)
            feats_used, scaler, clust, used_clusters = c.cluster()
        else:
            add_clustering_label(df, feats_used, scaler, clust)

        cleanname = re.sub(r'\.dat$', '', f)
        dump_clustered(df, used_clusters, cleanname+"-clustered.dat")
        del df

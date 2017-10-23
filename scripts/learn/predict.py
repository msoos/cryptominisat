#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2017  Mate Soos
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

from __future__ import print_function
import sqlite3
import optparse
import time
import pickle
import re
import pandas as pd

from sklearn.model_selection import train_test_split
import sklearn.tree
import sklearn.ensemble
import sklearn.metrics


class QueryHelper:
    def __init__(self, dbfname):
        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()
        self.runID = self.find_runID()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()

    def find_runID(self):
        q = """
        SELECT runID
        FROM startUp
        order by startTime desc
        limit 1
        """

        runID = None
        for row in self.c.execute(q):
            if runID is not None:
                print("ERROR: More than one RUN IDs in file!")
                exit(-1)
            runID = int(row[0])

        print("runID: %d" % runID)
        return runID


class Query2 (QueryHelper):
    def create_indexes(self):
        print("Recreating indexes...")
        t = time.time()
        q = """
        drop index if exists `idxclid`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;
        drop index if exists `idxclid4`;

        create index `idxclid` on `clauseStats` (`runID`,`clauseID`);
        create index `idxclid2` on `clauseStats` (`runID`,`prev_restart`);
        create index `idxclid3` on `goodClauses` (`runID`,`clauseID`);
        create index `idxclid4` on `restart` (`runID`, `restarts`);
        """
        for l in q.split('\n'):
            self.c.execute(l)

        print("indexes created %-3.2f" % (time.time() - t))

    def get_max_clauseID(self):
        q = """
        SELECT max(clauseID)
        FROM clauseStats
        WHERE runID = %d
        """ % self.runID

        max_clID = None
        for row in self.c.execute(q):
            max_clID = int(row[0])

        return max_clID

    def get_clstats(self):
        comment = ""
        if not options.restart_used:
            comment = "--"

        restart_dat = """
        {comment} restart.`runID` as `rst.runID`,
        {comment} restart.`simplifications` as `rst.simplifications`,
        {comment} restart.`restarts` as `rst.restarts`,
        {comment} restart.`conflicts` as `rst.conflicts`,
        {comment} restart.`runtime` as `rst.runtime`,
        {comment} restart.`numIrredBins` as `rst.numIrredBins`,
        {comment} restart.`numIrredLongs` as `rst.numIrredLongs`,
        {comment} restart.`numRedBins` as `rst.numRedBins`,
        {comment} restart.`numRedLongs` as `rst.numRedLongs`,
        {comment} restart.`numIrredLits` as `rst.numIrredLits`,
        {comment} restart.`numredLits` as `rst.numredLits`,
        {comment} restart.`glue` as `rst.glue`,
        {comment} restart.`glueSD` as `rst.glueSD`,
        {comment} restart.`glueMin` as `rst.glueMin`,
        {comment} restart.`glueMax` as `rst.glueMax`,
        {comment} restart.`size` as `rst.size`,
        {comment} restart.`sizeSD` as `rst.sizeSD`,
        {comment} restart.`sizeMin` as `rst.sizeMin`,
        {comment} restart.`sizeMax` as `rst.sizeMax`,
        {comment} restart.`resolutions` as `rst.resolutions`,
        {comment} restart.`resolutionsSD` as `rst.resolutionsSD`,
        {comment} restart.`resolutionsMin` as `rst.resolutionsMin`,
        {comment} restart.`resolutionsMax` as `rst.resolutionsMax`,
        {comment} restart.`branchDepth` as `rst.branchDepth`,
        {comment} restart.`branchDepthSD` as `rst.branchDepthSD`,
        {comment} restart.`branchDepthMin` as `rst.branchDepthMin`,
        {comment} restart.`branchDepthMax` as `rst.branchDepthMax`,
        {comment} restart.`branchDepthDelta` as `rst.branchDepthDelta`,
        {comment} restart.`branchDepthDeltaSD` as `rst.branchDepthDeltaSD`,
        {comment} restart.`branchDepthDeltaMin` as `rst.branchDepthDeltaMin`,
        {comment} restart.`branchDepthDeltaMax` as `rst.branchDepthDeltaMax`,
        {comment} restart.`trailDepth` as `rst.trailDepth`,
        {comment} restart.`trailDepthSD` as `rst.trailDepthSD`,
        {comment} restart.`trailDepthMin` as `rst.trailDepthMin`,
        {comment} restart.`trailDepthMax` as `rst.trailDepthMax`,
        {comment} restart.`trailDepthDelta` as `rst.trailDepthDelta`,
        {comment} restart.`trailDepthDeltaSD` as `rst.trailDepthDeltaSD`,
        {comment} restart.`trailDepthDeltaMin` as `rst.trailDepthDeltaMin`,
        {comment} restart.`trailDepthDeltaMax` as `rst.trailDepthDeltaMax`,
        {comment} restart.`propBinIrred` as `rst.propBinIrred`,
        {comment} restart.`propBinRed` as `rst.propBinRed`,
        {comment} restart.`propLongIrred` as `rst.propLongIrred`,
        {comment} restart.`propLongRed` as `rst.propLongRed`,
        {comment} restart.`conflBinIrred` as `rst.conflBinIrred`,
        {comment} restart.`conflBinRed` as `rst.conflBinRed`,
        {comment} restart.`conflLongIrred` as `rst.conflLongIrred`,
        {comment} restart.`conflLongRed` as `rst.conflLongRed`,
        {comment} restart.`learntUnits` as `rst.learntUnits`,
        {comment} restart.`learntBins` as `rst.learntBins`,
        {comment} restart.`learntLongs` as `rst.learntLongs`,
        {comment} restart.`resolBinIrred` as `rst.resolBinIrred`,
        {comment} restart.`resolBinRed` as `rst.resolBinRed`,
        {comment} restart.`resolLIrred` as `rst.resolLIrred`,
        {comment} restart.`resolLRed` as `rst.resolLRed`,
        {comment} restart.`propagations` as `rst.propagations`,
        {comment} restart.`decisions` as `rst.decisions`,
        {comment} restart.`flipped` as `rst.flipped`,
        {comment} restart.`varSetPos` as `rst.varSetPos`,
        {comment} restart.`varSetNeg` as `rst.varSetNeg`,
        {comment} restart.`free` as `rst.free`,
        {comment} restart.`replaced` as `rst.replaced`,
        {comment} restart.`eliminated` as `rst.eliminated`,
        {comment} restart.`set` as `rst.set`,
        {comment} restart.`clauseIDstartInclusive` as `rst.clauseIDstartInclusive`,
        {comment} restart.`clauseIDendExclusive` as `rst.clauseIDendExclusive`
        """.format(comment=comment)

        clause_dat = """
        clauseStats.`runID` as `cl.runID`,
        clauseStats.`simplifications` as `cl.simplifications`,
        clauseStats.`restarts` as `cl.restarts`,
        clauseStats.`prev_restart` as `cl.prev_restart`,
        clauseStats.`conflicts` as `cl.conflicts`,
        clauseStats.`clauseID` as `cl.clauseID`,
        clauseStats.`glue` as `cl.glue`,
        clauseStats.`size` as `cl.size`,
        clauseStats.`conflicts_this_restart` as `cl.conflicts_this_restart`,
        clauseStats.`num_overlap_literals` as `cl.num_overlap_literals`,
        clauseStats.`num_antecedents` as `cl.num_antecedents`,
        clauseStats.`antecedents_avg_size` as `cl.antecedents_avg_size`,
        clauseStats.`backtrack_level` as `cl.backtrack_level`,
        clauseStats.`decision_level` as `cl.decision_level`,
        clauseStats.`trail_depth_level` as `cl.trail_depth_level`,
        clauseStats.`atedecents_binIrred` as `cl.atedecents_binIrred`,
        clauseStats.`atedecents_binRed` as `cl.atedecents_binRed`,
        clauseStats.`atedecents_longIrred` as `cl.atedecents_longIrred`,
        clauseStats.`atedecents_longRed` as `cl.atedecents_longRed`,
        clauseStats.`vsids_vars_avg` as `cl.vsids_vars_avg`,
        clauseStats.`vsids_vars_var` as `cl.vsids_vars_var`,
        clauseStats.`vsids_vars_min` as `cl.vsids_vars_min`,
        clauseStats.`vsids_vars_max` as `cl.vsids_vars_max`,
        clauseStats.`antecedents_glue_long_reds_avg` as `cl.antecedents_glue_long_reds_avg`,
        clauseStats.`antecedents_glue_long_reds_var` as `cl.antecedents_glue_long_reds_var`,
        clauseStats.`antecedents_glue_long_reds_min` as `cl.antecedents_glue_long_reds_min`,
        clauseStats.`antecedents_glue_long_reds_max` as `cl.antecedents_glue_long_reds_max`,
        clauseStats.`antecedents_long_red_age_avg` as `cl.antecedents_long_red_age_avg`,
        clauseStats.`antecedents_long_red_age_var` as `cl.antecedents_long_red_age_var`,
        clauseStats.`antecedents_long_red_age_min` as `cl.antecedents_long_red_age_min`,
        clauseStats.`antecedents_long_red_age_max` as `cl.antecedents_long_red_age_max`,
        clauseStats.`vsids_of_resolving_literals_avg` as `cl.vsids_of_resolving_literals_avg`,
        clauseStats.`vsids_of_resolving_literals_var` as `cl.vsids_of_resolving_literals_var`,
        clauseStats.`vsids_of_resolving_literals_min` as `cl.vsids_of_resolving_literals_min`,
        clauseStats.`vsids_of_resolving_literals_max` as `cl.vsids_of_resolving_literals_max`,
        clauseStats.`vsids_of_all_incoming_lits_avg` as `cl.vsids_of_all_incoming_lits_avg`,
        clauseStats.`vsids_of_all_incoming_lits_var` as `cl.vsids_of_all_incoming_lits_var`,
        clauseStats.`vsids_of_all_incoming_lits_min` as `cl.vsids_of_all_incoming_lits_min`,
        clauseStats.`vsids_of_all_incoming_lits_max` as `cl.vsids_of_all_incoming_lits_max`,
        clauseStats.`antecedents_antecedents_vsids_avg` as `cl.antecedents_antecedents_vsids_avg`,
        clauseStats.`decision_level_hist` as `cl.decision_level_hist`,
        clauseStats.`backtrack_level_hist` as `cl.backtrack_level_hist`,
        clauseStats.`trail_depth_level_hist` as `cl.trail_depth_level_hist`,
        clauseStats.`vsids_vars_hist` as `cl.vsids_vars_hist`,
        clauseStats.`size_hist` as `cl.size_hist`,
        clauseStats.`glue_hist` as `cl.glue_hist`,
        clauseStats.`num_antecedents_hist` as `cl.num_antecedents_hist`,
        """

        # partially done with tablestruct_sql and SED: sed -e 's/`\(.*\)`.*/{comment} restart.`\1` as `rst.\1`,/' ../tmp.txt
        q = """
        SELECT
        {clause_dat}
        1 as good,
        {restart_dat}

        FROM clauseStats, goodClauses
        {comment} , restart
        WHERE

        clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        and clauseStats.restarts > 1 -- to avoid history being invalid
        and clauseStats.runID = {0}
        {comment} and restart.restarts = clauseStats.prev_restart
        {comment} and restart.runID = {0}

        limit {1}
        """.format(self.runID, options.limit, comment=comment, restart_dat=restart_dat, clause_dat=clause_dat)

        df = pd.read_sql_query(q, self.conn)

        # BAD caluses
        q = """
        SELECT
        {clause_dat}
        0 as good,
        {restart_dat}

        FROM clauseStats left join goodClauses
        on clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        {comment} , restart
        WHERE

        goodClauses.clauseID is NULL
        and goodClauses.runID is NULL
        and clauseStats.restarts > 1 -- to avoid history being invalid
        and clauseStats.runID = {0}
        {comment} and restart.restarts = clauseStats.prev_restart
        {comment} and restart.runID = {0}

        limit {1}
        """.format(self.runID, options.limit, comment=comment, restart_dat=restart_dat, clause_dat=clause_dat)
        df2 = pd.read_sql_query(q, self.conn)

        return pd.concat([df, df2])


def get_one_file(dbfname):
    print("Using sqlite3db file %s" % dbfname)

    df = None
    with Query2(dbfname) as q:
        if not options.no_recreate_indexes:
            q.create_indexes()
        df = q.get_clstats()
        print("Printing head:")
        print(df.head())
        print("Print head done.")

    return df


class Classify:
    def __init__(self, df):
        self.features = df.columns.values.flatten().tolist()

        toremove = ["cl.decision_level_hist",
                    "cl.backtrack_level_hist",
                    "cl.trail_depth_level_hist",
                    "cl.vsids_vars_hist",
                    "cl.runID",
                    "cl.simplifications",
                    "cl.restarts",
                    "cl.conflicts",
                    "cl.clauseID",
                    "cl.size_hist",
                    "cl.glue_hist",
                    "cl.num_antecedents_hist",
                    "cl.decision_level",
                    "cl.backtrack_level",
                    "good"]

        if True:
            toremove.extend(["cl.vsids_vars_avg",
                             "cl.vsids_vars_var",
                             "cl.vsids_vars_min",
                             "cl.vsids_vars_max",
                             "cl.vsids_of_resolving_literals_avg",
                             "cl.vsids_of_resolving_literals_var",
                             "cl.vsids_of_resolving_literals_min",
                             "cl.vsids_of_resolving_literals_max",
                             "cl.vsids_of_all_incoming_lits_avg",
                             "cl.vsids_of_all_incoming_lits_var",
                             "cl.vsids_of_all_incoming_lits_min",
                             "cl.vsids_of_all_incoming_lits_max"])

        if options.restart_used:
            toremove.extend([
                "rst.runID",
                "rst.simplifications",
                "rst.restarts",
                "rst.conflicts",
                "rst.runtime",
                "rst.clauseIDstartInclusive",
                "rst.clauseIDendExclusive"])

        for t in toremove:
            print("removing feature:", t)
            self.features.remove(t)
        print("features:", self.features)

    def learn(self, df, cleanname, classifiername="classifier"):

        print("total samples: %5d   percentage of good ones %-3.4f" %
              (df.shape[0],
               sum(df["good"]) / float(df.shape[0]) * 100.0))

        print("Training....")
        t = time.time()
        # clf = KNeighborsClassifier(5) # EXPENSIVE at prediction, NOT suitable
        # self.clf = sklearn.linear_model.LogisticRegression() # NOT good.
        self.clf = sklearn.tree.DecisionTreeClassifier(
            random_state=90, max_depth=5)
        # self.clf = sklearn.ensemble.RandomForestClassifier(min_samples_split=len(X)/20, n_estimators=6)
        # self.clf = sklearn.svm.SVC(max_iter=1000) # can't make it work too well..
        train, test = train_test_split(df, test_size=0.2, random_state=90)
        self.clf.fit(train[self.features], train["good"])

        print("Training finished. T: %-3.2f" % (time.time() - t))

        print("Calculating scores....")
        t = time.time()
        y_pred = self.clf.predict(test[self.features])
        recall = sklearn.metrics.recall_score(test["good"], y_pred)
        prec = sklearn.metrics.precision_score(test["good"], y_pred)
        print("prec: %-3.4f  recall: %-3.4f T: %-3.2f" %
              (prec, recall, (time.time() - t)))

        with open(classifiername, "wb") as f:
            pickle.dump(self.clf, f)

    def output_to_dot(self, fname):
        sklearn.tree.export_graphviz(self.clf, out_file=fname,
                                     feature_names=self.features,
                                     class_names=["BAD", "GOOD"],
                                     filled=True, rounded=True,
                                     special_characters=True,
                                     proportion=True
                                     )
        print("Run dot:")
        print("dot -Tpng %s -o tree.png" % fname)


class Check:
    def __init__(self, classf_fname):
        with open(classf_fname, "rb") as f:
            self.clf = pickle.load(f)

    def check(self, X, y):
        print("total samples: %5d   percentage of good ones %-3.2f" %
              (len(X), sum(y) / float(len(X)) * 100.0))

        t = time.time()
        y_pred = self.clf.predict(X)
        recall = sklearn.metrics.recall_score(y, y_pred)
        prec = sklearn.metrics.precision_score(y, y_pred)
        # avg_prec = self.clf.score(X, y)
        print("prec: %-3.3f  recall: %-3.3f T: %-3.2f" %
              (prec, recall, (time.time() - t)))


def transform(df):
    def check_clstat_row(self, row):
        if row[self.ntoc["cl.decision_level_hist"]] == 0 or \
                row[self.ntoc["cl.backtrack_level_hist"]] == 0 or \
                row[self.ntoc["cl.trail_depth_level_hist"]] == 0 or \
                row[self.ntoc["cl.vsids_vars_hist"]] == 0 or \
                row[self.ntoc["cl.size_hist"]] == 0 or \
                row[self.ntoc["cl.glue_hist"]] == 0 or \
                row[self.ntoc["cl.num_antecedents_hist"]] == 0:
            print("ERROR: Data is in error:", row)
            assert(False)
            exit(-1)

        return row

    df["cl.size_rel"] = df["cl.size"] / df["cl.size_hist"]
    df["cl.glue_rel"] = df["cl.glue"] / df["cl.glue_hist"]
    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"] / \
        df["cl.num_antecedents_hist"]
    df["cl.decision_level_rel"] = df["cl.decision_level"] / df["cl.decision_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"] / \
        df["cl.backtrack_level_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"] / \
        df["cl.trail_depth_level_hist"]
    df["cl.vsids_vars_rel"] = df["cl.vsids_vars_avg"] / df["cl.vsids_vars_hist"]
    old = set(df.columns.values.flatten().tolist())
    df = df.dropna(how="all")
    new = set(df.columns.values.flatten().tolist())
    if len(old - new) > 0:
        print("ERROR: a NaN number turned up")
        print("columns: ", (old - new))
        assert(False)
        exit(-1)

    return df


def one_predictor(dbfname, final_df):
    t = time.time()
    df = get_one_file(dbfname)
    cleanname = re.sub('\.cnf.gz.sqlite$', '', dbfname)
    print("Read in data in %-5.2f secs" % (time.time() - t))

    print("Describing----")
    print(df.describe())
    print("Describe done.---")
    print("Features: ", df.columns.values.flatten().tolist())
    df = transform(df)

    print("Describingi post-transform ----")
    print(df.describe())
    print("Describe done.---")

    if final_df is None:
        final_df = df
    else:
        final_df = final_df.concat([df, final_df])

    # display
    pd.options.display.mpl_style = "default"
    with open("pandasdata.dat", "wb") as f:
        pickle.dump(df, f)
    # df.hist()
    # df.boxplot()

    if options.check:
        check = Check(options.check)
        check.check(cl.X, cl.y)
    else:
        clf = Classify(df)
        clf.learn(df, "%s.classifier" % cleanname)
        clf.output_to_dot("%s.tree.dot" % cleanname)


if __name__ == "__main__":

    usage = "usage: %prog [options] dir1 [dir2...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--pow2", "-p", action="store_true", default=False,
                      dest="add_pow2", help="Add power of 2 of all data")

    parser.add_option("--limit", "-l", default=10**9, type=int,
                      dest="limit", help="Max number of good/bad clauses")

    parser.add_option("--check", "-c", type=str,
                      dest="check", help="Check classifier")

    parser.add_option("--data", "-d", action="store_true", default=False,
                      dest="data", help="Just get the dumped data")

    parser.add_option("--restart", "-r", action="store_true", default=False,
                      dest="restart_used", help="Help use restart stat about clause")

    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes", help="Don't recreate indexes")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)

    final_df = None
    for dbfname in args:
        print("----- INTERMEDIATE predictor -------\n")
        one_predictor(dbfname, final_df)

    # intermediate predictor is final
    if len(args) == 1:
        exit(0)

    # no need, we checked them all individually
    if options.check:
        exit(0)

    print("----- FINAL predictor -------\n")
    if options.check:
        check = Check()
        check.check(cl.X, cl.y)
    else:
        clf = Classify(final_df)
        clf.learn(final_df, "final.classifier")
        clf.output_to_dot("final.dot")

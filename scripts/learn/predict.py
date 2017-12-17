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
import numpy as np

from sklearn.model_selection import train_test_split
import sklearn.tree
import sklearn.ensemble
import sklearn.metrics


##############
# HOW TO GET A NICE LIST
##############
# go into .stdout.gz outputs:
# zgrep "s UNSAT" * | cut -d ":" -f 1 > ../candidate_files_large_fixed_adjust_guess-12-April
#
# --> edit file to have the format:
# zgrep -H "Total" large_hybr-12-April-2016-VAGTY-e4119a1b0-tout-1500-mout-1600/1dlx_c_iq57_a.cnf.gz.stdout.gz
#
# run:
# ./candidate_files_large_hybr-12-April-2016-VAGTY.sh | awk '{if ($5 < 600 && $5 > 200) print $1 " -- " $5}' | cut -d "/" -f 2 | cut -d ":" -f 1 | sed "s/.stdout.*//" > ../unsat_small_candidates2.txt


################
# EXAMPLE TO RUN THIS AGAINST
################
# 6s153.cnf.gz

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

        print("indexes created T: %-3.2f s" % (time.time() - t))

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

        # partially done with tablestruct_sql and SED: sed -e 's/`\(.*\)`.*/restart.`\1` as `rst.\1`/' ../tmp.txt
        restart_dat = """
        -- , restart.`runID` as `rst.runID`
        -- , restart.`simplifications` as `rst.simplifications`
        -- , restart.`restarts` as `rst.restarts`
        -- , restart.`conflicts` as `rst.conflicts`
        -- , restart.`latest_feature_calc` as `restart.latest_feature_calc`
        -- restart.`runtime` as `rst.runtime`
        , restart.`numIrredBins` as `rst.numIrredBins`
        , restart.`numIrredLongs` as `rst.numIrredLongs`
        , restart.`numRedBins` as `rst.numRedBins`
        , restart.`numRedLongs` as `rst.numRedLongs`
        , restart.`numIrredLits` as `rst.numIrredLits`
        , restart.`numredLits` as `rst.numredLits`
        , restart.`glue` as `rst.glue`
        , restart.`glueSD` as `rst.glueSD`
        , restart.`glueMin` as `rst.glueMin`
        , restart.`glueMax` as `rst.glueMax`
        , restart.`size` as `rst.size`
        , restart.`sizeSD` as `rst.sizeSD`
        , restart.`sizeMin` as `rst.sizeMin`
        , restart.`sizeMax` as `rst.sizeMax`
        , restart.`resolutions` as `rst.resolutions`
        , restart.`resolutionsSD` as `rst.resolutionsSD`
        , restart.`resolutionsMin` as `rst.resolutionsMin`
        , restart.`resolutionsMax` as `rst.resolutionsMax`
        , restart.`branchDepth` as `rst.branchDepth`
        , restart.`branchDepthSD` as `rst.branchDepthSD`
        , restart.`branchDepthMin` as `rst.branchDepthMin`
        , restart.`branchDepthMax` as `rst.branchDepthMax`
        , restart.`branchDepthDelta` as `rst.branchDepthDelta`
        , restart.`branchDepthDeltaSD` as `rst.branchDepthDeltaSD`
        , restart.`branchDepthDeltaMin` as `rst.branchDepthDeltaMin`
        , restart.`branchDepthDeltaMax` as `rst.branchDepthDeltaMax`
        , restart.`trailDepth` as `rst.trailDepth`
        , restart.`trailDepthSD` as `rst.trailDepthSD`
        , restart.`trailDepthMin` as `rst.trailDepthMin`
        , restart.`trailDepthMax` as `rst.trailDepthMax`
        , restart.`trailDepthDelta` as `rst.trailDepthDelta`
        , restart.`trailDepthDeltaSD` as `rst.trailDepthDeltaSD`
        , restart.`trailDepthDeltaMin` as `rst.trailDepthDeltaMin`
        , restart.`trailDepthDeltaMax` as `rst.trailDepthDeltaMax`
        , restart.`propBinIrred` as `rst.propBinIrred`
        , restart.`propBinRed` as `rst.propBinRed`
        , restart.`propLongIrred` as `rst.propLongIrred`
        , restart.`propLongRed` as `rst.propLongRed`
        , restart.`conflBinIrred` as `rst.conflBinIrred`
        , restart.`conflBinRed` as `rst.conflBinRed`
        , restart.`conflLongIrred` as `rst.conflLongIrred`
        , restart.`conflLongRed` as `rst.conflLongRed`
        , restart.`learntUnits` as `rst.learntUnits`
        , restart.`learntBins` as `rst.learntBins`
        , restart.`learntLongs` as `rst.learntLongs`
        , restart.`resolBinIrred` as `rst.resolBinIrred`
        , restart.`resolBinRed` as `rst.resolBinRed`
        , restart.`resolLIrred` as `rst.resolLIrred`
        , restart.`resolLRed` as `rst.resolLRed`
        -- , restart.`propagations` as `rst.propagations`
        -- , restart.`decisions` as `rst.decisions`
        -- , restart.`flipped` as `rst.flipped`
        -- , restart.`varSetPos` as `rst.varSetPos`
        -- , restart.`varSetNeg` as `rst.varSetNeg`
        -- , restart.`free` as `rst.free`
        -- , restart.`replaced` as `rst.replaced`
        -- , restart.`eliminated` as `rst.eliminated`
        -- , restart.`set` as `rst.set`
        -- , restart.`clauseIDstartInclusive` as `rst.clauseIDstartInclusive`
        -- , restart.`clauseIDendExclusive` as `rst.clauseIDendExclusive`
        """

        clause_dat = """
        -- , clauseStats.`runID` as `cl.runID`
        -- , clauseStats.`simplifications` as `cl.simplifications`
        -- , clauseStats.`restarts` as `cl.restarts`
        -- , clauseStats.`prev_restart` as `cl.prev_restart`
        -- , clauseStats.`conflicts` as `cl.conflicts`
        -- , clauseStats.`latest_feature_calc` as `clauseStats.latest_feature_calc`
        -- , clauseStats.`clauseID` as `cl.clauseID`
        clauseStats.`glue` as `cl.glue`
        , clauseStats.`size` as `cl.size`
        , clauseStats.`conflicts_this_restart` as `cl.conflicts_this_restart`
        , clauseStats.`num_overlap_literals` as `cl.num_overlap_literals`
        , clauseStats.`num_antecedents` as `cl.num_antecedents`
        , clauseStats.`antecedents_avg_size` as `cl.antecedents_avg_size`
        , clauseStats.`backtrack_level` as `cl.backtrack_level`
        , clauseStats.`decision_level` as `cl.decision_level`
        , clauseStats.`trail_depth_level` as `cl.trail_depth_level`
        , clauseStats.`atedecents_binIrred` as `cl.atedecents_binIrred`
        , clauseStats.`atedecents_binRed` as `cl.atedecents_binRed`
        , clauseStats.`atedecents_longIrred` as `cl.atedecents_longIrred`
        , clauseStats.`atedecents_longRed` as `cl.atedecents_longRed`
        , clauseStats.`vsids_vars_avg` as `cl.vsids_vars_avg`
        , clauseStats.`vsids_vars_var` as `cl.vsids_vars_var`
        , clauseStats.`vsids_vars_min` as `cl.vsids_vars_min`
        , clauseStats.`vsids_vars_max` as `cl.vsids_vars_max`
        , clauseStats.`antecedents_glue_long_reds_avg` as `cl.antecedents_glue_long_reds_avg`
        , clauseStats.`antecedents_glue_long_reds_var` as `cl.antecedents_glue_long_reds_var`
        , clauseStats.`antecedents_glue_long_reds_min` as `cl.antecedents_glue_long_reds_min`
        , clauseStats.`antecedents_glue_long_reds_max` as `cl.antecedents_glue_long_reds_max`
        , clauseStats.`antecedents_long_red_age_avg` as `cl.antecedents_long_red_age_avg`
        , clauseStats.`antecedents_long_red_age_var` as `cl.antecedents_long_red_age_var`
        , clauseStats.`antecedents_long_red_age_min` as `cl.antecedents_long_red_age_min`
        , clauseStats.`antecedents_long_red_age_max` as `cl.antecedents_long_red_age_max`
        , clauseStats.`vsids_of_resolving_literals_avg` as `cl.vsids_of_resolving_literals_avg`
        , clauseStats.`vsids_of_resolving_literals_var` as `cl.vsids_of_resolving_literals_var`
        , clauseStats.`vsids_of_resolving_literals_min` as `cl.vsids_of_resolving_literals_min`
        , clauseStats.`vsids_of_resolving_literals_max` as `cl.vsids_of_resolving_literals_max`
        , clauseStats.`vsids_of_all_incoming_lits_avg` as `cl.vsids_of_all_incoming_lits_avg`
        , clauseStats.`vsids_of_all_incoming_lits_var` as `cl.vsids_of_all_incoming_lits_var`
        , clauseStats.`vsids_of_all_incoming_lits_min` as `cl.vsids_of_all_incoming_lits_min`
        , clauseStats.`vsids_of_all_incoming_lits_max` as `cl.vsids_of_all_incoming_lits_max`
        , clauseStats.`antecedents_antecedents_vsids_avg` as `cl.antecedents_antecedents_vsids_avg`
        , clauseStats.`decision_level_hist` as `cl.decision_level_hist`
        , clauseStats.`backtrack_level_hist` as `cl.backtrack_level_hist`
        , clauseStats.`trail_depth_level_hist` as `cl.trail_depth_level_hist`
        , clauseStats.`vsids_vars_hist` as `cl.vsids_vars_hist`
        , clauseStats.`size_hist` as `cl.size_hist`
        , clauseStats.`glue_hist` as `cl.glue_hist`
        , clauseStats.`num_antecedents_hist` as `cl.num_antecedents_hist`
        """

        feat_dat = """
        -- , features.`simplifications` as `feat.simplifications`
        -- , features.`restarts` as `feat.restarts`
        , features.`conflicts` as `feat.conflicts`
        -- , features.`latest_feature_calc` as `feat.latest_feature_calc`
        , features.`numVars` as `feat.numVars`
        , features.`numClauses` as `feat.numClauses`
        , features.`var_cl_ratio` as `feat.var_cl_ratio`
        , features.`binary` as `feat.binary`
        , features.`horn` as `feat.horn`
        , features.`horn_mean` as `feat.horn_mean`
        , features.`horn_std` as `feat.horn_std`
        , features.`horn_min` as `feat.horn_min`
        , features.`horn_max` as `feat.horn_max`
        , features.`horn_spread` as `feat.horn_spread`
        , features.`vcg_var_mean` as `feat.vcg_var_mean`
        , features.`vcg_var_std` as `feat.vcg_var_std`
        , features.`vcg_var_min` as `feat.vcg_var_min`
        , features.`vcg_var_max` as `feat.vcg_var_max`
        , features.`vcg_var_spread` as `feat.vcg_var_spread`
        , features.`vcg_cls_mean` as `feat.vcg_cls_mean`
        , features.`vcg_cls_std` as `feat.vcg_cls_std`
        , features.`vcg_cls_min` as `feat.vcg_cls_min`
        , features.`vcg_cls_max` as `feat.vcg_cls_max`
        , features.`vcg_cls_spread` as `feat.vcg_cls_spread`
        , features.`pnr_var_mean` as `feat.pnr_var_mean`
        , features.`pnr_var_std` as `feat.pnr_var_std`
        , features.`pnr_var_min` as `feat.pnr_var_min`
        , features.`pnr_var_max` as `feat.pnr_var_max`
        , features.`pnr_var_spread` as `feat.pnr_var_spread`
        , features.`pnr_cls_mean` as `feat.pnr_cls_mean`
        , features.`pnr_cls_std` as `feat.pnr_cls_std`
        , features.`pnr_cls_min` as `feat.pnr_cls_min`
        , features.`pnr_cls_max` as `feat.pnr_cls_max`
        , features.`pnr_cls_spread` as `feat.pnr_cls_spread`
        , features.`avg_confl_size` as `feat.avg_confl_size`
        , features.`confl_size_min` as `feat.confl_size_min`
        , features.`confl_size_max` as `feat.confl_size_max`
        , features.`avg_confl_glue` as `feat.avg_confl_glue`
        , features.`confl_glue_min` as `feat.confl_glue_min`
        , features.`confl_glue_max` as `feat.confl_glue_max`
        , features.`avg_num_resolutions` as `feat.avg_num_resolutions`
        , features.`num_resolutions_min` as `feat.num_resolutions_min`
        , features.`num_resolutions_max` as `feat.num_resolutions_max`
        , features.`learnt_bins_per_confl` as `feat.learnt_bins_per_confl`
        , features.`avg_branch_depth` as `feat.avg_branch_depth`
        , features.`branch_depth_min` as `feat.branch_depth_min`
        , features.`branch_depth_max` as `feat.branch_depth_max`
        , features.`avg_trail_depth_delta` as `feat.avg_trail_depth_delta`
        , features.`trail_depth_delta_min` as `feat.trail_depth_delta_min`
        , features.`trail_depth_delta_max` as `feat.trail_depth_delta_max`
        , features.`avg_branch_depth_delta` as `feat.avg_branch_depth_delta`
        , features.`props_per_confl` as `feat.props_per_confl`
        , features.`confl_per_restart` as `feat.confl_per_restart`
        , features.`decisions_per_conflict` as `feat.decisions_per_conflict`
        , features.`red_glue_distr_mean` as `feat.red_glue_distr_mean`
        , features.`red_glue_distr_var` as `feat.red_glue_distr_var`
        , features.`red_size_distr_mean` as `feat.red_size_distr_mean`
        , features.`red_size_distr_var` as `feat.red_size_distr_var`
        -- , features.`red_activity_distr_mean` as `feat.red_activity_distr_mean`
        -- , features.`red_activity_distr_var` as `feat.red_activity_distr_var`
        -- , features.`irred_glue_distr_mean` as `feat.irred_glue_distr_mean`
        -- , features.`irred_glue_distr_var` as `feat.irred_glue_distr_var`
        , features.`irred_size_distr_mean` as `feat.irred_size_distr_mean`
        , features.`irred_size_distr_var` as `feat.irred_size_distr_var`
        -- , features.`irred_activity_distr_mean` as `feat.irred_activity_distr_mean`
        -- , features.`irred_activity_distr_var` as `feat.irred_activity_distr_var`
        """

        q_count = "SELECT count(*)"
        q_ok1 = """
        SELECT
        {clause_dat}
        {restart_dat}
        {feat_dat}
        , "OK" as good
        """

        q_ok2 = """
        FROM
        clauseStats
        , goodClauses
        , restart
        , features
        WHERE

        clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        and clauseStats.restarts > 1 -- to avoid history being invalid
        and clauseStats.runID = {runid}
        and features.runID = {runid}
        and features.latest_feature_calc = clauseStats.latest_feature_calc
        and restart.restarts = clauseStats.prev_restart
        and restart.runID = {runid}

        order by random()
        limit {limit}
        """


        # BAD caluses
        q_bad1 = """
        SELECT
        {clause_dat}
        {restart_dat}
        {feat_dat}
        , "BAD" as good
        """

        q_bad2 = """
        FROM clauseStats left join goodClauses
        on clauseStats.clauseID = goodClauses.clauseID
        and clauseStats.runID = goodClauses.runID
        , restart
        , features
        WHERE

        goodClauses.clauseID is NULL
        and goodClauses.runID is NULL
        and clauseStats.restarts > 1 -- to avoid history being invalid
        and clauseStats.runID = {runid}
        and features.runID = {runid}
        and features.latest_feature_calc = clauseStats.latest_feature_calc
        and restart.restarts = clauseStats.prev_restart
        and restart.runID = {runid}

        order by random()
        limit {limit}
        """
        myformat = {"runid" : self.runID,
                "limit" : 1000*1000*1000,
                "restart_dat" : restart_dat,
                "clause_dat" : clause_dat,
                "feat_dat" : feat_dat}

        t = time.time()

        q = q_count + q_ok2
        q = q.format(**myformat)
        cur = self.conn.execute(q.format(**myformat))
        num_lines_ok = int(cur.fetchone()[0])
        print("Num datapoints OK (K): %-3.5f" % (num_lines_ok/1000.0))

        q = q_count + q_bad2
        q = q.format(**myformat)
        cur = self.conn.execute(q.format(**myformat))
        num_lines_bad = int(cur.fetchone()[0])
        print("Num datpoints BAD (K): %-3.5f" % (num_lines_bad/1000.0))

        total_lines = num_lines_ok + num_lines_bad
        print("Total number of datapoints (K): %-3.2f" % (total_lines/1000.0))
        if options.fixed_num_datapoints != -1:
            if options.fixed_num_datapoints > total_lines:
                print("WARNING -- Your fixed num datapoints is too high:", options.fixed_num_datapoints)
                print("WARNING -- We only have:", total_lines)
                return False, None

        if total_lines == 0:
            print("WARNING: Total number of datapoints is 0, something is wrong or empty")
            return False, None

        print("Percentage of OK: %-3.2f" % (num_lines_ok/float(total_lines)*100.0))
        q = q_ok1 + q_ok2
        if options.fixed_num_datapoints != -1:
            myformat["limit"] = int(options.fixed_num_datapoints * num_lines_ok/float(total_lines))
        print("limit for OK:", myformat["limit"])
        q = q.format(**myformat)
        print("Running query for OK...")
        df = pd.read_sql_query(q, self.conn)

        print("Running query for BAD...")
        q = q_bad1 + q_bad2
        if options.fixed_num_datapoints != -1:
            myformat["limit"] = int(options.fixed_num_datapoints * num_lines_bad/float(total_lines))
        print("limit for bad:", myformat["limit"])
        q = q.format(**myformat)
        df2 = pd.read_sql_query(q, self.conn)
        print("Queries finished. T: %-3.2f" % (time.time() - t))

        if options.dump_sql:
            print("-- query starts --")
            print(q)
            print("-- query ends --")


        return True, pd.concat([df, df2])


def get_one_file(dbfname):
    print("Using sqlite3db file %s" % dbfname)

    df = None
    with Query2(dbfname) as q:
        if not options.no_recreate_indexes:
            q.create_indexes()
        ok, df = q.get_clstats()
        if not ok:
            return False, None

        if options.verbose:
            print("Printing head:")
            print(df.head())
            print("Print head done.")

    return True, df


class Classify:
    def __init__(self, df):
        self.features = df.columns.values.flatten().tolist()

        toremove = ["cl.decision_level_hist",
                    "cl.backtrack_level_hist",
                    "cl.trail_depth_level_hist",
                    "cl.vsids_vars_hist",
                    "cl.size_hist",
                    "cl.glue_hist",
                    "cl.num_antecedents_hist",
                    "cl.decision_level",
                    "cl.backtrack_level",
                    "good"]

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

        for t in toremove:
            if options.verbose:
                print("removing feature:", t)
            self.features.remove(t)

        if options.verbose:
            print("features:", self.features)
        print("Number of features:", len(self.features))

    def learn(self, df, cleanname, classifiername="classifier"):

        print("total samples: %5d" % df.shape[0])

        num_ok = df.loc[df['good'] == "OK"].shape[0]
        num_bad = df.loc[df['good'] == "BAD"].shape[0]
        if df.shape[0] > 0:
            perc_good = "%-3.4f" % (float(num_ok) / float(df.shape[0]) * 100.0)
        else:
            perc_good = "NaN"
        print("percentage of good ones: %s" % perc_good)

        if df.shape[0] == 0:
            return

        train, test = train_test_split(df, test_size=0.2, random_state=90)
        X_train = train[self.features]
        y_train = train["good"]
        X_test = test[self.features]
        y_test = test["good"]

        print("Training....")
        t = time.time()
        # self.clf = sklearn.KNeighborsClassifier(5) # EXPENSIVE at prediction, NOT suitable
        # self.clf = sklearn.linear_model.LogisticRegression() # NOT good.
        # self.clf = sklearn.ensemble.RandomForestClassifier(min_samples_split=len(X)/20, n_estimators=6)
        # self.clf = sklearn.svm.SVC(max_iter=1000) # can't make it work too well..
        self.clf = sklearn.tree.DecisionTreeClassifier(random_state=90, max_depth=options.tree_depth)
        self.clf.fit(X_train, y_train)
        print("Training finished. T: %-3.2f" % (time.time() - t))

        print("Calculating scores....")
        t = time.time()
        y_pred = self.clf.predict(X_test)

        # binarize the label OK/BAD
        lb = sklearn.preprocessing.LabelBinarizer()
        y_train = np.array([x[0] for x in lb.fit_transform(y_train)])
        y_test = np.array([x[0] for x in lb.fit_transform(y_test)])
        y_pred = np.array([x[0] for x in lb.fit_transform(y_pred)])

        # calculate accuracy/prec/recall for TEST
        accuracy = sklearn.metrics.accuracy_score(y_test, y_pred)
        precision = sklearn.metrics.precision_score(y_test, y_pred)
        recall = sklearn.metrics.recall_score(y_test, y_pred)
        print("prec: %-3.4f  recall: %-3.4f accuracy: %-3.4f T: %-3.2f" %
              (precision, recall, accuracy, (time.time() - t)))

        # calculate accuracy/prec/recall for cross-validation
        if options.cross_validate:
            accuracy = sklearn.model_selection.cross_val_score(self.clf, X_train, y_train, cv=10)
            precision = sklearn.model_selection.cross_val_score(self.clf, X_train, y_train, cv=10, scoring='precision')
            recall = sklearn.model_selection.cross_val_score(self.clf, X_train, y_train, cv=10, scoring='recall')
            print("cv-accuracy:", accuracy)
            print("cv-precision:", precision)
            print("cv-recall:", recall)
            accuracy = np.mean(accuracy)
            precision = np.mean(precision)
            recall = np.mean(recall)
            print("cv-prec: %-3.4f  cv-recall: %-3.4f cv-accuracy: %-3.4f T: %-3.2f" %
                  (precision, recall, accuracy, (time.time() - t)))

        # dump the classifier
        with open(classifiername, "wb") as f:
            pickle.dump(self.clf, f)

    def output_to_dot(self, fname):
        sklearn.tree.export_graphviz(self.clf, out_file=fname,
                                     feature_names=self.features,
                                     class_names=["BAD", "OK"],
                                     filled=True, rounded=True,
                                     special_characters=True,
                                     proportion=True
                                     )
        print("Run dot:")
        print("dot -Tpng {fname} -o {fname}.png".format(fname=fname))


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
    ok, df = get_one_file(dbfname)
    if not ok:
        return False, final_df

    cleanname = re.sub('\.cnf.gz.sqlite$', '', dbfname)

    if options.verbose:
        print("Describing----")
        dat = df.describe()
        print(dat)
        print("Describe done.---")
        print("Features: ", df.columns.values.flatten().tolist())

    df = transform(df)

    if options.verbose:
        print("Describing post-transform ----")
        print(df.describe())
        print("Describe done.---")

    if options.dump_csv:
        fname = "%s.csv" % cleanname
        print("Dumping CSV data to:", fname)
        df.to_csv(fname, index=False)

    if final_df is None:
        final_df = df
    else:
        final_df = pd.concat([final_df, df])

    if options.dump_csv:
        fname ="pandasdata.dat"
        print("Dumping pandas data to:", fname)
        with open(fname, "wb") as f:
            pickle.dump(df, f)

    # display
    if False:
        pd.options.display.mpl_style = "default"
        df.hist()
        df.boxplot()

    if options.check:
        check = Check(options.check)
        check.check(df)
    else:
        clf = Classify(df)
        clf.learn(df, "%s.classifier" % cleanname)
        clf.output_to_dot("%s.tree.dot" % cleanname)

    return True, final_df


if __name__ == "__main__":

    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--csv", action="store_true", default=False,
                      dest="dump_csv", help="Dump CSV (for weka)")

    parser.add_option("--cross", action="store_true", default=False,
                      dest="cross_validate", help="Cross-validate prec/recall/acc against training data")

    parser.add_option("--sql", action="store_true", default=False,
                      dest="dump_sql", help="Dump SQL query")

    parser.add_option("--fixed", default=-1, type=int,
                      dest="fixed_num_datapoints", help="Exact number of examples to take")

    parser.add_option("--depth", default=5, type=int,
                      dest="tree_depth", help="Depth of the tree to create")

    parser.add_option("--check", "-c", type=str,
                      dest="check", help="Check classifier")

    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes", help="Don't recreate indexes")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)

    final_df = None
    for dbfname in args:
        print("----- INTERMEDIATE predictor -------")
        ok, final_df = one_predictor(dbfname, final_df)

    # intermediate predictor is final
    if len(args) == 1:
        exit(0)

    # no need, we checked them all individually
    if options.check:
        exit(0)

    print("----- FINAL predictor -------")
    if options.check:
        check = Check()
        check.check(final_df)
    else:
        clf = Classify(final_df)
        clf.learn(final_df, "final.classifier")
        clf.output_to_dot("final.dot")

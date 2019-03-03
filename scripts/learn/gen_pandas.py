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
import os.path
import sys


class QueryHelper:
    def __init__(self, dbfname):
        if not os.path.isfile(dbfname):
            print("ERROR: Database file '%s' does not exist" % dbfname)
            exit(-1)

        self.conn = sqlite3.connect(dbfname)
        self.c = self.conn.cursor()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.conn.commit()
        self.conn.close()


class QueryFill (QueryHelper):
    def __init__(self, dbfname):
        super(QueryFill, self).__init__(dbfname)

    def create_indexes(self):
        print("Recreating indexes...")
        t = time.time()
        q = """
        drop index if exists `idxclid1`;
        drop index if exists `idxclid1-2`;
        drop index if exists `idxclid1-3`;
        drop index if exists `idxclid1-4`;
        drop index if exists `idxclid1-5`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;
        drop index if exists `idxclid4`;
        drop index if exists `idxclid5`;
        drop index if exists `idxclid6`;
        drop index if exists `idxclid6-2`;
        drop index if exists `idxclid6-3`;
        drop index if exists `idxclid6-4`;
        drop index if exists `idxclid7`;
        drop index if exists `idxclid8`;
        drop index if exists `idxclidUCLS-1`;
        drop index if exists `idxclidUCLS-2`;

        create index `idxclid1` on `clauseStats` (`clauseID`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid1-2` on `clauseStats` (`clauseID`);
        create index `idxclid1-3` on `clauseStats` (`clauseID`, restarts);
        create index `idxclid1-4` on `clauseStats` (`clauseID`, restarts, prev_restart);
        create index `idxclid1-5` on `clauseStats` (`clauseID`, prev_restart);
        create index `idxclid2` on `clauseStats` (clauseID, `prev_restart`, conflicts, restarts, latest_satzilla_feature_calc);
        create index `idxclid3` on `goodClauses` (`clauseID`);
        create index `idxclid4` on `restart` ( `restarts`);
        create index `idxclid5` on `tags` ( `tagname`);
        create index `idxclid6` on `reduceDB` (`clauseID`, conflicts, latest_satzilla_feature_calc);
        create index `idxclid6-2` on `reduceDB` (`clauseID`, `dump_no`);
        create index `idxclid6-3` on `reduceDB` (`clauseID`, `conflicts`, `dump_no`);
        create index `idxclid6-4` on `reduceDB` (`clauseID`, `conflicts`)
        create index `idxclid7` on `satzilla_features` (`latest_satzilla_feature_calc`);
        create index `idxclid8` on `varData` ( `var`, `conflicts`, `clid_start_incl`, `clid_end_notincl`);
        create index `idxclidUCLS-1` on `usedClauses` ( `clauseID`, `used_at`);
        create index `idxclidUCLS-2` on `usedClauses` ( `used_at`);
        """
        for l in q.split('\n'):
            t2 = time.time()

            if options.verbose:
                print("Creating index: ", l)
            self.c.execute(l)
            if options.verbose:
                print("Index creation T: %-3.2f s" % (time.time() - t2))

        print("indexes created T: %-3.2f s" % (time.time() - t))

    def fill_last_prop(self):
        print("Adding last prop...")
        t = time.time()
        q = """
        update goodClauses
        set last_prop_used =
        (select max(conflicts)
            from reduceDB
            where reduceDB.clauseID = goodClauses.clauseID
                and reduceDB.propagations_made > 0
        );
        """
        self.c.execute(q)
        print("last_prop_used filled T: %-3.2f s" % (time.time() - t))

    def fill_good_clauses_fixed(self):
        print("Filling good clauses fixed...")

        t = time.time()
        q = """DROP TABLE IF EXISTS `goodClausesFixed`;"""
        self.c.execute(q)
        q = """
        create table `goodClausesFixed` (
            `clauseID` bigint(20) NOT NULL,
            `num_used` bigint(20) NOT NULL,
            `first_confl_used` bigint(20),
            `last_confl_used` bigint(20),
            `sum_hist_used` bigint(20) DEFAULT NULL,
            `avg_hist_used` double,
            `var_hist_used` double,
            `last_prop_used` bigint(20) DEFAULT NULL
        );"""
        self.c.execute(q)
        print("goodClausesFixed recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """insert into goodClausesFixed
        (
        `clauseID`,
        `num_used`,
        `first_confl_used`,
        `last_confl_used`,
        `sum_hist_used`,
        `avg_hist_used`,
        `last_prop_used`
        )
        select
        clauseID
        , sum(num_used)
        , min(first_confl_used)
        , max(last_confl_used)
        , sum(sum_hist_used)
        , (1.0*sum(sum_hist_used))/(1.0*sum(num_used))
        , max(last_prop_used)
        from goodClauses as c group by clauseID;"""
        self.c.execute(q)
        print("goodClausesFixed filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        drop index if exists `idxclid20`;
        drop index if exists `idxclid21`;
        drop index if exists `idxclid21-2`;
        drop index if exists `idxclid22`;

        create index `idxclid20` on `goodClausesFixed` (`clauseID`, first_confl_used, last_confl_used, num_used, avg_hist_used);
        create index `idxclid21` on `goodClausesFixed` (`clauseID`);
        create index `idxclid21-2` on `goodClausesFixed` (`clauseID`, avg_hist_used);
        create index `idxclid22` on `goodClausesFixed` (`clauseID`, last_confl_used);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("goodClausesFixed indexes added T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """update goodClausesFixed
        set `var_hist_used` = (
        select
        sum(1.0*(used_at-cs.conflicts-avg_hist_used)*(used_at-cs.conflicts-avg_hist_used))/(num_used*1.0)
        from
        clauseStats as cs,
        usedClauses as u
        where goodClausesFixed.clauseID = u.clauseID
        and cs.clauseID = u.clauseID
        group by u.clauseID );
        """
        self.c.execute(q)
        print("goodClausesFixed added variance T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""
        insert into goodClausesFixed
        (
        `clauseID`,
        `num_used`,
        `first_confl_used`,
        `last_confl_used`,
        `sum_hist_used`,
        `avg_hist_used`,
        `last_prop_used`
        )
        select cl.clauseID,
        0,     --  `num_used`,
        NULL,   --  `first_confl_used`,
        NULL,   --  `last_confl_used`,
        0,      --  `sum_hist_used`,
        NULL,   --  `avg_hist_used`,
        NULL   --  `last_prop_used`
        from clauseStats as cl left join goodClausesFixed as goodcl
        on cl.clauseID = goodcl.clauseID
        where
        goodcl.clauseID is NULL
        and cl.clauseID != 0;
        """
        self.c.execute(q)
        print("goodClausesFixed added bad claues T: %-3.2f s" % (time.time() - t))

    def fill_var_data_use(self):
        print("Filling var data use...")

        t = time.time()
        q = "delete from `varDataUse`;"
        self.c.execute(q)
        print("varDataUse deleted T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        insert into varDataUse
        select
        v.restarts
        , v.conflicts

        -- data about var
        , v.var
        , v.dec_depth
        , v.decisions_below
        , v.conflicts_below
        , v.clauses_below

        , (v.decided*1.0)/(v.sum_decisions_at_picktime*1.0)
        , (v.decided_pos*1.0)/(v.decided*1.0)
        , (v.propagated*1.0)/(v.sum_propagations_at_picktime*1.0)
        , (v.propagated_pos*1.0)/(v.propagated*1.0)

        , v.decided
        , v.decided_pos
        , v.propagated
        , v.propagated_pos

        , v.sum_decisions_at_picktime
        , v.sum_propagations_at_picktime

        , v.total_conflicts_below_when_picked
        , v.total_decisions_below_when_picked
        , v.avg_inside_per_confl_when_picked
        , v.avg_inside_antecedents_when_picked

        -- measures for good
        , count(cls.num_used) as useful_clauses
        , sum(cls.num_used) as useful_clauses_used
        , sum(cls.sum_hist_used) as useful_clauses_sum_hist_used
        , min(cls.first_confl_used) as useful_clauses_first_used
        , max(cls.last_confl_used) as useful_clauses_last_used

        FROM varData as v join goodClausesFixed as cls
        on cls.clauseID >= v.clid_start_incl
        and cls.clauseID < v.clid_end_notincl

        -- avoid division by zero below
        where
        v.propagated > 0
        and v.sum_propagations_at_picktime > 0
        and v.decided > 0
        and v.sum_decisions_at_picktime > 0
        group by var, conflicts
        ;
        """
        if options.verbose:
            print("query:", q)
        self.c.execute(q)

        q = """
        UPDATE varDataUse SET useful_clauses_used = 0
        WHERE useful_clauses_used IS NULL
        """
        self.c.execute(q)

        q = """
        UPDATE varDataUse SET useful_clauses_first_used = 0
        WHERE useful_clauses_first_used IS NULL
        """
        self.c.execute(q)

        q = """
        UPDATE varDataUse SET useful_clauses_last_used = 0
        WHERE useful_clauses_last_used IS NULL
        """
        self.c.execute(q)

        print("varDataUse filled T: %-3.2f s" % (time.time() - t))

    def fill_later_useful_data(self):
        t = time.time()
        q = """
        DROP TABLE IF EXISTS `used_later`;
        DROP TABLE IF EXISTS `used_later10k`;
        DROP TABLE IF EXISTS `used_later100k`;
        DROP TABLE IF EXISTS `usedlater`;
        DROP TABLE IF EXISTS `usedlater10k`;
        DROP TABLE IF EXISTS `usedlater100k`;
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later dropped T: %-3.2f s" % (time.time() - t))

        q = """
        create table `used_later` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later` bigint(20)
        );"""
        self.c.execute(q)
        print("used_later recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        create table `used_later10k` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later10k` bigint(20)
        );"""
        self.c.execute(q)
        print("used_later10k recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        create table `used_later100k` (
            `clauseID` bigint(20) NOT NULL,
            `rdb0conflicts` bigint(20) NOT NULL,
            `used_later100k` bigint(20)
        );"""
        self.c.execute(q)
        print("used_later100k recreated T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""insert into used_later
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl.used_at) as `useful_later`
        FROM
        reduceDB as rdb0
        left join usedClauses as ucl

        -- for any point later than now
        on (ucl.clauseID = rdb0.clauseID
            and ucl.used_at > rdb0.conflicts)

        WHERE
        rdb0.clauseID != 0

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""
        insert into used_later10k
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later10k`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl10k.used_at) as `used_later10k`

        FROM
        reduceDB as rdb0
        left join usedClauses as ucl10k
        on (ucl10k.clauseID = rdb0.clauseID
            and ucl10k.used_at > rdb0.conflicts
            and ucl10k.used_at <= (rdb0.conflicts+10000))

        WHERE
        rdb0.clauseID != 0

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later10k filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q="""
        insert into used_later100k
        (
        `clauseID`,
        `rdb0conflicts`,
        `used_later100k`
        )
        SELECT
        rdb0.clauseID
        , rdb0.conflicts
        , count(ucl100k.used_at) as `used_later100k`

        FROM
        reduceDB as rdb0
        left join usedClauses as ucl100k
        on (ucl100k.clauseID = rdb0.clauseID
            and ucl100k.used_at > rdb0.conflicts
            and ucl100k.used_at <= (rdb0.conflicts+100000))

        WHERE
        rdb0.clauseID != 0

        group by rdb0.clauseID, rdb0.conflicts;"""
        self.c.execute(q)
        print("used_later100k filled T: %-3.2f s" % (time.time() - t))


        t = time.time()
        q = """
        drop index if exists `used_later_idx1`;
        drop index if exists `used_later_idx2`;
        drop index if exists `used_later10k_idx1`;
        drop index if exists `used_later10k_idx2`;
        drop index if exists `used_later100k_idx1`;
        drop index if exists `used_later100k_idx2`;

        create index `used_later_idx1` on `used_later` (`clauseID`, rdb0conflicts);
        create index `used_later_idx2` on `used_later` (`clauseID`, rdb0conflicts, used_later);
        create index `used_later10k_idx1` on `used_later10k` (`clauseID`, rdb0conflicts);
        create index `used_later10k_idx2` on `used_later10k` (`clauseID`, rdb0conflicts, used_later10k);
        create index `used_later100k_idx1` on `used_later100k` (`clauseID`, rdb0conflicts);
        create index `used_later100k_idx2` on `used_later100k` (`clauseID`, rdb0conflicts, used_later100k);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("used_later indexes added T: %-3.2f s" % (time.time() - t))


class QueryCls (QueryHelper):
    def __init__(self, dbfname, conf):
        super(QueryCls, self).__init__(dbfname)
        self.conf = conf

        self.goodcls = """
        , goodcl.`clauseID` as `goodcl.clauseID`
        , goodcl.`num_used` as `goodcl.num_used`
        , goodcl.`first_confl_used` as `goodcl.first_confl_used`
        , goodcl.`last_confl_used` as `goodcl.last_confl_used`
        , goodcl.`sum_hist_used` as `goodcl.sum_hist_used`
        , goodcl.`avg_hist_used` as `goodcl.avg_hist_used`
        , goodcl.`var_hist_used` as `goodcl.var_hist_used`
        , goodcl.`last_prop_used` as `goodcl.last_prop_used`
    """

        # partially done with tablestruct_sql and SED: sed -e 's/`\(.*\)`.*/rst.`\1` as `rst.\1`/' ../tmp.txt
        self.restart_dat = """
        -- , rst.`simplifications` as `rst.simplifications`
        -- , rst.`restarts` as `rst.restarts`
        -- , rst.`conflicts` as `rst.conflicts`
        -- , rst.`latest_satzilla_feature_calc` as `rst.latest_satzilla_feature_calc`
        -- rst.`runtime` as `rst.runtime`
        , rst.`numIrredBins` as `rst.numIrredBins`
        , rst.`numIrredLongs` as `rst.numIrredLongs`
        , rst.`numRedBins` as `rst.numRedBins`
        , rst.`numRedLongs` as `rst.numRedLongs`
        , rst.`numIrredLits` as `rst.numIrredLits`
        , rst.`numredLits` as `rst.numredLits`
        , rst.`glue` as `rst.glue`
        , rst.`glueSD` as `rst.glueSD`
        , rst.`glueMin` as `rst.glueMin`
        , rst.`glueMax` as `rst.glueMax`
        , rst.`size` as `rst.size`
        , rst.`sizeSD` as `rst.sizeSD`
        , rst.`sizeMin` as `rst.sizeMin`
        , rst.`sizeMax` as `rst.sizeMax`
        , rst.`resolutions` as `rst.resolutions`
        , rst.`resolutionsSD` as `rst.resolutionsSD`
        , rst.`resolutionsMin` as `rst.resolutionsMin`
        , rst.`resolutionsMax` as `rst.resolutionsMax`
        , rst.`branchDepth` as `rst.branchDepth`
        , rst.`branchDepthSD` as `rst.branchDepthSD`
        , rst.`branchDepthMin` as `rst.branchDepthMin`
        , rst.`branchDepthMax` as `rst.branchDepthMax`
        , rst.`branchDepthDelta` as `rst.branchDepthDelta`
        , rst.`branchDepthDeltaSD` as `rst.branchDepthDeltaSD`
        , rst.`branchDepthDeltaMin` as `rst.branchDepthDeltaMin`
        , rst.`branchDepthDeltaMax` as `rst.branchDepthDeltaMax`
        , rst.`trailDepth` as `rst.trailDepth`
        , rst.`trailDepthSD` as `rst.trailDepthSD`
        , rst.`trailDepthMin` as `rst.trailDepthMin`
        , rst.`trailDepthMax` as `rst.trailDepthMax`
        , rst.`trailDepthDelta` as `rst.trailDepthDelta`
        , rst.`trailDepthDeltaSD` as `rst.trailDepthDeltaSD`
        , rst.`trailDepthDeltaMin` as `rst.trailDepthDeltaMin`
        , rst.`trailDepthDeltaMax` as `rst.trailDepthDeltaMax`
        , rst.`propBinIrred` as `rst.propBinIrred`
        , rst.`propBinRed` as `rst.propBinRed`
        , rst.`propLongIrred` as `rst.propLongIrred`
        , rst.`propLongRed` as `rst.propLongRed`
        , rst.`conflBinIrred` as `rst.conflBinIrred`
        , rst.`conflBinRed` as `rst.conflBinRed`
        , rst.`conflLongIrred` as `rst.conflLongIrred`
        , rst.`conflLongRed` as `rst.conflLongRed`
        , rst.`learntUnits` as `rst.learntUnits`
        , rst.`learntBins` as `rst.learntBins`
        , rst.`learntLongs` as `rst.learntLongs`
        , rst.`resolBinIrred` as `rst.resolBinIrred`
        , rst.`resolBinRed` as `rst.resolBinRed`
        , rst.`resolLIrred` as `rst.resolLIrred`
        , rst.`resolLRed` as `rst.resolLRed`
        -- , rst.`propagations` as `rst.propagations`
        -- , rst.`decisions` as `rst.decisions`
        -- , rst.`flipped` as `rst.flipped`
        , rst.`varSetPos` as `rst.varSetPos`
        , rst.`varSetNeg` as `rst.varSetNeg`
        , rst.`free` as `rst.free`
        -- , rst.`replaced` as `rst.replaced`
        -- , rst.`eliminated` as `rst.eliminated`
        -- , rst.`set` as `rst.set`
        -- , rst.`clauseIDstartInclusive` as `rst.clauseIDstartInclusive`
        -- , rst.`clauseIDendExclusive` as `rst.clauseIDendExclusive`
        """

        self.rdb0_dat = """
        -- , rdb0.`simplifications` as `rdb0.simplifications`
        -- , rdb0.`restarts` as `rdb0.restarts`
        , rdb0.`conflicts` as `rdb0.conflicts`
        , rdb0.`cur_restart_type` as `rdb0.cur_restart_type`
        -- , rdb0.`runtime` as `rdb0.runtime`

        -- , rdb0.`clauseID` as `rdb0.clauseID`
        , rdb0.`dump_no` as `rdb0.dump_no`
        , rdb0.`conflicts_made` as `rdb0.conflicts_made`
        , rdb0.`sum_of_branch_depth_conflict` as `rdb0.sum_of_branch_depth_conflict`
        , rdb0.`propagations_made` as `rdb0.propagations_made`
        , rdb0.`clause_looked_at` as `rdb0.clause_looked_at`
        , rdb0.`used_for_uip_creation` as `rdb0.used_for_uip_creation`
        , rdb0.`last_touched_diff` as `rdb0.last_touched_diff`
        , rdb0.`activity_rel` as `rdb0.activity_rel`
        , rdb0.`locked` as `rdb0.locked`
        , rdb0.`in_xor` as `rdb0.in_xor`
        -- , rdb0.`glue` as `rdb0.glue`
        -- , rdb0.`size` as `rdb0.size`
        , rdb0.`ttl` as `rdb0.ttl`
        , rdb0.`act_ranking_top_10` as `rdb0.act_ranking_top_10`
        , rdb0.`act_ranking` as `rdb0.act_ranking`
        , rdb0.`sum_uip1_used` as `rdb0.sum_uip1_used`
        , rdb0.`sum_delta_confl_uip1_used` as `rdb0.sum_delta_confl_uip1_used`
        """

        self.clause_dat = """
        -- , cl.`simplifications` as `cl.simplifications`
        -- , cl.`restarts` as `cl.restarts`
        -- , cl.`prev_restart` as `cl.prev_restart`
        , cl.`conflicts` as `cl.conflicts`
        -- , cl.`latest_satzilla_feature_calc` as `cl.latest_satzilla_feature_calc`
        -- , cl.`clauseID` as `cl.clauseID`
        , cl.`glue` as `cl.glue`
        , cl.`size` as `cl.size`
        , cl.`conflicts_this_restart` as `cl.conflicts_this_restart`
        , cl.`num_overlap_literals` as `cl.num_overlap_literals`
        , cl.`num_antecedents` as `cl.num_antecedents`
        , cl.`num_total_lits_antecedents` as `cl.num_total_lits_antecedents`
        , cl.`antecedents_avg_size` as `cl.antecedents_avg_size`
        , cl.`backtrack_level` as `cl.backtrack_level`
        , cl.`decision_level` as `cl.decision_level`
        , cl.`decision_level_pre1` as `cl.decision_level_pre1`
        , cl.`decision_level_pre2` as `cl.decision_level_pre2`
        , cl.`trail_depth_level` as `cl.trail_depth_level`
        , cl.`cur_restart_type` as `cl.cur_restart_type`
        , cl.`atedecents_binIrred` as `cl.atedecents_binIrred`
        , cl.`atedecents_binRed` as `cl.atedecents_binRed`
        , cl.`atedecents_longIrred` as `cl.atedecents_longIrred`
        , cl.`atedecents_longRed` as `cl.atedecents_longRed`
        -- , cl.`vsids_vars_avg` as `cl.vsids_vars_avg`
        -- , cl.`vsids_vars_var` as `cl.vsids_vars_var`
        -- , cl.`vsids_vars_min` as `cl.vsids_vars_min`
        -- , cl.`vsids_vars_max` as `cl.vsids_vars_max`
        , cl.`antecedents_glue_long_reds_avg` as `cl.antecedents_glue_long_reds_avg`
        , cl.`antecedents_glue_long_reds_var` as `cl.antecedents_glue_long_reds_var`
        , cl.`antecedents_glue_long_reds_min` as `cl.antecedents_glue_long_reds_min`
        , cl.`antecedents_glue_long_reds_max` as `cl.antecedents_glue_long_reds_max`
        , cl.`antecedents_long_red_age_avg` as `cl.antecedents_long_red_age_avg`
        , cl.`antecedents_long_red_age_var` as `cl.antecedents_long_red_age_var`
        , cl.`antecedents_long_red_age_min` as `cl.antecedents_long_red_age_min`
        , cl.`antecedents_long_red_age_max` as `cl.antecedents_long_red_age_max`
        -- , cl.`vsids_of_resolving_literals_avg` as `cl.vsids_of_resolving_literals_avg`
        -- , cl.`vsids_of_resolving_literals_var` as `cl.vsids_of_resolving_literals_var`
        -- , cl.`vsids_of_resolving_literals_min` as `cl.vsids_of_resolving_literals_min`
        -- , cl.`vsids_of_resolving_literals_max` as `cl.vsids_of_resolving_literals_max`
        -- , cl.`vsids_of_all_incoming_lits_avg` as `cl.vsids_of_all_incoming_lits_avg`
        -- , cl.`vsids_of_all_incoming_lits_var` as `cl.vsids_of_all_incoming_lits_var`
        -- , cl.`vsids_of_all_incoming_lits_min` as `cl.vsids_of_all_incoming_lits_min`
        -- , cl.`vsids_of_all_incoming_lits_max` as `cl.vsids_of_all_incoming_lits_max`
        -- , cl.`antecedents_antecedents_vsids_avg` as `cl.antecedents_antecedents_vsids_avg`
        , cl.`decision_level_hist` as `cl.decision_level_hist`
        , cl.`backtrack_level_hist_lt` as `cl.backtrack_level_hist_lt`
        , cl.`trail_depth_level_hist` as `cl.trail_depth_level_hist`
        -- , cl.`vsids_vars_hist` as `cl.vsids_vars_hist`
        , cl.`size_hist` as `cl.size_hist`
        , cl.`glue_hist` as `cl.glue_hist`
        , cl.`num_antecedents_hist` as `cl.num_antecedents_hist`
        , cl.`antec_sum_size_hist` as `cl.antec_sum_size_hist`
        , cl.`antec_overlap_hist` as `cl.antec_overlap_hist`

        , cl.`branch_depth_hist_queue` as `cl.branch_depth_hist_queue`
        , cl.`trail_depth_hist` as `cl.trail_depth_hist`
        , cl.`trail_depth_hist_longer` as `cl.trail_depth_hist_longer`
        , cl.`num_resolutions_hist` as `cl.num_resolutions_hist`
        , cl.`confl_size_hist` as `cl.confl_size_hist`
        , cl.`trail_depth_delta_hist` as `cl.trail_depth_delta_hist`
        , cl.`backtrack_level_hist` as `cl.backtrack_level_hist`
        , cl.`glue_hist_queue` as `cl.glue_hist_queue`
        , cl.`glue_hist_long` as `cl.glue_hist_long`
        """

        self.satzfeat_dat = """
        -- , szfeat.`simplifications` as `szfeat.simplifications`
        -- , szfeat.`restarts` as `szfeat.restarts`
        , szfeat.`conflicts` as `szfeat.conflicts`
        -- , szfeat.`latest_satzilla_feature_calc` as `szfeat.latest_satzilla_feature_calc`
        , szfeat.`numVars` as `szfeat.numVars`
        , szfeat.`numClauses` as `szfeat.numClauses`
        , szfeat.`var_cl_ratio` as `szfeat.var_cl_ratio`
        , szfeat.`binary` as `szfeat.binary`
        , szfeat.`horn` as `szfeat.horn`
        , szfeat.`horn_mean` as `szfeat.horn_mean`
        , szfeat.`horn_std` as `szfeat.horn_std`
        , szfeat.`horn_min` as `szfeat.horn_min`
        , szfeat.`horn_max` as `szfeat.horn_max`
        , szfeat.`horn_spread` as `szfeat.horn_spread`
        , szfeat.`vcg_var_mean` as `szfeat.vcg_var_mean`
        , szfeat.`vcg_var_std` as `szfeat.vcg_var_std`
        , szfeat.`vcg_var_min` as `szfeat.vcg_var_min`
        , szfeat.`vcg_var_max` as `szfeat.vcg_var_max`
        , szfeat.`vcg_var_spread` as `szfeat.vcg_var_spread`
        , szfeat.`vcg_cls_mean` as `szfeat.vcg_cls_mean`
        , szfeat.`vcg_cls_std` as `szfeat.vcg_cls_std`
        , szfeat.`vcg_cls_min` as `szfeat.vcg_cls_min`
        , szfeat.`vcg_cls_max` as `szfeat.vcg_cls_max`
        , szfeat.`vcg_cls_spread` as `szfeat.vcg_cls_spread`
        , szfeat.`pnr_var_mean` as `szfeat.pnr_var_mean`
        , szfeat.`pnr_var_std` as `szfeat.pnr_var_std`
        , szfeat.`pnr_var_min` as `szfeat.pnr_var_min`
        , szfeat.`pnr_var_max` as `szfeat.pnr_var_max`
        , szfeat.`pnr_var_spread` as `szfeat.pnr_var_spread`
        , szfeat.`pnr_cls_mean` as `szfeat.pnr_cls_mean`
        , szfeat.`pnr_cls_std` as `szfeat.pnr_cls_std`
        , szfeat.`pnr_cls_min` as `szfeat.pnr_cls_min`
        , szfeat.`pnr_cls_max` as `szfeat.pnr_cls_max`
        , szfeat.`pnr_cls_spread` as `szfeat.pnr_cls_spread`
        , szfeat.`avg_confl_size` as `szfeat.avg_confl_size`
        , szfeat.`confl_size_min` as `szfeat.confl_size_min`
        , szfeat.`confl_size_max` as `szfeat.confl_size_max`
        , szfeat.`avg_confl_glue` as `szfeat.avg_confl_glue`
        , szfeat.`confl_glue_min` as `szfeat.confl_glue_min`
        , szfeat.`confl_glue_max` as `szfeat.confl_glue_max`
        , szfeat.`avg_num_resolutions` as `szfeat.avg_num_resolutions`
        , szfeat.`num_resolutions_min` as `szfeat.num_resolutions_min`
        , szfeat.`num_resolutions_max` as `szfeat.num_resolutions_max`
        , szfeat.`learnt_bins_per_confl` as `szfeat.learnt_bins_per_confl`
        , szfeat.`avg_branch_depth` as `szfeat.avg_branch_depth`
        , szfeat.`branch_depth_min` as `szfeat.branch_depth_min`
        , szfeat.`branch_depth_max` as `szfeat.branch_depth_max`
        , szfeat.`avg_trail_depth_delta` as `szfeat.avg_trail_depth_delta`
        , szfeat.`trail_depth_delta_min` as `szfeat.trail_depth_delta_min`
        , szfeat.`trail_depth_delta_max` as `szfeat.trail_depth_delta_max`
        , szfeat.`avg_branch_depth_delta` as `szfeat.avg_branch_depth_delta`
        , szfeat.`props_per_confl` as `szfeat.props_per_confl`
        , szfeat.`confl_per_restart` as `szfeat.confl_per_restart`
        , szfeat.`decisions_per_conflict` as `szfeat.decisions_per_conflict`
        , szfeat.`red_glue_distr_mean` as `szfeat.red_glue_distr_mean`
        , szfeat.`red_glue_distr_var` as `szfeat.red_glue_distr_var`
        , szfeat.`red_size_distr_mean` as `szfeat.red_size_distr_mean`
        , szfeat.`red_size_distr_var` as `szfeat.red_size_distr_var`
        -- , szfeat.`red_activity_distr_mean` as `szfeat.red_activity_distr_mean`
        -- , szfeat.`red_activity_distr_var` as `szfeat.red_activity_distr_var`
        -- , szfeat.`irred_glue_distr_mean` as `szfeat.irred_glue_distr_mean`
        -- , szfeat.`irred_glue_distr_var` as `szfeat.irred_glue_distr_var`
        , szfeat.`irred_size_distr_mean` as `szfeat.irred_size_distr_mean`
        , szfeat.`irred_size_distr_var` as `szfeat.irred_size_distr_var`
        -- , szfeat.`irred_activity_distr_mean` as `szfeat.irred_activity_distr_mean`
        -- , szfeat.`irred_activity_distr_var` as `szfeat.irred_activity_distr_var`
        """

        self.common_limits = """
        order by random()
        limit {limit}
        """

        # TODO magic queries
        if self.conf == 0:
            self.case_stmt_10k = """
            CASE WHEN

            goodcl.last_confl_used > rdb0.conflicts and
            (
                -- useful in the next round
                   used_later10k.used_later10k > 3

                   or
                   (used_later10k.used_later10k > 2 and used_later100k.used_later100k > 40)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 1:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k > 5
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k >= max(cast({avg_used_later10k}+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 3:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k >= max(cast({avg_used_later10k}/2+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 4:
            self.case_stmt_10k = """
            CASE WHEN

            -- useful in the next round
                   used_later10k.used_later10k >= max(cast({avg_used_later10k}*1.5+0.5 as int),1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """

        if self.conf == 0:
            self.case_stmt_100k = """
            CASE WHEN

            goodcl.last_confl_used > rdb0.conflicts+100000 and
            (
                -- used a lot over a wide range
                   (used_later100k.used_later100k > 10 and used_later.used_later > 20)

                -- used quite a bit but less dispersion
                or (used_later100k.used_later100k > 6 and used_later.used_later > 30)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 1:
            self.case_stmt_100k = """
            CASE WHEN

            goodcl.last_confl_used > rdb0.conflicts+100000 and
            (
                -- used a lot over a wide range
                   (used_later100k.used_later100k > 13 and used_later.used_later > 24)

                -- used quite a bit but less dispersion
                or (used_later100k.used_later100k > 8 and used_later.used_later > 40)
            )
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 2:
            self.case_stmt_100k = """
            CASE WHEN

           -- useful in the next round
               used_later100k.used_later100k >= max(cast({avg_used_later100k}+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 3:
            self.case_stmt_100k = """
            CASE WHEN

           -- useful in the next round
               used_later100k.used_later100k >= max(cast({avg_used_later100k}/2+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """
        elif self.conf == 4:
            self.case_stmt_100k = """
            CASE WHEN

           -- useful in the next round
               used_later100k.used_later100k >= max(cast({avg_used_later100k}*1.5+0.5 as int), 1)
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
            """

        self.dump_no_larger_than_zero = """
        and rdb0.dump_no > 0
        """

        # GOOD clauses
        self.q_ok_select = """
        SELECT
        tags.tag as `fname`
        {clause_dat}
        {restart_dat}
        {satzfeat_dat_cur}
        {rdb0_dat}
        {rdb1_dat}
        {goodcls}
        , goodcl.num_used as `x.num_used`
        , `goodcl`.`last_confl_used`-`cl`.`conflicts` as `x.lifetime`
        , {case_stmt}
        """

        self.q_ok = """
        FROM
        clauseStats as cl
        , goodClausesFixed as goodcl
        , restart as rst
        , satzilla_features as szfeat_cur
        , reduceDB as rdb0
        , reduceDB as rdb1
        , tags
        , used_later
        , used_later10k
        , used_later100k
        WHERE

        cl.clauseID = goodcl.clauseID
        and cl.clauseID != 0
        and used_later.clauseID = cl.clauseID
        and used_later.rdb0conflicts = rdb0.conflicts

        and rdb0.clauseID = cl.clauseID
        and rdb1.clauseID = cl.clauseID
        and rdb0.dump_no = rdb1.dump_no+1

        and used_later10k.clauseID = cl.clauseID
        and used_later10k.rdb0conflicts = rdb0.conflicts

        and used_later100k.clauseID = cl.clauseID
        and used_later100k.rdb0conflicts = rdb0.conflicts

        and cl.restarts > 1 -- to avoid history being invalid
        and szfeat_cur.latest_satzilla_feature_calc = rdb0.latest_satzilla_feature_calc
        and rst.restarts = cl.prev_restart
        and tags.tagname = "filename"
        """

        self.myformat = {
            "limit": 1000*1000*1000,
            "restart_dat": self.restart_dat,
            "clause_dat": self.clause_dat,
            "satzfeat_dat_cur": self.satzfeat_dat.replace("szfeat.", "szfeat_cur."),
            "rdb0_dat": self.rdb0_dat,
            "rdb1_dat": self.rdb0_dat.replace("rdb0", "rdb1"),
            "goodcls": self.goodcls
            }

    def add_dump_no_filter(self, q):
        q += self.dump_no_larger_than_zero
        return q

    def get_ok(self):
        t = time.time()

        # calc OK -> which can be both BAD and OK
        q = self.q_count + self.q_ok
        q = self.add_dump_no_filter(q)
        q = q.format(**self.myformat)
        if options.dump_sql:
            print("query:\n %s" % q)
        cur = self.conn.execute(q.format(**self.myformat))
        num_lines = int(cur.fetchone()[0])
        print("Num datapoints (K): %-3.5f T: %-3.1f" % ((num_lines/1000.0), time.time()-t))
        return num_lines

    def get_avg_used_later(self, long_or_short):
        cur = self.conn.cursor()
        if long_or_short == "short":
            q = "select avg(used_later10k) from used_later10k, used_later where used_later.clauseID = used_later10k.clauseID and used_later > 0;"
        else:
            q = "select avg(used_later100k) from used_later100k, used_later where used_later.clauseID = used_later100k.clauseID and used_later > 0;"
        cur.execute(q)
        rows = cur.fetchall()
        assert len(rows) == 1
        if rows[0][0] is None:
            return False, None
        avg = float(rows[0][0])
        print("%s avg used_later is: %.2f"  % (long_or_short, avg))
        return True, avg

    def one_query(self, q, ok_or_bad):
        q = q.format(**self.myformat)
        q = "select * from ( %s ) where `x.class`='%s' " % (q, ok_or_bad)
        q += self.common_limits
        q = q.format(**self.myformat)

        t = time.time()
        sys.stdout.write("Running query for %s..." % ok_or_bad)
        sys.stdout.flush()
        if options.dump_sql:
            print("query:", q)
        df = pd.read_sql_query(q, self.conn)
        print("T: %-3.1f" % (time.time() - t))
        return df

    def get_one_data_all_dumpnos(self, long_or_short):
        df = None

        ok, df, this_fixed = self.get_data(long_or_short)
        if not ok:
            return False, None

        if options.verbose:
            print("Printing head:")
            print(df.head())
            print("Print head done.")

        return True, df


    def get_data(self, long_or_short, this_fixed=None):
        # TODO magic numbers: SHORT vs LONG data availability guess
        subformat = {}
        ok, subformat["avg_used_later10k"] = self.get_avg_used_later("short");
        if not ok:
            return False, None, None

        ok, subformat["avg_used_later100k"] = self.get_avg_used_later("long");
        if not ok:
            return False, None, None

        if long_or_short == "short":
            self.myformat["case_stmt"] = self.case_stmt_10k.format(**subformat)
            fixed_mult = 1.0
        else:
            self.myformat["case_stmt"] = self.case_stmt_100k.format(**subformat)
            fixed_mult = 0.2

        print("Fixed multiplier set to  %s " % fixed_mult)

        t = time.time()
        if this_fixed is None:
            this_fixed = options.fixed
            this_fixed *= fixed_mult
        print("this_fixed is set to:", this_fixed)

        q = self.q_ok_select + self.q_ok
        q = self.add_dump_no_filter(q)
        self.myformat["limit"] = this_fixed

        #get OK data
        df_ok = self.one_query(q, "OK")
        print("size of data:", df_ok.shape)

        #get BAD data
        df_bad = self.one_query(q, "BAD")
        print("size of data:", df_bad.shape)

        print("Queries finished. T: %-3.2f" % (time.time() - t))
        return True, pd.concat([df_ok, df_bad]), this_fixed

class QueryVar (QueryHelper):
    def __init__(self, dbfname):
        super(QueryVar, self).__init__(dbfname)

    def vardata(self):
        q = """
select
*
, (1.0*useful_clauses)/(1.0*clauses_below) as useful_ratio

, CASE WHEN
 (1.0*useful_clauses)/(1.0*clauses_below) > 0.5
THEN "OK"
ELSE "BAD"
END AS `class`

from varDataUse
where
clauses_below > 10
and avg_inside_per_confl_when_picked > 0
"""

        df = pd.read_sql_query(q, self.conn)

        cleanname = re.sub(r'\.cnf.gz.sqlite$', '', dbfname)
        cleanname = re.sub(r'\.db$', '', dbfname)
        cleanname += "-vardata"
        dump_dataframe(df, cleanname)


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

    # relative overlaps
    print("Relative overlaps...")
    df["cl.num_overlap_literals_rel"] = df["cl.num_overlap_literals"]/df["cl.antec_overlap_hist"]
    df["cl.antec_num_total_lits_rel"] = df["cl.num_total_lits_antecedents"]/df["cl.antec_sum_size_hist"]
    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"]/df["cl.num_antecedents_hist"]
    df["rst.varset_neg_polar_ratio"] = df["rst.varSetNeg"]/(df["rst.varSetPos"]+df["rst.varSetNeg"])

    # relative RDB
    print("Relative RDB...")
    df["rdb.rel_conflicts_made"] = (df["rdb0.conflicts_made"] > df["rdb1.conflicts_made"]).astype(int)
    df["rdb.rel_propagations_made"] = (df["rdb0.propagations_made"] > df["rdb1.propagations_made"]).astype(int)
    df["rdb.rel_clause_looked_at"] = (df["rdb0.clause_looked_at"] > df["rdb1.clause_looked_at"]).astype(int)
    df["rdb.rel_used_for_uip_creation"] = (df["rdb0.used_for_uip_creation"] > df["rdb1.used_for_uip_creation"]).astype(int)
    df["rdb.rel_last_touched_diff"] = (df["rdb0.last_touched_diff"] > df["rdb1.last_touched_diff"]).astype(int)
    df["rdb.rel_activity_rel"] = (df["rdb0.activity_rel"] > df["rdb1.activity_rel"]).astype(int)

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    print("size/glue/trail rel...")
    df["cl.size_rel"] = df["cl.size"] / df["cl.size_hist"]
    df["cl.glue_rel_queue"] = df["cl.glue"] / df["cl.glue_hist_queue"]
    df["cl.glue_rel_long"] = df["cl.glue"] / df["cl.glue_hist_long"]
    df["cl.glue_rel"] = df["cl.glue"] / df["cl.glue_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"]/df["cl.trail_depth_level_hist"]
    df["cl.branch_depth_rel_queue"] = df["cl.decision_level"]/df["cl.branch_depth_hist_queue"]

    # smaller-than larger-than for glue and size
    print("smaller-than larger-than for glue and size...")
    df["cl.size_smaller_than_hist"] = (df["cl.size"] < df["cl.size_hist"]).astype(int)
    df["cl.glue_smaller_than_hist"] = (df["cl.glue"] < df["cl.glue_hist"]).astype(int)
    df["cl.glue_smaller_than_hist_lt"] = (df["cl.glue"] < df["cl.glue_hist_long"]).astype(int)
    df["cl.glue_smaller_than_hist_queue"] = (df["cl.glue"] < df["cl.glue_hist_queue"]).astype(int)

    # relative decisions
    print("relative decisions...")
    df["cl.decision_level_rel"] = df["cl.decision_level"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre1_rel"] = df["cl.decision_level_pre1"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"]/df["cl.decision_level_hist"]

    # relative props
    print("relative props...")
    df["rst.all_props"] = df["rst.propBinRed"] + df["rst.propBinIrred"] + df["rst.propLongRed"] + df["rst.propLongIrred"]
    df["rst.propBinRed_ratio"] = df["rst.propBinRed"]/df["rst.all_props"]
    df["rst.propBinIrred_ratio"] = df["rst.propBinIrred"]/df["rst.all_props"]
    df["rst.propLongRed_ratio"] = df["rst.propLongRed"]/df["rst.all_props"]
    df["rst.propLongIrred_ratio"] = df["rst.propLongIrred"]/df["rst.all_props"]

    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"]/df["rst.free"]

    # relative resolutions
    print("relative resolutions...")
    df["rst.resolBinIrred_ratio"] = df["rst.resolBinIrred"]/df["rst.resolutions"]
    df["rst.resolBinRed_ratio"] = df["rst.resolBinRed"]/df["rst.resolutions"]
    df["rst.resolLRed_ratio"] = df["rst.resolLRed"]/df["rst.resolutions"]
    df["rst.resolLIrred_ratio"] = df["rst.resolLIrred"]/df["rst.resolutions"]

    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"] / df["cl.num_antecedents_hist"]
    df["cl.decision_level_rel"] = df["cl.decision_level"] / df["cl.decision_level_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"] / df["cl.trail_depth_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"] / df["cl.backtrack_level_hist"]

    # smaller-or-greater comparisons
    print("smaller-or-greater comparisons...")
    df["cl.decision_level_smaller_than_hist"] = (df["cl.decision_level"] < df["cl.decision_level_hist"]).astype(int)
    df["cl.backtrack_level_smaller_than_hist"] = (df["cl.backtrack_level"] < df["cl.backtrack_level_hist"]).astype(int)
    df["cl.trail_depth_level_smaller_than_hist"] = (df["cl.trail_depth_level"] < df["cl.trail_depth_level_hist"]).astype(int)
    df["cl.num_antecedents_smaller_than_hist"] = (df["cl.num_antecedents"] < df["cl.num_antecedents_hist"]).astype(int)
    df["cl.antec_sum_size_smaller_than_hist"] = (df["cl.antec_sum_size_hist"] < df["cl.num_total_lits_antecedents"]).astype(int)
    df["cl.antec_overlap_smaller_than_hist"] = (df["cl.antec_overlap_hist"] < df["cl.num_overlap_literals"]).astype(int)
    df["cl.overlap_smaller_than_hist"] = (df["cl.num_overlap_literals"]<df["cl.antec_overlap_hist"]).astype(int)
    df["cl.branch_smaller_than_hist_queue"] = (df["cl.decision_level"]<df["cl.branch_depth_hist_queue"]).astype(int)

    # avg conf/used_per confl
    print("avg conf/used_per confl 1...")
    df["rdb0.avg_confl"] = df["rdb0.sum_uip1_used"]/df["rdb0.sum_delta_confl_uip1_used"]
    df["rdb0.avg_confl"].fillna(0, inplace=True)

    print("avg conf/used_per confl 2...")
    df["rdb0.used_per_confl"] = df["rdb0.sum_uip1_used"]/(df["rdb0.conflicts"] - df["cl.conflicts"])
    df["rdb0.used_per_confl"].fillna(0, inplace=True)

    print("flatten/list...")
    old = set(df.columns.values.flatten().tolist())
    df = df.dropna(how="all")
    new = set(df.columns.values.flatten().tolist())
    if len(old - new) > 0:
        print("ERROR: a NaN number turned up")
        print("columns: ", (old - new))
        assert(False)
        exit(-1)

    # making sure "x.class" is the last one
    new_no_class = list(new)
    new_no_class.remove("x.class")
    df = df[new_no_class + ["x.class"]]

    return df


def dump_dataframe(df, name):
    if options.dump_csv:
        fname = "%s.csv" % name
        print("Dumping CSV data to:", fname)
        df.to_csv(fname, index=False, columns=sorted(list(df)))

    fname = "%s.dat" % name
    print("Dumping pandas data to:", fname)
    with open(fname, "wb") as f:
        pickle.dump(df, f)


def one_database(dbfname):
    with QueryFill(dbfname) as q:
        if not options.no_recreate_indexes:
            q.create_indexes()
            q.fill_last_prop()
            q.fill_good_clauses_fixed()
            q.fill_later_useful_data()
            q.fill_var_data_use()

    # with QueryVar(dbfname) as q:
    #    q.vardata()

    match = re.match(r"^([0-9]*)-([0-9]*)$", options.confs)
    if not match:
        print("ERROR: we cannot parse your config options: '%s'" % options.confs)
        exit(-1)

    conf_from = int(match.group(1))
    conf_to = int(match.group(2))+1
    if conf_to <= conf_from:
        print("ERROR: Conf range is not increasing")
        exit(-1)

    print("Running configs:", range(conf_from, conf_to))
    print("Using sqlite3db file %s" % dbfname)
    for long_or_short in ["long", "short"]:
        for conf in range(conf_from, conf_to):
            print("------> Doing config {conf} -- {long_or_short}".format(
                conf=conf, long_or_short=long_or_short))

            with QueryCls(dbfname, conf) as q:
                ok, df = q.get_one_data_all_dumpnos(long_or_short)

            if not ok:
                print("-> Skipping file {file} config {conf} {ls}".format(
                    conf=conf, file=dbfname, ls=long_or_short))
                continue

            if options.verbose:
                print("Describing----")
                dat = df.describe()
                print(dat)
                print("Describe done.---")
                print("Features: ", df.columns.values.flatten().tolist())

            print("Transforming...")
            df = transform(df)

            if options.verbose:
                print("Describing post-transform ----")
                print(df.describe())
                print("Describe done.---")
                print("Features: ", df.columns.values.flatten().tolist())

            cleanname = re.sub(r'\.cnf.gz.sqlite$', '', dbfname)
            cleanname = re.sub(r'\.db$', '', dbfname)
            cleanname = re.sub(r'\.sqlitedb$', '', dbfname)
            cleanname = "{cleanname}-{long_or_short}-conf-{conf}".format(
                cleanname=cleanname,
                long_or_short=long_or_short,
                conf=conf)

            dump_dataframe(df, cleanname)


if __name__ == "__main__":
    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--csv", action="store_true", default=False,
                      dest="dump_csv", help="Dump CSV (for weka)")

    parser.add_option("--sql", action="store_true", default=False,
                      dest="dump_sql", help="Dump SQL queries")

    parser.add_option("--fixed", default=20000, type=int,
                      dest="fixed", help="Exact number of examples to take. -1 is to take all. Default: %default")

    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    parser.add_option("--confs", default="0-0", type=str,
                      dest="confs", help="Configs to generate. Default: %default")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)

    np.random.seed(2097483)
    for dbfname in args:
        print("----- FILE %s -------" % dbfname)
        one_database(dbfname)

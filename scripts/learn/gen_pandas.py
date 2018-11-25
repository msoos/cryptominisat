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


class QueryCls (QueryHelper):
    def __init__(self, dbfname):
        super(QueryCls, self).__init__(dbfname)
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
        """

        self.clause_dat = """
        -- , cl.`simplifications` as `cl.simplifications`
        -- , cl.`restarts` as `cl.restarts`
        -- , cl.`prev_restart` as `cl.prev_restart`
        -- , cl.`conflicts` as `cl.conflicts`
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

        self.common_restrictions = """
        and cl.restarts > 1 -- to avoid history being invalid
        and szfeat.latest_satzilla_feature_calc = cl.latest_satzilla_feature_calc
        and szfeat_cur.latest_satzilla_feature_calc = rdb0.latest_satzilla_feature_calc
        and rst.restarts = cl.prev_restart
        and tags.tagname = "filename"
        """

        self.common_limits = """
        order by random()
        limit {limit}
        """

        self.case_stmt_10k = """
        CASE WHEN

        -- used a lot
        (goodcl.last_confl_used > rdb0.conflicts and `goodcl`.`num_used` > 3)
        or (goodcl.last_confl_used > rdb0.conflicts
            AND `goodcl`.`num_used` <= 3
            AND goodcl.last_confl_used-rdb0.conflicts < 10000)
        THEN "OK"
        ELSE "BAD"
        END AS `x.class`
        """

        self.case_stmt_100k = """
        CASE WHEN goodcl.last_confl_used > (rdb0.conflicts+100000)
            -- and (goodcl.last_confl_used2 is NULL OR goodcl.last_confl_used2 > (rdb0.conflicts+100000))
            THEN "OK"
            ELSE "BAD"
            END AS `x.class`
        """

        self.q_count = """
        SELECT count(*) as count,
        {case_stmt}
        """

        self.q_ok_select = """
        SELECT
        tags.tag as "fname"
        {clause_dat}
        {restart_dat}
        {satzfeat_dat}
        {satzfeat_dat_cur}
        {rdb0_dat}
        {rdb1_dat}
        , goodcl.num_used as `x.num_used`
        , `goodcl`.`last_confl_used`-`cl`.`conflicts` as `x.lifetime`
        , {case_stmt}
        """

        self.q_ok = """
        FROM
        clauseStats as cl
        , goodClausesFixed as goodcl
        , restart as rst
        , satzilla_features as szfeat
        , satzilla_features as szfeat_cur
        , reduceDB as rdb0
        , reduceDB as rdb1
        , tags
        WHERE

        cl.clauseID = goodcl.clauseID
        and cl.clauseID != 0
        and rdb0.clauseID = cl.clauseID

        and rdb1.clauseID = cl.clauseID
        and rdb1.dump_no = rdb0.dump_no-1
        and rdb0.dump_no > 0
        """
        self.q_ok += self.common_restrictions

        # BAD caluses
        self.q_bad_select = """
        SELECT
        tags.tag as "fname"
        {clause_dat}
        {restart_dat}
        {satzfeat_dat}
        {satzfeat_dat_cur}
        {rdb0_dat}
        {rdb1_dat}
        , 0 as `x.num_used`
        , 0 as `x.lifetime`
        , "BAD" as `x.class`
        """

        self.q_bad = """
        FROM clauseStats as cl left join goodClausesFixed as goodcl
        on cl.clauseID = goodcl.clauseID
        , restart as rst
        , satzilla_features as szfeat
        , satzilla_features as szfeat_cur
        , reduceDB as rdb0
        , reduceDB as rdb1
        , tags
        WHERE

        goodcl.clauseID is NULL
        and cl.clauseID != 0
        and cl.clauseID is not NULL
        and rdb0.clauseID = cl.clauseID

        and rdb1.clauseID = cl.clauseID
        and rdb1.dump_no = rdb0.dump_no-1
        """
        self.q_bad += self.common_restrictions

        self.myformat = {
            "limit": 1000*1000*1000,
            "restart_dat": self.restart_dat,
            "clause_dat": self.clause_dat,
            "satzfeat_dat": self.satzfeat_dat,
            "satzfeat_dat_cur": self.satzfeat_dat.replace("szfeat.", "szfeat_cur."),
            "rdb0_dat": self.rdb0_dat,
            "rdb1_dat": self.rdb0_dat.replace("rdb0", "rdb1"),
            }

    def create_indexes(self):
        print("Recreating indexes...")
        t = time.time()
        q = """
        drop index if exists `idxclid`;
        drop index if exists `idxclid2`;
        drop index if exists `idxclid3`;
        drop index if exists `idxclid4`;
        drop index if exists `idxclid5`;
        drop index if exists `idxclid6`;
        drop index if exists `idxclid7`;
        drop index if exists `idxclid8`;

        create index `idxclid` on `clauseStats` (`clauseID`);
        create index `idxclid2` on `clauseStats` (`prev_restart`);
        create index `idxclid3` on `goodClauses` (`clauseID`);
        create index `idxclid4` on `restart` ( `restarts`);
        create index `idxclid5` on `tags` ( `tagname`);
        create index `idxclid6` on `reduceDB` (`clauseID`, `dump_no`);
        create index `idxclid7` on `reduceDB` (`clauseID`, `propagations_made`);
        create index `idxclid8` on `varData` ( `var`, `conflicts`, `clid_start_incl`, `clid_end_notincl`);
        """
        for l in q.split('\n'):
            self.c.execute(l)

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
        q = """
        delete from goodClausesFixed;
        """
        self.c.execute(q)
        print("goodClausesFixed deleted T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """insert into goodClausesFixed
        select
        clauseID
        , sum(num_used)
        , min(first_confl_used)
        , max(last_confl_used)
        , max(last_confl_used2)
        , max(last_prop_used)
        from goodClauses as c group by clauseID;"""
        self.c.execute(q)
        print("goodClausesFixed filled T: %-3.2f s" % (time.time() - t))

        t = time.time()
        q = """
        drop index if exists `idxclid20`;
        create index `idxclid20` on `goodClausesFixed` (`clauseID`);
        """
        for l in q.split('\n'):
            self.c.execute(l)
        print("goodClausesFixed indexes added T: %-3.2f s" % (time.time() - t))

    def fill_var_data_use(self):
        print("Filling var data use...")

        t = time.time()
        q = "DROP TABLE IF EXISTS `varDataUse`;"
        self.c.execute(q)
        q = """
        create table `varDataUse` (
            `restarts` int(20) NOT NULL,
            `conflicts` bigint(20) NOT NULL,

            `var` int(20) NOT NULL,
            `dec_depth` int(20) NOT NULL,
            `decisions_below` int(20) NOT NULL,
            `conflicts_below` int(20) NOT NULL,
            `clauses_below` int(20) NOT NULL,

            `decided_avg` double NOT NULL,
            `decided_pos_perc` double NOT NULL,
            `propagated_avg` double NOT NULL,
            `propagated_pos_perc` double NOT NULL,

            `propagated` bigint(20) NOT NULL,
            `propagated_pos` bigint(20) NOT NULL,
            `decided` bigint(20) NOT NULL,
            `decided_pos` bigint(20) NOT NULL,

            `sum_decisions_at_picktime` bigint(20) NOT NULL,
            `sum_propagations_at_picktime` bigint(20) NOT NULL,

            `total_conflicts_below_when_picked` bigint(20) NOT NULL,
            `total_decisions_below_when_picked` bigint(20) NOT NULL,
            `avg_inside_per_confl_when_picked` bigint(20) NOT NULL,
            `avg_inside_antecedents_when_picked` bigint(20) NOT NULL,

            `useful_clauses` int(20) DEFAULT NULL,
            `useful_clauses_used` int(20) DEFAULT NULL,
            `useful_clauses_first_used` int(20) DEFAULT NULL,
            `useful_clauses_last_used` int(20) DEFAULT NULL

            -- features when picked
            --`activity` double NOT NULL
        );
        """
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
        , min(cls.first_confl_used) as useful_clauses_first_used
        , max(cls.last_confl_used) as useful_clauses_last_used

        FROM varData as v left join goodClausesFixed as cls
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
            print("query:", q);
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

    def get_ok(self, subfilter):
        # calc OK -> which can be both BAD and OK
        q = self.q_count + self.q_ok + " and `x.class` == '%s'" % subfilter
        q = q.format(**self.myformat)
        if options.verbose:
            print("query:")
            print(q)
        cur = self.conn.execute(q.format(**self.myformat))
        num_lines = int(cur.fetchone()[0])
        print("Num datapoints OK -- %s (K): %-3.5f" % (subfilter, (num_lines/1000.0)))
        return num_lines

    def get_bad(self):
        q = self.q_count + self.q_bad
        q = q.format(**self.myformat)
        if options.verbose:
            print("query:")
            print(q)
        cur = self.conn.execute(q.format(**self.myformat))
        num_lines = int(cur.fetchone()[0])
        print("Num datpoints BAD (K): %-3.5f" % (num_lines/1000.0))
        return num_lines

    def get_clstats(self, long_or_short):
        if long_or_short == "short":
            self.myformat["case_stmt"] = self.case_stmt_10k
            fixed_mult = 1.0
            distrib = 0.5 #prefer OK by a factor of this. If < 0.5 then preferring BAD
        else:
            self.myformat["case_stmt"] = self.case_stmt_100k
            fixed_mult = 0.1
            distrib = 0.1 #prefer BAD by a factor of 1-this
        print("Distrib OK vs BAD set to %s " % distrib)
        print("Fixed multiplier set to  %s " % fixed_mult)

        t = time.time()

        num_lines_ok = 0
        num_lines_bad = 0

        num_lines_ok_ok = self.get_ok("OK")
        num_lines_ok_bad = self.get_ok("BAD")
        num_lines_bad_bad = self.get_bad()

        num_lines_bad = num_lines_ok_bad + num_lines_bad_bad
        num_lines_ok = num_lines_ok_ok

        total_lines = num_lines_ok + num_lines_bad
        print("Total number of datapoints (K): %-3.2f" % (total_lines/1000.0))
        if options.fixed != -1:
            if options.fixed*fixed_mult > total_lines:
                print("WARNING -- Your fixed num datapoints is too high:", options.fixed*fixed_mult)
                print("WARNING -- We only have:", total_lines)
                print("WARNING --> Not returning data.")
                return False, None

        if total_lines == 0:
            print("WARNING: Total number of datapoints is 0. Potential issues:")
            print(" --> Minimum no. conflicts set too high")
            print(" --> Less than 1 restarts were made")
            print(" --> No conflicts in SQL")
            print(" --> Something went wrong")
            return False, None

        # OK-OK
        print("OK-OK")
        q = self.q_ok_select + self.q_ok + " and `x.class` == 'OK'"
        if options.fixed != -1:
            self.myformat["limit"] = int(options.fixed*fixed_mult * distrib)
            if self.myformat["limit"] > num_lines_ok_ok:
                print("WARNING -- Your fixed num datapoints is too high, cannot generate OK-OK")
                print("        -- Wanted to create %d but only had %d" % (self.myformat["limit"], num_lines_ok_ok))
                return False, None
            q += self.common_limits

        print("limit for OK-OK:", self.myformat["limit"])
        q = q.format(**self.myformat)
        print("Running query for OK-OK...")
        if options.verbose:
            print("query:", q)
        df_ok_ok = pd.read_sql_query(q, self.conn)

        # OK-BAD
        print("OK-BAD")
        q = self.q_ok_select + self.q_ok + " and `x.class` == 'BAD'"
        if options.fixed != -1:
            self.myformat["limit"] = int(options.fixed*fixed_mult * num_lines_ok_bad/float(num_lines_bad) * (1.0-distrib))
            if self.myformat["limit"] > num_lines_ok_bad:
                print("WARNING -- Your fixed num datapoints is too high, cannot generate OK-BAD")
                print("        -- Wanted to create %d but only had %d" % (self.myformat["limit"], num_lines_ok_bad))
                return False, None
            q += self.common_limits
        print("limit for OK-BAD:", self.myformat["limit"])
        q = q.format(**self.myformat)
        print("Running query for OK-BAD...")
        if options.verbose:
            print("query:", q)
        df_ok_bad = pd.read_sql_query(q, self.conn)

        # BAD-BAD
        print("BAD-BAD")
        q = self.q_bad_select + self.q_bad
        if options.fixed*fixed_mult != -1:
            self.myformat["limit"] = int(options.fixed*fixed_mult * num_lines_bad_bad/float(num_lines_bad) * (1.0-distrib))
            if self.myformat["limit"] > num_lines_bad_bad:
                print("WARNING -- Your fixed num datapoints is too high, cannot generate BAD-BAD")
                print("        -- Wanted to create %d but only had %d" % (self.myformat["limit"], num_lines_bad_bad))
                return False, None
            q += self.common_limits

        print("limit for bad:", self.myformat["limit"])
        q = q.format(**self.myformat)
        print("Running query for BAD...")
        if options.verbose:
            print("query:", q)
        df_bad_bad = pd.read_sql_query(q, self.conn)
        print("Queries finished. T: %-3.2f" % (time.time() - t))

        if options.dump_sql:
            print("-- query starts --")
            print(q)
            print("-- query ends --")

        return True, pd.concat([df_ok_ok, df_ok_bad, df_bad_bad])

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

        cleanname = re.sub('\.cnf.gz.sqlite$', '', dbfname)
        cleanname += "-vardata"
        dump_dataframe(df, cleanname)


def get_one_file(dbfname, long_or_short):
    print("Using sqlite3db file %s" % dbfname)

    df = None
    with QueryCls(dbfname) as q:
        if not options.no_recreate_indexes:
            q.create_indexes()
            q.fill_last_prop()
            q.fill_good_clauses_fixed()
            q.fill_var_data_use()

    with QueryVar(dbfname) as q:
        q.vardata()

    with QueryCls(dbfname) as q:
        ok, df = q.get_clstats(long_or_short)
        if not ok:
            return False, None

        if options.verbose:
            print("Printing head:")
            print(df.head())
            print("Print head done.")

    return True, df


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
    df["cl.num_overlap_literals_rel"] = df["cl.num_overlap_literals"]/df["cl.antec_overlap_hist"]
    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"]/df["cl.num_antecedents_hist"]
    df["rst.varset_neg_polar_ratio"] = df["rst.varSetNeg"]/(df["rst.varSetPos"]+df["rst.varSetNeg"])

    # relative RDB
    df["rdb.rel_conflicts_made"] = (df["rdb0.conflicts_made"] > df["rdb1.conflicts_made"]).astype(int)
    df["rdb.rel_propagations_made"] = (df["rdb0.propagations_made"] > df["rdb1.propagations_made"]).astype(int)
    df["rdb.rel_clause_looked_at"] = (df["rdb0.clause_looked_at"] > df["rdb1.clause_looked_at"]).astype(int)
    df["rdb.rel_used_for_uip_creation"] = (df["rdb0.used_for_uip_creation"] > df["rdb1.used_for_uip_creation"]).astype(int)
    df["rdb.rel_last_touched_diff"] = (df["rdb0.last_touched_diff"] > df["rdb1.last_touched_diff"]).astype(int)
    df["rdb.rel_activity_rel"] = (df["rdb0.activity_rel"] > df["rdb1.activity_rel"]).astype(int)

    # ************
    # TODO decision level and branch depth are the same, right???
    # ************
    df["cl.size_rel"] = df["cl.size"] / df["cl.size_hist"]
    df["cl.glue_rel_queue"] = df["cl.glue"] / df["cl.glue_hist_queue"]
    df["cl.glue_rel_long"] = df["cl.glue"] / df["cl.glue_hist_long"]
    df["cl.glue_rel"] = df["cl.glue"] / df["cl.glue_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"]/df["cl.trail_depth_level_hist"]
    df["cl.branch_depth_rel_queue"] = df["cl.decision_level"]/df["cl.branch_depth_hist_queue"]

    # smaller-than larger-than for glue and size
    df["cl.size_smaller_than_hist"] = (df["cl.size"] < df["cl.size_hist"]).astype(int)
    df["cl.glue_smaller_than_hist"] = (df["cl.glue"] < df["cl.glue_hist"]).astype(int)
    df["cl.glue_smaller_than_hist_lt"] = (df["cl.glue"] < df["cl.glue_hist_long"]).astype(int)
    df["cl.glue_smaller_than_hist_queue"] = (df["cl.glue"] < df["cl.glue_hist_queue"]).astype(int)

    # relative decisions
    df["cl.decision_level_rel"] = df["cl.decision_level"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre1_rel"] = df["cl.decision_level_pre1"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.decision_level_pre2_rel"] = df["cl.decision_level_pre2"]/df["cl.decision_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"]/df["cl.decision_level_hist"]

    # relative props
    df["rst.all_props"] = df["rst.propBinRed"] + df["rst.propBinIrred"] + df["rst.propLongRed"] + df["rst.propLongIrred"]
    df["rst.propBinRed_ratio"] = df["rst.propBinRed"]/df["rst.all_props"]
    df["rst.propBinIrred_ratio"] = df["rst.propBinIrred"]/df["rst.all_props"]
    df["rst.propLongRed_ratio"] = df["rst.propLongRed"]/df["rst.all_props"]
    df["rst.propLongIrred_ratio"] = df["rst.propLongIrred"]/df["rst.all_props"]

    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"]/df["rst.free"]

    # relative resolutions
    df["rst.resolBinIrred_ratio"] = df["rst.resolBinIrred"]/df["rst.resolutions"]
    df["rst.resolBinRed_ratio"] = df["rst.resolBinRed"]/df["rst.resolutions"]
    df["rst.resolLRed_ratio"] = df["rst.resolLRed"]/df["rst.resolutions"]
    df["rst.resolLIrred_ratio"] = df["rst.resolLIrred"]/df["rst.resolutions"]

    df["cl.num_antecedents_rel"] = df["cl.num_antecedents"] / df["cl.num_antecedents_hist"]
    df["cl.decision_level_rel"] = df["cl.decision_level"] / df["cl.decision_level_hist"]
    df["cl.trail_depth_level_rel"] = df["cl.trail_depth_level"] / df["cl.trail_depth_level_hist"]
    df["cl.backtrack_level_rel"] = df["cl.backtrack_level"] / df["cl.backtrack_level_hist"]

    # smaller-or-greater comparisons
    df["cl.decision_level_smaller_than_hist"] = (df["cl.decision_level"] < df["cl.decision_level_hist"]).astype(int)
    df["cl.backtrack_level_smaller_than_hist"] = (df["cl.backtrack_level"] < df["cl.backtrack_level_hist"]).astype(int)
    df["cl.trail_depth_level_smaller_than_hist"] = (df["cl.trail_depth_level"] < df["cl.trail_depth_level_hist"]).astype(int)
    df["cl.num_antecedents_smaller_than_hist"] = (df["cl.num_antecedents"] < df["cl.num_antecedents_hist"]).astype(int)
    df["cl.antec_sum_size_smaller_than_hist"] = (df["cl.antec_sum_size_hist"] < df["cl.num_total_lits_antecedents"]).astype(int)
    df["cl.antec_overlap_smaller_than_hist"] = (df["cl.antec_overlap_hist"] < df["cl.num_overlap_literals"]).astype(int)
    df["cl.overlap_smaller_than_hist"] = (df["cl.num_overlap_literals"]<df["cl.antec_overlap_hist"]).astype(int)
    df["cl.branch_smaller_than_hist_queue"] = (df["cl.decision_level"]<df["cl.branch_depth_hist_queue"]).astype(int)



    # df["cl.vsids_vars_rel"] = df["cl.vsids_vars_avg"] / df["cl.vsids_vars_hist"]

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

    fname = "%s-pandasdata.dat" % name
    print("Dumping pandas data to:", fname)
    with open(fname, "wb") as f:
        pickle.dump(df, f)


def one_dataframe(dbfname, long_or_short):
    ok, df = get_one_file(dbfname, long_or_short)
    if not ok:
        return False, None

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

    cleanname = re.sub('\.cnf.gz.sqlite$', '', dbfname)
    cleanname += "-"+long_or_short
    dump_dataframe(df, cleanname)

    return True, df


if __name__ == "__main__":
    usage = "usage: %prog [options] file1.sqlite [file2.sqlite ...]"
    parser = optparse.OptionParser(usage=usage)

    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Print more output")

    parser.add_option("--csv", action="store_true", default=False,
                      dest="dump_csv", help="Dump CSV (for weka)")

    parser.add_option("--sql", action="store_true", default=False,
                      dest="dump_sql", help="Dump SQL query")

    parser.add_option("--fixed", default=-1, type=int,
                      dest="fixed", help="Exact number of examples to take. -1 is to take all. Default: %default")

    parser.add_option("--noind", action="store_true", default=False,
                      dest="no_recreate_indexes",
                      help="Don't recreate indexes")

    (options, args) = parser.parse_args()

    if len(args) < 1:
        print("ERROR: You must give at least one file")
        exit(-1)

    np.random.seed(2097483)
    for long_or_short in ["short", "long"]:
        dfs = []
        for dbfname in args:
            print("----- INTERMEDIATE data %s -------" % long_or_short)
            ok, df = one_dataframe(dbfname, long_or_short)
            if ok:
                dfs.append(df)

        if len(dfs) == 0:
            print("Error, nothing got ingested, something is off")
            exit(-1)

        if len(args) > 1:
            print("----- FINAL predictor %s -------" % long_or_short)
            if len(dfs) == 0:
                print("Ooops, final predictor is None, probably no meaningful data. Exiting.")
                exit(0)

            final_df = pd.concat(dfs)
            dump_dataframe(final_df, "final-"+long_or_short)

DROP TABLE IF EXISTS `tags`;
CREATE TABLE `tags` (
  `name` varchar(500) NOT NULL,
  `val` varchar(500) NOT NULL
);

DROP TABLE IF EXISTS `timepassed`;
CREATE TABLE `timepassed` (
  `simplifications` bigint(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `runtime` float NOT NULL,
  `name` varchar(200) NOT NULL,
  `elapsed` float NOT NULL,
  `timeout` int(20) DEFAULT NULL,
  `percenttimeremain` float DEFAULT NULL
);

DROP TABLE IF EXISTS `memused`;
CREATE TABLE `memused` (
  `simplifications` bigint(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `runtime` float NOT NULL,
  `name` varchar(200) NOT NULL,
  `MB` int(20) NOT NULL
);

DROP TABLE IF EXISTS `solverRun`;
CREATE TABLE `solverRun` (
  `runtime` float NOT NULL,
  `gitrev` varchar(100) NOT NULL
);

DROP TABLE IF EXISTS `startup`;
CREATE TABLE `startup` (
  `startTime` datetime NOT NULL
);

DROP TABLE IF EXISTS `finishup`;
CREATE TABLE `finishup` (
  `endTime` datetime NOT NULL,
  `status` varchar(255) NOT NULL
);

DROP TABLE IF EXISTS `reduceDB_common`;
CREATE TABLE `reduceDB_common` (
  `reduceDB_called` int(20) NOT NULL,
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `cur_restart_type` int(20) NOT NULL,
  `tot_cls_in_db` int(20) NOT NULL,

  `median_act` float NOT NULL,
  `median_uip1_used` int(20) NOT NULL,
  `median_props` int(20) NOT NULL,
  `median_sum_uip1_per_time` float NOT NULL,
  `median_sum_props_per_time` float NOT NULL,

  -- no avg_glue: ternary resolvents have no glue
  `avg_props` float NOT NULL,
  `avg_uip1_used` float NOT NULL,
  `avg_sum_uip1_per_time` float NOT NULL,
  `avg_sum_props_per_time` float NOT NULL,

  num_vars int(20) NOT NULL,
  num_long_irred_cls int(20) NOT NULL,
  num_long_irred_cls_lits int(20) NOT NULL,
  num_long_red_cls int(20) NOT NULL,
  num_long_red_cls_lits int(20) NOT NULL,
  num_bin_irred_cls int(20) NOT NULL,
  num_bin_red_cls int(20) NOT NULL,

  trailDepthHistLT_avg float NOT NULL,
  backtrackLevelHistLT_avg float NOT NULL,
  conflSizeHistLT_avg float NOT NULL,
  numResolutionsHistLT_avg float NOT NULL,
  glueHistLT_avg float NOT NULL,
  antec_data_sum_sizeHistLT_avg float NOT NULL,
  overlapHistLT_avg float NOT NULL
);


DROP TABLE IF EXISTS `reduceDB`;
CREATE TABLE `reduceDB` (
  `reduceDB_called` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `introduced_at_conflict` bigint(20) NOT NULL, -- ternary resolvents have no clause_stats, so it's here

  `clauseID` int(20) NOT NULL,
  `dump_no` int(20) NOT NULL,
  `conflicts_made` bigint(20) NOT NULL,
  `props_made` bigint(20) NOT NULL,
  `sum_props_made` bigint(20) NOT NULL,
  `uip1_used` bigint(20) NOT NULL,
  `sum_uip1_used`  int(20) NOT NULL,

  `last_touched_any_diff` bigint(20) NOT NULL,
  `activity_rel` float(20) NOT NULL,
  `locked` int(20) NOT NULL,
  `glue` int(20) DEFAULT NULL, -- NULL for ternary resolvents and eagerly subsumed clauses
  `size` int(20) NOT NULL,
  `used` int(20) NOT NULL, -- kissat-style 'used' life, what reduce goes by
  `is_ternary_resolvent` int(20) NOT NULL,
  `is_decision` int(20) NOT NULL,
  `is_distilled` int(20) NOT NULL,

  -- ranking
  `act_ranking` int(20) NOT NULL,
  `prop_ranking` int(20) NOT NULL,
  `uip1_ranking` int(20) NOT NULL,
  `sum_uip1_per_time_ranking` int(20) NOT NULL,
  `sum_props_per_time_ranking` int(20) NOT NULL,

  -- discounted
  `discounted_uip1_used` float(20) NOT NULL,
  `discounted_props_made` float(20) NOT NULL,
  `discounted_uip1_used2` float(20) NOT NULL,
  `discounted_props_made2` float(20) NOT NULL,
  `discounted_uip1_used3` float(20) NOT NULL,
  `discounted_props_made3` float(20) NOT NULL
);


DROP TABLE IF EXISTS `restart`;
CREATE TABLE `restart` (
  `restartID` int (20) NOT NULL,
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `conflicts_this_restart` int(20) NOT NULL,
  `runtime` float NOT NULL,

  `numIrredBins` int(20) NOT NULL,
  `numIrredLongs` int(20) NOT NULL,
  `numRedBins` int(20) NOT NULL,
  `numRedLongs` int(20) NOT NULL,

  `numIrredLits` bigint(20) NOT NULL,
  `numredLits` bigint(20) NOT NULL,

  -- conflict stats
  `glue` float,
  `glueSD` float,
  `glueMin` int(20),
  `glueMax` int(20),

  `size` float,
  `sizeSD` float,
  `sizeMin` int(20),
  `sizeMax` int(20),

  `resolutions` float,
  `resolutionsSD` float,
  `resolutionsMin` int(20),
  `resolutionsMax` int(20),

  -- search stats
  `branchDepth` float,
  `branchDepthSD` float,
  `branchDepthMin` int(20),
  `branchDepthMax` int(20),

  `branchDepthDelta` float,
  `branchDepthDeltaSD` float,
  `branchDepthDeltaMin` int(20),
  `branchDepthDeltaMax` int(20),

  `trailDepth` float,
  `trailDepthSD` float,
  `trailDepthMin` int(20),
  `trailDepthMax` int(20),

  `trailDepthDelta` float,
  `trailDepthDeltaSD` float,
  `trailDepthDeltaMin` int(20),
  `trailDepthDeltaMax` int(20),

  `learntUnits` int(20) NOT NULL,
  `learntBins` int(20) NOT NULL,
  `learntLongs` int(20) NOT NULL,

  `resolBinIrred` bigint(20) NOT NULL,
  `resolBinRed` bigint(20) NOT NULL,
  `resolLIrred` bigint(20) NOT NULL,
  `resolLRed` bigint(20) NOT NULL,

  `propagations` bigint(20) NOT NULL,
  `decisions` bigint(20) NOT NULL,

  `flipped` bigint(20) NOT NULL,
  `varSetPos` bigint(20) NOT NULL,
  `varSetNeg` bigint(20) NOT NULL,
  `free` int(20) NOT NULL,
  `replaced` int(20) NOT NULL,
  `eliminated` int(20) NOT NULL,
  `set` int(20) NOT NULL,

  `branch_strategy` int NOT NULL,
  `restart_type` int(20) NOT NULL
);

DROP TABLE IF EXISTS `clause_stats`;
CREATE TABLE `clause_stats` (
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `clauseID` bigint(20) NOT NULL,
  `restartID` int(20) NOT NULL,

  `orig_glue` int(20) NOT NULL,
  `glue_before_minim` int(20) NOT NULL,
  `orig_size` int(20) NOT NULL,
  `size_before_minim` int(20) NOT NULL,
  `num_overlap_literals` int(20) NOT NULL,
  `num_antecedents` int(20) NOT NULL,
  `num_total_lits_antecedents` int(20) NOT NULL,
  `is_decision` int(20) NOT NULL,

  `backtrack_level` int(20) NOT NULL,
  `decision_level` int(20) NOT NULL,
  `trail_depth_level` int(20) NOT NULL,
  `cur_restart_type` int(20) NOT NULL,

  `antecedents_binIrred` int(20) NOT NULL,
  `antecedents_binRed` int(20) NOT NULL,
  `antecedents_longIrred` int(20) NOT NULL,
  `antecedents_longRed` int(20) NOT NULL,

  -- long-term (all restarts) averages at learning time
  `trailDepthHistLT_avg` float,
  `conflSizeHistLT_avg` float,
  `glueHistLT_avg` float,
  `numResolutionsHistLT_avg` float,
  `antec_data_sum_sizeHistLT_avg` float,
  `overlapHistLT_avg` float,

  -- short-term (this restart / recent) averages at learning time
  `branchDepthHistQueue_avg` float,
  `trailDepthHist_avg` float,
  `conflSizeHist_avg` float,
  `glueHist_avg` float,
  `glueHist_longterm_avg` float
);


DROP TABLE IF EXISTS `satzilla_features`;
CREATE TABLE `satzilla_features` (
  `simplifications` int(20) NOT NULL,
  `restarts` int(20) NOT NULL,
  `conflicts` bigint(20) NOT NULL,
  `latest_satzilla_feature_calc` int(20) NOT NULL,

  -- instance (irredundant clauses)
  `numVars` int(20) NOT NULL,
  `numClauses` int(20) NOT NULL,
  `var_cl_ratio` double NOT NULL,
  `binary` double NOT NULL,
  `horn` double NOT NULL,

  -- conflicts so far
  `avg_confl_size` double NOT NULL,
  `avg_confl_glue` double NOT NULL,
  `avg_num_resolutions` double NOT NULL,
  `learnt_bins_per_confl` double NOT NULL,

  -- search so far
  `avg_branch_depth` double NOT NULL,
  `avg_trail_depth_delta` double NOT NULL,
  `avg_branch_depth_delta` double NOT NULL,
  `props_per_confl` double NOT NULL,
  `confl_per_restart` double NOT NULL,
  `decisions_per_conflict` double NOT NULL,

  -- learnt clause DB
  `red_glue_distr_mean` double NOT NULL,
  `red_glue_distr_var` double NOT NULL,
  `red_size_distr_mean` double NOT NULL,
  `red_size_distr_var` double NOT NULL
);


DROP TABLE IF EXISTS `cl_last_in_solver`;
create table `cl_last_in_solver` (
  `conflicts` bigint(20) NOT NULL
  , `clauseID` bigint(20) NOT NULL
);

DROP TABLE IF EXISTS `update_id`;
create table `update_id` (
  `old_id` bigint(20) NOT NULL
  , `new_id` bigint(20) NOT NULL
);

DROP TABLE IF EXISTS `set_id_confl`;
create table `set_id_confl` (
  `id` bigint(20) NOT NULL
  , `conflicts` bigint(20) NOT NULL
);


